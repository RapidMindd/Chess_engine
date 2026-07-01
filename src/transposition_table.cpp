#include "transposition_table.hpp"
#include <cstdint>

namespace chess
{
  uint64_t TranspositionTable::normalizeSize(uint64_t size) noexcept
  {
    if (size < bucket_size_)
    {
      return bucket_size_;
    }
    uint64_t remainder = size % bucket_size_;
    return remainder == 0 ? size : size + bucket_size_ - remainder;
  }

  TranspositionTable::TranspositionTable():
    TranspositionTable(1ULL << 22)
  {}

  TranspositionTable::TranspositionTable(uint64_t size):
    data_(normalizeSize(size))
  {}

  uint64_t TranspositionTable::index(uint64_t key) const noexcept
  {
    return (key % (data_.size() / bucket_size_)) * bucket_size_;
  }

  void TranspositionTable::addEntry(TTEntry entry)
  {
    uint64_t ind = index(entry.key_);
    std::lock_guard< std::mutex > lock(mutexes_[(ind / bucket_size_) & (mutex_count_ - 1)]);
    TTEntry* replace = &data_[ind];
    for (uint64_t i = 0; i < bucket_size_; ++i)
    {
      TTEntry& current = data_[ind + i];
      if (!current.used_)
      {
        current = entry;
        return;
      }
      if (current.key_ == entry.key_)
      {
        if (entry.depth_ >= current.depth_ || entry.type_ == EXACT)
        {
          current = entry;
        }
        return;
      }
      if (current.depth_ < replace->depth_ || (current.depth_ == replace->depth_ && current.type_ != EXACT))
      {
        replace = &current;
      }
    }
    *replace = entry;
  }

  TTEntry TranspositionTable::getEntry(uint64_t key)
  {
    uint64_t ind = index(key);
    std::lock_guard< std::mutex > lock(mutexes_[(ind / bucket_size_) & (mutex_count_ - 1)]);
    for (uint64_t i = 0; i < bucket_size_; ++i)
    {
      const TTEntry& entry = data_[ind + i];
      if (entry.used_ && entry.key_ == key)
      {
        return entry;
      }
    }
    return TTEntry{};
  }
}
