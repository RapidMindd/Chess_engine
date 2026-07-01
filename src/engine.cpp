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
#include <chrono>
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <thread>
#include <vector>

namespace chess
{
  const int MIN = -30000;
  const int MATE = 29000;

  bool useTTEntry(const TTEntry& entry, uint64_t hash, int depth, int alpha, int beta, Move& tt_move, int& eval)
  {
    if (!entry.used_ || entry.key_ != hash)
    {
      return false;
    }

    if (entry.depth_ >= depth)
    {
      if (entry.type_ == EXACT)
      {
        eval = entry.eval_;
        return true;
      }
      else if (entry.type_ == LOWER_BOUND && entry.eval_ >= beta)
      {
        eval = entry.eval_;
        return true;
      }
      else if (entry.type_ == UPPER_BOUND && entry.eval_ <= alpha)
      {
        eval = entry.eval_;
        return true;
      }
    }
    tt_move = entry.bestMove_;
    return false;
  }

  TTEntryType getTTEntryType(int eval, int alpha_orig, int beta)
  {
    if (eval <= alpha_orig)
    {
      return UPPER_BOUND;
    }
    else if (eval >= beta)
    {
      return LOWER_BOUND;
    }
    return EXACT;
  }

  int pieceSquareDelta(Position& pos, const Move& move)
  {
    Piece cur_piece = static_cast< Piece >(pos.getPiece(move.from_));
    int abs_piece = cur_piece > 0 ? cur_piece : -cur_piece;
    switch (abs_piece)
    {
      case WHITE_KING:
        return king_table[move.to_] - king_table[move.from_];
      case WHITE_QUEEN:
        return queen_table[move.to_] - queen_table[move.from_];
      case WHITE_KNIGHT:
        return knight_table[move.to_] - knight_table[move.from_];
      case WHITE_BISHOP:
        return bishop_table[move.to_] - bishop_table[move.from_];
      case WHITE_PAWN:
        return pawn_table[move.to_] - pawn_table[move.from_];
    }
    return 0;
  }

  bool isQuietMove(Position& pos, const Move& move)
  {
    return pos.getPiece(move.to_) == EMPTY
      && move.promotionPiece_ == EMPTY
      && !move.isEnPassant_
      && !move.isCastling_;
  }

  int getLMRReduction(int depth, int move_number)
  {
    int reduction = 1 + move_number / 4;
    if (reduction > depth - 2)
    {
      reduction = depth - 2;
    }
    return reduction;
  }

  bool canReduceMove(Position& pos, const Move& move, int depth, int move_number)
  {
    return depth > 3 && move_number > 2 && isQuietMove(pos, move);
  }
  uint64_t nullMoveHash(const Position& pos)
  {
    Position null_pos = pos.getToggledSideToMovePosition();
    null_pos.setEnPassantSquare(-1);
    return zobristHash(null_pos);
  }

  bool canTryNullMove(int depth, bool in_check, bool allow_null)
  {
    return allow_null && depth >= 3 && !in_check;
  }

  unsigned defaultThreadCount()
  {
    const char* env = std::getenv("CHESS_THREADS");
    if (env != nullptr)
    {
      unsigned threads = 0;
      for (int i = 0; env[i] != '\0'; ++i)
      {
        if (env[i] < '0' || env[i] > '9')
        {
          return 1;
        }
        threads = threads * 10 + static_cast< unsigned >(env[i] - '0');
      }
      return threads == 0 ? 1 : threads;
    }

    unsigned threads = std::thread::hardware_concurrency();
    if (threads == 0)
    {
      return 1;
    }
    return std::min(threads, 4U);
  }

