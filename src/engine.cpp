#include "engine.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#include "bitboard.hpp"
#include "piece.hpp"
#include "piece_square_tables.hpp"
#include "zobrist.hpp"

namespace chess
{
  namespace
  {
    /// how often the clock is looked at, in nodes
    constexpr uint64_t TIME_CHECK_INTERVAL = 2048;

    constexpr int MAX_HISTORY = 8192;

    int reductionTable[64][64];

    struct ReductionInit
    {
      ReductionInit()
      {
        for (int depth = 0; depth < 64; ++depth)
        {
          for (int moves = 0; moves < 64; ++moves)
          {
            if (depth == 0 || moves == 0)
            {
              reductionTable[depth][moves] = 0;
              continue;
            }
            const double value = 0.80 + std::log(double(depth)) * std::log(double(moves)) / 2.30;
            reductionTable[depth][moves] = static_cast< int >(value);
          }
        }
      }
    };

    const ReductionInit reduction_init;

    int lmrReduction(int depth, int move_number)
    {
      return reductionTable[std::min(depth, 63)][std::min(move_number, 63)];
    }

    bool isCapture(const Position& pos, const Move& move)
    {
      return pos.getPiece(move.to_) != EMPTY || move.isEnPassant_;
    }

    bool isQuiet(const Position& pos, const Move& move)
    {
      return !isCapture(pos, move) && move.promotionPiece_ == EMPTY;
    }

    /// mate scores are stored relative to the node they were found in, so that
    /// the same entry stays correct when it is reused at another distance
    int scoreToTT(int score, int ply)
    {
      if (score >= VALUE_MATE_IN_MAX_PLY)
      {
        return score + ply;
      }
      if (score <= -VALUE_MATE_IN_MAX_PLY)
      {
        return score - ply;
      }
      return score;
    }

    int scoreFromTT(int score, int ply)
    {
      if (score >= VALUE_MATE_IN_MAX_PLY)
      {
        return score - ply;
      }
      if (score <= -VALUE_MATE_IN_MAX_PLY)
      {
        return score + ply;
      }
      return score;
    }

    bool ttCutoff(const TTEntry& entry, int depth, int alpha, int beta, int ply, int& score)
    {
      if (!entry.used_ || entry.depth_ < depth)
      {
        return false;
      }
      const int value = scoreFromTT(entry.eval_, ply);
      if (entry.type_ == EXACT
        || (entry.type_ == LOWER_BOUND && value >= beta)
        || (entry.type_ == UPPER_BOUND && value <= alpha))
      {
        score = value;
        return true;
      }
      return false;
    }

    void bonusTo(int16_t& slot, int bonus)
    {
      const int value = slot;
      slot = static_cast< int16_t >(value + bonus - value * std::abs(bonus) / MAX_HISTORY);
    }

    int historyBonus(int depth)
    {
      return std::min(depth * depth * 16 + 32 * depth, 1800);
    }
  }

  void SearchThread::clear()
  {
    std::memset(history, 0, sizeof(history));
    std::memset(captureHistory, 0, sizeof(captureHistory));
    for (int i = 0; i < MAX_PLY + 4; ++i)
    {
      killers[i][0] = null_move;
      killers[i][1] = null_move;
    }
    for (int i = 0; i < 12; ++i)
    {
      for (int j = 0; j < 64; ++j)
      {
        counterMoves[i][j] = null_move;
      }
    }
    for (int i = 0; i <= MAX_PLY; ++i)
    {
      pvLength[i] = 0;
    }
    normalNodes = 0;
    quiescenceNodes = 0;
    publishedNodes.store(0, std::memory_order_relaxed);
    rootBest = null_move;
    rootScore = 0;
    completedDepth = 0;
    selDepth = 0;
  }

  Engine::Engine():
    tt_(64),
    stopped_(false)
  {
    setThreads(1);
  }

  Engine::Engine(uint64_t size):
    stopped_(false)
  {
    tt_.resizeEntries(size);
    setThreads(1);
  }

  void Engine::setThreads(int threads)
  {
    const unsigned hardware = std::max(1u, std::thread::hardware_concurrency());
    threadCount_ = std::max(1, std::min(threads, static_cast< int >(hardware)));
    while (static_cast< int >(threads_.size()) < threadCount_)
    {
      threads_.emplace_back(new SearchThread());
      threads_.back()->id = static_cast< int >(threads_.size()) - 1;
      threads_.back()->clear();
    }
  }

  int Engine::getThreads() const
  {
    return threadCount_;
  }

  void Engine::setHashSizeMb(uint64_t megabytes)
  {
    tt_.resize(megabytes);
  }

