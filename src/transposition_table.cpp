#include "transposition_table.hpp"
#include <cstdint>

namespace chess
{
  TranspositionTable::TranspositionTable()
  {
    uint64_t size = 1ULL << 22;
    data_ = new TTEntry[size];
    size_ = size;
  }

  TranspositionTable::TranspositionTable(uint64_t size)
  {
    data_ = new TTEntry[size];
    size_ = size;
  }

  TranspositionTable::~TranspositionTable()
  {
    delete[] data_;
  }

  void TranspositionTable::addEntry(TTEntry entry)
  {
    data_[entry.key_ & (size_ - 1)] = entry;
  }

  TTEntry& TranspositionTable::getEntry(uint64_t key)
  {
    return data_[key & (size_ - 1)];
  }
}
