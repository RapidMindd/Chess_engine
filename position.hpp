#ifndef POSITION_HPP
#define POSITION_HPP

#include <cstddef>
#include <iostream>

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

  struct Position
  {
  private:
    Piece board_[64];
    bool whiteToMove_;

    bool whiteKingCastling_ = 0;
    bool whiteQueenCastling_ = 0;
    bool blackKingCastling_ = 0;
    bool blackQueenCastling_ = 0;

    int enPassantSquare_ = -1;

    int whiteKingSquare_ = -1;
    int blackKingSquare_ = -1;

  public:
    Position();
    Position(std::initializer_list< std::pair< Square, Piece > > pieces, bool whiteToMove = true);

    void setInitial() noexcept;
    void clear() noexcept;

    int getPiece(int square) const;
    int getEnPassantSquare() const;
    int getOppositeColourKingSquare() const;
    bool isWhiteToMove() const noexcept;

    void makeMove(const Move& move, UndoInfo& undo) noexcept;
    void undoMove(const Move& move, const UndoInfo& undo) noexcept;

    void print() const;

    // для тестов
    void placePiece(int square, Piece piece);
    void removePiece(int square);
    void setEnPassantSquare(int square);
  };
}

#endif