  void Engine::newGame()
  {
    tt_.clear();
    for (size_t i = 0; i < threads_.size(); ++i)
    {
      threads_[i]->clear();
    }
  }

  void Engine::setInfoOutput(bool enabled)
  {
    printInfo_ = enabled;
  }

  void Engine::stop()
  {
    stopped_.store(true, std::memory_order_relaxed);
  }

  bool Engine::isTimeUp()
  {
    if (useTime_ && std::chrono::steady_clock::now() >= deadline_)
    {
      stopped_.store(true, std::memory_order_relaxed);
      return true;
    }
    return false;
  }

  bool Engine::checkStop(SearchThread& thread)
  {
    if (stopped_.load(std::memory_order_relaxed))
    {
      return true;
    }
    const uint64_t total = thread.searchedNodes();
    if ((total & (TIME_CHECK_INTERVAL - 1)) != 0)
    {
      return false;
    }
    thread.publishNodes();
    if (nodeLimit_ != 0)
    {
      uint64_t searched = 0;
      for (size_t i = 0; i < threads_.size(); ++i)
      {
        searched += threads_[i]->publishedNodes.load(std::memory_order_relaxed);
      }
      if (searched >= nodeLimit_)
      {
        stopped_.store(true, std::memory_order_relaxed);
        return true;
      }
    }
    return isTimeUp();
  }

  void Engine::updatePv(SearchThread& thread, int ply, const Move& move)
  {
    thread.pv[ply][0] = move;
    const int child_length = thread.pvLength[ply + 1];
    for (int i = 0; i < child_length && i + 1 <= MAX_PLY; ++i)
    {
      thread.pv[ply][i + 1] = thread.pv[ply + 1][i];
    }
    thread.pvLength[ply] = std::min(child_length + 1, MAX_PLY);
  }

  void Engine::pickBestMove(MoveArray& moves, int index)
  {
    int best = index;
    int best_score = moves.get(index).score_;
    for (int i = index + 1; i < moves.size(); ++i)
    {
      if (moves.get(i).score_ > best_score)
      {
        best_score = moves.get(i).score_;
        best = i;
      }
    }
    if (best != index)
    {
      std::swap(moves.get(best), moves.get(index));
    }
  }

  void Engine::scoreMoves(SearchThread& thread, MoveArray& moves, const Move& tt_move, int ply,
    bool captures_only)
  {
    const Position& pos = thread.position;
    const bool white = pos.isWhiteToMove();
    const int side_index = white ? 0 : 1;
    const Move& killer1 = thread.killers[ply][0];
    const Move& killer2 = thread.killers[ply][1];

    Move counter = null_move;
    if (!captures_only && ply > 0)
    {
      const Move& previous = thread.stack[ply - 1].currentMove;
      const int previous_piece = thread.stack[ply - 1].movedPiece;
      if (previous != null_move && previous_piece != EMPTY)
      {
        counter = thread.counterMoves[pieceIndexOf(previous_piece)][previous.to_];
      }
    }

    for (int i = 0; i < moves.size(); ++i)
    {
      Move& move = moves.get(i);
      if (move == tt_move)
      {
        move.score_ = 30000;
        continue;
      }

      const int captured = move.isEnPassant_ ? PAWN : typeOf(pos.getPiece(move.to_));
      if (captured != NO_PIECE_TYPE)
      {
        const int attacker = typeOf(pos.getPiece(move.from_));
        const int mvv_lva = see_weights[captured] * 8 - see_weights[attacker] / 8;
        const int history_score =
          thread.captureHistory[pieceIndexOf(pos.getPiece(move.from_))][move.to_][captured] / 16;
        if (seeGreaterOrEqual(pos, move, -20))
        {
          move.score_ = static_cast< int16_t >(std::min(20000 + mvv_lva / 4 + history_score, 29000));
        }
        else
        {
          move.score_ = static_cast< int16_t >(std::max(-20000 + mvv_lva / 4, -29000));
        }
        continue;
      }

      if (move.promotionPiece_ != EMPTY)
      {
        move.score_ = static_cast< int16_t >(typeOf(move.promotionPiece_) == QUEEN ? 25000 : -15000);
        continue;
      }

      if (move == killer1)
      {
        move.score_ = 15000;
        continue;
      }
      if (move == killer2)
      {
        move.score_ = 14000;
        continue;
      }
      if (move == counter)
      {
        move.score_ = 13000;
        continue;
      }

      const int history_score = thread.history[side_index][move.from_][move.to_] / 2;
      const int piece = pos.getPiece(move.from_);
      const int psqt = piece == EMPTY ? 0
        : midgame_psqt[typeOf(piece)][white ? 0 : 1][move.to_]
          - midgame_psqt[typeOf(piece)][white ? 0 : 1][move.from_];
      move.score_ = static_cast< int16_t >(std::max(-12000, std::min(12000, history_score + psqt)));
    }
  }

