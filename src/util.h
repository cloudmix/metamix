#pragma once

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <boost/config.hpp>
#include <boost/optional.hpp>

namespace metamix {

#define NULL_PROTECT(p) (::metamix::null_protect(p, "The expression `" BOOST_STRINGIZE(p) "` returned a null pointer."))

void
hex_dump(const uint8_t *buffer, size_t size);

template<class It>
void
hex_dump(It first, It last)
{
  std::vector<uint8_t> vec(first, last);
  hex_dump(vec.data(), vec.size());
}

template<typename T>
std::optional<T>
boost_optional_to_std(boost::optional<T> bo)
{
  if (bo) {
    return *bo;
  } else {
    return std::nullopt;
  }
}

struct NullPointerError : public std::runtime_error
{
  inline explicit NullPointerError(const char *what)
    : std::runtime_error(what)
  {}
  inline explicit NullPointerError(const std::string &what)
    : std::runtime_error(what)
  {}
};

template<typename T>
inline T *
null_protect(T *ptr, const char *msg)
{
  if (ptr != nullptr) {
    return ptr;
  } else {
    throw NullPointerError(msg);
  }
}
}
