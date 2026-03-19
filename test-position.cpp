#include <boost/test/unit_test.hpp>
#include "Position.hpp"

using namespace chess;

BOOST_AUTO_TEST_CASE(default_constructor)
{
  Position empty_pos;
  for (size_t i = 0; i < 64; ++i)
  {
    BOOST_TEST(empty_pos.getPiece(i) == EMPTY);
  }
  BOOST_TEST(empty_pos.isWhiteToMove() == true);
}

BOOST_AUTO_TEST_CASE(setInitial)
{
  Position pos;
  pos.setInitial();
  BOOST_TEST(pos.isWhiteToMove() == true);
  BOOST_TEST(pos.getPiece(E1) == WHITE_KING);
  BOOST_TEST(pos.getPiece(F8) == BLACK_BISHOP);
  BOOST_TEST(pos.getPiece(G5) == EMPTY);
}
