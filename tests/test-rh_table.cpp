#include <boost/test/unit_test.hpp>
#include <string>
#include "datastructures/robinHoodTable.hpp"

using namespace tarasenko;

using HTable = RobinHoodTable< int, int >;

struct ConstHash
{
  size_t operator()(int) const
  {
    return 777;
  }
};

using CollisionTable = RobinHoodTable< int, int, ConstHash >;

BOOST_AUTO_TEST_CASE(hood_default_constructor)
{
  HTable table;
  BOOST_TEST(table.empty());
  BOOST_TEST(table.size() == 0ul);
  BOOST_TEST(table.bucket_count() > 0ul);
}

BOOST_AUTO_TEST_CASE(bucket_count_constructor)
{
  HTable table(16);
  BOOST_TEST(table.empty());
  BOOST_TEST(table.size() == 0ul);
  BOOST_TEST(table.bucket_count() == 16ul);
}

BOOST_AUTO_TEST_CASE(empty_table)
{
  HTable table(0);
  BOOST_TEST(table.bucket_count() == 1ul);
}

BOOST_AUTO_TEST_CASE(insert)
{
  HTable table;
  BOOST_TEST(table.insert({1, 1}));
  BOOST_TEST(table.size() == 1ul);
  BOOST_TEST(!table.empty());
}

BOOST_AUTO_TEST_CASE(find)
{
  HTable table;
  table.insert({1, 1});
  BOOST_CHECK(table.find(1) != table.end());
  BOOST_TEST(table.find(1)->second == 1);
  BOOST_TEST(table.size() == 1ul);
}

BOOST_AUTO_TEST_CASE(find_empty)
{
  HTable table;
  BOOST_CHECK(table.find(1) == table.end());
}

BOOST_AUTO_TEST_CASE(count)
{
  HTable table;
  table.insert({1, 1});
  BOOST_TEST(table.count(1) == 1ul);
  BOOST_TEST(table.count(2) == 0ul);
}

BOOST_AUTO_TEST_CASE(at)
{
  HTable table;
  table.insert({1, 1});
  BOOST_TEST(table.at(1) == 1);
}

BOOST_AUTO_TEST_CASE(at_empty)
{
  HTable table;
  BOOST_CHECK_THROW(table.at(1), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(index_operator)
{
  HTable table;
  table[1] = 10;
  BOOST_TEST(table.size() == 1ul);
  BOOST_TEST(table.at(1) == 10);
}

BOOST_AUTO_TEST_CASE(default_template_parameters)
{
  RobinHoodTable< int, std::string > table;
  table.insert({1, "hello"});
  BOOST_TEST(table.find(1)->second == "hello");
}

BOOST_AUTO_TEST_CASE(insert_elems_by_same_key)
{
  HTable table;
  BOOST_TEST(table.insert({1, 1}));
  BOOST_TEST(!table.insert({1, 2}));
  BOOST_TEST(table.size() == 1ul);
  BOOST_TEST(table.find(1)->second == 1);
}

BOOST_AUTO_TEST_CASE(collision_insert_and_find)
{
  CollisionTable table;
  table.insert({1, 10});
  table.insert({2, 20});
  table.insert({3, 30});
  BOOST_TEST(table.size() == 3ul);
  BOOST_TEST(table.find(1)->second == 10);
  BOOST_TEST(table.find(2)->second == 20);
  BOOST_TEST(table.find(3)->second == 30);
}
