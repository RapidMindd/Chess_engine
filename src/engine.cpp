#include "engine.hpp"
#include "evaluator.hpp"
#include "move.hpp"
#include "move_generator.hpp"
#include "piece.hpp"
#include "position.hpp"
#include "transposition_table.hpp"
#include "zobrist.hpp"
#include "piece_square_tables.hpp"
#include <cstdint>

namespace chess
{
  const int MIN = -30000;
  const int MATE = 29000;

  int Engine::negamax(Position& pos, int depth, int alpha, int beta, int ply, SearchNodes& nodes, uint64_t hash)
  {
    Move tt_move = null_move;
    TTEntry entry = tt_.getEntry(hash);
    if (entry.used_ && entry.key_ == hash)
    {
      if (entry.depth_ >= depth)
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
      tt_move = entry.bestMove_;
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

    rateMoves(moves, pos, tt_move);

    int alpha_orig = alpha;
    int eval = MIN;
    UndoInfo undo;
    Move best = null_move;
    for (int i = 0; i < moves.size(); ++i)
    {
      MvBestMoveToBeg(moves, i);
      ++nodes.nnodes;
      uint64_t cur_hash = incrementZobristHash(hash, pos, moves.get(i));
      pos.makeMove(moves.get(i), undo);
      // uint64_t cur_hash = zobristHash(pos);
      int curr_eval = -negamax(pos, depth - 1, -beta, -alpha, ply + 1, nodes, cur_hash);
      if (curr_eval > eval)
      {
        eval = curr_eval;
        best = moves.get(i);
      }
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

    tt_.addEntry(TTEntry{hash, eval, depth, type, true, best});

    return eval;
  }

  int Engine::quiescence(Position& pos, int alpha, int beta, int ply, SearchNodes& nodes)
  {
    UndoInfo undo;
    MoveArray moves;

    bool isActive = 0;

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
      isActive = 1;
    }

    isActive ? rateCaptures(moves, pos) : rateMoves(moves, pos);
    int eval = MIN;
    for (int i = 0; i < moves.size(); ++i)
    {
      MvBestMoveToBeg(moves, i);
      if (moves.get(i).score_ == -1000)
      {
        break;
      }
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

  std::pair< Move, float > Engine::findBestMove(Position& pos, int depth, SearchNodes* nodes)
  {
    SearchNodes local_nodes;
    SearchNodes& search_nodes = nodes ? *nodes : local_nodes;
    uint64_t init_hash = zobristHash(pos);
    MoveArray moves = MoveGenerator{}.generateLegalMoves(pos);
    if (moves.empty())
    {
      if (MoveGenerator{}.isCheck(pos))
      {
        return {null_move,-MATE};
      }
      return {null_move,0};
    }
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
      int cur_eval = -negamax(pos, depth - 1, MIN, -alpha, 0, search_nodes, hash);
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

  void Engine::rateMoves(MoveArray& moves, Position& pos, const Move& tt_move)
  {
    for (int i = 0; i < moves.size(); ++i)
    {
      rateMove(moves.moves_[i], pos, tt_move);
    }
  }

  void Engine::rateMove(Move& move, Position& pos, const Move& tt_move)
  {
    int score = 0;

    if (move == tt_move)
    {
      move.score_ = 100000;
      return;
    }

    int to = pos.getPiece(move.to_);
    if (to != EMPTY)
    {
      int from = pos.getPiece(move.from_);
      int attacker = weights[std::abs(from)];
      int victim = weights[std::abs(to)];
      if (victim < attacker)
      {
        int see = seeCapture(pos, move);
        if (see < 0)
        {
          score -= 5000 + see;
        }
        else
        {
          score += 5000 + see;
        }
      }
      else
      {
        score += 10000 + weights[std::abs(to)] - weights[std::abs(pos.getPiece(move.from_))] / 8;
      }
    }
    if (move.promotionPiece_ != EMPTY)
    {
      score += 12000 + std::abs(move.promotionPiece_);
    }

    Piece cur_piece = static_cast< Piece >(pos.getPiece(move.from_));
    int abs_piece = cur_piece > 0 ? cur_piece : -cur_piece;
    switch (abs_piece)
    {
      case WHITE_KING:
        score += king_table[move.to_] - king_table[move.from_];
        break;
      case WHITE_QUEEN:
        score += queen_table[move.to_] - queen_table[move.from_];
        break;
      case WHITE_KNIGHT:
        score += knight_table[move.to_] - knight_table[move.from_];
        break;
      case WHITE_BISHOP:
        score += bishop_table[move.to_] - bishop_table[move.from_];
        break;
      case WHITE_PAWN:
        score += pawn_table[move.to_] - pawn_table[move.from_];
        break;
    }

    move.score_ = score;
  }

  void Engine::rateCaptures(MoveArray& moves, Position& pos)
  {
    for (int i = 0; i < moves.size(); ++i)
    {
      rateCapture(moves.moves_[i], pos);
    }
  }

  void Engine::rateCapture(Move& move, Position& pos)
  {
    int score = 0;

    int from = pos.getPiece(move.from_);
    int to = pos.getPiece(move.to_);
    int attacker = weights[std::abs(from)];
    int victim = weights[std::abs(to)];
    if (victim < attacker)
    {
      int see = seeCapture(pos, move);
      if (see < 50)
      {
        move.score_ = -1000;
        return;
      }
      score += 10000 + see;
    }
    else
    {
      score += 10000 + weights[std::abs(to)] - weights[std::abs(pos.getPiece(move.from_))] / 8;
    }

    Piece cur_piece = static_cast< Piece >(pos.getPiece(move.from_));
    int abs_piece = cur_piece > 0 ? cur_piece : -cur_piece;
    switch (abs_piece)
    {
      case WHITE_KING:
        score += king_table[move.to_] - king_table[move.from_];
        break;
      case WHITE_QUEEN:
        score += queen_table[move.to_] - queen_table[move.from_];
        break;
      case WHITE_KNIGHT:
        score += knight_table[move.to_] - knight_table[move.from_];
        break;
      case WHITE_BISHOP:
        score += bishop_table[move.to_] - bishop_table[move.from_];
        break;
      case WHITE_PAWN:
        score += pawn_table[move.to_] - pawn_table[move.from_];
        break;
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

  Move Engine::leastValuableAttacker(const Position& pos, int square)
  {
    const int side = pos.isWhiteToMove() ? 1 : -1;

    Move attacker = MoveGenerator::findPawnAttacker(pos, square, side);
    if (attacker != null_move)
    {
      return attacker;
    }

    attacker = MoveGenerator::findKnightAttacker(pos, square, side);
    if (attacker != null_move)
    {
      return attacker;
    }

    attacker = MoveGenerator::findBishopAttacker(pos, square, side);
    if (attacker != null_move)
    {
      return attacker;
    }

    attacker = MoveGenerator::findRookAttacker(pos, square, side);
    if (attacker != null_move)
    {
      return attacker;
    }

    attacker = MoveGenerator::findQueenAttacker(pos, square, side);
    if (attacker != null_move)
    {
      return attacker;
    }

    return null_move;
  }

  int Engine::see(Position& pos, int square)
  {
    constexpr int max_exchanges = 32;
    Move attacks[max_exchanges];
    UndoInfo undo[max_exchanges];
    int captured_values[max_exchanges];
    int depth = 0;

    while (depth < max_exchanges)
    {
      Move attack = leastValuableAttacker(pos, square);
      if (attack == null_move)
      {
        break;
      }

      attacks[depth] = attack;
      captured_values[depth] = weights[std::abs(pos.getPiece(square))];
      pos.makeMove(attack, undo[depth]);
      ++depth;
    }

    int value = 0;
    for (int i = depth - 1; i >= 0; --i)
    {
      int current_value = captured_values[i] - value;
      value = current_value > 0 ? current_value : 0;
    }

    for (int i = depth - 1; i >= 0; --i)
    {
      pos.undoMove(attacks[i], undo[i]);
    }

    return value;
  }

  int Engine::seeCapture(Position& pos, const Move& move)
  {
    int captured_value = weights[std::abs(pos.getPiece(move.to_))];
    UndoInfo undo;

    pos.makeMove(move, undo);
    int opponent_gain = see(pos, move.to_);
    pos.undoMove(move, undo);

    return captured_value - opponent_gain;
  }
};
