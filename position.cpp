#include "position.hpp"

namespace chess
{
  Position::Position():
  whiteToMove_(true)
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

    enPassantSquare_ = -1;
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

    enPassantSquare_ = -1;
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

  void Position::makeMove(const Move& move, UndoInfo& undo) noexcept
  {
    undo.capturedPiece_ = board_[move.to_];
    board_[move.to_] = board_[move.from_];
    board_[move.from_] = EMPTY;
    whiteToMove_ = !whiteToMove_;
  }

  void Position::undoMove(const Move& move, const UndoInfo& undo) noexcept
  {
    board_[move.from_] = board_[move.to_];
    board_[move.to_] = undo.capturedPiece_;
    whiteToMove_ = !whiteToMove_;
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
