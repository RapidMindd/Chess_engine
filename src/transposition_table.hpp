#ifndef TRANSPOSITION_TABLE_HPP
#define TRANSPOSITION_TABLE_HPP

#include <cstdint>
#include "position.hpp"
#include "move.hpp"

namespace chess
{
  enum TTEntryType
  {
    EXACT,
    LOWER_BOUND,
    UPPER_BOUND
  };

  struct TTEntry
  {
    uint64_t key_;
    int eval_;
    int depth_;
    TTEntryType type_;
    bool used_ = 0;
    Move bestMove_;
  };

  struct TranspositionTable
  {
  private:
    TTEntry* data_;
    uint64_t size_;

  public:
    TranspositionTable();
    TranspositionTable(uint64_t size);
    ~TranspositionTable();

    void addEntry(TTEntry entry);
    TTEntry& getEntry(uint64_t key);
  };
}

#endif
