#include "engine.hpp"
#include "evaluator.hpp"
#include "move.hpp"
#include "move_generator.hpp"

namespace chess
{
  const int MIN = -30000;
  const int MAX = 30000;
  const int MATE = 29000;

  int Engine::negamax(Position& pos, int depth, int alpha, int beta, int ply)
  {
    if (depth == 0)
    {
      return Evaluator{}.relative_eval(pos);
    }
    MoveArray moves = MoveGenerator{}.generateLegalMoves(pos);
    if (moves.empty())
    {
      if (MoveGenerator{}.isMateUnsafe(pos))
      {
        return -MATE + ply;
      }
      return 0;
    }

    int eval = MIN;
    UndoInfo undo;
    for (int i = 0; i < moves.size(); ++i)
    {
      pos.makeMove(moves.get(i), undo);
      eval = std::max(eval, -negamax(pos, depth - 1, -beta, -alpha, ply + 1));
      pos.undoMove(moves.get(i), undo);

      alpha = std::max(alpha, eval);
      if (alpha >= beta)
      {
        break;
      }
    }

    return eval;
  }

  std::pair< Move, int > Engine::findBestMove(Position& pos, int depth)
  {
    MoveArray moves = MoveGenerator{}.generateLegalMoves(pos);
    Move move;
    int eval = MIN;
    int alpha = MIN;
    UndoInfo undo;
    for (int i = 0; i < moves.size(); ++i)
    {
      pos.makeMove(moves.get(i), undo);
      int cur_eval = -negamax(pos, depth - 1, MIN, -alpha, 0);
      if (cur_eval > eval)
      {
        eval = cur_eval;
        move = moves.get(i);
        alpha = eval;
      }
      pos.undoMove(moves.get(i), undo);
    }
    return {move, eval};
  }
};