  int Engine::negamax(Position& pos, int depth, int alpha, int beta, int ply, SearchNodes& nodes, uint64_t hash,
    bool allow_null)
  {
    if (isTimeUp())
    {
      return alpha;
    }

    Move tt_move = null_move;
    TTEntry entry = tt_.getEntry(hash);
    int tt_eval = 0;
    if (useTTEntry(entry, hash, depth, alpha, beta, tt_move, tt_eval))
    {
      return tt_eval;
    }

    if (depth == 0)
    {
      return quiescence(pos, alpha, beta, ply, nodes);
    }

    MoveGenerator gen;
    bool in_check = gen.isCheck(pos);
    if (canTryNullMove(depth, in_check, allow_null))
    {
      int reduction = depth >= 6 ? 3 : 2;
      Position null_pos = pos.getToggledSideToMovePosition();
      null_pos.setEnPassantSquare(-1);
      int null_eval = -negamax(null_pos, depth - 1 - reduction, -beta, -beta + 1, ply + 1, nodes,
        nullMoveHash(pos), false);
      if (null_eval >= beta)
      {
        return beta;
      }
    }

    MoveArray moves = gen.generateLegalMoves(pos);

    rateMoves(moves, pos, tt_move);

    int alpha_orig = alpha;
    int eval = MIN;
    UndoInfo undo;
    Move best = null_move;
    bool no_moves = moves.empty();
    int move_number = 0;
    for (int i = 0; i < moves.size(); ++i)
    {
      MvBestMoveToBeg(moves, i);
      Move move = moves.get(i);
      bool can_reduce = canReduceMove(pos, move, depth, move_number);
      uint64_t cur_hash = incrementZobristHash(hash, pos, move);
      pos.makeMove(move, undo);
      ++nodes.nnodes;
      int reduction = can_reduce ? getLMRReduction(depth, move_number) : 0;
      int curr_eval = -negamax(pos, depth - 1 - reduction, -beta, -alpha, ply + 1, nodes, cur_hash);
      if (stopped_.load())
      {
        pos.undoMove(move, undo);
        return alpha;
      }
      if (reduction && curr_eval > alpha)
      {
        curr_eval = -negamax(pos, depth - 1, -beta, -alpha, ply + 1, nodes, cur_hash);
        if (stopped_.load())
        {
          pos.undoMove(move, undo);
          return alpha;
        }
      }
      if (curr_eval > eval)
      {
        eval = curr_eval;
        best = move;
      }
      pos.undoMove(move, undo);
      ++move_number;

      alpha = std::max(alpha, eval);
      if (alpha >= beta)
      {
        break;
      }
    }

    if (no_moves)
    {
      if (gen.isCheck(pos))
      {
        return -MATE + ply;
      }
      return 0;
    }

    TTEntryType type = getTTEntryType(eval, alpha_orig, beta);
    tt_.addEntry(TTEntry{hash, eval, depth, type, true, best});

    return eval;
  }

  int Engine::quiescence(Position& pos, int alpha, int beta, int ply, SearchNodes& nodes)
  {
    if (isTimeUp())
    {
      return alpha;
    }

    UndoInfo undo;
    MoveArray moves;

    bool isActive = 0;

    MoveGenerator gen;
    if (gen.isCheck(pos))
    {
      moves = gen.generateLegalMoves(pos);
    }

    else
    {
      int stand_pat = Evaluator{}.relative_eval(pos);
      alpha = std::max(alpha, stand_pat);
      if (stand_pat >= beta)
      {
        return beta;
      }

      gen.generatePseudoLegalActiveMoves(pos, moves);
      isActive = 1;
    }

    isActive ? rateCaptures(moves, pos) : rateMoves(moves, pos);
    int eval = MIN;
    bool side = !pos.isWhiteToMove();
    bool no_moves = true;
    for (int i = 0; i < moves.size(); ++i)
    {
      MvBestMoveToBeg(moves, i);
      if (moves.get(i).score_ == -1000)
      {
        break;
      }
      pos.makeMove(moves.get(i), undo);
      if (gen.isSquareAttackedQuick(pos, static_cast< Square >(pos.getOppositeColourKingSquare()), side))
      {
        pos.undoMove(moves.get(i), undo);
        continue;
      }
      no_moves = false;
      ++nodes.qnodes;
      eval = std::max(eval, -quiescence(pos, -beta, -alpha, ply + 1, nodes));
      if (stopped_.load())
      {
        pos.undoMove(moves.get(i), undo);
        return alpha;
      }
      pos.undoMove(moves.get(i), undo);

      alpha = std::max(alpha, eval);
      if (alpha >= beta)
      {
        return beta;
      }
    }

    if (no_moves && !isActive)
    {
      return -MATE + ply;
    }

    return alpha;
  }

