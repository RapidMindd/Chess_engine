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
  Square squares[8] = {D5, E5, E4, E3, D3, C3, C4, C5};
  for (size_t i = 0; i < 8; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({D4, squares[i]}));
  }

  pos.clear();
  pos.placePiece(D4, WHITE_KING);
  pos.placePiece(E4, WHITE_PAWN);
  moves.clear();
  generator.generateKingMoves(pos, D4, moves);
  Square squares2[7] = {D5, E5, E3, D3, C3, C4, C5};
  for (size_t i = 0; i < 7; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({D4, squares2[i]}));
  }

  pos.clear();
  pos.placePiece(D4, WHITE_KING);
  pos.placePiece(E4, BLACK_PAWN);
  moves.clear();
  generator.generateKingMoves(pos, D4, moves);
  Square squares3[8] = {D5, E5, E4, E3, D3, C3, C4, C5};
  for (size_t i = 0; i < 8; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({D4, squares3[i]}));
  }

  pos.clear();
  pos.placePiece(H2, WHITE_KING);
  moves.clear();
  generator.generateKingMoves(pos, H2, moves);
  Square squares4[5] = {H3, H1, G1, G2, G3};
  for (size_t i = 0; i < 5; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({H2, squares4[i]}));
  }
}
