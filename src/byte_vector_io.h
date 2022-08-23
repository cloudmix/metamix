#pragma once

#include <iomanip>
#include <ostream>
#include <vector>

#include <boost/scope_exit.hpp>

namespace std {

template<class CharType, class CharTrait>
inline std::basic_ostream<CharType, CharTrait> &
operator<<(std::basic_ostream<CharType, CharTrait> &os, const std::vector<uint8_t> &data)
{
  ios::fmtflags f(os.flags());

  BOOST_SCOPE_EXIT_ALL(&) { os.flags(f); };

  os << "[ ";
  os << std::hex;
  for (const auto &x : data) {
    os << std::setw(2) << std::setfill('0') << static_cast<int>(x) << " ";
  }
  os << "]";
  return os;
}
}
