#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP

#include <cstdint>
#include <memory>

#include "position.hpp"

namespace chess
{
  /// Small direct mapped cache for the pawn structure term, which only depends
  /// on the pawn placement and is therefore stable across most sibling nodes.
  struct PawnHashTable
  {
    static constexpr int SIZE = 1 << 14;

    struct Entry
    {
      uint64_t key = 0;
      int32_t midgame = 0;
      int32_t endgame = 0;
      bool used = false;
    };

    Entry entries[SIZE];
  };

  struct Evaluator
  {
    Evaluator();

    /// score from white's point of view, in centipawns
    int evaluate(const Position& pos);
    /// score from the point of view of the side to move
    int relativeEval(const Position& pos);

    /// stateless helpers, kept so that callers without an Evaluator still work
    static int staticEvaluate(const Position& pos);
    static int relative_eval(const Position& pos);

  private:
    /// on the heap: a few hundred kilobytes have no business on the stack
    std::unique_ptr< PawnHashTable > pawns_;

    void pawnStructure(const Position& pos, int& midgame, int& endgame);
  };
}

#endif
