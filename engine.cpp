#include "engine.hpp"
#include "evaluator.hpp"
#include "move.hpp"
#include "move_generator.hpp"
#include "transposition_table.hpp"
#include "zobrist.hpp"
#include <cstdint>

namespace chess
{
  const int MIN = -30000;
  const int MATE = 29000;

  int Engine::negamax(Position& pos, int depth, int alpha, int beta, int ply, SearchNodes& nodes, uint64_t hash)
  {
    TTEntry entry = tt_.getEntry(hash);
      if (entry.used_ && entry.key_ == hash && entry.depth_ >= depth)
      {
        if (entry.type_ == EXACT)
        {
          return entry.eval_;
        }
        else if (entry.type_ == LOWER_BOUND && entry.eval_ >= beta)
        {
          return beta;
        }
        else if (entry.type_ == UPPER_BOUND && entry.eval_ <= alpha)
        {
          return alpha;
        }
      }

    if (depth == 0)
    {
      return quiescence(pos, alpha, beta, ply, nodes);
    }

    MoveArray moves = MoveGenerator{}.generateLegalMoves(pos);
    if (moves.empty())
    {
      if (MoveGenerator{}.isCheck(pos))
      {
        return -MATE + ply;
      }
      return 0;
    }

    rateMoves(moves, pos);

    int alpha_orig = alpha;
    int eval = MIN;
    UndoInfo undo;
    for (int i = 0; i < moves.size(); ++i)
    {
      MvBestMoveToBeg(moves, i);
      ++nodes.nnodes;
      uint64_t cur_hash = incrementZobristHash(hash, pos, moves.get(i));
      pos.makeMove(moves.get(i), undo);
      // uint64_t cur_hash = zobristHash(pos);
      eval = std::max(eval, -negamax(pos, depth - 1, -beta, -alpha, ply + 1, nodes, cur_hash));
      pos.undoMove(moves.get(i), undo);

      alpha = std::max(alpha, eval);
      if (alpha >= beta)
      {
        break;
      }
    }

    TTEntryType type;
    if (eval <= alpha_orig)
    {
      type = UPPER_BOUND;
    }
    else if (eval >= beta)
    {
      type = LOWER_BOUND;
    }
    else
    {
      type = EXACT;
    }

    tt_.addEntry(TTEntry{hash, eval, depth, type, true});

    return eval;
  }

  int Engine::quiescence(Position& pos, int alpha, int beta, int ply, SearchNodes& nodes)
  {
    UndoInfo undo;
    MoveArray moves;

    if (MoveGenerator{}.isCheck(pos))
    {
      moves = MoveGenerator{}.generateLegalMoves(pos);
      if (moves.empty())
      {
        return -MATE + ply;
      }
    }

    else
    {
      int stand_pat = Evaluator{}.relative_eval(pos);
      alpha = std::max(alpha, stand_pat);
      if (stand_pat >= beta)
      {
        return beta;
      }

      moves = MoveGenerator{}.generateActiveMoves(pos);
    }

    rateMoves(moves, pos);
    int eval = MIN;
    for (int i = 0; i < moves.size(); ++i)
    {
      MvBestMoveToBeg(moves, i);
      ++nodes.qnodes;
      pos.makeMove(moves.get(i), undo);
      eval = std::max(eval, -quiescence(pos, -beta, -alpha, ply + 1, nodes));
      pos.undoMove(moves.get(i), undo);

      alpha = std::max(alpha, eval);
      if (alpha >= beta)
      {
        return beta;
      }
    }

    return alpha;
  }

  std::pair< Move, float > Engine::findBestMove(Position& pos, int depth)
  {
    uint64_t init_hash = zobristHash(pos);
    SearchNodes nodes;
    MoveArray moves = MoveGenerator{}.generateLegalMoves(pos);
    Move move;
    int eval = MIN;
    int alpha = MIN;
    UndoInfo undo;
    rateMoves(moves, pos);
    for (int i = 0; i < moves.size(); ++i)
    {
      MvBestMoveToBeg(moves, i);
      uint64_t hash = incrementZobristHash(init_hash, pos, moves.get(i));
      pos.makeMove(moves.get(i), undo);
      // uint64_t hash = zobristHash(pos);
      int cur_eval = -negamax(pos, depth - 1, MIN, -alpha, 0, nodes, hash);
      if (cur_eval > eval)
      {
        eval = cur_eval;
        move = moves.get(i);
        alpha = eval;
      }
      pos.undoMove(moves.get(i), undo);
    }
    return {move, (pos.isWhiteToMove() ? eval : -eval) / 100.0};
  }

  std::pair< Move, float > Engine::findBestMove(Position& pos, int depth, SearchNodes& nodes)
  {
    uint64_t init_hash = zobristHash(pos);
    MoveArray moves = MoveGenerator{}.generateLegalMoves(pos);
    Move move;
    int eval = MIN;
    int alpha = MIN;
    UndoInfo undo;
    rateMoves(moves, pos);
    for (int i = 0; i < moves.size(); ++i)
    {
      MvBestMoveToBeg(moves, i);
      uint64_t hash = incrementZobristHash(init_hash, pos, moves.get(i));
      pos.makeMove(moves.get(i), undo);
      // uint64_t hash = zobristHash(pos);
      int cur_eval = -negamax(pos, depth - 1, MIN, -alpha, 0, nodes, hash);
      if (cur_eval > eval)
      {
        eval = cur_eval;
        move = moves.get(i);
        alpha = eval;
      }
      pos.undoMove(moves.get(i), undo);
    }
    return {move, (pos.isWhiteToMove() ? eval : -eval) / 100.0};
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
    if (move.promotionPiece_ != EMPTY)
    {
      score += 12000 + std::abs(move.promotionPiece_);
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

  Engine::Engine()
  {
    initZobristHash();
  }

  Engine::Engine(uint64_t size):
    tt_(size)
  {
    initZobristHash();
  }
};
