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
  UndoInfo undo;
  pos.makeMove({E2, E4}, undo);
  generator.generateKingMoves(pos, E1, moves);
  BOOST_TEST(moves.get(0) == Move({E1, E2}));
  BOOST_TEST(moves.size() == 1);

  pos.clear();
  pos.placePiece(D4, WHITE_KING);
  moves.clear();
  generator.generateKingMoves(pos, D4, moves);
  Square square = D4;
  Square squares[8] = {D5, E5, E4, E3, D3, C3, C4, C5};
  for (size_t i = 0; i < 8; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({D4, squares[i]}));
  }
}
