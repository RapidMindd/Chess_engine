#ifndef MOVE_HPP
#define MOVE_HPP

#include <cstdint>
#include <iosfwd>
#include <string>

#include "piece.hpp"
#include "square.hpp"

namespace chess
{
  struct Position;

  /// Packed into 8 bytes so that a whole MoveArray stays inside a handful of
  /// cache lines per node. The field order is kept aggregate-initialisable as
  /// {from, to, promotion, en passant, castling}; the members deliberately have
  /// no default initialisers, which keeps Move trivial and makes declaring a
  /// MoveArray free instead of running 256 constructors. Brace initialisation
  /// still zeroes every field that is left out.
  struct Move
  {
    Square from_;
    Square to_;
    Piece promotionPiece_;
    bool isEnPassant_;
    bool isCastling_;
    int16_t score_;

    /// defaulted, so `Move moves[256]` costs nothing to declare
    Move() = default;

    constexpr Move(Square from, Square to, Piece promotion = EMPTY,
      bool en_passant = false, bool castling = false) noexcept:
      from_(from),
      to_(to),
      promotionPiece_(promotion),
      isEnPassant_(en_passant),
      isCastling_(castling),
      score_(0)
    {}
  };

  static_assert(sizeof(Move) == 8, "Move is expected to be 8 bytes");

  const Move null_move = {A1, A1};

  /// 218 is the largest number of legal moves in a reachable position
  constexpr int MAX_MOVES = 256;

  /// A plain stack buffer: no allocation, and every accessor is inline so that
  /// generating moves stays a tight loop even without link time optimisation.
  struct MoveArray
  {
    friend struct Engine;

    MoveArray() noexcept:
      size_(0)
    {}

    void push(const Move& move) noexcept
    {
      if (size_ < MAX_MOVES)
      {
        moves_[size_++] = move;
      }
    }

    const Move& get(int index) const noexcept { return moves_[index]; }
    Move& get(int index) noexcept { return moves_[index]; }

    void clear() noexcept { size_ = 0; }
    int size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    void print() const;

  private:
    Move moves_[MAX_MOVES];
    int size_;
  };

  bool operator==(const Move& move1, const Move& move2);
  bool operator!=(const Move& move1, const Move& move2);
  std::ostream& operator<<(std::ostream& out, const Move& move);
  std::ostream& operator<<(std::ostream& out, Square square);
  std::istream& operator>>(std::istream& in, Move& move);

  bool containsMove(const MoveArray& moves, Move move);
  bool isEqualArraysUnordered(const MoveArray& moves1, const MoveArray& moves2);
  Move getMove(const MoveArray& moves, Square from, Square to);
  Move getMove(const MoveArray& moves, Move move);
  void printMove(const Move& move, const Position& pos);
  /// long algebraic notation, e.g. "e2e4" or "e7e8q"
  std::string moveToUci(const Move& move);
  /// standard algebraic notation, e.g. "Nf3", "exd5", "0-0", "Qh7#"
  std::string moveToAlgebraic(const Move& move, const Position& pos);
}

#endif
