#include <boost/test/unit_test.hpp>

#include <src/clock.h>

using namespace metamix;

BOOST_AUTO_TEST_SUITE(clock_test)

BOOST_AUTO_TEST_CASE(increment)
{
  Clock c(10_clock);
  BOOST_TEST(c.now() == 10_clock);
  c += 10_clock;
  BOOST_TEST(c.now() == 20_clock);
}

BOOST_AUTO_TEST_CASE(negative_increment_is_noop)
{
  Clock c(10_clock);
  BOOST_TEST(c.now() == 10_clock);
  c += ClockTS(-10);
  BOOST_TEST(c.now() == 10_clock);
}

BOOST_AUTO_TEST_SUITE_END()
