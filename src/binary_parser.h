#pragma once

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

namespace metamix {

class BinaryParseError : public std::runtime_error
{
private:
  size_t m_bytes_consumed;
  size_t m_bytes_left;

public:
  explicit BinaryParseError(const char *msg, size_t bytes_consumed = 0, size_t bytes_left = 0)
    : std::runtime_error(msg)
    , m_bytes_consumed{ bytes_consumed }
    , m_bytes_left{ bytes_left }
  {}

  explicit BinaryParseError(std::string msg, size_t bytes_consumed = 0, size_t bytes_left = 0)
    : std::runtime_error(std::move(msg))
    , m_bytes_consumed{ bytes_consumed }
    , m_bytes_left{ bytes_left }
  {}

  size_t bytes_consumed() const { return m_bytes_consumed; }

  void bytes_consumed(size_t bytes_consumed) { m_bytes_consumed = bytes_consumed; }

  size_t bytes_left() const { return m_bytes_left; }

  void bytes_left(size_t bytes_left) { m_bytes_left = bytes_left; }

  friend std::ostream &operator<<(std::ostream &os, const BinaryParseError &error)
  {
    return os << error.what() << " ["
              << "Bytes consumed: " << error.m_bytes_consumed << ", "
              << "Bytes left: " << error.m_bytes_left << "]";
  }
};

class BinaryParserContext
{
private:
  const uint8_t *m_startptr;
  const uint8_t *m_endptr;

public:
  constexpr BinaryParserContext(const uint8_t *startptr, const uint8_t *endptr)
    : m_startptr(startptr)
    , m_endptr(endptr)
  {}

  BinaryParserContext(const std::vector<uint8_t> &vec)
    : m_startptr(vec.data())
    , m_endptr(vec.data() + vec.size())
  {}

  constexpr const uint8_t *startptr() const { return m_startptr; }

  constexpr const uint8_t *endptr() const { return m_endptr; }
};

class BinaryParserBounds
{
private:
  const uint8_t *m_startptr;
  size_t m_length;

public:
  constexpr BinaryParserBounds(const uint8_t *startptr, size_t length)
    : m_startptr(startptr)
    , m_length(length)
  {}

  constexpr const uint8_t *startptr() const { return m_startptr; }

  constexpr size_t length() const { return m_length; }
};

template<class Packet,
         class Context,
         class Bounds,

         /// \brief Find next packet boundaries and shift.
         /// \return pointer to start of next element and its length, or std::nullopt on EOF
         /// \throw BinaryParseError on parsing error
         std::optional<Bounds> Next(const uint8_t *startptr, size_t length) noexcept(false),

         Packet Pack(const Context &ctx, const Bounds &bounds)>
class BinaryParser
{
private:
  using Self = BinaryParser<Packet, Context, Bounds, Next, Pack>;

public:
  using PacketType = Packet;
  using ContextType = Context;

private:
  Context m_ctx{};
  const uint8_t *m_start{};
  const uint8_t *m_data{};
  const uint8_t *m_end{};

public:
  BinaryParser() = default;

  explicit BinaryParser(Context ctx)
    : m_ctx(ctx)
    , m_start(m_ctx.startptr())
    , m_data(m_start)
    , m_end(m_ctx.endptr())
  {}

  BinaryParser(const BinaryParser &other) = default;
  BinaryParser(BinaryParser &&other) noexcept = default;

  virtual ~BinaryParser() = default;

  BinaryParser &operator=(const BinaryParser &rhs) = default;
  BinaryParser &operator=(BinaryParser &&rhs) noexcept(false) = default;

  std::optional<Packet> next() noexcept(false)
  {
    try {
      // Check whether there is any data left to parse
      if (!has_next()) {
        return std::nullopt;
      }

      // Find next packet
      auto result_opt = Next(m_data, m_end - m_data);
      if (!result_opt) {
        return std::nullopt;
      }

      const Bounds &bounds = *result_opt;

      assert(m_data <= bounds.startptr());
      assert(bounds.startptr() + bounds.length() <= m_end);
      assert(bounds.length() > 0);

      // Shift
      m_data = bounds.startptr() + bounds.length();

      // Pack the packet
      return Pack(m_ctx, bounds);
    } catch (BinaryParseError &err) {
      // Inject position information
      err.bytes_consumed(bytes_consumed());
      err.bytes_left(bytes_left());
      throw;
    }
  }

  bool has_next() const noexcept(false) { return m_data < m_end; }

  size_t bytes_consumed() const { return m_data - m_start; }

  size_t bytes_left() const { return m_end - m_data; }

  explicit operator bool() const noexcept(false) { return has_next(); }

  bool operator==(const BinaryParser &rhs) const
  {
    return std::tie(m_ctx, m_start, m_data, m_end) == std::tie(rhs.m_ctx, rhs.m_start, rhs.m_data, rhs.m_end);
  }

  bool operator!=(const BinaryParser &rhs) const { return !(rhs == *this); }

  Self &operator>>(Packet &pkt)
  {
    pkt = std::move(*next());
    return *this;
  }

public:
  template<typename... Args>
  static Self create(Args &&... args)
  {
    return Self(Context(std::forward<Args>(args)...));
  }
};
}
