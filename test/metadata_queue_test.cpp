#include <boost/test/unit_test.hpp>

#include <boost/test/test_tools.hpp>

#include <src/metadata_kind.h>
#include <src/metadata_queue.h>
#include <src/optional_io.h>

using namespace metamix;

METAMIX_METADATA_KIND(TestKind, test, test, "TEST", "Test")
METAMIX_METADATA_KIND(AltKind, alt, alt, "ALT", "Alt")

METAMIX_METADATA_KIND_MAP_TO_VALUE(TestKind, ClockTS)
METAMIX_METADATA_KIND_MAP_TO_VALUE(AltKind, ClockTS)

using TestMetadata = Metadata<TestKind>;
using AltMetadata = Metadata<AltKind>;
using TestMetadataQueue = MetadataQueue<TestKind>;
using TestMetadataQueueGroup = MetadataQueueGroup<TestKind, AltKind>;

static constexpr ClockTS
nts(float i)
{
  return static_cast<ClockTS>(100.0f * i);
}

template<class K>
static Metadata<K>
nthk(float i, InputId input_id = 0, int order = 0)
{
  return Metadata<K>(input_id, nts(i), nts(i), order, std::make_shared<ClockTS>(nts(i)));
}

static TestMetadata
nth(float i, InputId input_id = 0, int order = 0)
{
  return nthk<TestKind>(i, input_id, order);
}

template<int Size>
static TestMetadataQueue
make_tsq()
{
  TestMetadataQueue q;
  for (int i = 1; i <= Size; i++) {
    q.push(nth(i));
  }
  return q;
}

BOOST_AUTO_TEST_SUITE(timestamp_queue_test)

BOOST_AUTO_TEST_CASE(empty)
{
  auto q = make_tsq<0>();
  BOOST_TEST(q.empty());
  BOOST_TEST(q.size() == 0);
}

BOOST_AUTO_TEST_CASE(not_empty)
{
  auto q = make_tsq<1>();
  BOOST_TEST(!q.empty());
  BOOST_TEST(q.size() == 1);
}

BOOST_AUTO_TEST_CASE(pop_head)
{
  auto q = make_tsq<3>();
  BOOST_TEST(q.pop(0, nts(0.5), nts(1.5)) == nth(1));
  BOOST_TEST(q.size() == 2);
  BOOST_TEST(q.top() == nth(2));
}

BOOST_AUTO_TEST_CASE(pop_head_id_before)
{
  auto q = make_tsq<3>();
  BOOST_TEST(!q.pop(-1, nts(0.5), nts(1.5)));
  BOOST_TEST(q.size() == 2);
  BOOST_TEST(q.top() == nth(2));
}

BOOST_AUTO_TEST_CASE(pop_head_id_after)
{
  auto q = make_tsq<3>();
  BOOST_TEST(!q.pop(1, nts(0.5), nts(1.5)));
  BOOST_TEST(q.size() == 2);
  BOOST_TEST(q.top() == nth(2));
}

BOOST_AUTO_TEST_CASE(pop_head_first_id)
{
  auto q = make_tsq<3>();
  q.push(nth(1, 1));
  BOOST_TEST(q.pop(0, nts(0.5), nts(1.5)) == nth(1, 0));
  BOOST_TEST(q.size() == 2);
  BOOST_TEST(q.top() == nth(2));
}

BOOST_AUTO_TEST_CASE(pop_head_second_id)
{
  auto q = make_tsq<3>();
  q.push(nth(1, 1));
  BOOST_TEST(q.pop(1, nts(0.5), nts(1.5)) == nth(1, 1));
  BOOST_TEST(q.size() == 2);
  BOOST_TEST(q.top() == nth(2));
}

BOOST_AUTO_TEST_CASE(pop_middle)
{
  auto q = make_tsq<3>();
  BOOST_TEST(q.pop(0, nts(1.5), nts(2.5)) == nth(2));
  BOOST_TEST(q.size() == 1);
  BOOST_TEST(q.top() == nth(3));
}

BOOST_AUTO_TEST_CASE(pop_middle_id_before)
{
  auto q = make_tsq<3>();
  BOOST_TEST(!q.pop(-1, nts(1.5), nts(2.5)));
  BOOST_TEST(q.size() == 1);
  BOOST_TEST(q.top() == nth(3));
}

BOOST_AUTO_TEST_CASE(pop_middle_id_after)
{
  auto q = make_tsq<3>();
  BOOST_TEST(!q.pop(1, nts(1.5), nts(2.5)));
  BOOST_TEST(q.size() == 1);
  BOOST_TEST(q.top() == nth(3));
}

BOOST_AUTO_TEST_CASE(pop_middle_first_id)
{
  auto q = make_tsq<3>();
  q.push(nth(2, 1));
  BOOST_TEST(q.pop(0, nts(1.5), nts(2.5)) == nth(2, 0));
  BOOST_TEST(q.size() == 1);
  BOOST_TEST(q.top() == nth(3));
}

