#include <boost/test/unit_test.hpp>
#include "move_generator.hpp"

using namespace chess;

BOOST_AUTO_TEST_CASE(king_moves)
{
  Position pos;
  pos.setInitial();
  MoveArray moves;
  MoveGenerator generator;
  generator.generateKingMoves(pos, E1, moves);
  BOOST_TEST(moves.size() == 0);
}