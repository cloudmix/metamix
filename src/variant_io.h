#pragma once

#include <ostream>
#include <variant>

namespace std {

template<class CharType, class CharTrait, typename... Types>
inline basic_ostream<CharType, CharTrait> &
operator<<(basic_ostream<CharType, CharTrait> &os, const variant<Types...> &data)
{
  if (data.valueless_by_exception()) {
    os << "<invalid variant>";
  } else {
    os << "[" << data.index() << "](";
    visit([&os](const auto &v) { os << v; }, data);
    os << ")";
  }
  return os;
}
}