BOOST_AUTO_TEST_CASE(pop_middle_second_id)
{
  auto q = make_tsq<3>();
  q.push(nth(2, 1));
  BOOST_TEST(q.pop(1, nts(1.5), nts(2.5)) == nth(2, 1));
  BOOST_TEST(q.size() == 1);
  BOOST_TEST(q.top() == nth(3));
}

BOOST_AUTO_TEST_CASE(pop_last)
{
  auto q = make_tsq<3>();
  BOOST_TEST(q.pop(0, nts(2.5), nts(3.5)) == nth(3));
  BOOST_TEST(q.empty());
}

BOOST_AUTO_TEST_CASE(pop_last_id_before)
{
  auto q = make_tsq<3>();
  BOOST_TEST(!q.pop(-1, nts(2.5), nts(3.5)));
  BOOST_TEST(q.empty());
}

BOOST_AUTO_TEST_CASE(pop_last_id_after)
{
  auto q = make_tsq<3>();
  BOOST_TEST(!q.pop(1, nts(2.5), nts(3.5)));
  BOOST_TEST(q.empty());
}

BOOST_AUTO_TEST_CASE(pop_last_first_id)
{
  auto q = make_tsq<3>();
  q.push(nth(3, 1));
  BOOST_TEST(q.pop(0, nts(2.5), nts(3.5)) == nth(3, 0));
  BOOST_TEST(q.empty());
}

BOOST_AUTO_TEST_CASE(pop_last_second_id)
{
  auto q = make_tsq<3>();
  q.push(nth(3, 1));
  BOOST_TEST(q.pop(1, nts(2.5), nts(3.5)) == nth(3, 1));
  BOOST_TEST(q.empty());
}

BOOST_AUTO_TEST_CASE(pop_long)
{
  auto q = make_tsq<3>();
  BOOST_TEST(q.pop(0, nts(1.5), nts(5)) == nth(2));
  BOOST_TEST(q.size() == 1);
  BOOST_TEST(q.top() == nth(3));
}

BOOST_AUTO_TEST_CASE(pop_before)
{
  auto q = make_tsq<1>();
  BOOST_TEST(!q.pop(0, nts(0.25), nts(0.75)));
  BOOST_TEST(q.size() == 1);
  BOOST_TEST(q.top() == nth(1));
}

BOOST_AUTO_TEST_CASE(pop_after)
{
  auto q = make_tsq<1>();
  BOOST_TEST(!q.pop(0, nts(5), nts(6)));
  BOOST_TEST(q.empty());
}

BOOST_AUTO_TEST_CASE(pop_all_pops_all)
{
  std::vector<TestMetadata> expected;
  expected.push_back(nth(1));
  expected.push_back(nth(2));
  expected.push_back(nth(3));

  TestMetadataQueue q(expected.cbegin(), expected.cend());

  std::vector<TestMetadata> actual;
  q.pop_all(0, nts(1), nts(4), std::back_inserter(actual));

  BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(pop_all_drops_other_inputs)
{
  std::vector<TestMetadata> expected;
  expected.push_back(nth(1, 0));
  expected.push_back(nth(2, 0));
  expected.push_back(nth(3, 0));

  TestMetadataQueue q;
  q.push(nth(0, 0));
  q.push(nth(0, 1));

  q.push(nth(1, 0));
  q.push(nth(2, 0));
  q.push(nth(3, 0));
  q.push(nth(1, 2));
  q.push(nth(2, 1));
  q.push(nth(3, 2));

  q.push(nth(4, 0));
  q.push(nth(4, 2));

  std::vector<TestMetadata> actual;
  q.pop_all(0, nts(1), nts(4), std::back_inserter(actual));

  BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
  BOOST_TEST(q.size() == 2);
}

BOOST_AUTO_TEST_CASE(drop_id)
{
  TestMetadataQueue q;
  q.push(nth(2, 0));
  q.push(nth(1, 1));
  q.push(nth(2, 1));

  auto dropped = q.drop_id(1);
  BOOST_TEST(dropped == 2);
  BOOST_TEST(q.size() == 1);
}

BOOST_AUTO_TEST_CASE(group_drop_id)
{
  TestMetadataQueueGroup q;
  q.push(nthk<TestKind>(2, 0));
  q.push(nthk<TestKind>(1, 1));
  q.push(nthk<TestKind>(2, 1));
  q.push(nthk<AltKind>(2, 1));
  q.push(nthk<AltKind>(1, 0));
  q.push(nthk<AltKind>(2, 0));

  auto dropped = q.drop_id(1);
  BOOST_TEST(dropped == 3);
  BOOST_TEST(q.get<TestKind>().size() == 1);
  BOOST_TEST(q.get<AltKind>().size() == 2);
}

BOOST_AUTO_TEST_SUITE_END()
