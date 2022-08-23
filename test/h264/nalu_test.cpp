#include <boost/test/unit_test.hpp>

#include <src/h264/nalu.h>

using namespace metamix;
using namespace metamix::h264;

OwnedNalu
pps()
{
  return OwnedNalu{ 0x68, 0xca, 0xe1, 0xbc, 0xb0 };
}

OwnedNalu
sei()
{
  return OwnedNalu{ 0x06, 0x00, 0x06, 0x8a, 0x38, 0x70, 0xbc, 0x0a, 0xc0, 0x80 };
}

BOOST_AUTO_TEST_SUITE(nalu_test)

BOOST_AUTO_TEST_CASE(is_valid)
{
  BOOST_TEST(pps().is_valid());
  BOOST_TEST(sei().is_valid());
  BOOST_TEST(!OwnedNalu{}.is_valid());
  BOOST_TEST(!OwnedNalu{ 0b1'11'00011 }.is_valid());
}

BOOST_AUTO_TEST_CASE(type)
{
  BOOST_TEST(pps().type() == NaluType::PPS);
  BOOST_TEST(sei().type() == NaluType::SEI);
}

BOOST_AUTO_TEST_CASE(is_iterable)
{
  auto nalu = pps();
  std::fill(nalu.begin(), nalu.end(), 0x68);
  BOOST_TEST(std::all_of(nalu.cbegin(), nalu.cend(), [](auto x) { return x == 0x68; }));
}

BOOST_AUTO_TEST_SUITE_END()
