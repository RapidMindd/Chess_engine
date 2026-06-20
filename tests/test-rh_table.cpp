#include <boost/test/unit_test.hpp>
#include "datastructures/robinHoodTable.hpp"

using namespace tarasenko;

struct ModHash
{
  size_t operator()(int val) const
  {
    return val % 4;
  }
};

BOOST_AUTO_TEST_CASE(table_default_constructor)
{
  RobinHoodTable< int, int > table;
  BOOST_TEST(table.empty());
  BOOST_TEST(table.size() == 0ul);
  BOOST_TEST(table.bucket_count() > 0ul);
}

BOOST_AUTO_TEST_CASE(bucket_count_constructor)
{
  RobinHoodTable< int, int > table(16);
  BOOST_TEST(table.empty());
  BOOST_TEST(table.size() == 0ul);
  BOOST_TEST(table.bucket_count() == 16ul);
}

BOOST_AUTO_TEST_CASE(insert_and_find)
{
  RobinHoodTable< int, int > table;
  BOOST_TEST(table.insert({1, 10}));
  BOOST_TEST(table.insert({2, 20}));
  BOOST_TEST(!table.empty());
  BOOST_TEST(table.size() == 2ul);
  BOOST_CHECK(table.find(1) != table.end());
  BOOST_TEST(table.find(1)->second == 10);
  BOOST_CHECK(table.find(3) == table.end());
}

BOOST_AUTO_TEST_CASE(insert_collision)
{
  RobinHoodTable< int, int, ModHash > table(8);
  BOOST_TEST(table.insert({1, 10}));
  BOOST_TEST(table.insert({5, 50}));
  BOOST_TEST(table.insert({9, 90}));
  BOOST_TEST(table.size() == 3ul);
  BOOST_TEST(table.find(1)->second == 10);
  BOOST_TEST(table.find(5)->second == 50);
  BOOST_TEST(table.find(9)->second == 90);
}

BOOST_AUTO_TEST_CASE(insert_duplicate)
{
  RobinHoodTable< int, int > table;
  BOOST_TEST(table.insert({1, 10}));
  BOOST_TEST(!table.insert({1, 20}));
  BOOST_TEST(table.size() == 1ul);
  BOOST_TEST(table.find(1)->second == 10);
}

BOOST_AUTO_TEST_CASE(insert_duplicate_after_collision)
{
  RobinHoodTable< int, int, ModHash > table(8);
  BOOST_TEST(table.insert({1, 10}));
  BOOST_TEST(table.insert({5, 50}));
  BOOST_TEST(!table.insert({5, 500}));
  BOOST_TEST(table.size() == 2ul);
  BOOST_TEST(table.find(5)->second == 50);
}
