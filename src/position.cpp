#include "position.hpp"
#include "move.hpp"
#include "piece.hpp"

#include <sstream>

namespace chess
{
  int Position::bitboardIndex(Piece piece) noexcept
  {
    return piece > 0 ? piece - 1 : 5 - piece;
  }

  uint64_t Position::squareMask(int square) noexcept
  {
    return 1ULL << square;
  }

  void Position::addPiece(int square, Piece piece) noexcept
  {
    if (piece == EMPTY)
    {
      return;
    }

    const uint64_t mask = squareMask(square);
    pieceBitboards_[bitboardIndex(piece)] |= mask;
    squarePieces_[square] = piece;
    if (piece > 0)
    {
      whitePieces_ |= mask;
    }
    else
    {
      blackPieces_ |= mask;
    }
    occupied_ |= mask;
  }

  void Position::erasePiece(int square, Piece piece) noexcept
  {
    if (piece == EMPTY)
    {
      return;
    }

    const uint64_t mask = squareMask(square);
    pieceBitboards_[bitboardIndex(piece)] &= ~mask;
    squarePieces_[square] = EMPTY;
    whitePieces_ &= ~mask;
    blackPieces_ &= ~mask;
    occupied_ &= ~mask;
  }

  void Position::movePiece(int from, int to, Piece piece) noexcept
  {
    erasePiece(from, piece);
    addPiece(to, piece);
  }

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
      addPiece(square, piece);
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

  Position::Position(const char* FEN)
  {
    clear();
    std::stringstream stream(FEN);
    std::string section_board;
    stream >> section_board;
    int cur_square = 56;
    for (size_t i = 0; i < section_board.size(); ++i)
    {
      if (isdigit(section_board[i]))
      {
        cur_square += section_board[i] - '0';
      }
      else if (isalpha(section_board[i]))
      {
        Piece piece = charToPiece(section_board[i]);
        addPiece(cur_square, piece);
        if (section_board[i] == 'K') whiteKingSquare_ = cur_square;
        if (section_board[i] == 'k') blackKingSquare_ = cur_square;
        ++cur_square;
      }
      else
      {
        cur_square = ((cur_square / 8) - 2) * 8;
      }
    }

    char color;
    stream >> color;
    color == 'w' ? whiteToMove_ = 1 : whiteToMove_ = 0;

    std::string castlings;
    stream >> castlings;
    for (size_t i = 0; i < castlings.size(); ++i)
    {
      switch (castlings[i])
      {
        case 'K': whiteKingCastling_ = 1; break;
        case 'Q': whiteQueenCastling_ = 1; break;
        case 'k': blackKingCastling_ = 1; break;
        case 'q': blackQueenCastling_ = 1; break;
      }
    }

    std::string enPassantSquare;
    stream >> enPassantSquare;
    if (enPassantSquare != "-")
    {
      int col = enPassantSquare[0] - 'a';
      int row = enPassantSquare[1] - '1';
      enPassantSquare_ = row * 8 + col;
    }
    else
    {
      enPassantSquare_ = -1;
    }
  }

  bool Position::operator==(const Position& another) const noexcept
  {
    for (size_t i = 0; i < 12; ++i)
    {
      if (pieceBitboards_[i] != another.pieceBitboards_[i])
      {
        return false;
      }
    }
    if (whitePieces_ != another.whitePieces_) return false;
    if (blackPieces_ != another.blackPieces_) return false;
    if (occupied_ != another.occupied_) return false;
    if (whiteToMove_ != another.whiteToMove_) return false;

    if (whiteKingCastling_ != another.whiteKingCastling_) return false;
    if (whiteQueenCastling_ != another.whiteQueenCastling_) return false;
    if (blackKingCastling_ != another.blackKingCastling_) return false;
    if (blackQueenCastling_ != another.blackQueenCastling_) return false;

    if (enPassantSquare_ != another.enPassantSquare_) return false;

    if (whiteKingSquare_ != another.whiteKingSquare_) return false;
    if (blackKingSquare_ != another.blackKingSquare_) return false;

    return true;
  }

  void Position::clear() noexcept
  {
    for (size_t i = 0; i < 12; ++i)
    {
      pieceBitboards_[i] = 0;
    }
    for (size_t i = 0; i < 64; ++i)
    {
      squarePieces_[i] = EMPTY;
    }
    whitePieces_ = 0;
    blackPieces_ = 0;
    occupied_ = 0;

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
      addPiece(i, WHITE_PAWN);
    }

    for (size_t i = A7; i <= H7; ++i)
    {
      addPiece(i, BLACK_PAWN);
    }

    addPiece(A1, WHITE_ROOK);
    addPiece(B1, WHITE_KNIGHT);
    addPiece(C1, WHITE_BISHOP);
    addPiece(D1, WHITE_QUEEN);
    addPiece(E1, WHITE_KING);
    addPiece(F1, WHITE_BISHOP);
    addPiece(G1, WHITE_KNIGHT);
    addPiece(H1, WHITE_ROOK);

