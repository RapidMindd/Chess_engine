#include "position.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

#include "move.hpp"
#include "piece.hpp"
#include "piece_square_tables.hpp"
#include "zobrist.hpp"

namespace chess
{
  namespace
  {
    constexpr int REPETITION_MASK = REPETITION_RING - 1;

    /// castling rights that survive a move touching a given square
    struct CastlingMasks
    {
      uint8_t value[64];

      CastlingMasks()
      {
        for (int i = 0; i < 64; ++i)
        {
          value[i] = 15;
        }
        value[A1] = static_cast< uint8_t >(15 & ~WHITE_QUEEN_SIDE);
        value[H1] = static_cast< uint8_t >(15 & ~WHITE_KING_SIDE);
        value[E1] = static_cast< uint8_t >(15 & ~(WHITE_KING_SIDE | WHITE_QUEEN_SIDE));
        value[A8] = static_cast< uint8_t >(15 & ~BLACK_QUEEN_SIDE);
        value[H8] = static_cast< uint8_t >(15 & ~BLACK_KING_SIDE);
        value[E8] = static_cast< uint8_t >(15 & ~(BLACK_KING_SIDE | BLACK_QUEEN_SIDE));
      }
    };

    const CastlingMasks castling_masks;
  }

  Bitboard Position::squareMask(int square) noexcept
  {
    return 1ULL << square;
  }

  void Position::addPiece(int square, Piece piece) noexcept
  {
    if (piece == EMPTY)
    {
      return;
    }

    const int index = pieceIndexOf(piece);
    const Bitboard mask = squareMask(square);
    pieceBitboards_[index] |= mask;
    colorPieces_[piece > 0 ? 0 : 1] |= mask;
    occupied_ |= mask;
    squarePieces_[square] = piece;

    hash_ ^= zobrist_board[index][square];
    midgameScore_ += midgame_table[index][square];
    endgameScore_ += endgame_table[index][square];
    phase_ += static_cast< int16_t >(phase_values[typeOf(piece)]);
    if (typeOf(piece) == PAWN)
    {
      pawnHash_ ^= zobrist_board[index][square];
    }
  }

  void Position::erasePiece(int square, Piece piece) noexcept
  {
    if (piece == EMPTY)
    {
      return;
    }

    const int index = pieceIndexOf(piece);
    const Bitboard mask = squareMask(square);
    pieceBitboards_[index] &= ~mask;
    colorPieces_[piece > 0 ? 0 : 1] &= ~mask;
    occupied_ &= ~mask;
    squarePieces_[square] = EMPTY;

    hash_ ^= zobrist_board[index][square];
    midgameScore_ -= midgame_table[index][square];
    endgameScore_ -= endgame_table[index][square];
    phase_ -= static_cast< int16_t >(phase_values[typeOf(piece)]);
    if (typeOf(piece) == PAWN)
    {
      pawnHash_ ^= zobrist_board[index][square];
    }
  }

  void Position::movePiece(int from, int to, Piece piece) noexcept
  {
    erasePiece(from, piece);
    addPiece(to, piece);
  }

  void Position::updateCastlingRights(uint8_t new_rights) noexcept
  {
    if (new_rights != castlingRights_)
    {
      hash_ ^= zobrist_castling[castlingRights_] ^ zobrist_castling[new_rights];
      castlingRights_ = new_rights;
    }
  }

