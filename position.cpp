#include "position.hpp"
#include "move.hpp"

namespace chess
{
  Position::Position()
  {
    clear();
  }

  Position::Position(std::initializer_list< std::pair< Square, Piece > > pieces, bool whiteToMove,
    bool wkc, bool wqc, bool bkc, bool bqc)
  {
    clear();
    for (auto it = pieces.begin(); it != pieces.end(); ++it)
    {
      Square square = it->first;
      Piece piece = it->second;
      if (piece == WHITE_KING)
      {
        whiteKingSquare_ = square;
      }
      if (piece == BLACK_KING)
      {
        blackKingSquare_ = square;
      }

      board_[square] = piece;
      if (piece == WHITE_KING)
      {
        whiteKingSquare_ = square;
      }
      else if (piece == BLACK_KING)
      {
        blackKingSquare_ = square;
      }
    }
    whiteToMove_ = whiteToMove;

    whiteKingCastling_ = wkc;
    whiteQueenCastling_ = wqc;
    blackKingCastling_ = bkc;
    blackQueenCastling_ = bqc;
  }

  void Position::clear() noexcept
  {
    for (size_t i = 0; i < 64; ++i)
    {
      board_[i] = EMPTY;
    }

    whiteToMove_ = true;

    whiteKingCastling_ = false;
    whiteQueenCastling_ = false;
    blackKingCastling_ = false;
    blackQueenCastling_ = false;

    enPassantSquare_ = -1;

    whiteKingSquare_ = -1;
    blackKingSquare_ = -1;
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

    whiteKingSquare_ = E1;
    blackKingSquare_ = E8;
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
    const int is_white_piece = isWhiteToMove() ? 1 : -1;
    if (getPiece(move.from_) == WHITE_KING * is_white_piece)
    {
      is_white_piece == 1 ? whiteKingSquare_ = move.to_ : blackKingSquare_ = move.to_;
    }

    undo.capturedPiece_ = board_[move.to_];
    board_[move.to_] = board_[move.from_];
    board_[move.from_] = EMPTY;
    whiteToMove_ = !whiteToMove_;

    undo.enPassantSquare_ = enPassantSquare_;
    enPassantSquare_ = -1;

    if (move.to_ == move.from_ + (16 * is_white_piece)
    && getPiece(move.to_) == WHITE_PAWN * is_white_piece)
    {
      enPassantSquare_ = move.from_ + (8 * is_white_piece);
    }

    if (move.promotionPiece_ != EMPTY)
    {
      board_[move.to_] = move.promotionPiece_;
    }

    if (move.isEnPassant_)
    {
      board_[move.to_ - (8 * is_white_piece)] = EMPTY;
    }

    undo.whiteKingCastling_ = whiteKingCastling_;
    undo.whiteQueenCastling_ = whiteQueenCastling_;
    undo.blackKingCastling_ = blackKingCastling_;
    undo.blackQueenCastling_ = blackQueenCastling_;

    if (move.isCastling_)
    {
      if (move.to_ - move.from_ == 2)
      {
        board_[move.to_ - 1] = static_cast< Piece >(WHITE_ROOK * is_white_piece);
        board_[move.to_ + 1] = EMPTY;
      }
      else if (move.from_ - move.to_ == 2)
      {
        board_[move.to_ + 1] = static_cast< Piece >(WHITE_ROOK * is_white_piece);
        board_[move.to_ - 2] = EMPTY;
      }
    }

    if (move.to_ == H1 || move.from_ == H1)
    {
      whiteKingCastling_ = 0;
    }
    if (move.to_ == A1 || move.from_ == A1)
    {
      whiteQueenCastling_ = 0;
    }
    if (move.to_ == H8 || move.from_ == H8)
    {
      blackKingCastling_ = 0;
    }
    if (move.to_ == A8 || move.from_ == A8)
    {
      blackQueenCastling_ = 0;
    }
    if (move.from_ == E1)
    {
      whiteKingCastling_ = 0;
      whiteQueenCastling_ = 0;
    }
    if (move.from_ == E8)
    {
      blackKingCastling_ = 0;
      blackQueenCastling_ = 0;
    }
  }

  void Position::undoMove(const Move& move, const UndoInfo& undo) noexcept
  {
    const int is_white_piece = isWhiteToMove() ? -1 : 1;
    if (getPiece(move.to_) == WHITE_KING * is_white_piece)
    {
      is_white_piece == 1 ? whiteKingSquare_ = move.from_ : blackKingSquare_ = move.from_;
    }

    board_[move.from_] = board_[move.to_];
    board_[move.to_] = undo.capturedPiece_;
    whiteToMove_ = !whiteToMove_;

    enPassantSquare_ = undo.enPassantSquare_;

    if (move.promotionPiece_ != EMPTY)
    {
      board_[move.from_] = static_cast< Piece >(WHITE_PAWN * is_white_piece);
    }

    if (move.isEnPassant_)
    {
      board_[move.to_ - (8 * is_white_piece)] = static_cast< Piece >(WHITE_PAWN * -is_white_piece);
    }

    whiteKingCastling_ = undo.whiteKingCastling_;
    whiteQueenCastling_ = undo.whiteQueenCastling_;
    blackKingCastling_ = undo.blackKingCastling_;
    blackQueenCastling_ = undo.blackQueenCastling_;

    if (move.isCastling_)
    {
      if (move.to_ - move.from_ == 2)
      {
        board_[move.to_ - 1] = EMPTY;
        board_[move.to_ + 1] = static_cast< Piece >(WHITE_ROOK * is_white_piece);
      }
      else if (move.from_ - move.to_ == 2)
      {
        board_[move.to_ + 1] = EMPTY;
        board_[move.to_ - 2] = static_cast< Piece >(WHITE_ROOK * is_white_piece);
      }
    }
  }

  void Position::placePiece(int square, Piece piece)
  {
    board_[square] = piece;
    const int is_white_piece = piece > 0 ? 1 : -1;
    if (piece == WHITE_KING * is_white_piece)
    {
      is_white_piece == 1 ? whiteKingSquare_ = square : blackKingSquare_ = square;
    }
  }

  void Position::removePiece(int square)
  {
    if (board_[square] == WHITE_KING)
    {
      whiteKingSquare_ = -1;
    }
    else if (board_[square] == BLACK_KING)
    {
      blackKingSquare_ = -1;
    }
    board_[square] = EMPTY;
  }

  int Position::getEnPassantSquare() const
  {
    return enPassantSquare_;
  }

  void Position::setEnPassantSquare(int square)
  {
    enPassantSquare_ = square;
  }

  int Position::getOppositeColourKingSquare() const
  {
    return isWhiteToMove() ? blackKingSquare_ : whiteKingSquare_;
  }

  Castling Position::getCastling() const
  {
    if (whiteToMove_)
    {
      return Castling{whiteKingCastling_, whiteQueenCastling_};
    }
    return Castling{blackKingCastling_, blackQueenCastling_};
  }

  Position Position::getToggledSideToMovePosition() const
  {
    Position new_pos = *this;
    new_pos.whiteToMove_ = !new_pos.whiteToMove_;
    return new_pos;
  }

  char pieceToChar(Piece piece) noexcept
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
