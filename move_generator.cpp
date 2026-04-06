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
    generateRookMoves(pos, square, moves);
    generateBishopMoves(pos, square, moves);
  }

  void MoveGenerator::generateKnightMoves(const Position& pos, Square square, MoveArray& moves)
  {
    constexpr int possible_moves = 8;
    const int is_white_piece = pos.getPiece(square) > 0 ? 1 : -1;
    const int row = square / 8;
    const int col = square % 8;

    // начиная с клетки сверху справа, по часовой
    int row_offset[possible_moves] = {2, 1, -1, -2, -2, -1, 1, 2};
    int col_offset[possible_moves] = {1, 2, 2, 1, -1, -2, -2, -1};

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

  void MoveGenerator::generateBishopMoves(const Position& pos, Square square, MoveArray& moves)
  {
    const int is_white_piece = pos.getPiece(square) > 0 ? 1 : -1;

    // вверх вправо
    int col = (square % 8) + 1;
    int dest_square = square + 9;
    while (8 - col > 0 && dest_square <= H8 && pos.getPiece(dest_square) * is_white_piece < 1)
    {
      moves.push({square, static_cast< Square >(dest_square)});
      if (pos.getPiece(dest_square) != EMPTY)
      {
        break;
      }
      dest_square += 9;
      ++col;
    }

    // вниз вправо
    col = (square % 8) + 1;
    dest_square = square - 7;
    while (8 - col > 0 && dest_square >= A1 && pos.getPiece(dest_square) * is_white_piece < 1)
    {
      moves.push({square, static_cast< Square >(dest_square)});
      if (pos.getPiece(dest_square) != EMPTY)
      {
        break;
      }
      dest_square -= 7;
      ++col;
    }

    // вниз влево
    col = (square % 8) - 1;
    dest_square = square - 9;
    while (col >= 0 && dest_square >= A1 && pos.getPiece(dest_square) * is_white_piece < 1)
    {
      moves.push({square, static_cast< Square >(dest_square)});
      if (pos.getPiece(dest_square) != EMPTY)
      {
        break;
      }
      dest_square -= 9;
      --col;
    }

    // вверх влево
    col = (square % 8) - 1;
    dest_square = square + 7;
    while (col >= 0 && dest_square <= H8 && pos.getPiece(dest_square) * is_white_piece < 1)
    {
      moves.push({square, static_cast< Square >(dest_square)});
      if (pos.getPiece(dest_square) != EMPTY)
      {
        break;
      }
      dest_square += 7;
      --col;
    }
  }

  void MoveGenerator::generateRookMoves(const Position& pos, Square square, MoveArray& moves)
  {
    const int is_white_piece = pos.getPiece(square) > 0 ? 1 : -1;

    // вверх
    int dest_square = square + 8;
    while (dest_square <= H8 && pos.getPiece(dest_square) * is_white_piece < 1)
    {
      moves.push({square, static_cast< Square >(dest_square)});
      if (pos.getPiece(dest_square) != EMPTY)
      {
        break;
      }
      dest_square += 8;
    }

    // вниз
    dest_square = square - 8;
    while (dest_square >= A1 && pos.getPiece(dest_square) * is_white_piece < 1)
    {
      moves.push({square, static_cast< Square >(dest_square)});
      if (pos.getPiece(dest_square) != EMPTY)
      {
        break;
      }
      dest_square -= 8;
    }

    // вправо
    int col = (square % 8) + 1;
    dest_square = square + 1;
    while (8 - col > 0 && pos.getPiece(dest_square) * is_white_piece < 1)
    {
      moves.push({square, static_cast< Square >(dest_square)});
      if (pos.getPiece(dest_square) != EMPTY)
      {
        break;
      }
      ++dest_square;
      ++col;
    }

    // влево
    col = (square % 8) - 1;
    dest_square = square - 1;
    while (col >= 0 && pos.getPiece(dest_square) * is_white_piece < 1)
    {
      moves.push({square, static_cast< Square >(dest_square)});
      if (pos.getPiece(dest_square) != EMPTY)
      {
        break;
      }
      --dest_square;
      --col;
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