    addPiece(A8, BLACK_ROOK);
    addPiece(B8, BLACK_KNIGHT);
    addPiece(C8, BLACK_BISHOP);
    addPiece(D8, BLACK_QUEEN);
    addPiece(E8, BLACK_KING);
    addPiece(F8, BLACK_BISHOP);
    addPiece(G8, BLACK_KNIGHT);
    addPiece(H8, BLACK_ROOK);

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
    return squarePieces_[square];
  }

  uint64_t Position::getBitboard(Piece piece) const noexcept
  {
    if (piece == EMPTY)
    {
      return 0;
    }
    return pieceBitboards_[bitboardIndex(piece)];
  }

  uint64_t Position::getWhitePieces() const noexcept
  {
    return whitePieces_;
  }

  uint64_t Position::getBlackPieces() const noexcept
  {
    return blackPieces_;
  }

  uint64_t Position::getOccupied() const noexcept
  {
    return occupied_;
  }

  uint64_t Position::getSidePieces(bool white) const noexcept
  {
    return white ? whitePieces_ : blackPieces_;
  }

  bool Position::isWhiteToMove() const noexcept
  {
    return whiteToMove_;
  }

  void Position::print() const
  {
    std::cout << *this;
  }

  std::ostream& operator<<(std::ostream& out, const Position& pos)
  {
    for (int row = 7; row >= 0; --row)
    {
      out << row + 1;
      for (int col = 0; col < 8; ++col)
      {
        out << " " << pieceToChar(static_cast< Piece >(pos.getPiece(8 * row + col)));
      }
      out << "\n";
    }
    out << "  a b c d e f g h" << "\n";
    pos.isWhiteToMove() ? out << "White " : out << "Black ";
    out << "to move\n";
    return out;
  }

  void Position::makeMove(const Move& move, UndoInfo& undo) noexcept
  {
    const int is_white_piece = isWhiteToMove() ? 1 : -1;
    const Piece moving_piece = static_cast< Piece >(getPiece(move.from_));
    if (moving_piece == WHITE_KING * is_white_piece)
    {
      is_white_piece == 1 ? whiteKingSquare_ = move.to_ : blackKingSquare_ = move.to_;
    }

    undo.capturedPiece_ = static_cast< Piece >(getPiece(move.to_));
    erasePiece(move.from_, moving_piece);
    erasePiece(move.to_, undo.capturedPiece_);
    addPiece(move.to_, moving_piece);
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
      erasePiece(move.to_, moving_piece);
      addPiece(move.to_, move.promotionPiece_);
    }

    if (move.isEnPassant_)
    {
      erasePiece(move.to_ - (8 * is_white_piece), static_cast< Piece >(WHITE_PAWN * -is_white_piece));
    }

    undo.whiteKingCastling_ = whiteKingCastling_;
    undo.whiteQueenCastling_ = whiteQueenCastling_;
    undo.blackKingCastling_ = blackKingCastling_;
    undo.blackQueenCastling_ = blackQueenCastling_;

    if (move.isCastling_)
    {
      if (move.to_ - move.from_ == 2)
      {
        movePiece(move.to_ + 1, move.to_ - 1, static_cast< Piece >(WHITE_ROOK * is_white_piece));
      }
      else if (move.from_ - move.to_ == 2)
      {
        movePiece(move.to_ - 2, move.to_ + 1, static_cast< Piece >(WHITE_ROOK * is_white_piece));
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
    const Piece piece_on_to = static_cast< Piece >(getPiece(move.to_));
    if (piece_on_to == WHITE_KING * is_white_piece)
    {
      is_white_piece == 1 ? whiteKingSquare_ = move.from_ : blackKingSquare_ = move.from_;
    }

    erasePiece(move.to_, piece_on_to);
    addPiece(move.from_, move.promotionPiece_ != EMPTY ? static_cast< Piece >(WHITE_PAWN * is_white_piece) : piece_on_to);
    addPiece(move.to_, undo.capturedPiece_);
    whiteToMove_ = !whiteToMove_;

    enPassantSquare_ = undo.enPassantSquare_;

    if (move.isEnPassant_)
    {
      addPiece(move.to_ - (8 * is_white_piece), static_cast< Piece >(WHITE_PAWN * -is_white_piece));
    }

    whiteKingCastling_ = undo.whiteKingCastling_;
    whiteQueenCastling_ = undo.whiteQueenCastling_;
    blackKingCastling_ = undo.blackKingCastling_;
    blackQueenCastling_ = undo.blackQueenCastling_;

    if (move.isCastling_)
    {
      if (move.to_ - move.from_ == 2)
      {
        movePiece(move.to_ - 1, move.to_ + 1, static_cast< Piece >(WHITE_ROOK * is_white_piece));
      }
      else if (move.from_ - move.to_ == 2)
      {
        movePiece(move.to_ + 1, move.to_ - 2, static_cast< Piece >(WHITE_ROOK * is_white_piece));
      }
    }
  }

  void Position::placePiece(int square, Piece piece)
  {
    removePiece(square);
    addPiece(square, piece);
    const int is_white_piece = piece > 0 ? 1 : -1;
    if (piece == WHITE_KING * is_white_piece)
    {
      is_white_piece == 1 ? whiteKingSquare_ = square : blackKingSquare_ = square;
    }
  }

  void Position::removePiece(int square)
  {
    Piece piece = static_cast< Piece >(getPiece(square));
    if (piece == WHITE_KING)
    {
      whiteKingSquare_ = -1;
    }
    else if (piece == BLACK_KING)
    {
      blackKingSquare_ = -1;
    }
    erasePiece(square, piece);
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

  int Position::getCurentColourKingSquare() const
  {
    return isWhiteToMove() ? whiteKingSquare_ : blackKingSquare_;
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

  int Position::getCastlingRights() const
  {
    return( whiteKingCastling_ * 8) + (whiteQueenCastling_ * 4) + (blackKingCastling_ * 2) + blackQueenCastling_;
  }

  int Position::getWhiteKingSquare() const
  {
    return whiteKingSquare_;
  }

  int Position::getBlackKingSquare() const
  {
    return blackKingSquare_;
  }
}
