#include "evaluator.hpp"

namespace chess
{
  int Evaluator::evaluate(const Position& pos)
  {
    int eval = 0;

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

    return eval;
  }

  int Evaluator::relative_eval(const Position& pos)
  {
    int eval = evaluate(pos);
    return pos.isWhiteToMove() ? eval : -eval;
  }
}
