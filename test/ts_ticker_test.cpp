#include <boost/test/unit_test.hpp>

#include <boost/test/data/monomorphic.hpp>
#include <boost/test/data/test_case.hpp>

#include <src/clock.h>

namespace data = boost::unit_test::data;
using namespace metamix;

BOOST_AUTO_TEST_SUITE(ts_ticker_test)

BOOST_DATA_TEST_CASE(tick,
                     data::make({
                       std::make_tuple(20, 30, 10),
                       std::make_tuple(0, 20, 20),
                       std::make_tuple(-10, 10, 20),
                       std::make_tuple(-20, 0, 20),
                       std::make_tuple(-30, -20, 10),
                       std::make_tuple(30, 30, 0),
                       std::make_tuple(30, 20, 0),
                       std::make_tuple(30, -20, 0),
                       std::make_tuple(-30, -40, 0),
                     }),
                     t1,
                     t2,
                     delta)
{
  auto c = std::make_shared<Clock>();
  TSTicker ts(c);
  ts.tick(ClockTS(t1));
  auto pnow = c->now();
  ts.tick(ClockTS(t2));
  BOOST_TEST(c->now() - pnow == delta);
}

BOOST_AUTO_TEST_CASE(tick2)
{
  auto c = std::make_shared<Clock>(0_clock);
  TSTicker ts(c);
  ts.tick(10_clock);
  BOOST_TEST(c->now() == 10_clock);
  ts.tick(20_clock);
  BOOST_TEST(c->now() == 20_clock);
}

BOOST_AUTO_TEST_CASE(multiple_consecutive_tickers)
{
  auto c = std::make_shared<Clock>();

  {
    TSTicker ts(c);
    ts.tick(10_clock);
    BOOST_TEST(c->now() == 10_clock);
    ts.tick(20_clock);
    BOOST_TEST(c->now() == 20_clock);
  }

  {
    TSTicker ts(c);
    ts.tick(40_clock);
    BOOST_TEST(c->now() == 60_clock);
    ts.tick(60_clock);
    BOOST_TEST(c->now() == 80_clock);
  }
}

BOOST_AUTO_TEST_SUITE_END()
