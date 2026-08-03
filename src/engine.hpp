#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "evaluator.hpp"
#include "move.hpp"
#include "move_generator.hpp"
#include "position.hpp"
#include "transposition_table.hpp"

namespace chess
{
  constexpr int MAX_PLY = 128;
  constexpr int VALUE_INFINITE = 30000;
  constexpr int VALUE_MATE = 29000;
  constexpr int VALUE_MATE_IN_MAX_PLY = VALUE_MATE - MAX_PLY;
  constexpr int VALUE_NONE = 30001;

  struct SearchNodes
  {
    uint64_t nnodes = 0;
    uint64_t qnodes = 0;
  };

  struct SearchLimits
  {
    int depth = 0;
    int timeMs = 0;
    uint64_t maxNodes = 0;
    bool infinite = false;
  };

  struct SearchResult
  {
    Move best = null_move;
    /// score in centipawns from white's point of view
    float score = 0.0f;
    /// score from the point of view of the side to move
    int centipawns = 0;
    int depth = 0;
    int selDepth = 0;
    uint64_t nodes = 0;
    double seconds = 0.0;
    Move pv[MAX_PLY];
    int pvLength = 0;
  };

  struct Engine;

  /// Everything one searching thread owns. Lazy SMP runs several of these on
  /// the same shared transposition table.
  struct SearchThread
  {
    Position position;
    Evaluator evaluator;

    /// Plain counters: only the owning thread ever touches them, and making
    /// them atomic costs about a third of the search speed. They are copied
    /// into `publishedNodes` at every stop check so that the main thread has
    /// something race free to read while the search is running; after the
    /// helpers are joined the plain values can be read directly.
    uint64_t normalNodes = 0;
    uint64_t quiescenceNodes = 0;
    std::atomic< uint64_t > publishedNodes;

    uint64_t searchedNodes() const
    {
      return normalNodes + quiescenceNodes;
    }

    void publishNodes()
    {
      publishedNodes.store(searchedNodes(), std::memory_order_relaxed);
    }

    Move killers[MAX_PLY + 4][2];
    Move counterMoves[12][64];
    int16_t history[2][64][64];
    int16_t captureHistory[12][64][7];

    struct StackEntry
    {
      int staticEval = VALUE_NONE;
      Move currentMove = null_move;
      Move excluded = null_move;
      int movedPiece = EMPTY;
    };
    StackEntry stack[MAX_PLY + 8];

    Move pv[MAX_PLY + 1][MAX_PLY + 1];
    int pvLength[MAX_PLY + 1];

    Move rootBest = null_move;
    int rootScore = 0;
    int completedDepth = 0;
    int selDepth = 0;
    int id = 0;

    void clear();
  };

  struct Engine
  {
  public:
    Engine();
    /// kept for compatibility: the argument is a requested entry count
    explicit Engine(uint64_t size);

    /// number of Lazy SMP helper threads plus one, clamped to the hardware
    void setThreads(int threads);
    int getThreads() const;
    void setHashSizeMb(uint64_t megabytes);
    /// forget everything learned so far, used between unrelated positions
    void newGame();
    void setInfoOutput(bool enabled);

    SearchResult search(Position& pos, const SearchLimits& limits, SearchNodes* nodes = nullptr);
    std::pair< Move, float > findBestMove(Position& pos, int depth, SearchNodes* nodes = nullptr,
      int time_ms = 0);
    /// asks a running search to return its best move so far; safe to call from
    /// another thread, which is how the UCI "stop" command is served
    void stop();

    /// static exchange evaluation of a capture, in centipawns
    static int seeCapture(const Position& pos, const Move& move);
    /// true when the exchange on move.to_ wins at least `threshold`
    static bool seeGreaterOrEqual(const Position& pos, const Move& move, int threshold);

  private:
    TranspositionTable tt_;
    std::vector< std::unique_ptr< SearchThread > > threads_;
    int threadCount_ = 1;
    bool printInfo_ = false;

    std::atomic< bool > stopped_;
    bool useTime_ = false;
    uint64_t nodeLimit_ = 0;
    std::chrono::steady_clock::time_point deadline_;
    std::chrono::steady_clock::time_point startTime_;

    bool isTimeUp();
    bool checkStop(SearchThread& thread);

    void iterativeDeepening(SearchThread& thread, int max_depth, bool report);
    int searchRoot(SearchThread& thread, int depth, int alpha, int beta, Move& best_move);
    int negamax(SearchThread& thread, int depth, int alpha, int beta, int ply, bool cut_node,
      bool allow_null);
    int quiescence(SearchThread& thread, int alpha, int beta, int ply);

    void scoreMoves(SearchThread& thread, MoveArray& moves, const Move& tt_move, int ply, bool captures_only);
    static void pickBestMove(MoveArray& moves, int index);
    void updateQuietStats(SearchThread& thread, const Move& move, int ply, int depth,
      const Move* tried, int tried_count);
    void updatePv(SearchThread& thread, int ply, const Move& move);

    /// `bound` is -1 for a fail low, +1 for a fail high and 0 for an exact score
    void reportIteration(const SearchThread& thread, int depth, int score, int bound) const;
  };
}

#endif
