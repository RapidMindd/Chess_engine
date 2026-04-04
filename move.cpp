#include "move.hpp"

namespace chess
{
  MoveArray::MoveArray():
    size_(0)
  {}

  void MoveArray::push(const Move& move) noexcept
  {
    moves_[size_] = move;
    ++size_;
  }

  const Move& MoveArray::get(int index) const noexcept
  {
    return moves_[index];
  }

  void MoveArray::clear() noexcept
  {
    size_ = 0;
  }

  int MoveArray::size() const noexcept
  {
    return size_;
  }

  bool MoveArray::empty() const noexcept
  {
    return !size_;
  }
}
