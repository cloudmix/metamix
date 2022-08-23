#pragma once

#include <cassert>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>

namespace metamix {

template<class Deriving, typename T>
class AbstractSlice
{
public:
  using ValueType = T;

public:
  virtual ~AbstractSlice() = default;

  virtual size_t size() const noexcept = 0;
  virtual T *data() noexcept = 0;
  virtual const T *data() const noexcept = 0;

  bool empty() const noexcept { return size() == 0; }

  T &operator[](size_t idx) noexcept
  {
    assert(idx < size());
    return *(data() + idx);
  }

  const T &operator[](size_t idx) const noexcept
  {
    assert(idx < size());
    return *(data() + idx);
  }

  T &at(size_t idx) noexcept(false)
  {
    if (idx >= size()) {
      throw std::out_of_range("index out of range");
    }
    return (*this)[idx];
  }

  const T &at(size_t idx) const noexcept(false)
  {
    if (idx >= size()) {
      throw std::out_of_range("index out of range");
    }
    return (*this)[idx];
  }

  T &front() { return (*this)[0]; }

  const T &front() const { return (*this)[0]; }

  T &back() { return (*this)[size() - 1]; }

  const T &back() const { return (*this)[size() - 1]; }

private:
  template<typename Value>
  class IteratorImpl : public boost::iterator_facade<IteratorImpl<Value>, Value, boost::random_access_traversal_tag>
  {
  private:
    struct Enabler
    {};
    using ContainerPtr = typename std::conditional<std::is_const<Value>::value, const Deriving *, Deriving *>::type;

  private:
    ContainerPtr m_container{ nullptr };
    size_t m_idx{ 0 };

  public:
    constexpr IteratorImpl() = default;
    constexpr IteratorImpl(ContainerPtr container, size_t idx) noexcept
      : m_container{ container }
      , m_idx{ idx }
    {}

    template<typename OtherValue>
    explicit constexpr IteratorImpl(
      const IteratorImpl<OtherValue> &other,
      typename std::enable_if<std::is_convertible<OtherValue *, Value *>::value, Enabler>::type = Enabler()) noexcept
      : m_container{ other.m_container }
      , m_idx{ other.m_idx }
    {}

  private:
    friend class boost::iterator_core_access;

    Value &dereference() const noexcept(false)
    {
      if (m_container == nullptr) {
        throw std::out_of_range("iterator is not initialized");
      }

      if (m_idx >= m_container->size()) {
        throw std::out_of_range("dereferencing out-of-range iterator");
      }

      return (*m_container)[m_idx];
    }

    template<typename OtherValue>
    constexpr bool equal(const IteratorImpl<OtherValue> &other) const noexcept
    {
      return m_container == other.m_container && m_idx == other.m_idx;
    }

    void increment() noexcept { m_idx++; }

    void decrement() noexcept { m_idx--; }

    void advance(std::ptrdiff_t diff) noexcept { m_idx += diff; }

    template<typename OtherValue>
    constexpr std::ptrdiff_t distance_to(const IteratorImpl<OtherValue> &other) const noexcept
    {
      return other.m_idx - m_idx;
    }
  };

public:
  using Iterator = IteratorImpl<T>;
  using ConstIterator = IteratorImpl<const T>;

  Iterator begin() { return Iterator(static_cast<Deriving *>(this), 0); }

  Iterator end() { return Iterator(static_cast<Deriving *>(this), size()); }

  ConstIterator cbegin() const { return ConstIterator(static_cast<const Deriving *>(this), 0); }

  ConstIterator cend() const { return ConstIterator(static_cast<const Deriving *>(this), size()); }
};

template<class Deriving, class DerivingBase, typename T = typename DerivingBase::ValueType>
class AbstractOwnedSlice : public DerivingBase
{
  static_assert(std::is_base_of<AbstractSlice<DerivingBase, T>, DerivingBase>::value,
                "Deriving base has to inherit from AbstractSlice<DerivingBase, T>");

private:
  std::vector<T> m_data{};

public:
  AbstractOwnedSlice() = default;
  explicit AbstractOwnedSlice(std::vector<T> data)
    : m_data(std::move(data))
  {}
  explicit AbstractOwnedSlice(const DerivingBase &base)
    : m_data(base.data(), base.data() + base.size())
  {}
  AbstractOwnedSlice(std::initializer_list<T> init)
    : m_data(init)
  {}

  size_t size() const noexcept override { return m_data.size(); }

  T *data() noexcept override { return m_data.data(); }

  const T *data() const noexcept override { return m_data.data(); }
};
}
