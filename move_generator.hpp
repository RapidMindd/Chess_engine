#ifndef MOVE_GENERATOR_HPP
#define MOVE_GENERATOR_HPP

#include "position.hpp"
#include "move.hpp"

namespace chess
{
  struct MoveGenerator
  {
    static MoveArray generatePseudoLegalMoves(const Position& pos, bool castling = 1);
    static MoveArray generateLegalMoves(const Position& pos);

    // методы генерируют псевдолегальные ходы
    static void generateKingMoves(const Position& pos, Square square, MoveArray& moves);
    static void generateQueenMoves(const Position& pos, Square square, MoveArray& moves);
    static void generateKnightMoves(const Position& pos, Square square, MoveArray& moves);
    static void generateBishopMoves(const Position& pos, Square square, MoveArray& moves);
    static void generateRookMoves(const Position& pos, Square square, MoveArray& moves);
    static void generatePawnMoves(const Position& pos, Square square, MoveArray& moves);
    static void generateCastlingMoves(const Position& pos, Square square, MoveArray& moves);

    static bool isSquareAttacked(const Position& pos, Square square);

    static bool isMate(const Position& pos);
    static bool isStaleMate(const Position& pos);

    static bool isMateUnsafe(const Position& pos);
  };
}

#endif
