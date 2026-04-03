#ifndef MOVE_HPP
#define MOVE_HPP

// #include "/home/yaroslav/programming/aads_2_semester/vector/class_17_03/vector.hpp"

namespace chess
{
  struct Move
  {
    int from;
    int to;
    int promotionPiece = 0;
    bool isEnPassant = 0;
    bool isCastling = 0;
  };

  struct MoveArray
  {
    MoveArray();
    void push(const Move& move);
    Move& get(int index);

    void clear();
    int size();

  private:
    Move moves_[256];
    int size_;
  };

  MoveArray::MoveArray():
    size_(0)
  {}

  void MoveArray::push(const Move& move)
  {
    moves_[size_] = move;
    ++size_;
  }

  Move& MoveArray::get(int index)
  {
    return moves_[index];
  }

  void MoveArray::clear()
  {
    size_ = 0;
  }

  int MoveArray::size()
  {
    return size_;
  }
}

#endif
