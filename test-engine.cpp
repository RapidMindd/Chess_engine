#include <boost/test/unit_test.hpp>
#include "engine.hpp"

using namespace chess;

BOOST_AUTO_TEST_CASE(mate_in_1)
{
  Position pos({
    {H6, WHITE_KING}, {H8, BLACK_KING}, {F1, WHITE_ROOK}
  });
  BOOST_TEST(Engine{}.findBestMove(pos, 4).first == Move({F1, F8}));
}

BOOST_AUTO_TEST_CASE(mate_in_1_from_depth_1)
{
  Position pos({
    {H6, WHITE_KING}, {H8, BLACK_KING}, {F1, WHITE_ROOK}
  });
  BOOST_TEST(Engine{}.findBestMove(pos, 1).first == Move({F1, F8}));
}