  void Engine::updateQuietStats(SearchThread& thread, const Move& move, int ply, int depth,
    const Move* tried, int tried_count)
  {
    const Position& pos = thread.position;
    const int side_index = pos.isWhiteToMove() ? 0 : 1;

    if (thread.killers[ply][0] != move)
    {
      thread.killers[ply][1] = thread.killers[ply][0];
      thread.killers[ply][0] = move;
    }

    const int bonus = historyBonus(depth);
    bonusTo(thread.history[side_index][move.from_][move.to_], bonus);
    for (int i = 0; i < tried_count; ++i)
    {
      if (tried[i] != move)
      {
        bonusTo(thread.history[side_index][tried[i].from_][tried[i].to_], -bonus);
      }
    }

    if (ply > 0)
    {
      const Move& previous = thread.stack[ply - 1].currentMove;
      const int previous_piece = thread.stack[ply - 1].movedPiece;
      if (previous != null_move && previous_piece != EMPTY)
      {
        thread.counterMoves[pieceIndexOf(previous_piece)][previous.to_] = move;
      }
    }
  }

  int Engine::quiescence(SearchThread& thread, int alpha, int beta, int ply)
  {
    Position& pos = thread.position;
    if (checkStop(thread))
    {
      return 0;
    }
    if (ply >= MAX_PLY - 1)
    {
      return thread.evaluator.relativeEval(pos);
    }
    thread.selDepth = std::max(thread.selDepth, ply);

    if (pos.isRepetition() || pos.isFiftyMoveDraw() || pos.isInsufficientMaterial())
    {
      return 0;
    }

    const LegalInfo info = MoveGenerator::buildLegalInfo(pos);
    const bool in_check = info.inCheck_;

    TTEntry entry;
    const bool has_entry = tt_.probe(pos.hash(), pos.isWhiteToMove(), entry);
    int tt_score = 0;
    if (has_entry && ttCutoff(entry, 0, alpha, beta, ply, tt_score))
    {
      return tt_score;
    }
    const Move tt_move = has_entry ? entry.bestMove_ : null_move;

    int best_score = -VALUE_INFINITE;
    if (!in_check)
    {
      best_score = has_entry && entry.staticEval_ != VALUE_NONE
        ? entry.staticEval_ : thread.evaluator.relativeEval(pos);
      if (best_score >= beta)
      {
        if (!has_entry)
        {
          tt_.store(pos.hash(), -1, scoreToTT(best_score, ply), best_score, LOWER_BOUND, null_move);
        }
        return best_score;
      }
      alpha = std::max(alpha, best_score);
    }

    MoveArray moves;
    MoveGenerator::generateActiveMoves(pos, info, moves);
    scoreMoves(thread, moves, tt_move, ply, true);

    const int alpha_original = alpha;
    Move best_move = null_move;
    UndoInfo undo;
    int legal_moves = 0;

    for (int i = 0; i < moves.size(); ++i)
    {
      pickBestMove(moves, i);
      const Move move = moves.get(i);

      if (!in_check)
      {
        /// losing captures cannot lift alpha, and neither can hopeless ones
        if (move.score_ < 0)
        {
          break;
        }
        const int captured = move.isEnPassant_ ? PAWN : typeOf(pos.getPiece(move.to_));
        const int promotion_gain = move.promotionPiece_ != EMPTY
          ? midgame_values[typeOf(move.promotionPiece_)] - midgame_values[PAWN] : 0;
        if (best_score + midgame_values[captured] + promotion_gain + 200 < alpha)
        {
          continue;
        }
      }

      ++legal_moves;
      ++thread.quiescenceNodes;
      thread.stack[ply].currentMove = move;
      thread.stack[ply].movedPiece = pos.getPiece(move.from_);
      pos.makeMove(move, undo);
      tt_.prefetch(pos.hash());
      const int score = -quiescence(thread, -beta, -alpha, ply + 1);
      pos.undoMove(move, undo);

      if (stopped_.load(std::memory_order_relaxed))
      {
        return 0;
      }

      if (score > best_score)
      {
        best_score = score;
        best_move = move;
        if (score > alpha)
        {
          alpha = score;
          if (score >= beta)
          {
            break;
          }
        }
      }
    }

    if (in_check && legal_moves == 0)
    {
      return -VALUE_MATE + ply;
    }

    const TTEntryType type = best_score >= beta ? LOWER_BOUND
      : (best_score > alpha_original ? EXACT : UPPER_BOUND);
    tt_.store(pos.hash(), 0, scoreToTT(best_score, ply), VALUE_NONE, type, best_move);
    return best_score;
  }

