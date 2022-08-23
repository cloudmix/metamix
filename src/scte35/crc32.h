#pragma once

#include <cstdint>
#include <iterator>

namespace metamix::scte35 {

class CRC32
{
private:
  using Self = CRC32;

  uint64_t m_value{ 0xffff'ffff };

public:
  void update(uint8_t byte);

  template<class InputIt>
  void update(InputIt begin, InputIt end)
  {
    static_assert(
      std::is_base_of<std::input_iterator_tag, typename std::iterator_traits<InputIt>::iterator_category>::value,
      "input iterator must be of input iterator category");

    while (begin != end) {
      update(*begin++);
    }
  }

  constexpr uint32_t value() const { return m_value & 0xffff'ffff; }
  constexpr operator uint32_t() const { return value(); }

  template<class InputIt>
  static inline Self compute(InputIt begin, InputIt end)
  {
    Self crc{};
    crc.update(begin, end);
    return crc;
  }
};

template<class InnerIt>
class CRC32OutputIterator
{
  static_assert(
    std::is_base_of<std::output_iterator_tag, typename std::iterator_traits<InnerIt>::iterator_category>::value,
    "inner iterator must be of output iterator category");

private:
  using Self = CRC32OutputIterator;

public:
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = void;
  using pointer = void;
  using reference = void;

private:
  InnerIt *m_inner;
  CRC32 *m_crc;

public:
  explicit CRC32OutputIterator(InnerIt &inner, CRC32 &crc32)
    : m_inner{ std::addressof(inner) }
    , m_crc{ std::addressof(crc32) }
  {}

  CRC32OutputIterator(const CRC32OutputIterator &) noexcept = default;
  CRC32OutputIterator(CRC32OutputIterator &&) noexcept = default;
  CRC32OutputIterator &operator=(const CRC32OutputIterator &) noexcept = default;
  CRC32OutputIterator &operator=(CRC32OutputIterator &&) noexcept = default;

  class OutputProxy
  {
  private:
    Self &self;

  public:
    explicit OutputProxy(Self &self)
      : self(self)
    {}

    OutputProxy &operator=(uint8_t value)
    {
      **self.m_inner = value;
      self.m_crc->update(value);
      return *this;
    }
  };

  OutputProxy operator*() { return OutputProxy(*this); }

  Self &operator++()
  {
    ++(*m_inner);
    return *this;
  }

  Self operator++(int)
  {
    Self tmp(*this);
    operator++();
    return tmp;
  }
};

template<class InnerIt>
inline CRC32OutputIterator<InnerIt>
make_crc32_output_iterator(InnerIt &inner, CRC32 &crc32)
{
  return CRC32OutputIterator<InnerIt>(inner, crc32);
}
}
