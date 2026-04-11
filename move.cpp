#include "move.hpp"
#include <iostream>

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

  void MoveArray::print() const
  {
    if (!empty())
    {
      std::cout << moves_[0];
    }
    for (int i = 1; i < size(); ++i)
    {
      std::cout << " " << moves_[i];
    }
    std::cout << "\n";
  }

  bool operator==(const Move& move1, const Move& move2)
  {
    return (move1.from_ == move2.from_) && (move1.to_ == move2.to_)
    && (move1.promotionPiece_ == move2.promotionPiece_)
    && (move1.isCastling_ == move2.isCastling_)
    && (move1.isEnPassant_ == move2.isEnPassant_);
  }

  bool operator!=(const Move& move1, const Move& move2)
  {
    return !(move1 == move2);
  }

  std::ostream& operator<<(std::ostream& out, const Move& move)
  {
    return out << move.from_ << "-" << move.to_;
  }

  std::ostream& operator<<(std::ostream& out, Square square)
  {
    char col = 'a' + (square % 8);
    char row = '1' + (square / 8);
    return out << col << row;
  }

  bool containsMove(const MoveArray& moves, Move move)
  {
    for (int i = 0; i < moves.size(); ++i)
    {
      if (moves.get(i) == move)
      {
        return true;
      }
    }
    return false;
  }

  bool isEqualArraysUnordered(const MoveArray& moves1, const MoveArray& moves2)
  {
    if (moves1.size() != moves2.size())
    {
      return false;
    }

    for (int i = 0; i < moves1.size(); ++i)
    {
      if (!containsMove(moves2, moves1.get(i)))
      {
        return false;
      }
    }
    return true;
  }

  Move getMove(const MoveArray& moves, Square from, Square to)
  {
    for (int i = 0; i < moves.size(); ++i)
    {
      if (moves.get(i).from_ == from && moves.get(i).to_ == to)
      {
        return moves.get(i);
      }
    }
    return Move{A1, A1};
  }
}
