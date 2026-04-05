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

  pos.clear();
  pos.placePiece(D4, BLACK_KING);
  pos.placePiece(E4, BLACK_PAWN);
  moves.clear();
  generator.generateKingMoves(pos, D4, moves);
  Square squares5[7] = {D5, E5, E3, D3, C3, C4, C5};
  for (size_t i = 0; i < 7; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({D4, squares5[i]}));
  }

  pos.clear();
  pos.placePiece(D4, BLACK_KING);
  pos.placePiece(E4, WHITE_PAWN);
  moves.clear();
  generator.generateKingMoves(pos, D4, moves);
  Square squares6[8] = {D5, E5, E4, E3, D3, C3, C4, C5};
  for (size_t i = 0; i < 8; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({D4, squares6[i]}));
  }
}

BOOST_AUTO_TEST_CASE(rook_moves)
{
  Position pos;
  pos.setInitial();
  MoveArray moves;
  MoveGenerator generator;
  generator.generateRookMoves(pos, A1, moves);
  generator.generateRookMoves(pos, H1, moves);
  BOOST_TEST(moves.size() == 0);

  pos.clear();
  pos.placePiece(D4, WHITE_ROOK);
  moves.clear();
  generator.generateRookMoves(pos, D4, moves);
  Square squares[14] = {D5, D6, D7, D8, D3, D2, D1,
  E4, F4, G4, H4, C4, B4, A4};
  for (size_t i = 0; i < 14; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({D4, squares[i]}));
  }
  BOOST_TEST(moves.size() == 14);

  pos.clear();
  pos.placePiece(D4, WHITE_ROOK);
  pos.placePiece(B4, WHITE_PAWN);
  pos.placePiece(D6, BLACK_PAWN);
  moves.clear();
  generator.generateRookMoves(pos, D4, moves);
  Square squares2[10] = {D5, D6, D3, D2, D1, E4, F4, G4, H4, C4};
  for (size_t i = 0; i < 10; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({D4, squares2[i]}));
  }
  BOOST_TEST(moves.size() == 10);

  pos.clear();
  pos.placePiece(D4, BLACK_ROOK);
  pos.placePiece(B4, BLACK_PAWN);
  pos.placePiece(D6, WHITE_PAWN);
  moves.clear();
  generator.generateRookMoves(pos, D4, moves);
  Square squares3[10] = {D5, D6, D3, D2, D1, E4, F4, G4, H4, C4};
  for (size_t i = 0; i < 10; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({D4, squares3[i]}));
  }
  BOOST_TEST(moves.size() == 10);
}