  std::pair< Move, int > Engine::findBestMoveSingle(Position& pos, int depth, SearchNodes& search_nodes)
  {
    uint64_t init_hash = zobristHash(pos);
    MoveGenerator gen;
    MoveArray moves = gen.generateLegalMoves(pos);
    if (moves.empty())
    {
      if (gen.isCheck(pos))
      {
        return {null_move, -MATE};
      }
      return {null_move, 0};
    }
    Move best_move = moves.get(0);
    int best_eval = 0;
    int max_depth = use_time_ ? 64 : depth;
    for (int cur_depth = 1; cur_depth <= max_depth; ++cur_depth)
    {
      if (isTimeUp())
      {
        break;
      }
      int eval = 0;

      int window = 50;
      int alpha = cur_depth <= 4 ? MIN : best_eval - 50;
      int beta = cur_depth <= 4 ? -MIN : best_eval + 50;
      while (true)
      {
        std::pair< Move, int > result = searchRoot(pos, moves, init_hash,
          cur_depth, alpha, beta, search_nodes, best_move);
        if (stopped_.load())
        {
          break;
        }
        eval = result.second;

        if (eval <= alpha)
        {
          alpha -= window;
        }
        else if (eval >= beta)
        {
          beta += window;
        }
        else
        {
          best_move = result.first;
          break;
        }
        window *= 2;
      }
      if (stopped_.load())
      {
        break;
      }
      best_eval = eval;
    }
    return {best_move, best_eval};
  }

  std::pair< Move, float > Engine::findBestMove(Position& pos, int depth, SearchNodes* nodes, int time_ms)
  {
    use_time_ = time_ms > 0;
    stopped_.store(false);
    if (use_time_)
    {
      deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(time_ms);
    }

    SearchNodes local_nodes;
    SearchNodes& search_nodes = nodes ? *nodes : local_nodes;
    std::pair< Move, int > result = findBestMoveSingle(pos, depth, search_nodes);
    use_time_ = false;
    return {result.first, (pos.isWhiteToMove() ? result.second : -result.second) / 100.0};
  }