  int Engine::negamax(SearchThread& thread, int depth, int alpha, int beta, int ply, bool cut_node,
    bool allow_null)
  {
    Position& pos = thread.position;
    const bool pv_node = beta - alpha > 1;

    thread.pvLength[ply] = 0;

    if (depth <= 0)
    {
      return quiescence(thread, alpha, beta, ply);
    }

    if (checkStop(thread))
    {
      return 0;
    }

    if (ply > 0)
    {
      if (pos.isRepetition() || pos.isFiftyMoveDraw() || pos.isInsufficientMaterial())
      {
        return 0;
      }
      if (ply >= MAX_PLY - 1)
      {
        return thread.evaluator.relativeEval(pos);
      }

      /// mate distance pruning: a shorter mate is already known elsewhere
      alpha = std::max(alpha, -VALUE_MATE + ply);
      beta = std::min(beta, VALUE_MATE - ply - 1);
      if (alpha >= beta)
      {
        return alpha;
      }
    }

    thread.selDepth = std::max(thread.selDepth, ply);

    TTEntry entry;
    const bool has_entry = tt_.probe(pos.hash(), pos.isWhiteToMove(), entry);
    int tt_score = 0;
    if (!pv_node && has_entry && ttCutoff(entry, depth, alpha, beta, ply, tt_score))
    {
      return tt_score;
    }
    Move tt_move = has_entry ? entry.bestMove_ : null_move;
    if (tt_move != null_move && !MoveGenerator::isPseudoLegal(pos, tt_move))
    {
      tt_move = null_move;
    }

    const LegalInfo info = MoveGenerator::buildLegalInfo(pos);
    const bool in_check = info.inCheck_;

    int static_eval = VALUE_NONE;
    if (!in_check)
    {
      static_eval = has_entry && entry.staticEval_ != VALUE_NONE
        ? entry.staticEval_ : thread.evaluator.relativeEval(pos);
    }
    thread.stack[ply].staticEval = static_eval;

    /// "improving" tells whether our position got better than two plies ago;
    /// when it did not, pruning may be more aggressive
    const bool improving = !in_check && ply >= 2
      && thread.stack[ply - 2].staticEval != VALUE_NONE
      && static_eval > thread.stack[ply - 2].staticEval;

    if (!pv_node && !in_check && ply > 0)
    {
      /// reverse futility: a huge static advantage will not be given back
      if (depth <= 7 && static_eval - 85 * (depth - (improving ? 1 : 0)) >= beta
        && static_eval < VALUE_MATE_IN_MAX_PLY)
      {
        return static_eval;
      }

      /// razoring: hopeless positions get verified by the quiescence search
      if (depth <= 3 && static_eval + 140 * depth < alpha)
      {
        const int score = quiescence(thread, alpha - 1, alpha, ply);
        if (score < alpha)
        {
          return score;
        }
      }

      /// null move pruning, disabled where zugzwang is likely
      if (allow_null && depth >= 3 && static_eval >= beta
        && pos.nonPawnMaterial(pos.isWhiteToMove()) > 0)
      {
        const int reduction = 3 + depth / 5 + std::min(3, (static_eval - beta) / 200);
        UndoInfo undo;
        pos.makeNullMove(undo);
        thread.stack[ply].currentMove = null_move;
        thread.stack[ply].movedPiece = EMPTY;
        const int score = -negamax(thread, depth - reduction, -beta, -beta + 1, ply + 1, !cut_node, false);
        pos.undoNullMove(undo);

        if (stopped_.load(std::memory_order_relaxed))
        {
          return 0;
        }
        if (score >= beta)
        {
          return score >= VALUE_MATE_IN_MAX_PLY ? beta : score;
        }
      }
    }

    /// with no move to follow, a shallower search is cheaper than guessing
    if (depth >= 6 && tt_move == null_move && !in_check)
    {
      negamax(thread, depth - 4, alpha, beta, ply, cut_node, false);
      TTEntry refreshed;
      if (tt_.probe(pos.hash(), pos.isWhiteToMove(), refreshed) && refreshed.bestMove_ != null_move
        && MoveGenerator::isPseudoLegal(pos, refreshed.bestMove_))
      {
        tt_move = refreshed.bestMove_;
      }
    }

    MoveArray moves;
    MoveGenerator::generateLegalMoves(pos, info, moves);
    if (moves.empty())
    {
      return in_check ? -VALUE_MATE + ply : 0;
    }
    scoreMoves(thread, moves, tt_move, ply, false);

    const int alpha_original = alpha;
    int best_score = -VALUE_INFINITE;
    Move best_move = null_move;
    /// quiet moves that failed, used to punish them when another one cuts off
    Move quiets_tried[48];
    int quiets_count = 0;
    UndoInfo undo;
    int move_number = 0;

    const bool futile = !pv_node && !in_check && depth <= 6
      && static_eval + 110 * depth + 90 <= alpha;

    for (int i = 0; i < moves.size(); ++i)
    {
      pickBestMove(moves, i);
      const Move move = moves.get(i);
      const bool quiet = isQuiet(pos, move);
      const bool capture = isCapture(pos, move);

      const bool gives_check = MoveGenerator::givesCheck(pos, info, move);

      /// Checks are never pruned here. A sacrifice that gives check is exactly
      /// the move a static exchange evaluation rates worst and a mating attack
      /// needs most, so pruning it would hide short forced mates for several
      /// iterations.
      if (ply > 0 && !in_check && !gives_check && best_score > -VALUE_MATE_IN_MAX_PLY)
      {
        if (quiet)
        {
          /// late move pruning: deep in the list, quiet moves rarely matter
          if (depth <= 8 && move_number >= (3 + depth * depth) / (improving ? 1 : 2))
          {
            continue;
          }
          if (futile && move_number > 0)
          {
            continue;
          }
          /// giving a piece away for nothing is not worth a subtree
          if (depth <= 5 && !seeGreaterOrEqual(pos, move, -30 * depth * depth))
          {
            continue;
          }
        }
        /// scoreMoves already marked losing captures with a negative score, so
        /// the exact exchange value is only needed for those
        else if (depth <= 6 && move.score_ < 0 && !seeGreaterOrEqual(pos, move, -100 * depth))
        {
          continue;
        }
      }

      int extension = 0;
      if (gives_check && (pv_node || depth <= 8))
      {
        extension = 1;
      }

      thread.stack[ply].currentMove = move;
      thread.stack[ply].movedPiece = pos.getPiece(move.from_);
      pos.makeMove(move, undo);
      tt_.prefetch(pos.hash());
      ++thread.normalNodes;

      const int new_depth = depth - 1 + extension;
      int score;

      if (move_number == 0)
      {
        score = -negamax(thread, new_depth, -beta, -alpha, ply + 1, false, true);
      }
      else
      {
        int reduction = 0;
        if (depth >= 3 && move_number >= 2 && !in_check && !gives_check)
        {
          reduction = lmrReduction(depth, move_number);
          if (quiet)
          {
            if (!improving)
            {
              ++reduction;
            }
            if (cut_node)
            {
              ++reduction;
            }
            if (pv_node)
            {
              --reduction;
            }
            const int history_score = thread.history[pos.isWhiteToMove() ? 1 : 0][move.from_][move.to_];
            reduction -= history_score / 3000;
          }
          else
          {
            reduction = reduction / 2;
          }
          reduction = std::max(0, std::min(reduction, new_depth - 1));
        }

        score = -negamax(thread, new_depth - reduction, -alpha - 1, -alpha, ply + 1, true, true);
        if (score > alpha && reduction > 0)
        {
          score = -negamax(thread, new_depth, -alpha - 1, -alpha, ply + 1, !cut_node, true);
        }
        if (score > alpha && score < beta)
        {
          score = -negamax(thread, new_depth, -beta, -alpha, ply + 1, false, true);
        }
      }

      pos.undoMove(move, undo);

      if (stopped_.load(std::memory_order_relaxed))
      {
        return 0;
      }

      ++move_number;

      if (score > best_score)
      {
        best_score = score;
        best_move = move;
        if (score > alpha)
        {
          alpha = score;
          if (pv_node)
          {
            updatePv(thread, ply, move);
          }
          if (score >= beta)
          {
            if (quiet)
            {
              updateQuietStats(thread, move, ply, depth, quiets_tried, quiets_count);
            }
            else if (capture)
            {
              const int captured = move.isEnPassant_ ? PAWN : typeOf(pos.getPiece(move.to_));
              bonusTo(thread.captureHistory[pieceIndexOf(pos.getPiece(move.from_))][move.to_][captured],
                historyBonus(depth));
            }
            break;
          }
        }
      }

      if (quiet && quiets_count < 48)
      {
        quiets_tried[quiets_count++] = move;
      }
    }

    if (move_number == 0)
    {
      /// every move was pruned away, so this node simply fails low
      return alpha;
    }

    const TTEntryType type = best_score >= beta ? LOWER_BOUND
      : (best_score > alpha_original ? EXACT : UPPER_BOUND);
    tt_.store(pos.hash(), depth, scoreToTT(best_score, ply), static_eval, type, best_move);
    return best_score;
  }

