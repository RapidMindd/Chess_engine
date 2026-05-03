#include "evaluator.hpp"
#include "move_generator.hpp"
#include "piece.hpp"
#include "piece_square_tables.hpp"

namespace chess
{
  int Evaluator::evaluate(const Position& pos)
  {
    int eval = 0;

    material(pos, eval);
    mobility(pos, eval);
    piece_square_tables(pos, eval);
    pawn_structure(pos, eval);
    king_safety(pos, eval);

    return eval;
  }

  int Evaluator::relative_eval(const Position& pos)
  {
    int eval = evaluate(pos);
    return pos.isWhiteToMove() ? eval : -eval;
  }

  void Evaluator::material(const Position& pos, int& eval)
  {
    static const int weights[7] = {0, 100, 320, 330, 500, 900, 0};
    for (int i = A1; i <= H8; ++i)
    {
      Piece cur = static_cast< Piece >(pos.getPiece(i));
      if (cur > 0)
      {
        eval += weights[cur];
      }
      else if (cur < 0)
      {
        eval -= weights[-cur];
      }
    }
  }

  void Evaluator::mobility(const Position &pos, int &eval)
  {
    for (int i = A1; i <= H8; ++i)
    {
      Piece cur_piece = static_cast< Piece >(pos.getPiece(i));
      const int piece_color = cur_piece > 0 ? 1 : -1;
      int abs_piece = cur_piece * piece_color;
      switch (abs_piece)
      {
        case WHITE_QUEEN:
          eval += MoveGenerator::countPseudoLegalQueenMoves(pos, static_cast< Square >(i)) * piece_color * 1;
          break;
        case WHITE_KNIGHT:
          eval += MoveGenerator::countPseudoLegalKnightMoves(pos, static_cast< Square >(i)) * piece_color * 4;
          break;
        case WHITE_BISHOP:
          eval += MoveGenerator::countPseudoLegalBishopMoves(pos, static_cast< Square >(i)) * piece_color * 5;
          break;
        case WHITE_ROOK:
          eval += MoveGenerator::countPseudoLegalRookMoves(pos, static_cast< Square >(i)) * piece_color * 2;
          break;
      }
    }
  }

  void Evaluator::piece_square_tables(const Position &pos, int &eval)
  {
    for (int i = A1; i <= H8; ++i)
    {
      Piece cur_piece = static_cast< Piece >(pos.getPiece(i));
      const int piece_color = cur_piece > 0 ? 1 : -1;
      int abs_piece = cur_piece * piece_color;
      switch (abs_piece)
      {
        case WHITE_KING:
          eval += king_table[piece_color == 1 ? i : i ^ 56] * piece_color;
          break;
        case WHITE_QUEEN:
          eval += queen_table[piece_color == 1 ? i : i ^ 56] * piece_color;
          break;
        case WHITE_KNIGHT:
          eval += knight_table[piece_color == 1 ? i : i ^ 56] * piece_color;
          break;
        case WHITE_BISHOP:
          eval += bishop_table[piece_color == 1 ? i : i ^ 56] * piece_color;
          break;
        case WHITE_PAWN:
          eval += pawn_table[piece_color == 1 ? i : i ^ 56] * piece_color;
          break;
      }
    }
  }

  void Evaluator::pawn_structure(const Position &pos, int &eval)
  {
    int white_pawn_cols[8] = {};
    int black_pawn_cols[8] = {};
    for (int i = A1; i <= H8; ++i)
    {
      Piece cur_piece = static_cast< Piece >(pos.getPiece(i));
      if (cur_piece == WHITE_PAWN)
      {
        white_pawn_cols[i % 8] += 1;
      }
      else if (cur_piece == BLACK_PAWN)
      {
        black_pawn_cols[i % 8] += 1;
      }
    }

    for (int i = 1; i < 7; ++i)
    {
      if (white_pawn_cols[i] >= 2)
      {
        if (white_pawn_cols[i + 1] == 0 && white_pawn_cols[i - 1] == 0)
        {
          eval -= 40;
        }
        else
        {
          eval -= 15;
        }
      }
      if (black_pawn_cols[i] >= 2)
      {
        if (black_pawn_cols[i + 1] == 0 && black_pawn_cols[i - 1] == 0)
        {
          eval += 40;
        }
        else
        {
          eval += 15;
        }
      }
    }
    if (white_pawn_cols[0] >= 2) eval -= 40;
    if (black_pawn_cols[0] >= 2) eval += 40;
    if (white_pawn_cols[7] >= 2) eval -= 40;
    if (black_pawn_cols[7] >= 2) eval += 40;
  }

  void Evaluator::king_safety(const Position &pos, int &eval)
  {
    const int white_king_square = pos.getWhiteKingSquare();
    const int black_king_square = pos.getBlackKingSquare();
    if (white_king_square % 8 != 3 && white_king_square % 8 != 4)
    {
      int row = white_king_square / 8;
      int col = white_king_square % 8;
      if (row < 2)
      {
        int c1 = col - 1;
        int c2 = col + 1;

        if (c1 < 0) c1 = 0;
        if (c2 > 7) c2 = 7;

        for (int r = row; r <= row + 1; ++r)
        {
          for (int c = c1; c <= c2; ++c)
          {
            int square = r * 8 + c;
            if (square == white_king_square)
                continue;
            if (pos.getPiece(square) == WHITE_PAWN)
            {
              eval += 5;
            }
          }
        }
      }
    }

    if (black_king_square % 8 != 3 && black_king_square % 8 != 4)
    {
      int row = black_king_square / 8;
      int col = black_king_square % 8;
      if (row > 5)
      {
        int c1 = col - 1;
        int c2 = col + 1;

        if (c1 < 0) c1 = 0;
        if (c2 > 7) c2 = 7;

        for (int r = row; r >= row - 1; --r)
        {
          for (int c = c1; c <= c2; ++c)
          {
            int square = r * 8 + c;
            if (square == black_king_square)
                continue;
            if (pos.getPiece(square) == BLACK_PAWN)
            {
              eval -= 5;
            }
          }
        }
      }
    }
  }
}
