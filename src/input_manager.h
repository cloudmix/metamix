#pragma once

#include <atomic>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <boost/iterator/transform_iterator.hpp>

#include "abstract_input.h"
#include "iospec.h"
#include "log.h"

namespace metamix {

class InputManager
{
public:
  template<typename T>
  struct Inputs
  {
    using Kinds = MetadataKindPack;
    T sei;
    T scte;
  };

  template<typename T>
  struct InputsChangeset
  {
    using Kinds = MetadataKindPack;
    std::optional<T> sei{};
    std::optional<T> scte{};
  };

private:
  struct CurrentInputsIds
  {
    using Kinds = MetadataKindPack;
    std::atomic<InputId> sei{ 0 };
    std::atomic<InputId> scte{ 0 };

    inline Inputs<InputId> read() const volatile
    {
      Inputs<InputId> r;
      Inputs<InputId>::Kinds::for_each([&](auto k) {
        using K = decltype(k);
        K::get(r) = K::get(*this);
      });
      return r;
    }
  };

  CurrentInputsIds m_current_inputs_ids{};
  std::map<InputId, std::unique_ptr<AbstractInput>> m_inputs;

public:
  InputManager(std::vector<std::unique_ptr<AbstractInput>> inputs) noexcept(false)
    : m_inputs{}
  {

    for (auto &ptr : inputs) {
      assert(ptr);
      auto id = ptr->spec().id;
      m_inputs[id] = std::move(ptr);
    }
  }

  std::optional<std::reference_wrapper<AbstractInput>> get_input_by_id(InputId input_id) noexcept
  {
    if (m_inputs.find(input_id) != m_inputs.end()) {
      return *(m_inputs[input_id]);
    } else {
      return std::nullopt;
    }
  }

  std::optional<std::reference_wrapper<AbstractInput>> get_input_by_name(const std::string &name) noexcept
  {
    for (auto &[_, p] : m_inputs) {
      if (p->spec().name == name) {
        return *p;
      }
    }
    return std::nullopt;
  }

  template<class T>
  std::optional<std::reference_wrapper<T>> get_input_by_id(InputId input_id) noexcept
  {
    if (auto aio = get_input_by_id(input_id); aio) {
      return std::ref(dynamic_cast<T &>(aio->get()));
    } else {
      return std::nullopt;
    }
  }

  template<class T>
  std::optional<std::reference_wrapper<T>> get_input_by_name(const std::string &name) noexcept
  {
    if (auto aio = get_input_by_name(name); aio) {
      return std::ref(dynamic_cast<T &>(aio->get()));
    } else {
      return std::nullopt;
    }
  }

  Inputs<InputId> get_current_input_ids() volatile const noexcept { return m_current_inputs_ids.read(); }

  template<class K>
  InputId get_current_input_id() volatile const noexcept
  {
    return K::get(m_current_inputs_ids);
  }

  template<class K>
  AbstractInput &get_current_input() noexcept
  {
    return *(m_inputs[get_current_input_id<K>()]);
  }

  void set_current_inputs(InputsChangeset<InputId> changeset) noexcept(false)
  {
    InputsChangeset<InputId>::Kinds::for_each([&](auto k) {
      using K = decltype(k);

      auto input_id_opt = K::get(changeset);
      if (input_id_opt) {
        const auto &input_id = *input_id_opt;

        if (!is_input_id_valid(input_id)) {
          std::stringstream ss;
          ss << "input id #" << input_id << " is out of range";
          throw std::out_of_range("input id is out of range");
        }

        const auto &input = get_input_by_id(input_id)->get();
        InputCapabilities caps = input.caps();
        if (!caps.has<K>()) {
          LOG(warning) << "Input #" << input_id << " does not declare capability for " << K::NAME;
        }
      }
    });

    InputsChangeset<InputId>::Kinds::for_each([&](auto k) {
      using K = decltype(k);

      auto input_id_opt = K::get(changeset);
      if (input_id_opt) {
        const auto &input_id = *input_id_opt;

        auto prev_id = K::get(m_current_inputs_ids).exchange(input_id);
        if (prev_id != input_id) {
          LOG(info) << "Changed current input id for " << K::NAME << " to #" << input_id;
        }
      }
    });
  }

  void set_current_inputs(const InputsChangeset<std::string> &changeset) noexcept(false)
  {
    InputsChangeset<InputId> id_changeset;
    InputsChangeset<InputId>::Kinds::for_each([&](auto k) {
      using K = decltype(k);

      const auto &name_opt = K::get(changeset);
      if (name_opt) {
        const auto &input_opt = get_input_by_name(*name_opt);
        if (!input_opt) {
          std::stringstream ss;
          ss << "unknown input name: " << std::quoted(*name_opt);
          throw std::invalid_argument(ss.str());
        }

        K::get(id_changeset) = input_opt->get().spec().id;
      }
    });

    set_current_inputs(id_changeset);
  }

  bool is_input_id_valid(InputId input_id) const { return m_inputs.find(input_id) != m_inputs.end(); }

  size_t size() const { return m_inputs.size(); }

  auto begin() { return boost::make_transform_iterator(m_inputs.begin(), iter_unwrap); }

  auto end() { return boost::make_transform_iterator(m_inputs.end(), iter_unwrap); }

  auto cbegin() const { return boost::make_transform_iterator(m_inputs.cbegin(), const_iter_unwrap); }

  auto cend() const { return boost::make_transform_iterator(m_inputs.cend(), const_iter_unwrap); }

private:
  inline static AbstractInput &iter_unwrap(const std::pair<const InputId, std::unique_ptr<AbstractInput>> &x)
  {
    return *x.second;
  }

  inline static const AbstractInput &const_iter_unwrap(
    const std::pair<const InputId, std::unique_ptr<AbstractInput>> &x)
  {
    return *x.second;
  }
};
}