  int Engine::searchRoot(SearchThread& thread, int depth, int alpha, int beta, Move& best_move)
  {
    Position& pos = thread.position;
    const LegalInfo info = MoveGenerator::buildLegalInfo(pos);
    MoveArray moves;
    MoveGenerator::generateLegalMoves(pos, info, moves);
    if (moves.empty())
    {
      best_move = null_move;
      return info.inCheck_ ? -VALUE_MATE : 0;
    }

    TTEntry entry;
    Move tt_move = best_move;
    if (tt_move == null_move && tt_.probe(pos.hash(), pos.isWhiteToMove(), entry))
    {
      tt_move = entry.bestMove_;
    }
    scoreMoves(thread, moves, tt_move, 0, false);

    const int alpha_original = alpha;
    int best_score = -VALUE_INFINITE;
    Move local_best = moves.get(0);
    UndoInfo undo;
    thread.pvLength[0] = 0;

    for (int i = 0; i < moves.size(); ++i)
    {
      pickBestMove(moves, i);
      const Move move = moves.get(i);
      const bool quiet = isQuiet(pos, move);

      thread.stack[0].currentMove = move;
      thread.stack[0].movedPiece = pos.getPiece(move.from_);
      pos.makeMove(move, undo);
      ++thread.normalNodes;

      int score;
      if (i == 0)
      {
        score = -negamax(thread, depth - 1, -beta, -alpha, 1, false, true);
      }
      else
      {
        const int reduction = depth >= 3 && i >= 3 && quiet
          ? std::max(0, std::min(lmrReduction(depth, i) - 1, depth - 2)) : 0;
        score = -negamax(thread, depth - 1 - reduction, -alpha - 1, -alpha, 1, true, true);
        if (score > alpha)
        {
          score = -negamax(thread, depth - 1, -beta, -alpha, 1, false, true);
        }
      }

      pos.undoMove(move, undo);

      if (stopped_.load(std::memory_order_relaxed))
      {
        break;
      }

      if (score > best_score)
      {
        best_score = score;
        local_best = move;
        if (score > alpha)
        {
          alpha = score;
          updatePv(thread, 0, move);
          if (score >= beta)
          {
            break;
          }
        }
      }
    }

    if (best_score > -VALUE_INFINITE)
    {
      best_move = local_best;
      const TTEntryType type = best_score >= beta ? LOWER_BOUND
        : (best_score > alpha_original ? EXACT : UPPER_BOUND);
      tt_.store(pos.hash(), depth, scoreToTT(best_score, 0), VALUE_NONE, type, local_best);
    }
    return best_score;
  }

