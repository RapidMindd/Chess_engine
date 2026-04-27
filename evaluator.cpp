#include "evaluator.hpp"
#include "move_generator.hpp"
#include "piece.hpp"

namespace chess
{
  int Evaluator::evaluate(const Position& pos)
  {
    int eval = 0;

    material(pos, eval);
    mobility(pos, eval);
    piece_square_tables(pos, eval);

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
    constexpr int king_table[64] = {
      20, 30, 10, 0, 0, 10, 30, 20,
      20, 20, 0, 0, 0, 0, 20, 20,
      -10, -20, -20, -20, -20, -20, -20, -10,
      -20, -30, -30, -40, -40, -30, -30, -20,
      -30, -40, -40, -50, -50, -40, -40, -30,
      -30, -40, -40, -50, -50, -40, -40, -30,
      -30, -40, -40, -50, -50, -40, -40, -30,
      -30, -40, -40, -50, -50, -40, -40, -30
    };

    constexpr int queen_table[64] = {
      -20, -10, -10, -10, -10, -10, -10, -20,
      -10, 0, 0, 0, 0, 0, 0, -10,
      -10, 0, 5, 5, 5, 5, 0, -10,
      -5, 0, 5, 5, 5, 5, 0, -5,
      0, 0, 5, 5, 5, 5, 0, 0,
      0, 5, 5, 5, 5, 5, 5, -0,
      -10, 0, 0, 0, 0, 0, 0, -10,
      -10, -10, -10, -10, -10, -10, -10, -10
    };

    constexpr int knight_table[64] = {
      -50, -40, -20, -30, -30, -20, -40, -50,
      -40, -20, 0, 5, 5, 0, -20, -40,
      -30, 5, 10, 15, 15, 10, 5, -30,
      -30, 0, 15, 20, 20, 15, 0, -30,
      -30, 5, 15, 20, 20, 15, 5, -30,
      -30, 0, 10, 15, 15, 10, 0, -30,
      -40, -20, 0, 0, 0, 0, -20, -40,
      -50, -40, -30, -30, -30, -30, -40, -50,
    };

    constexpr int bishop_table[64] = {
      -20, -10, -40, -10, -10, -40, -10, -20,
      -10, 5, 0, 0, 0, 0, 5, -10,
      -10, 10, 10, 10, 10, 10, 10, -10,
      -10, 0, 10, 10, 10, 10, 0, -10,
      -10, 5, 5, 10, 10, 5, 5, -10,
      -10, 0, 5, 10, 10, 5, 0, -10,
      -10, 0, 0, 0, 0, 0, 0, -10,
      -20, -10, -10, -10, -10, -10, -10, -20,
    };

    constexpr int pawn_table[64] = {
      0, 0, 0, 0, 0, 0, 0, 0,
      5, 10, 10, -25, -25, 10, 10, 5,
      5, -5, -10, 0, 0, -10, -5, 5,
      0, 0, 0, 25, 25, 0, 0, 0,
      5, 5, 10, 27, 27, 10, 5, 5,
      10, 10, 20, 30, 30, 20, 10, 10,
      50, 50, 50, 50, 50, 50, 50, 50,
      0, 0, 0, 0, 0, 0, 0, 0,
    };

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
}
