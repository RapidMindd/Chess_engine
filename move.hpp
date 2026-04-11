#ifndef MOVE_HPP
#define MOVE_HPP

#include <iosfwd>
#include "position.hpp"

// #include "/home/yaroslav/programming/aads_2_semester/vector/class_17_03/vector.hpp"

namespace chess
{
  struct Move
  {
    Square from_;
    Square to_;
    Piece promotionPiece_ = EMPTY;
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
    bool empty() const noexcept;

    void print() const;

  private:
    Move moves_[256];
    int size_;
  };

  bool operator==(const Move& move1, const Move& move2);
  bool operator!=(const Move& move1, const Move& move2);
  std::ostream& operator<<(std::ostream& out, const Move& move);
  std::ostream& operator<<(std::ostream& out, Square square);

  bool containsMove(const MoveArray& moves, Move move);
  bool isEqualArraysUnordered(const MoveArray& moves1, const MoveArray& moves2);
  Move getMove(const MoveArray& moves, Square from, Square to);
}

#endif
