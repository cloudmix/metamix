#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "metadata.h"

namespace metamix {

template<class K>
class MetadataQueue
{
public:
  using Self = MetadataQueue<K>;
  using Kind = K;
  using MetaType = Metadata<K>;
  using ValueType = typename MetaType::ValueType;

private:
  std::mutex m{};
  std::vector<MetaType> q{};

public:
  MetadataQueue() = default;

  explicit MetadataQueue(size_t reserved) { q.reserve(reserved); }

  template<class InputIt>
  MetadataQueue(InputIt first, InputIt last)
    : q(first, last)
  {
    std::make_heap(q.begin(), q.end(), comparator);
  }

  MetadataQueue(std::initializer_list<MetaType> init)
    : q{ init }
  {
    std::make_heap(q.begin(), q.end(), comparator);
  }

  MetadataQueue(const Self &other)
    : q{ other.q }
  {}

  Self &operator=(const Self &other)
  {
    q = other.q;
    return *this;
  }

  MetadataQueue(Self &&other) noexcept = default;
  Self &operator=(Self &&other) noexcept = default;

  bool empty() const noexcept { return q.empty(); }

  size_t size() const noexcept { return q.size(); }

  const MetaType &top() const { return q.front(); }

  void push(MetaType value)
  {
    std::lock_guard<std::mutex> guard(m);
    q.push_back(std::move(value));
    std::push_heap(q.begin(), q.end(), comparator);
  }

  /// Pops value earliest in given time frame, assigned to given input id from queue.
  /// Drops all values which were earlier in queue.
  ///
  /// \param id     input id, popped value must by assigned to it, other values will be dropped
  /// \param since  start time for lookup, inclusive
  /// \param until  end time for lookup, exclusive
  /// \return popped value or nothing if queue is empty
  std::optional<MetaType> pop(InputId id, TS since, TS until)
  {
    std::lock_guard<std::mutex> guard(m);
    return pop_impl(id, since, until);
  }

  /// Pops all values in given time frame, assigned to given input id from queue. Moves popped value to output iterator.
  /// Drops all other values since queue begin to `until` time from queue.
  ///
  /// \tparam     OutputIt  output iterator type
  /// \param      id        input id, popped value must by assigned to it, other values will be dropped
  /// \param      since     start time for lookup, inclusive
  /// \param      until     end time for lookup, exclusive
  /// \param[out] out       output iterator, popped values will be moved here
  /// \return               count of popped items
  template<class OutputIt>
  unsigned int pop_all(InputId id, TS since, TS until, OutputIt out)
  {
    static_assert(
      std::is_base_of<std::output_iterator_tag, typename std::iterator_traits<OutputIt>::iterator_category>::value,
      "output iterator must be of output iterator category");

    std::lock_guard<std::mutex> guard(m);

    int count = 0;
    for (;;) {
      auto opt = pop_impl(id, since, until);
      if (!opt) {
        break;
      }

      *out++ = std::move(*opt);
      count++;
    }
    return count;
  }

  size_t drop_id(InputId id)
  {
    std::lock_guard<std::mutex> guard(m);
    size_t old_size = q.size();
    auto tail = std::remove_if(q.begin(), q.end(), [&](const auto &t) { return t.input_id == id; });
    q.erase(tail, q.end());
    return old_size - q.size();
  }

private:
  MetaType pop_any_impl()
  {
    std::pop_heap(q.begin(), q.end(), comparator);
    auto value = std::move(q.back());
    q.pop_back();
    return value;
  }

  std::optional<MetaType> pop_impl(InputId id, TS since, TS until)
  {
    for (;;) {
      // If queue is empty, nothing can be popped
      if (q.empty()) {
        return std::nullopt;
      }

      // If the very next item is past the range, nothing more can be popped.
      if (q.front().pts >= until) {
        return std::nullopt;
      }

      // Pop earliest item.
      auto value = pop_any_impl();

      // If the item is within the range, and it's assigned to requested input...
      if (value.pts >= since && value.input_id == id) {
        // ...drop any other colliding metadata from queue.
        while (!q.empty() && q.front().pts == value.pts) {
          assert(q.front().input_id != value.input_id);
          pop_any_impl();
        }

        // Item must match query conditions.
        assert(value.pts >= since && value.pts < until && value.input_id == id);

        // ...return matched item.
        return value;
      }

      // Otherwise, this is out-of-range item, before the since timestamp.
      // This means it should be dropped, which we are doing now and proceed again.
    }
  }

  static bool comparator(const MetaType &lhs, const MetaType &rhs)
  {
    if (lhs.pts == rhs.pts) {
      if (lhs.input_id == rhs.input_id) {
        return lhs.order > rhs.order;
      } else {
        return lhs.input_id > rhs.input_id;
      }
    } else {
      return lhs.pts > rhs.pts;
    }
  }
};

template<class... Ks>
class MetadataQueueGroup
{
private:
  std::tuple<MetadataQueue<Ks>...> m_queues{};

public:
  template<class K>
  inline MetadataQueue<K> &get()
  {
    return std::get<MetadataQueue<K>>(m_queues);
  }

  template<class K>
  inline const MetadataQueue<K> &get() const
  {
    return std::get<MetadataQueue<K>>(m_queues);
  }

  template<class K, class Visitor>
  inline auto visit(Visitor &&vis)
  {
    return std::invoke(std::forward<Visitor>(vis), get<K>());
  }

  template<class Visitor>
  inline void visit_each(Visitor &&vis)
  {
    std::apply([&](auto &... q) { (..., std::invoke(std::forward<Visitor>(vis), q)); }, m_queues);
  }

  template<class M>
  inline void push(M meta)
  {
    visit<typename M::Kind>([meta{ std::move(meta) }](auto &q) { q.push(std::move(meta)); });
  }

  template<class K>
  inline std::optional<K> pop(InputId id, TS since, TS until)
  {
    return visit<K>([&](auto &q) { return q.pop(id, since, until); });
  }

  template<class K, class OutputIt>
  unsigned int pop_all(InputId id, TS since, TS until, OutputIt out)
  {
    return visit<K>([&, out{ std::move(out) }](auto &q) { return q.pop_all(id, since, until, std::move(out)); });
  }

  inline size_t drop_id(InputId id)
  {
    size_t count = 0;
    visit_each([&](auto &q) { count += q.drop_id(id); });
    return count;
  }
};
}