  void Position::recomputeDerivedState() noexcept
  {
    kingSquare_[0] = pieceBitboards_[pieceIndexOf(WHITE_KING)] != 0
      ? static_cast< int8_t >(lsb(pieceBitboards_[pieceIndexOf(WHITE_KING)])) : -1;
    kingSquare_[1] = pieceBitboards_[pieceIndexOf(BLACK_KING)] != 0
      ? static_cast< int8_t >(lsb(pieceBitboards_[pieceIndexOf(BLACK_KING)])) : -1;
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
      addPiece(it->first, it->second);
    }
    setWhiteToMove(whiteToMove);
    updateCastlingRights(static_cast< uint8_t >((wkc ? WHITE_KING_SIDE : 0) | (wqc ? WHITE_QUEEN_SIDE : 0)
      | (bkc ? BLACK_KING_SIDE : 0) | (bqc ? BLACK_QUEEN_SIDE : 0)));
    recomputeDerivedState();
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
      if (isdigit(static_cast< unsigned char >(section_board[i])))
      {
        cur_square += section_board[i] - '0';
      }
      else if (isalpha(static_cast< unsigned char >(section_board[i])))
      {
        addPiece(cur_square, charToPiece(section_board[i]));
        ++cur_square;
      }
      else
      {
        cur_square = ((cur_square / 8) - 2) * 8;
      }
    }

    char color = 'w';
    stream >> color;
    setWhiteToMove(color != 'b');

    std::string castlings;
    stream >> castlings;
    uint8_t rights = 0;
    for (size_t i = 0; i < castlings.size(); ++i)
    {
      switch (castlings[i])
      {
        case 'K': rights |= WHITE_KING_SIDE; break;
        case 'Q': rights |= WHITE_QUEEN_SIDE; break;
        case 'k': rights |= BLACK_KING_SIDE; break;
        case 'q': rights |= BLACK_QUEEN_SIDE; break;
      }
    }
    updateCastlingRights(rights);

    std::string en_passant_square;
    if (stream >> en_passant_square && en_passant_square != "-" && en_passant_square.size() >= 2)
    {
      const int col = en_passant_square[0] - 'a';
      const int row = en_passant_square[1] - '1';
      if (col >= 0 && col < 8 && row >= 0 && row < 8)
      {
        setEnPassantSquare(row * 8 + col);
      }
    }

    int halfmove = 0;
    if (stream >> halfmove && halfmove >= 0)
    {
      halfmoveClock_ = static_cast< uint16_t >(std::min(halfmove, 200));
    }

    recomputeDerivedState();
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
    if (colorPieces_[0] != another.colorPieces_[0]) return false;
    if (colorPieces_[1] != another.colorPieces_[1]) return false;
    if (occupied_ != another.occupied_) return false;
    if (whiteToMove_ != another.whiteToMove_) return false;
    if (castlingRights_ != another.castlingRights_) return false;
    if (enPassantSquare_ != another.enPassantSquare_) return false;
    if (kingSquare_[0] != another.kingSquare_[0]) return false;
    if (kingSquare_[1] != another.kingSquare_[1]) return false;

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
    colorPieces_[0] = 0;
    colorPieces_[1] = 0;
    occupied_ = 0;

    midgameScore_ = 0;
    endgameScore_ = 0;
    phase_ = 0;
    pawnHash_ = 0;

    whiteToMove_ = true;
    castlingRights_ = 0;
    enPassantSquare_ = -1;
    kingSquare_[0] = -1;
    kingSquare_[1] = -1;

    halfmoveClock_ = 0;
    pliesFromNull_ = 0;
    gamePly_ = 0;

    hash_ = zobrist_side ^ zobrist_castling[0];
  }

  void Position::setInitial() noexcept
  {
    clear();

    for (int i = A2; i <= H2; ++i)
    {
      addPiece(i, WHITE_PAWN);
    }
    for (int i = A7; i <= H7; ++i)
    {
      addPiece(i, BLACK_PAWN);
    }

    const Piece back_rank[8] = {
      WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP, WHITE_QUEEN,
      WHITE_KING, WHITE_BISHOP, WHITE_KNIGHT, WHITE_ROOK
    };
    for (int i = 0; i < 8; ++i)
    {
      addPiece(A1 + i, back_rank[i]);
      addPiece(A8 + i, static_cast< Piece >(-back_rank[i]));
    }

    updateCastlingRights(WHITE_KING_SIDE | WHITE_QUEEN_SIDE | BLACK_KING_SIDE | BLACK_QUEEN_SIDE);
    kingSquare_[0] = E1;
    kingSquare_[1] = E8;
  }

  int Position::getPiece(int square) const noexcept
  {
    return squarePieces_[square];
  }

  Bitboard Position::getBitboard(Piece piece) const noexcept
  {
    if (piece == EMPTY)
    {
      return 0;
    }
    return pieceBitboards_[pieceIndexOf(piece)];
  }

  Bitboard Position::getPieces(int type, bool white) const noexcept
  {
    return pieceBitboards_[white ? type - 1 : type + 5];
  }

  Bitboard Position::getWhitePieces() const noexcept
  {
    return colorPieces_[0];
  }

  Bitboard Position::getBlackPieces() const noexcept
  {
    return colorPieces_[1];
  }

  Bitboard Position::getOccupied() const noexcept
  {
    return occupied_;
  }

  Bitboard Position::getSidePieces(bool white) const noexcept
  {
    return colorPieces_[white ? 0 : 1];
  }

  bool Position::isWhiteToMove() const noexcept
  {
    return whiteToMove_;
  }

  uint64_t Position::hash() const noexcept
  {
    return hash_;
  }

  uint64_t Position::pawnHash() const noexcept
  {
    return pawnHash_;
  }

  int Position::midgameScore() const noexcept
  {
    return midgameScore_;
  }

  int Position::endgameScore() const noexcept
  {
    return endgameScore_;
  }

  int Position::phase() const noexcept
  {
    return phase_;
  }

  int Position::halfmoveClock() const noexcept
  {
    return halfmoveClock_;
  }

  int Position::nonPawnMaterial(bool white) const noexcept
  {
    return popcount(getPieces(KNIGHT, white)) * midgame_values[KNIGHT]
      + popcount(getPieces(BISHOP, white)) * midgame_values[BISHOP]
      + popcount(getPieces(ROOK, white)) * midgame_values[ROOK]
      + popcount(getPieces(QUEEN, white)) * midgame_values[QUEEN];
  }

  bool Position::isRepetition() const noexcept
  {
    const int limit = std::min< int >(halfmoveClock_, pliesFromNull_);
    const int end = std::min< int >(limit, gamePly_);
    for (int i = 4; i <= end; i += 2)
    {
      if (repetitionRing_[(gamePly_ - i) & REPETITION_MASK] == hash_)
      {
        return true;
      }
    }
    return false;
  }

  bool Position::isFiftyMoveDraw() const noexcept
  {
    return halfmoveClock_ >= 100;
  }

  bool Position::isInsufficientMaterial() const noexcept
  {
    if ((pieceBitboards_[pieceIndexOf(WHITE_PAWN)] | pieceBitboards_[pieceIndexOf(BLACK_PAWN)]) != 0)
    {
      return false;
    }
    if ((pieceBitboards_[pieceIndexOf(WHITE_ROOK)] | pieceBitboards_[pieceIndexOf(BLACK_ROOK)]
      | pieceBitboards_[pieceIndexOf(WHITE_QUEEN)] | pieceBitboards_[pieceIndexOf(BLACK_QUEEN)]) != 0)
    {
      return false;
    }
    /// only kings and at most one minor piece per side are left
    return popcount(colorPieces_[0]) <= 2 && popcount(colorPieces_[1]) <= 2;
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

  std::string Position::toFen() const
  {
    std::string fen;
    for (int row = 7; row >= 0; --row)
    {
      int empty = 0;
      for (int col = 0; col < 8; ++col)
      {
        const Piece piece = squarePieces_[row * 8 + col];
        if (piece == EMPTY)
        {
          ++empty;
          continue;
        }
        if (empty != 0)
        {
          fen += static_cast< char >('0' + empty);
          empty = 0;
        }
        fen += pieceToChar(piece);
      }
      if (empty != 0)
      {
        fen += static_cast< char >('0' + empty);
      }
      if (row != 0)
      {
        fen += '/';
      }
    }

    fen += whiteToMove_ ? " w " : " b ";
    if (castlingRights_ == 0)
    {
      fen += '-';
    }
    else
    {
      if (castlingRights_ & WHITE_KING_SIDE) fen += 'K';
      if (castlingRights_ & WHITE_QUEEN_SIDE) fen += 'Q';
      if (castlingRights_ & BLACK_KING_SIDE) fen += 'k';
      if (castlingRights_ & BLACK_QUEEN_SIDE) fen += 'q';
    }

    fen += ' ';
    if (enPassantSquare_ < 0)
    {
      fen += '-';
    }
    else
    {
      fen += static_cast< char >('a' + fileOf(enPassantSquare_));
      fen += static_cast< char >('1' + rankOf(enPassantSquare_));
    }

    fen += ' ';
    fen += std::to_string(halfmoveClock_);
    fen += " 1";
    return fen;
  }

  void Position::makeMove(const Move& move, UndoInfo& undo) noexcept
  {
    const bool white = whiteToMove_;
    const int side = white ? 1 : -1;
    const Piece moving_piece = squarePieces_[move.from_];
    const Piece captured_piece = move.isEnPassant_
      ? makePiece(PAWN, !white) : squarePieces_[move.to_];

    undo.capturedPiece_ = captured_piece;
    undo.enPassantSquare_ = enPassantSquare_;
    undo.hash_ = hash_;
    undo.pawnHash_ = pawnHash_;
    undo.midgameScore_ = midgameScore_;
    undo.endgameScore_ = endgameScore_;
    undo.phase_ = phase_;
    undo.halfmoveClock_ = halfmoveClock_;
    undo.pliesFromNull_ = pliesFromNull_;
    undo.whiteKingCastling_ = (castlingRights_ & WHITE_KING_SIDE) != 0;
    undo.whiteQueenCastling_ = (castlingRights_ & WHITE_QUEEN_SIDE) != 0;
    undo.blackKingCastling_ = (castlingRights_ & BLACK_KING_SIDE) != 0;
    undo.blackQueenCastling_ = (castlingRights_ & BLACK_QUEEN_SIDE) != 0;

    repetitionRing_[gamePly_ & REPETITION_MASK] = hash_;
    ++gamePly_;
    ++halfmoveClock_;
    ++pliesFromNull_;

    if (enPassantSquare_ >= 0)
    {
      hash_ ^= zobrist_enpassant[enPassantSquare_];
      enPassantSquare_ = -1;
    }

    if (captured_piece != EMPTY)
    {
      erasePiece(move.isEnPassant_ ? move.to_ - 8 * side : static_cast< int >(move.to_), captured_piece);
      halfmoveClock_ = 0;
    }

    erasePiece(move.from_, moving_piece);
    addPiece(move.to_, move.promotionPiece_ != EMPTY ? move.promotionPiece_ : moving_piece);

    const int moving_type = typeOf(moving_piece);
    if (moving_type == PAWN)
    {
      halfmoveClock_ = 0;
      if (move.to_ == move.from_ + 16 * side)
      {
        enPassantSquare_ = static_cast< int8_t >(move.from_ + 8 * side);
        hash_ ^= zobrist_enpassant[enPassantSquare_];
      }
    }
    else if (moving_type == KING)
    {
      kingSquare_[white ? 0 : 1] = static_cast< int8_t >(move.to_);
      if (move.isCastling_)
      {
        const Piece rook = makePiece(ROOK, white);
        if (move.to_ > move.from_)
        {
          movePiece(move.to_ + 1, move.to_ - 1, rook);
        }
        else
        {
          movePiece(move.to_ - 2, move.to_ + 1, rook);
        }
      }
    }

    updateCastlingRights(static_cast< uint8_t >(castlingRights_
      & castling_masks.value[move.from_] & castling_masks.value[move.to_]));

    whiteToMove_ = !white;
    hash_ ^= zobrist_side;
  }

  void Position::undoMove(const Move& move, const UndoInfo& undo) noexcept
  {
    whiteToMove_ = !whiteToMove_;
    const bool white = whiteToMove_;
    const int side = white ? 1 : -1;

    const Piece piece_on_to = squarePieces_[move.to_];
    erasePiece(move.to_, piece_on_to);
    const Piece original_piece = move.promotionPiece_ != EMPTY ? makePiece(PAWN, white) : piece_on_to;
    addPiece(move.from_, original_piece);

    if (undo.capturedPiece_ != EMPTY)
    {
      addPiece(move.isEnPassant_ ? move.to_ - 8 * side : static_cast< int >(move.to_), undo.capturedPiece_);
    }

    if (typeOf(original_piece) == KING)
    {
      kingSquare_[white ? 0 : 1] = static_cast< int8_t >(move.from_);
      if (move.isCastling_)
      {
        const Piece rook = makePiece(ROOK, white);
        if (move.to_ > move.from_)
        {
          movePiece(move.to_ - 1, move.to_ + 1, rook);
        }
        else
        {
          movePiece(move.to_ + 1, move.to_ - 2, rook);
        }
      }
    }

    castlingRights_ = static_cast< uint8_t >((undo.whiteKingCastling_ ? WHITE_KING_SIDE : 0)
      | (undo.whiteQueenCastling_ ? WHITE_QUEEN_SIDE : 0)
      | (undo.blackKingCastling_ ? BLACK_KING_SIDE : 0)
      | (undo.blackQueenCastling_ ? BLACK_QUEEN_SIDE : 0));
    enPassantSquare_ = static_cast< int8_t >(undo.enPassantSquare_);
    halfmoveClock_ = undo.halfmoveClock_;
    pliesFromNull_ = undo.pliesFromNull_;
    hash_ = undo.hash_;
    pawnHash_ = undo.pawnHash_;
    midgameScore_ = undo.midgameScore_;
    endgameScore_ = undo.endgameScore_;
    phase_ = undo.phase_;
    --gamePly_;
  }

  void Position::makeNullMove(UndoInfo& undo) noexcept
  {
    undo.capturedPiece_ = EMPTY;
    undo.enPassantSquare_ = enPassantSquare_;
    undo.hash_ = hash_;
    undo.pawnHash_ = pawnHash_;
    undo.midgameScore_ = midgameScore_;
    undo.endgameScore_ = endgameScore_;
    undo.phase_ = phase_;
    undo.halfmoveClock_ = halfmoveClock_;
    undo.pliesFromNull_ = pliesFromNull_;

    repetitionRing_[gamePly_ & REPETITION_MASK] = hash_;
    ++gamePly_;
    ++halfmoveClock_;
    pliesFromNull_ = 0;

    if (enPassantSquare_ >= 0)
    {
      hash_ ^= zobrist_enpassant[enPassantSquare_];
      enPassantSquare_ = -1;
    }
    whiteToMove_ = !whiteToMove_;
    hash_ ^= zobrist_side;
  }

  void Position::undoNullMove(const UndoInfo& undo) noexcept
  {
    whiteToMove_ = !whiteToMove_;
    enPassantSquare_ = static_cast< int8_t >(undo.enPassantSquare_);
    halfmoveClock_ = undo.halfmoveClock_;
    pliesFromNull_ = undo.pliesFromNull_;
    hash_ = undo.hash_;
    --gamePly_;
  }

  void Position::placePiece(int square, Piece piece)
  {
    removePiece(square);
    addPiece(square, piece);
    if (typeOf(piece) == KING)
    {
      kingSquare_[piece > 0 ? 0 : 1] = static_cast< int8_t >(square);
    }
  }

  void Position::removePiece(int square)
  {
    const Piece piece = squarePieces_[square];
    if (typeOf(piece) == KING)
    {
      kingSquare_[piece > 0 ? 0 : 1] = -1;
    }
    erasePiece(square, piece);
  }

  int Position::getEnPassantSquare() const noexcept
  {
    return enPassantSquare_;
  }

  void Position::setEnPassantSquare(int square)
  {
    if (enPassantSquare_ >= 0)
    {
      hash_ ^= zobrist_enpassant[enPassantSquare_];
    }
    enPassantSquare_ = static_cast< int8_t >(square);
    if (enPassantSquare_ >= 0)
    {
      hash_ ^= zobrist_enpassant[enPassantSquare_];
    }
  }

  void Position::setWhiteToMove(bool white) noexcept
  {
    if (white != whiteToMove_)
    {
      whiteToMove_ = white;
      hash_ ^= zobrist_side;
    }
  }

  int Position::getOppositeColourKingSquare() const noexcept
  {
    return kingSquare_[whiteToMove_ ? 1 : 0];
  }

  int Position::getCurentColourKingSquare() const noexcept
  {
    return kingSquare_[whiteToMove_ ? 0 : 1];
  }

  Castling Position::getCastling() const noexcept
  {
    if (whiteToMove_)
    {
      return Castling{(castlingRights_ & WHITE_KING_SIDE) != 0, (castlingRights_ & WHITE_QUEEN_SIDE) != 0};
    }
    return Castling{(castlingRights_ & BLACK_KING_SIDE) != 0, (castlingRights_ & BLACK_QUEEN_SIDE) != 0};
  }

  Position Position::getToggledSideToMovePosition() const
  {
    Position new_pos = *this;
    new_pos.setWhiteToMove(!new_pos.whiteToMove_);
    return new_pos;
  }

  int Position::getCastlingRights() const noexcept
  {
    return castlingRights_;
  }

  int Position::getWhiteKingSquare() const noexcept
  {
    return kingSquare_[0];
  }

  int Position::getBlackKingSquare() const noexcept
  {
    return kingSquare_[1];
  }
}