  std::pair< Move, int > Engine::searchRoot(Position& pos, MoveArray& moves, uint64_t init_hash,
    int depth, int alpha, int beta, SearchNodes& nodes, const Move& prev_best)
  {
    int eval = MIN;
    Move best_move = prev_best;
    UndoInfo undo;

    rateMoves(moves, pos, null_move, prev_best);
    if (depth >= 12 && moves.size() > 1)
    {
      for (int i = 0; i < moves.size(); ++i)
      {
        MvBestMoveToBeg(moves, i);
      }

      uint64_t hash = incrementZobristHash(init_hash, pos, moves.get(0));
      pos.makeMove(moves.get(0), undo);
      eval = -negamax(pos, depth - 1, -beta, -alpha, 0, nodes, hash);
      if (stopped_.load())
      {
        pos.undoMove(moves.get(0), undo);
        return {best_move, eval};
      }
      best_move = moves.get(0);
      pos.undoMove(moves.get(0), undo);
      if (eval >= beta || moves.size() == 1)
      {
        return {best_move, eval};
      }
      alpha = std::max(alpha, eval);

      const int split_alpha = alpha;
      std::vector< int > results(moves.size(), MIN);
      std::atomic< int > next_index(1);
      unsigned worker_count = threads_ > 1
        ? std::min< unsigned >(threads_ - 1, static_cast< unsigned >(moves.size() - 1))
        : 0;
      std::vector< SearchNodes > thread_nodes(worker_count + 1);
      std::vector< std::thread > workers;
      workers.reserve(worker_count);

      for (unsigned thread_index = 0; thread_index < worker_count; ++thread_index)
      {
        workers.push_back(std::thread([&, thread_index](){
          while (!stopped_.load())
          {
            int move_index = next_index.fetch_add(1);
            if (move_index >= moves.size())
            {
              break;
            }
            Position local_pos = pos;
            UndoInfo local_undo;
            uint64_t local_hash = incrementZobristHash(init_hash, local_pos, moves.get(move_index));
            local_pos.makeMove(moves.get(move_index), local_undo);
            results[move_index] = -negamax(local_pos, depth - 1, -beta, -split_alpha, 0,
              thread_nodes[thread_index + 1], local_hash);
            local_pos.undoMove(moves.get(move_index), local_undo);
          }
        }));
      }

      while (!stopped_.load())
      {
        int move_index = next_index.fetch_add(1);
        if (move_index >= moves.size())
        {
          break;
        }
        Position local_pos = pos;
        UndoInfo local_undo;
        hash = incrementZobristHash(init_hash, local_pos, moves.get(move_index));
        local_pos.makeMove(moves.get(move_index), local_undo);
        results[move_index] = -negamax(local_pos, depth - 1, -beta, -split_alpha, 0,
          thread_nodes[0], hash);
        local_pos.undoMove(moves.get(move_index), local_undo);
      }

      for (size_t i = 0; i < workers.size(); ++i)
      {
        workers[i].join();
      }
      for (size_t i = 0; i < thread_nodes.size(); ++i)
      {
        nodes.nnodes += thread_nodes[i].nnodes;
        nodes.qnodes += thread_nodes[i].qnodes;
      }
      for (int i = 1; i < moves.size(); ++i)
      {
        if (results[i] > eval)
        {
          eval = results[i];
          best_move = moves.get(i);
        }
      }
      return {best_move, eval};
    }

    for (int i = 0; i < moves.size(); ++i)
    {
      if (isTimeUp())
      {
        break;
      }
      MvBestMoveToBeg(moves, i);
      uint64_t hash = incrementZobristHash(init_hash, pos, moves.get(i));
      pos.makeMove(moves.get(i), undo);
      int cur_eval = -negamax(pos, depth - 1, -beta, -alpha, 0, nodes, hash);
      if (stopped_.load())
      {
        pos.undoMove(moves.get(i), undo);
        break;
      }
      if (cur_eval > eval)
      {
        eval = cur_eval;
        best_move = moves.get(i);
        alpha = eval;
      }
      pos.undoMove(moves.get(i), undo);

      if (alpha >= beta)
      {
        break;
      }
    }

    return {best_move, eval};
  }

  void Engine::rateMoves(MoveArray& moves, Position& pos, const Move& tt_move, const Move& prev_best)
  {
    for (int i = 0; i < moves.size(); ++i)
    {
      rateMove(moves.moves_[i], pos, tt_move, prev_best);
    }
  }

  void Engine::rateMove(Move& move, Position& pos, const Move& tt_move, const Move& prev_best)
  {
    int score = 0;

    if (move == prev_best)
    {
      move.score_ = 200000;
      return;
    }

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

    score += pieceSquareDelta(pos, move);

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

    score += pieceSquareDelta(pos, move);

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
    : Engine(1ULL << 22, defaultThreadCount())
  {
  }

  Engine::Engine(uint64_t size):
    Engine(size, defaultThreadCount())
  {}

  Engine::Engine(uint64_t size, unsigned threads):
    tt_(size),
    stopped_(false),
    threads_(threads == 0 ? 1 : threads)
  {
    initZobristHash();
  }

  bool Engine::isTimeUp()
  {
    if (use_time_ && std::chrono::steady_clock::now() >= deadline_)
    {
      stopped_.store(true);
      return true;
    }
    return false;
  }

  void Engine::setThreadCount(unsigned threads)
  {
    threads_ = threads == 0 ? 1 : threads;
  }

  unsigned Engine::getThreadCount() const
  {
    return threads_;
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
