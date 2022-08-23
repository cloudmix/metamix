#pragma once

#include <cstdint>
#include <type_traits>

#include <boost/rational.hpp>

namespace metamix {

/// Holds ~649.5 average Gregorian millennia assuming time base = 1/SYS_CLOCK_RATE.
using TS = int64_t;
using UTS = std::make_unsigned_t<TS>;
using TimeBase = boost::rational<std::int_fast32_t>;

constexpr TS SYS_CLOCK_RATE = 90'000;

#define CLOCK_STRONG_TYPEDEF(D, T)                                                                                     \
  T val{};                                                                                                             \
                                                                                                                       \
  constexpr D() = default;                                                                                             \
                                                                                                                       \
  constexpr explicit D(const T &val) noexcept                                                                          \
    : val(val)                                                                                                         \
  {}                                                                                                                   \
                                                                                                                       \
  constexpr D &operator=(const T &val) noexcept                                                                        \
  {                                                                                                                    \
    this->val = val;                                                                                                   \
    return *this;                                                                                                      \
  }                                                                                                                    \
                                                                                                                       \
  constexpr D(const D &) noexcept = default;                                                                           \
  constexpr D(D &&) noexcept = default;                                                                                \
                                                                                                                       \
  constexpr D &operator=(const D &) noexcept = default;                                                                \
  constexpr D &operator=(D &&) noexcept = default;                                                                     \
                                                                                                                       \
  constexpr operator const T &() const noexcept { return val; }                                                        \
  operator T &() noexcept { return val; }                                                                              \
                                                                                                                       \
  bool operator==(const D &rhs) const noexcept { return val == rhs.val; }                                              \
  bool operator!=(const D &rhs) const noexcept { return val != rhs.val; }                                              \
  bool operator<(const D &rhs) const noexcept { return val < rhs.val; }                                                \
  bool operator>(const D &rhs) const noexcept { return val > rhs.val; }                                                \
  bool operator<=(const D &rhs) const noexcept { return val <= rhs.val; }                                              \
  bool operator>=(const D &rhs) const noexcept { return val >= rhs.val; }

#define CLOCK_STRONG_TYPEDEF_ARITH(D, T)                                                                               \
  D &operator+=(const D &rhs) noexcept                                                                                 \
  {                                                                                                                    \
    val += rhs.val;                                                                                                    \
    return *this;                                                                                                      \
  }                                                                                                                    \
  D &operator-=(const D &rhs) noexcept                                                                                 \
  {                                                                                                                    \
    val -= rhs.val;                                                                                                    \
    return *this;                                                                                                      \
  }                                                                                                                    \
  D &operator*=(const D &rhs) noexcept                                                                                 \
  {                                                                                                                    \
    val *= rhs.val;                                                                                                    \
    return *this;                                                                                                      \
  }                                                                                                                    \
  D &operator/=(const D &rhs) noexcept                                                                                 \
  {                                                                                                                    \
    val /= rhs.val;                                                                                                    \
    return *this;                                                                                                      \
  }                                                                                                                    \
                                                                                                                       \
  friend D operator+(D lhs, const D &rhs) noexcept { return D(lhs.val + rhs.val); }                                    \
  friend D operator-(D lhs, const D &rhs) noexcept { return D(lhs.val - rhs.val); }                                    \
  friend D operator*(D lhs, const D &rhs) noexcept { return D(lhs.val * rhs.val); }                                    \
  friend D operator/(D lhs, const D &rhs) noexcept { return D(lhs.val / rhs.val); }

struct StreamTimeBase
{
  CLOCK_STRONG_TYPEDEF(StreamTimeBase, TimeBase);
  StreamTimeBase(TimeBase::int_type n, TimeBase::int_type d)
    : val(n, d)
  {}

  friend std::ostream &operator<<(std::ostream &os, const StreamTimeBase &tb)
  {
    return os << "StreamTimeBase{" << tb.val.numerator() << "/" << tb.val.denominator() << "}";
  }
};

struct ClockTimeBase
{
  CLOCK_STRONG_TYPEDEF(ClockTimeBase, TimeBase);
  ClockTimeBase(TimeBase::int_type n, TimeBase::int_type d)
    : val(n, d)
  {}

  friend std::ostream &operator<<(std::ostream &os, const ClockTimeBase &tb)
  {
    return os << "ClockTimeBase{" << tb.val.numerator() << "/" << tb.val.denominator() << "}";
  }
};

/// Timestamp expressed in stream time base.
struct StreamTS
{
  CLOCK_STRONG_TYPEDEF(StreamTS, TS);
  CLOCK_STRONG_TYPEDEF_ARITH(StreamTS, TS);

  using TimeBaseType = StreamTimeBase;

  friend std::ostream &operator<<(std::ostream &os, const StreamTS &tb) { return os << "StreamTS{" << tb.val << "}"; }
};

constexpr StreamTS operator"" _stream(unsigned long long ts)
{
  return StreamTS(TS(ts));
}

/// Timestamp expressed in global clock time base.
struct ClockTS
{
  CLOCK_STRONG_TYPEDEF(ClockTS, TS);
  CLOCK_STRONG_TYPEDEF_ARITH(ClockTS, TS);

  using TimeBaseType = ClockTimeBase;

  friend std::ostream &operator<<(std::ostream &os, const ClockTS &tb) { return os << "ClockTS{" << tb.val << "}"; }
};

constexpr ClockTS operator"" _clock(unsigned long long ts)
{
  return ClockTS(TS(ts));
}

#undef CLOCK_STRONG_TYPEDEF_ARITH
#undef CLOCK_STRONG_TYPEDEF
}
