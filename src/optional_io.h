// boost/optional/optional_io.hpp modified for std::optional

// Copyright (C) 2005, Fernando Luis Cacciola Carballal.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/optional for documentation.
//
// You are welcome to contact the author at:
//  fernando_cacciola@hotmail.com

#pragma once

#include <istream>
#include <optional>
#include <ostream>

namespace std {

template<class CharType, class CharTrait>
inline basic_ostream<CharType, CharTrait> &
operator<<(basic_ostream<CharType, CharTrait> &out, nullopt_t)
{
  if (out.good()) {
    out << "--";
  }

  return out;
}

template<class CharType, class CharTrait, class T>
inline basic_ostream<CharType, CharTrait> &
operator<<(basic_ostream<CharType, CharTrait> &out, const optional<T> &v)
{
  if (out.good()) {
    if (!v)
      out << "--";
    else
      out << ' ' << *v;
  }

  return out;
}

template<class CharType, class CharTrait, class T>
inline basic_istream<CharType, CharTrait> &
operator>>(basic_istream<CharType, CharTrait> &in, optional<T> &v)
{
  if (in.good()) {
    int d = in.get();
    if (d == ' ') {
      T x;
      in >> x;
      v = move(x);
    } else {
      if (d == '-') {
        d = in.get();

        if (d == '-') {
          v = nullopt;
          return in;
        }
      }

      in.setstate(ios::failbit);
    }
  }

  return in;
}
}
