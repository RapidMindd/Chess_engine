#ifndef POSITION_HPP
#define POSITION_HPP

#include <cstddef>
#include <iostream>

namespace chess
{
  char pieceToChar(int piece) noexcept;

  enum Piece
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

  enum Square {
    A1 = 0, B1, C1, D1, E1, F1, G1, H1,
    A2 = 8, B2, C2, D2, E2, F2, G2, H2,
    A3 = 16, B3, C3, D3, E3, F3, G3, H3,
    A4 = 24, B4, C4, D4, E4, F4, G4, H4,
    A5 = 32, B5, C5, D5, E5, F5, G5, H5,
    A6 = 40, B6, C6, D6, E6, F6, G6, H6,
    A7 = 48, B7, C7, D7, E7, F7, G7, H7,
    A8 = 56, B8, C8, D8, E8, F8, G8, H8
  };

  struct Position
  {
  private:
    int board_[64];
    bool whiteToMove_;

    bool whiteKingCastling_;
    bool whiteQueenCastling_;
    bool blackKingCastling_;
    bool blackQueenCastling_;

    int enPassant_;

  public:
    Position();

    void setInitial() noexcept;
    void clear() noexcept;

    int getPiece(int square) const;
    bool isWhiteToMove() const noexcept;

    void print() const;
  };

  Position::Position():
  whiteToMove_(true),
  whiteKingCastling_(false),
  whiteQueenCastling_(false),
  blackKingCastling_(false),
  blackQueenCastling_(false),
  enPassant_(-1)
  {
    for (size_t i = 0; i < 64; ++i)
    {
      board_[i] = 0;
    }
  }

  void Position::clear() noexcept
  {
    for (size_t i = 0; i < 64; ++i)
    {
      board_[i] = 0;
    }

    whiteToMove_ = true;

    whiteKingCastling_ = false;
    whiteQueenCastling_ = false;
    blackKingCastling_ = false;
    blackQueenCastling_ = false;

    enPassant_ = -1;
  }

  void Position::setInitial() noexcept
  {
    clear();

    for (size_t i = A2; i <= H2; ++i)
    {
      board_[i] = WHITE_PAWN;
    }

    for (size_t i = A7; i <= H7; ++i)
    {
      board_[i] = BLACK_PAWN;
    }

    board_[A1] = WHITE_ROOK;
    board_[B1] = WHITE_KNIGHT;
    board_[C1] = WHITE_BISHOP;
    board_[D1] = WHITE_QUEEN;
    board_[E1] = WHITE_KING;
    board_[F1] = WHITE_BISHOP;
    board_[G1] = WHITE_KNIGHT;
    board_[H1] = WHITE_ROOK;

    board_[A8] = BLACK_ROOK;
    board_[B8] = BLACK_KNIGHT;
    board_[C8] = BLACK_BISHOP;
    board_[D8] = BLACK_QUEEN;
    board_[E8] = BLACK_KING;
    board_[F8] = BLACK_BISHOP;
    board_[G8] = BLACK_KNIGHT;
    board_[H8] = BLACK_ROOK;

    whiteToMove_ = true;

    whiteKingCastling_ = true;
    whiteQueenCastling_ = true;
    blackKingCastling_ = true;
    blackQueenCastling_ = true;

    enPassant_ = -1;
  }

  int Position::getPiece(int square) const
  {
    return board_[square];
  }

  bool Position::isWhiteToMove() const noexcept
  {
    return whiteToMove_;
  }

  void Position::print() const
  {
    for (int row = 7; row >= 0; --row)
    {
      std::cout << row + 1;
      for (int col = 0; col < 8; ++col)
      {
        std::cout << " " << pieceToChar(board_[8 * row + col]);
      }
      std::cout << "\n";
    }
    std::cout << "  a b c d e f g h" << "\n";
    whiteToMove_ ? std::cout << "White " : std::cout << "Black ";
    std::cout << "to move\n";
  }

  char pieceToChar(int piece) noexcept
  {
    switch (piece)
    {
      case WHITE_PAWN: return 'P';
      case WHITE_KNIGHT: return 'N';
      case WHITE_BISHOP: return 'B';
      case WHITE_ROOK: return 'R';
      case WHITE_QUEEN: return 'Q';
      case WHITE_KING: return 'K';

      case BLACK_PAWN: return 'p';
      case BLACK_KNIGHT: return 'n';
      case BLACK_BISHOP: return 'b';
      case BLACK_ROOK: return 'r';
      case BLACK_QUEEN: return 'q';
      case BLACK_KING: return 'k';

      default: return '.';
    }
  }
}

#endif