  void Engine::reportIteration(const SearchThread& thread, int depth, int score, int bound) const
  {
    /// the reporting thread can read its own counter directly, the helpers only
    /// through the value they publish at every stop check
    uint64_t total = thread.searchedNodes();
    for (size_t i = 0; i < threads_.size(); ++i)
    {
      if (threads_[i].get() != &thread)
      {
        total += threads_[i]->publishedNodes.load(std::memory_order_relaxed);
      }
    }
    const double seconds = std::chrono::duration< double >(
      std::chrono::steady_clock::now() - startTime_).count();

    std::cout << "info depth " << depth << " seldepth " << thread.selDepth;
    if (std::abs(score) >= VALUE_MATE_IN_MAX_PLY)
    {
      const int plies = VALUE_MATE - std::abs(score);
      std::cout << " score mate " << (score > 0 ? (plies + 1) / 2 : -((plies + 1) / 2));
    }
    else
    {
      std::cout << " score cp " << score;
    }
    if (bound < 0)
    {
      std::cout << " upperbound";
    }
    else if (bound > 0)
    {
      std::cout << " lowerbound";
    }
    std::cout << " nodes " << total
      << " nps " << static_cast< uint64_t >(seconds > 0 ? total / seconds : 0)
      << " hashfull " << tt_.hashfull()
      << " time " << static_cast< uint64_t >(seconds * 1000) << " pv";
    for (int i = 0; i < thread.pvLength[0]; ++i)
    {
      std::cout << " " << moveToUci(thread.pv[0][i]);
    }
    std::cout << std::endl;
  }

