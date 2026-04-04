#include "move_generator.hpp"

namespace chess
{
  void MoveGenerator::generateKingMoves(const Position& pos, Square square, MoveArray& moves)
  {
    constexpr int possible_moves = 8;

    // начиная с клетки сверху, по часовой
    int squares[possible_moves] = {square + 8, square + 9, square + 1, square - 7, square - 8, square - 9, square - 1, square + 7};
    for (int i = 0; i < possible_moves; ++i)
    {
      if (((A1 <= squares[i]) && (squares[i]<= H8)) && (pos.getPiece(squares[i]) == EMPTY))
      {
        moves.push({square, static_cast< Square >(squares[i])});
      }
    }
  }

  /*void MoveGenerator::generateQueenMoves(const Position& pos, int square, MoveArray& moves)
  {

  }

  void MoveGenerator::generateKnightMoves(const Position& pos, int square, MoveArray& moves)
  {

  }

  void MoveGenerator::generateBishopMoves(const Position& pos, int square, MoveArray& moves)
  {

  }

  void MoveGenerator::generateRookMoves(const Position& pos, int square, MoveArray& moves)
  {
    //int pos_in_row =
  }

  void MoveGenerator::generatePawnMoves(const Position& pos, int square, MoveArray& moves)
  {

  }

  bool MoveGenerator::isKingAttacked(const Position& pos)
  {

  }

  MoveArray MoveGenerator::generatePseudoLegalMoves(const Position& pos)
  {

  }

  MoveArray MoveGenerator::generateLegalMoves(const Position& pos)
  {

  }*/
}
