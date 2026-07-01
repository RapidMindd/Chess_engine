#ifndef TRANSPOSITION_TABLE_HPP
#define TRANSPOSITION_TABLE_HPP

#include <array>
#include <cstdint>
#include <mutex>
#include <vector>
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

  struct TTKeyHash
  {
    size_t operator()(uint64_t key) const noexcept
    {
      return static_cast< size_t >(key);
    }
  };

  struct TranspositionTable
  {
  private:
    enum
    {
      bucket_size_ = 4,
      mutex_count_ = 256
    };
    std::vector< TTEntry > data_;
    mutable std::array< std::mutex, mutex_count_ > mutexes_;
    static uint64_t normalizeSize(uint64_t size) noexcept;
    uint64_t index(uint64_t key) const noexcept;

  public:
    TranspositionTable();
    TranspositionTable(uint64_t size);

    void addEntry(TTEntry entry);
    TTEntry getEntry(uint64_t key);
  };
}

#endif
