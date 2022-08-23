#pragma once

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <type_traits>

namespace metamix {

#define METAMIX_METADATA_KIND(CLASS, FIELD, API_FIELD, NAME_STR, DESCRIPTION_STR)                                      \
  class CLASS                                                                                                          \
  {                                                                                                                    \
  public:                                                                                                              \
    constexpr CLASS() = default;                                                                                       \
                                                                                                                       \
    static constexpr const char *NAME = (NAME_STR);                                                                    \
    static constexpr const char *DESCRIPTION = (DESCRIPTION_STR);                                                      \
    static constexpr const char *API_NAME = #API_FIELD;                                                                \
                                                                                                                       \
    template<class C>                                                                                                  \
    static inline auto get(C &sc) -> decltype((sc.FIELD))                                                              \
    {                                                                                                                  \
      return sc.FIELD;                                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    template<class C>                                                                                                  \
    static constexpr inline auto get(const C &sc) -> decltype((sc.FIELD))                                              \
    {                                                                                                                  \
      return sc.FIELD;                                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    friend std::ostream &operator<<(std::ostream &os, const CLASS &k) { return os << k.NAME; }                         \
  };

METAMIX_METADATA_KIND(ScteKind, scte, adMarker, "SCTE35", "SCTE-35")
METAMIX_METADATA_KIND(SeiKind, sei, closedCaption, "SEICC", "H.264 with 608/708 SEI")
METAMIX_METADATA_KIND(TimeSourceKind, time_source, timeSource, "TS", "System Time Source")

template<typename... K>
struct KindPack
{
  static constexpr size_t SIZE = sizeof...(K);

  template<template<typename...> typename T>
  using Apply = T<K...>;

  template<template<typename> typename F>
  using Map = KindPack<F<K>...>;

  template<class F>
  inline static auto unpack(F &&f)
  {
    return std::invoke<F, const K &...>(std::forward<F>(f), K()...);
  }

  template<class F>
  inline static void for_each(F &&f)
  {
    unpack([&](const K &... args) { (..., std::invoke(std::forward<F>(f), args)); });
  }
};
}
