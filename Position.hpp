#ifndef POSITION_HPP
#define POSITION_HPP

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
    int board[64];
    bool whiteToMove;

    bool whiteKingCastling;
    bool whiteQueenCastling;
    bool blackKingCastling;
    bool blackQueenCastling;

    int enPassant;

  public:
    Position();

    void setInitial() noexcept;
    void clear() noexcept;

    int getPiece(int square) const;
    bool isWhiteToMove() const noexcept;

    void print() const;
  };

  Position::Position():
  whiteToMove(true),
  whiteKingCastling(false),
  whiteQueenCastling(false),
  blackKingCastling(false),
  blackQueenCastling(false),
  enPassant(-1)
  {
    for (size_t i = 0; i < 64; ++i)
    {
      board[i] = 0;
    }
  }

  void Position::clear() noexcept
  {
    for (size_t i = 0; i < 64; ++i)
    {
      board[i] = 0;
    }

    whiteToMove = true;

    whiteKingCastling = false;
    whiteQueenCastling = false;
    blackKingCastling = false;
    blackQueenCastling = false;

    enPassant = -1;
  }

  void Position::setInitial() noexcept
  {
    clear();

    for (size_t i = A2; i <= H2; ++i)
    {
      board[i] = WHITE_PAWN;
    }

    for (size_t i = A7; i <= H7; ++i)
    {
      board[i] = BLACK_PAWN;
    }
    
    board[A1] = WHITE_ROOK;
    board[B1] = WHITE_KNIGHT;
    board[C1] = WHITE_BISHOP;
    board[D1] = WHITE_QUEEN;
    board[E1] = WHITE_KING;
    board[F1] = WHITE_BISHOP;
    board[G1] = WHITE_KNIGHT;
    board[H1] = WHITE_ROOK;

    board[A8] = BLACK_ROOK;
    board[B8] = BLACK_KNIGHT;
    board[C8] = BLACK_BISHOP;
    board[D8] = BLACK_QUEEN;
    board[E8] = BLACK_KING;
    board[F8] = BLACK_BISHOP;
    board[G8] = BLACK_KNIGHT;
    board[H8] = BLACK_ROOK;

    whiteToMove = true;

    whiteKingCastling = true;
    whiteQueenCastling = true;
    blackKingCastling = true;
    blackQueenCastling = true;

    enPassant = -1;
  }

  int Position::getPiece(int square) const
  {
    return board[square];
  }

  bool Position::isWhiteToMove() const noexcept
  {
    return whiteToMove;
  }

  void Position::print() const
  {
    for (int row = 7; row >= 0; --row)
    {
      std::cout << row + 1;
      //std::cout <<pieceToChar(board[8 * row]);
      for (int col = 0; col < 8; ++col)
      {
        std::cout << " " << pieceToChar(board[8 * row + col]);
      }
      std::cout << "\n";
    }
    std::cout << "  a b c d e f g h" << "\n";
    whiteToMove ? std::cout << "White " : std::cout << "Black ";
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