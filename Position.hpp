#ifndef POSITION_HPP
#define POSITION_HPP

namespace chess
{
  enum class Piece
  {
    EMPTY = 0,

    WHITE_PAWN = 1,
    WHITE_KNIGHT = 2,
    WHITE_BISHOP = 3,
    WHITE_ROOK = 4,
    WHITE_QUEEN = 5,
    WHITE_KING = 6,

    BLACK_PAWN = -1,
    BLACK_KNIGHT = -2,
    BLACK_BISHOP = -3,
    BLACK_ROOK = -4,
    BLACK_QUEEN = -5,
    BLACK_KING = -6,
  };

  struct Position
  {
  private:
    int pos[64];
    bool whiteToMove;

    bool whiteKingCastling;
    bool whiteQueenCastling;
    bool blackKingCastling;
    bool blackQueenCastling;

    int enPassant;

  public:
    Position();

    void setInitial() const noexcept;
    void clear() const noexcept;

    int getPiece(int square) const;
    bool isWhiteToMove();

    void print() const;
  };
}

#endif