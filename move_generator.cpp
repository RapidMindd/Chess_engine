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
    const int is_white_piece = pos.getPiece(square) > 0 ? 1 : -1;
    const int displacement = is_white_piece == 1 ? 8 : -8;
    const int start_row = is_white_piece == 1 ? 1 : 6;
    const int promotion_row = is_white_piece == 1 ? 6 : 1;
    const int enPassant_row = is_white_piece == 1 ? 4 : 3;

    const int row = square / 8;
    const int col = square % 8;

    auto promote = [&moves, square, is_white_piece](int displacement){
        moves.push({square, static_cast< Square >(square + displacement), static_cast< Piece >(WHITE_QUEEN * is_white_piece)});
        moves.push({square, static_cast< Square >(square + displacement), static_cast< Piece >(WHITE_KNIGHT * is_white_piece)});
        moves.push({square, static_cast< Square >(square + displacement), static_cast< Piece >(WHITE_ROOK * is_white_piece)});
        moves.push({square, static_cast< Square >(square + displacement), static_cast< Piece >(WHITE_BISHOP * is_white_piece)});
    };

    // ходы вперед
    if (pos.getPiece(square + displacement) == EMPTY)
    {
      if (row == promotion_row)
      {
        promote(displacement);
      }
      else
      {
        moves.push({square, static_cast< Square >(square + displacement)});
        if (row == start_row && pos.getPiece(square + displacement * 2) == EMPTY)
        {
          moves.push({square, static_cast< Square >(square + displacement * 2)});
        }
      }
    }

    // взятия
    const int take_displacements[2] = {9 * is_white_piece, 7 * is_white_piece};
    const int corner_col_for_take[2] = {is_white_piece == 1 ? 7 : 0, is_white_piece == 1 ? 0 : 7};

    for (size_t i = 0; i < 2; ++i)
    {
      if (col != corner_col_for_take[i])
      {
        const int take_piece = pos.getPiece(square + take_displacements[i]);
        if (take_piece * is_white_piece < 0)
        {
          if (row == promotion_row)
          {
            promote(take_displacements[i]);
          }
          else
          {
            moves.push({square, static_cast< Square >(square + take_displacements[i])});
          }
        }
        // взятие на проходе
        else if (row == enPassant_row && pos.getEnPassantSquare() == square + take_displacements[i])
        {
          moves.push({square, static_cast< Square >(square + take_displacements[i]), EMPTY, true});
        }
      }
    }

    /*const int take1_displacement = 9 * is_white_piece;
    const int corner_col_for_take_1 = is_white_piece == 1 ? 7 : 0;
    if (col != corner_col_for_take_1)
    {
      const int take1_piece = pos.getPiece(square + 9 * is_white_piece);
      if (take1_piece * is_white_piece < 0)
      {
        if (row == promotion_row)
        {
          promote(take1_displacement);
        }
        else
        {
          moves.push({square, static_cast< Square >(square + take1_displacement)});
        }
      }
    }

    const int take2_displacement = 7 * is_white_piece;
    const int corner_col_for_take_2 = is_white_piece == 1 ? 0 : 7;
    if (col != corner_col_for_take_2)
    {
      const int take2_piece = pos.getPiece(square + 7 * is_white_piece);
      if (take2_piece * is_white_piece < 0)
      {
        if (row == promotion_row)
        {
          promote(take2_displacement);
        }
        else
        {
          moves.push({square, static_cast< Square >(square + take2_displacement)});
        }
      }
    }*/
  }

  bool MoveGenerator::isKingAttacked(const Position& pos)
  {

  }

  MoveArray MoveGenerator::generatePseudoLegalMoves(const Position& pos)
  {
    MoveArray moves;
    const int side_to_move = pos.isWhiteToMove() ? 1 : -1;
    for (int i = A1; i <= H8; ++i)
    {
      const int piece = pos.getPiece(i);
      if (piece * side_to_move <= 0)
      {
        continue;
      }
      const int is_white_piece = piece > 0 ? 1 : -1;
      const int abs_piece = piece * is_white_piece;
      if (abs_piece <= 0)
      {
        continue;
      }
      switch (abs_piece)
      {
        case WHITE_KNIGHT:
          generateKnightMoves(pos, static_cast< Square >(i), moves);
          break;
        case WHITE_BISHOP:
          generateBishopMoves(pos, static_cast< Square >(i), moves);
          break;
        case WHITE_QUEEN:
          generateQueenMoves(pos, static_cast< Square >(i), moves);
          break;
        case WHITE_PAWN:
          generatePawnMoves(pos, static_cast< Square >(i), moves);
          break;
        case WHITE_ROOK:
          generateRookMoves(pos, static_cast< Square >(i), moves);
          break;
        case WHITE_KING:
          generateKingMoves(pos, static_cast< Square >(i), moves);
          break;
      }
    }
    return moves;
  }

  MoveArray MoveGenerator::generateLegalMoves(const Position& pos)
  {

  }
}
