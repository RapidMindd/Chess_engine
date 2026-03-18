#include <boost/test/unit_test.hpp>
#include "Position.hpp"

BOOST_AUTO_TEST_CASE(default_constructor)
{
  chess::Position empty_pos;
  for (size_t i = 0; i < 64; ++i)
  {
    BOOST_TEST(empty_pos.getPiece(i) == 0);
  }
  BOOST_TEST(empty_pos.isWhiteToMove() == true);
}
