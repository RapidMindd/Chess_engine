#include "evaluator.hpp"
#include "move_generator.hpp"

namespace chess
{
  int Evaluator::evaluate(const Position& pos)
  {
    int eval = 0;

    material(pos, eval);
    mobility(pos, eval);
    // center_control(pos, eval);

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
}
