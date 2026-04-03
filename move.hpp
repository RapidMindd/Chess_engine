#ifndef MOVE_HPP
#define MOVE_HPP

// #include "/home/yaroslav/programming/aads_2_semester/vector/class_17_03/vector.hpp"

namespace chess
{
  struct Move
  {
    int from_;
    int to_;
    int promotionPiece_ = 0;
    bool isEnPassant_ = 0;
    bool isCastling_ = 0;
  };

  struct MoveArray
  {
    MoveArray();
    void push(const Move& move) noexcept;
    const Move& get(int index) const noexcept;

    void clear() noexcept;
    int size() const noexcept;

  private:
    Move moves_[256];
    int size_;
  };

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
}

#endif
