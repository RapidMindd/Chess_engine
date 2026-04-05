#include "move_generator.hpp"

namespace chess
{
  void MoveGenerator::generateKingMoves(const Position& pos, Square square, MoveArray& moves)
  {
    constexpr int possible_moves = 8;
    const int is_white_piece = pos.getPiece(square) > 0 ? 1 : -1;
    const int row = square / 8;
    const int col = square % 8;

    // начиная с клетки сверху, по часовой
    int row_offset[possible_moves] = {1, 1, 0, -1, -1, -1, 0, 1};
    int col_offset[possible_moves] = {0, 1, 1, 1, 0, -1, -1, -1};

    for (int i = 0; i < possible_moves; ++i)
    {
      int new_row = row + row_offset[i];
      int new_col = col + col_offset[i];
      if (new_row >= 0 && new_row < 8 && new_col >= 0 && new_col < 8)
      {
        int dest_square = new_row * 8 + new_col;
        if (pos.getPiece(dest_square) * is_white_piece < 1)
        {
          moves.push({square, static_cast< Square >(dest_square)});
        }
      }
    }
  }

  void MoveGenerator::generateQueenMoves(const Position& pos, Square square, MoveArray& moves)
  {

  }

  void MoveGenerator::generateKnightMoves(const Position& pos, Square square, MoveArray& moves)
  {

  }

  void MoveGenerator::generateBishopMoves(const Position& pos, Square square, MoveArray& moves)
  {

  }

  void MoveGenerator::generateRookMoves(const Position& pos, Square square, MoveArray& moves)
  {
    int col = square % 8;

    // вверх
    int sq = square + 8;
    while (sq <= H8 && pos.getPiece(sq) == EMPTY)
    {
      moves.push({square, static_cast< Square >(sq)});
      sq += 8;
    }

    // вниз
    sq = square - 8;
    while (sq >= A1 && pos.getPiece(sq) == EMPTY)
    {
      moves.push({square, static_cast< Square >(sq)});
      sq -= 8;
    }

    // вправо
    sq = col;
    while (8 - sq > 0 && pos.getPiece(sq) == EMPTY)
    {
      moves.push({square, static_cast< Square >(sq)});
      ++sq;
    }

    // влево
    sq = col;
    while (sq >= 0 && pos.getPiece(sq) == EMPTY)
    {
      moves.push({square, static_cast< Square >(sq)});
      --sq;
    }
  }

  void MoveGenerator::generatePawnMoves(const Position& pos, Square square, MoveArray& moves)
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

  }
}