  void Engine::iterativeDeepening(SearchThread& thread, int max_depth, bool report)
  {
    Move best_move = null_move;
    int best_score = 0;

    /// helper threads start one ply ahead on odd ids to desynchronise them
    const int skip = thread.id == 0 ? 0 : (thread.id % 2);

    for (int depth = 1 + skip; depth <= max_depth; ++depth)
    {
      thread.selDepth = 0;
      int alpha = -VALUE_INFINITE;
      int beta = VALUE_INFINITE;
      int window = 18;

      if (depth >= 5)
      {
        alpha = std::max(-VALUE_INFINITE, best_score - window);
        beta = std::min(VALUE_INFINITE, best_score + window);
      }

      int score = 0;
      while (true)
      {
        Move iteration_best = best_move;
        score = searchRoot(thread, depth, alpha, beta, iteration_best);
        if (stopped_.load(std::memory_order_relaxed))
        {
          break;
        }

        if (score <= alpha && alpha > -VALUE_INFINITE)
        {
          if (report && thread.id == 0)
          {
            reportIteration(thread, depth, score, -1);
          }
          /// widen downwards and pull beta in, the score is worse than expected
          beta = (alpha + beta) / 2;
          alpha = std::max(-VALUE_INFINITE, score - window);
          window += window / 2;
          continue;
        }
        if (score >= beta && beta < VALUE_INFINITE)
        {
          if (report && thread.id == 0)
          {
            reportIteration(thread, depth, score, 1);
          }
          beta = std::min(VALUE_INFINITE, score + window);
          window += window / 2;
          best_move = iteration_best;
          continue;
        }

        best_move = iteration_best;
        break;
      }

      if (stopped_.load(std::memory_order_relaxed))
      {
        break;
      }

      best_score = score;
      thread.rootBest = best_move;
      thread.rootScore = best_score;
      thread.completedDepth = depth;

      if (report && thread.id == 0)
      {
        reportIteration(thread, depth, score, 0);
      }

      /// a forced mate has been proven, going deeper cannot improve on it
      if (std::abs(best_score) >= VALUE_MATE_IN_MAX_PLY && depth >= 4)
      {
        break;
      }
      /// do not start an iteration there is clearly no time for
      if (useTime_ && thread.id == 0)
      {
        const double spent = std::chrono::duration< double >(
          std::chrono::steady_clock::now() - startTime_).count();
        const double budget = std::chrono::duration< double >(deadline_ - startTime_).count();
        if (spent > budget * 0.55)
        {
          break;
        }
      }
    }

    if (thread.rootBest == null_move)
    {
      thread.rootBest = best_move;
      thread.rootScore = best_score;
    }
  }

  SearchResult Engine::search(Position& pos, const SearchLimits& limits, SearchNodes* nodes)
  {
    startTime_ = std::chrono::steady_clock::now();
    stopped_.store(false, std::memory_order_relaxed);
    useTime_ = limits.timeMs > 0;
    nodeLimit_ = limits.maxNodes;
    if (useTime_)
    {
      deadline_ = startTime_ + std::chrono::milliseconds(limits.timeMs);
    }

    const int max_depth = limits.depth > 0 ? std::min(limits.depth, MAX_PLY - 2)
      : (useTime_ || limits.infinite || limits.maxNodes ? MAX_PLY - 2 : 1);

    tt_.newSearch();

    SearchResult result;
    MoveArray root_moves;
    MoveGenerator::generateLegalMoves(pos, root_moves);
    if (root_moves.empty())
    {
      result.best = null_move;
      result.centipawns = MoveGenerator::isCheck(pos) ? -VALUE_MATE : 0;
      result.score = (pos.isWhiteToMove() ? result.centipawns : -result.centipawns) / 100.0f;
      return result;
    }

    for (int i = 0; i < threadCount_; ++i)
    {
      threads_[i]->position = pos;
      threads_[i]->normalNodes = 0;
      threads_[i]->quiescenceNodes = 0;
      threads_[i]->publishedNodes.store(0, std::memory_order_relaxed);
      threads_[i]->rootBest = root_moves.get(0);
      threads_[i]->rootScore = 0;
      threads_[i]->completedDepth = 0;
      threads_[i]->selDepth = 0;
      for (int ply = 0; ply < MAX_PLY + 8; ++ply)
      {
        threads_[i]->stack[ply] = SearchThread::StackEntry();
      }
    }

    std::vector< std::thread > helpers;
    helpers.reserve(static_cast< size_t >(threadCount_ - 1));
    for (int i = 1; i < threadCount_; ++i)
    {
      SearchThread* worker = threads_[i].get();
      helpers.emplace_back([this, worker, max_depth]() { iterativeDeepening(*worker, max_depth, false); });
    }

    iterativeDeepening(*threads_[0], max_depth, printInfo_);
    stopped_.store(true, std::memory_order_relaxed);
    for (size_t i = 0; i < helpers.size(); ++i)
    {
      helpers[i].join();
    }

    /// pick the deepest finished result, breaking ties by score
    SearchThread* best_thread = threads_[0].get();
    for (int i = 1; i < threadCount_; ++i)
    {
      SearchThread* candidate = threads_[i].get();
      if (candidate->rootBest == null_move)
      {
        continue;
      }
      if (candidate->completedDepth > best_thread->completedDepth
        || (candidate->completedDepth == best_thread->completedDepth
          && candidate->rootScore > best_thread->rootScore))
      {
        best_thread = candidate;
      }
    }

    uint64_t total_normal = 0;
    uint64_t total_quiescence = 0;
    for (int i = 0; i < threadCount_; ++i)
    {
      total_normal += threads_[i]->normalNodes;
      total_quiescence += threads_[i]->quiescenceNodes;
    }
    if (nodes != nullptr)
    {
      nodes->nnodes += total_normal;
      nodes->qnodes += total_quiescence;
    }

    result.best = best_thread->rootBest == null_move ? root_moves.get(0) : best_thread->rootBest;
    result.centipawns = best_thread->rootScore;
    result.score = (pos.isWhiteToMove() ? result.centipawns : -result.centipawns) / 100.0f;
    result.depth = best_thread->completedDepth;
    result.selDepth = best_thread->selDepth;
    result.nodes = total_normal + total_quiescence;
    result.seconds = std::chrono::duration< double >(
      std::chrono::steady_clock::now() - startTime_).count();
    result.pvLength = std::min(best_thread->pvLength[0], MAX_PLY);
    for (int i = 0; i < result.pvLength; ++i)
    {
      result.pv[i] = best_thread->pv[0][i];
    }
    return result;
  }

