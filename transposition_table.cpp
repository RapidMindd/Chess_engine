#include "transposition_table.hpp"
#include <cstdint>

namespace chess
{
  TranspositionTable::TranspositionTable()
  {
    data_ = new TTEntry[1000000];
    size_ = 1000000;
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
    data_[entry.key_ % size_] = entry;
  }

  TTEntry& TranspositionTable::getEntry(uint64_t key)
  {
    return data_[key % size_];
  }
}
