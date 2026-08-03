#ifndef POSITION_HPP
#define POSITION_HPP

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iosfwd>
#include <string>
#include <utility>

#include "bitboard.hpp"
#include "piece.hpp"
#include "square.hpp"

namespace chess
{
  struct Move;

  char pieceToChar(Piece piece) noexcept;

  enum CastlingRight
  {
    BLACK_QUEEN_SIDE = 1,
    BLACK_KING_SIDE = 2,
    WHITE_QUEEN_SIDE = 4,
    WHITE_KING_SIDE = 8
  };

  struct UndoInfo
  {
    Piece capturedPiece_ = EMPTY;

    bool whiteKingCastling_ = 0;
    bool whiteQueenCastling_ = 0;
    bool blackKingCastling_ = 0;
    bool blackQueenCastling_ = 0;

    int enPassantSquare_ = -1;

    uint64_t hash_ = 0;
    uint64_t pawnHash_ = 0;
    int32_t midgameScore_ = 0;
    int32_t endgameScore_ = 0;
    int16_t phase_ = 0;
    uint16_t halfmoveClock_ = 0;
    uint16_t pliesFromNull_ = 0;
  };

  struct Castling
  {
    bool king_;
    bool queen_;
  };

  /// number of plies kept for threefold-repetition detection; the fifty move
  /// rule guarantees that no repetition can be further back than 100 plies
  constexpr int REPETITION_RING = 128;

  struct Position
  {
  private:
    Bitboard pieceBitboards_[12];
    Bitboard colorPieces_[2];
    Bitboard occupied_;
    Piece squarePieces_[64];

    uint64_t hash_ = 0;
    uint64_t pawnHash_ = 0;
    int32_t midgameScore_ = 0;
    int32_t endgameScore_ = 0;
    int16_t phase_ = 0;

    bool whiteToMove_ = true;
    uint8_t castlingRights_ = 0;
    int8_t enPassantSquare_ = -1;
    int8_t kingSquare_[2] = {-1, -1};

    uint16_t halfmoveClock_ = 0;
    uint16_t pliesFromNull_ = 0;
    uint16_t gamePly_ = 0;
    uint64_t repetitionRing_[REPETITION_RING] = {};

    static Bitboard squareMask(int square) noexcept;

    void addPiece(int square, Piece piece) noexcept;
    void erasePiece(int square, Piece piece) noexcept;
    void movePiece(int from, int to, Piece piece) noexcept;
    void updateCastlingRights(uint8_t new_rights) noexcept;
    void recomputeDerivedState() noexcept;

  public:
    Position();
    Position(std::initializer_list< std::pair< Square, Piece > > pieces,
      bool whiteToMove = true, bool wkc = 0, bool wqc = 0, bool bkc = 0, bool bqc = 0);
    Position(const char* FEN);

    bool operator==(const Position& another) const noexcept;

    void setInitial() noexcept;
    void clear() noexcept;

    int getPiece(int square) const noexcept;
    Bitboard getBitboard(Piece piece) const noexcept;
    Bitboard getPieces(int type, bool white) const noexcept;
    Bitboard getWhitePieces() const noexcept;
    Bitboard getBlackPieces() const noexcept;
    Bitboard getOccupied() const noexcept;
    Bitboard getSidePieces(bool white) const noexcept;
    int getEnPassantSquare() const noexcept;
    int getOppositeColourKingSquare() const noexcept;
    int getCurentColourKingSquare() const noexcept;
    int getWhiteKingSquare() const noexcept;
    int getBlackKingSquare() const noexcept;
    Castling getCastling() const noexcept;
    int getCastlingRights() const noexcept;
    bool isWhiteToMove() const noexcept;

    uint64_t hash() const noexcept;
    uint64_t pawnHash() const noexcept;
    int midgameScore() const noexcept;
    int endgameScore() const noexcept;
    int phase() const noexcept;
    int halfmoveClock() const noexcept;
    /// non pawn material of one side, used to disable null move in zugzwang
    int nonPawnMaterial(bool white) const noexcept;
    /// true when the current position already occurred earlier in the game
    bool isRepetition() const noexcept;
    bool isFiftyMoveDraw() const noexcept;
    /// king plus minor piece endings and the like can never be won
    bool isInsufficientMaterial() const noexcept;

    void makeMove(const Move& move, UndoInfo& undo) noexcept;
    void undoMove(const Move& move, const UndoInfo& undo) noexcept;
    void makeNullMove(UndoInfo& undo) noexcept;
    void undoNullMove(const UndoInfo& undo) noexcept;

    Position getToggledSideToMovePosition() const;

    void print() const;
    std::string toFen() const;

    void placePiece(int square, Piece piece);
    void removePiece(int square);
    void setEnPassantSquare(int square);
    void setWhiteToMove(bool white) noexcept;
  };

  std::ostream& operator<<(std::ostream& out, const Position& pos);
}

#endif
