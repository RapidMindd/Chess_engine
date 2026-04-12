#include "engine.hpp"
#include "evaluator.hpp"
#include "move.hpp"
#include "move_generator.hpp"

namespace chess
{
  int MIN = std::numeric_limits< int >::min() + 1;
  int MAX = std::numeric_limits< int >::max();
  int Engine::negamax(Position& pos, int depth)
  {
    if (depth == 0)
    {
      return Evaluator{}.relative_eval(pos);
    }
    MoveArray moves = MoveGenerator{}.generateLegalMoves(pos);
    if (moves.empty())
    {
      if (MoveGenerator{}.isMate(pos))
      {
        return MIN;
      }
      return 0;
    }

    int eval = MIN;
    UndoInfo undo;
    for (int i = 0; i < moves.size(); ++i)
    {
      pos.makeMove(moves.get(i), undo);
      eval = std::max(eval, -negamax(pos, depth - 1));
      pos.undoMove(moves.get(i), undo);
    }

    return eval;
  }

  std::pair< Move, int > Engine::findBestMove(Position& pos, int depth)
  {
    MoveArray moves = MoveGenerator{}.generateLegalMoves(pos);
    Move move;
    int eval = MIN;
    UndoInfo undo;
    for (int i = 0; i < moves.size(); ++i)
    {
      pos.makeMove(moves.get(i), undo);
      int cur_eval = -negamax(pos, depth - 1);
      if (cur_eval > eval)
      {
        eval = cur_eval;
        move = moves.get(i);
      }
      pos.undoMove(moves.get(i), undo);
    }
    return {move, eval};
  }
};