  std::pair< Move, float > Engine::findBestMove(Position& pos, int depth, SearchNodes* nodes, int time_ms)
  {
    SearchLimits limits;
    limits.depth = depth;
    limits.timeMs = time_ms;
    const SearchResult result = search(pos, limits, nodes);
    return {result.best, result.score};
  }

  bool Engine::seeGreaterOrEqual(const Position& pos, const Move& move, int threshold)
  {
    return seeCapture(pos, move) >= threshold;
  }

  int Engine::seeCapture(const Position& pos, const Move& move)
  {
    const int to = move.to_;
    const bool mover_is_white = pos.isWhiteToMove();

    int gain[32];
    int depth = 0;

    const int captured = move.isEnPassant_ ? PAWN : typeOf(pos.getPiece(to));
    int next_victim = typeOf(pos.getPiece(move.from_));
    gain[0] = see_weights[captured];
    if (move.promotionPiece_ != EMPTY)
    {
      gain[0] += see_weights[typeOf(move.promotionPiece_)] - see_weights[PAWN];
      next_victim = typeOf(move.promotionPiece_);
    }

    Bitboard occupied = pos.getOccupied() ^ squareBB(move.from_);
    if (move.isEnPassant_)
    {
      occupied ^= squareBB(to - (mover_is_white ? 8 : -8));
    }

    const Bitboard rooks_queens = pos.getBitboard(WHITE_ROOK) | pos.getBitboard(BLACK_ROOK)
      | pos.getBitboard(WHITE_QUEEN) | pos.getBitboard(BLACK_QUEEN);
    const Bitboard bishops_queens = pos.getBitboard(WHITE_BISHOP) | pos.getBitboard(BLACK_BISHOP)
      | pos.getBitboard(WHITE_QUEEN) | pos.getBitboard(BLACK_QUEEN);

    Bitboard attackers = MoveGenerator::attackersTo(pos, to, occupied) & occupied;
    bool white = !mover_is_white;

    while (true)
    {
      const Bitboard side_attackers = attackers & pos.getSidePieces(white);
      if (side_attackers == 0)
      {
        break;
      }

      int attacker_type = 0;
      Bitboard from = 0;
      for (int type = PAWN; type <= KING; ++type)
      {
        const Bitboard candidates = side_attackers & pos.getPieces(type, white);
        if (candidates != 0)
        {
          attacker_type = type;
          from = candidates & (~candidates + 1);
          break;
        }
      }

      /// the king may only take when nothing of the other side is left
      if (attacker_type == KING && (attackers & pos.getSidePieces(!white)) != 0)
      {
        break;
      }

      ++depth;
      gain[depth] = see_weights[next_victim] - gain[depth - 1];

      occupied ^= from;
      attackers &= occupied;
      attackers |= (rookAttacks(to, occupied) & rooks_queens & occupied)
        | (bishopAttacks(to, occupied) & bishops_queens & occupied);

      next_victim = attacker_type;
      white = !white;

      if (depth >= 30)
      {
        break;
      }
    }

    /// fold the swap list back: at every step the side to move keeps the
    /// exchange going only while that beats stopping right there
    while (depth > 0)
    {
      gain[depth - 1] = -std::max(-gain[depth - 1], gain[depth]);
      --depth;
    }
    return gain[0];
  }
}
