#include "engine.hpp"
#include "evaluator.hpp"
#include "move.hpp"
#include "move_generator.hpp"

namespace chess
{
  const int MIN = -30000;
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

    rateMoves(moves, pos);

    int eval = MIN;
    UndoInfo undo;
    for (int i = 0; i < moves.size(); ++i)
    {
      MvBestMoveToBeg(moves, i);
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
    rateMoves(moves, pos);
    for (int i = 0; i < moves.size(); ++i)
    {
      MvBestMoveToBeg(moves, i);
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

  void Engine::rateMoves(MoveArray& moves, const Position& pos)
  {
    for (int i = 0; i < moves.size(); ++i)
    {
      rateMove(moves.moves_[i], pos);
    }
  }

  void Engine::rateMove(Move& move, const Position& pos)
  {
    int score = 0;

    static const int weights[7] = {0, 100, 320, 330, 500, 900, 0};
    int to = pos.getPiece(move.to_);
    if (to != EMPTY)
    {
      score += 10000 + weights[std::abs(to)] - weights[std::abs(pos.getPiece(move.from_))] / 8;
    }

    move.score_ = score;
  }

  void Engine::MvBestMoveToBeg(MoveArray& moves, int ind)
  {
    int max_eval = MIN;
    int best_ind = ind;
    for (int i = ind; i < moves.size(); ++i)
    {
      int curr = moves.get(i).score_;
      if (curr > max_eval)
      {
        max_eval = curr;
        best_ind = i;
      }
    }

    std::swap(moves.moves_[best_ind], moves.moves_[ind]);
  }
};
