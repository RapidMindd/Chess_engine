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

BOOST_AUTO_TEST_CASE(knight_moves)
{
  Position pos;
  pos.setInitial();
  MoveArray moves;
  MoveGenerator generator;
  generator.generateKnightMoves(pos, B1, moves);
  generator.generateKnightMoves(pos, G1, moves);
  Square squares[4] = {C3, A3, H3, F3};
  for (size_t i = 0; i < 4; ++i)
  {
    BOOST_TEST(moves.get(i).to_ == squares[i]);
  }
  BOOST_TEST(moves.size() == 4);

  pos.clear();
  pos.placePiece(D4, WHITE_KNIGHT);
  moves.clear();
  generator.generateKnightMoves(pos, D4, moves);
  Square squares1[8] = {E6, F5, F3, E2, C2, B3, B5, C6};
  for (size_t i = 0; i < 8; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({D4, squares1[i]}));
  }

  pos.clear();
  pos.placePiece(D4, WHITE_KNIGHT);
  pos.placePiece(F3, WHITE_PAWN);
  moves.clear();
  generator.generateKnightMoves(pos, D4, moves);
  Square squares2[7] = {E6, F5, E2, C2, B3, B5, C6};
  for (size_t i = 0; i < 7; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({D4, squares2[i]}));
  }

  pos.clear();
  pos.placePiece(D4, WHITE_KNIGHT);
  pos.placePiece(F3, BLACK_PAWN);
  moves.clear();
  generator.generateKnightMoves(pos, D4, moves);
  Square squares3[8] = {E6, F5, F3, E2, C2, B3, B5, C6};
  for (size_t i = 0; i < 8; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({D4, squares3[i]}));
  }

  pos.clear();
  pos.placePiece(H1, WHITE_KNIGHT);
  moves.clear();
  generator.generateKnightMoves(pos, H1, moves);
  Square squares4[2] = {F2, G3};
  for (size_t i = 0; i < 2; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({H1, squares4[i]}));
  }

  pos.setInitial();
  moves.clear();
  generator.generateKnightMoves(pos, B8, moves);
  generator.generateKnightMoves(pos, G8, moves);
  Square squares6[4] = {C6, A6, H6, F6};
  for (size_t i = 0; i < 4; ++i)
  {
    BOOST_TEST(moves.get(i).to_ == squares6[i]);
  }
}

BOOST_AUTO_TEST_CASE(bishop_moves)
{
  Position pos;
  pos.setInitial();
  MoveArray moves;
  MoveGenerator generator;
  generator.generateBishopMoves(pos, C1, moves);
  generator.generateBishopMoves(pos, F1, moves);
  BOOST_TEST(moves.size() == 0);

  pos.clear();
  pos.placePiece(D4, WHITE_BISHOP);
  moves.clear();
  generator.generateBishopMoves(pos, D4, moves);
  Square squares[13] = {E5, F6, G7, H8, E3, F2, G1, C3, B2, A1, C5, B6, A7};
  for (size_t i = 0; i < 13; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({D4, squares[i]}));
  }
  BOOST_TEST(moves.size() == 13);

  pos.clear();
  pos.placePiece(E4, WHITE_BISHOP);
  pos.placePiece(C6, BLACK_PAWN);
  pos.placePiece(F5, WHITE_PAWN);
  moves.clear();
  Square squares2[8] = {F3, G2, H1, D3, C2, B1, D5, C6};
  generator.generateBishopMoves(pos, E4, moves);
  for (size_t i = 0; i < 8; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({E4, squares2[i]}));
  }
  BOOST_TEST(moves.size() == 8);

  pos.clear();
  pos.placePiece(E4, BLACK_BISHOP);
  pos.placePiece(C6, WHITE_PAWN);
  pos.placePiece(F5, BLACK_PAWN);
  moves.clear();
  generator.generateBishopMoves(pos, E4, moves);
  for (size_t i = 0; i < 8; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({E4, squares2[i]}));
  }
  BOOST_TEST(moves.size() == 8);
}

BOOST_AUTO_TEST_CASE(queen_moves)
{
  Position pos;
  pos.setInitial();
  MoveArray moves;
  MoveGenerator generator;
  generator.generateQueenMoves(pos, D1, moves);
  BOOST_TEST(moves.size() == 0);

  pos.clear();
  pos.placePiece(A1, WHITE_QUEEN);
  pos.placePiece(F6, BLACK_PAWN);
  pos.placePiece(G1, WHITE_ROOK);
  moves.clear();
  generator.generateQueenMoves(pos, A1, moves);
  Square squares[17] = {A2, A3, A4, A5, A6, A7, A8, B1, C1, D1, E1, F1, B2, C3, D4, E5, F6};
  for (size_t i = 0; i < 17; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({A1, squares[i]}));
  }
  BOOST_TEST(moves.size() == 17);

  pos.clear();
  pos.placePiece(A1, BLACK_QUEEN);
  pos.placePiece(F6, WHITE_PAWN);
  pos.placePiece(G1, BLACK_ROOK);
  moves.clear();
  generator.generateQueenMoves(pos, A1, moves);
  for (size_t i = 0; i < 17; ++i)
  {
    BOOST_TEST(moves.get(i) == Move({A1, squares[i]}));
  }
  BOOST_TEST(moves.size() == 17);
}
