#ifndef PIECE_HPP
#define PIECE_HPP

#include <cstdint>

namespace chess
{
  enum Piece : int8_t
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
    BLACK_KING = -6
  };

  /// piece type without colour: PAWN .. KING == 1 .. 6
  enum PieceType
  {
    NO_PIECE_TYPE = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6
  };

  constexpr int weights[7] = {0, 100, 320, 330, 500, 900, 0};
  /// values used by SEE, where the king has to be worth more than anything else
  constexpr int see_weights[7] = {0, 100, 320, 330, 500, 900, 30000};

  constexpr int typeOf(int piece) noexcept
  {
    return piece < 0 ? -piece : piece;
  }

  constexpr bool isWhitePiece(int piece) noexcept
  {
    return piece > 0;
  }

  /// dense index used by the bitboard array and by zobrist keys:
  /// 0..5 white pawn..king, 6..11 black pawn..king
  constexpr int pieceIndexOf(int piece) noexcept
  {
    return piece > 0 ? piece - 1 : 5 - piece;
  }

  constexpr Piece makePiece(int type, bool white) noexcept
  {
    return static_cast< Piece >(white ? type : -type);
  }

  Piece charToPiece(char c) noexcept;
  char pieceToChar(Piece piece) noexcept;
}

#endif
