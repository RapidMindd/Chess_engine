#ifndef POSITION_HPP
#define POSITION_HPP

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

#include "square.hpp"
#include "piece.hpp"

namespace chess
{
  struct Move;

  char pieceToChar(Piece piece) noexcept;

  struct UndoInfo
  {
    Piece capturedPiece_;

    bool whiteKingCastling_ = 0;
    bool whiteQueenCastling_ = 0;
    bool blackKingCastling_ = 0;
    bool blackQueenCastling_ = 0;

    int enPassantSquare_ = -1;
  };

  struct Castling
  {
    bool king_;
    bool queen_;
  };

  struct Position
  {
  private:
    uint64_t pieceBitboards_[12];
    uint64_t whitePieces_;
    uint64_t blackPieces_;
    uint64_t occupied_;
    Piece squarePieces_[64];
    bool whiteToMove_;

    bool whiteKingCastling_ = 0;
    bool whiteQueenCastling_ = 0;
    bool blackKingCastling_ = 0;
    bool blackQueenCastling_ = 0;

    int enPassantSquare_ = -1;

    int whiteKingSquare_ = -1;
    int blackKingSquare_ = -1;

    static int bitboardIndex(Piece piece) noexcept;
    static uint64_t squareMask(int square) noexcept;

    void addPiece(int square, Piece piece) noexcept;
    void erasePiece(int square, Piece piece) noexcept;
    void movePiece(int from, int to, Piece piece) noexcept;

  public:
    Position();
    Position(std::initializer_list< std::pair< Square, Piece > > pieces,
      bool whiteToMove = true, bool wkc = 0, bool wqc = 0, bool bkc = 0, bool bqc = 0);
    Position(const char* FEN);

    bool operator==(const Position& another) const noexcept;

    void setInitial() noexcept;
    void clear() noexcept;

    int getPiece(int square) const;
    uint64_t getBitboard(Piece piece) const noexcept;
    uint64_t getWhitePieces() const noexcept;
    uint64_t getBlackPieces() const noexcept;
    uint64_t getOccupied() const noexcept;
    uint64_t getSidePieces(bool white) const noexcept;
    int getEnPassantSquare() const;
    int getOppositeColourKingSquare() const;
    int getCurentColourKingSquare() const;
    int getWhiteKingSquare() const;
    int getBlackKingSquare() const;
    Castling getCastling() const;
    int getCastlingRights() const;
    bool isWhiteToMove() const noexcept;

    void makeMove(const Move& move, UndoInfo& undo) noexcept;
    void undoMove(const Move& move, const UndoInfo& undo) noexcept;

    Position getToggledSideToMovePosition() const;

    void print() const;

    void placePiece(int square, Piece piece);
    void removePiece(int square);
    void setEnPassantSquare(int square);
  };

  std::ostream& operator<<(std::ostream& out, const Position& pos);
}

#endif
