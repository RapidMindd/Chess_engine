#ifndef TRANSPOSITION_TABLE_HPP
#define TRANSPOSITION_TABLE_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "move.hpp"

namespace chess
{
  enum TTEntryType
  {
    NO_BOUND = 0,
    EXACT = 1,
    LOWER_BOUND = 2,
    UPPER_BOUND = 3
  };

  /// What a probe gives back. It is a plain value, never a reference into the
  /// table, so a concurrent overwrite cannot be observed halfway.
  struct TTEntry
  {
    uint64_t key_ = 0;
    int eval_ = 0;
    int staticEval_ = 0;
    int depth_ = 0;
    TTEntryType type_ = NO_BOUND;
    bool used_ = false;
    Move bestMove_ = null_move;
  };

  /// Fixed size table with 4 way buckets and depth preferred replacement.
  ///
  /// Every slot is a pair of 64 bit words holding `key ^ data` and `data`, so a
  /// torn read produced by another search thread simply fails the key check
  /// instead of returning a corrupted entry (Hyatt's lockless hashing).
  struct TranspositionTable
  {
  public:
    static constexpr int BUCKET_SIZE = 4;

    TranspositionTable();
    explicit TranspositionTable(uint64_t megabytes);

    /// keeps the old constructor meaning: a request for a number of entries
    void resizeEntries(uint64_t entries);
    void resize(uint64_t megabytes);
    void clear();
    /// call once per search so that old entries can be recycled first
    void newSearch();

    /// `white_to_move` is only needed to give the stored promotion its colour
    bool probe(uint64_t key, bool white_to_move, TTEntry& entry) const;
    void store(uint64_t key, int depth, int eval, int static_eval, TTEntryType type, const Move& best);
    void prefetch(uint64_t key) const;

    /// permille of the table filled with entries of the current search
    int hashfull() const;
    uint64_t entryCount() const;

  private:
    struct Slot
    {
      std::atomic< uint64_t > check;
      std::atomic< uint64_t > data;
    };

    std::unique_ptr< Slot[] > slots_;
    uint64_t bucketCount_ = 0;
    uint64_t bucketMask_ = 0;
    uint8_t generation_ = 0;

    size_t bucketIndex(uint64_t key) const noexcept;
  };
}

#endif
