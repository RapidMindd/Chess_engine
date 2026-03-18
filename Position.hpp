#ifndef POSITION_HPP
#define POSITION_HPP

struct Position
{
  int pos[64];
  bool whiteToMove;

  bool whiteKingCastling;
  bool whiteQueenCastling;
  bool blackKingCastling;
  bool blackQueenCastling;

  int enPassant;
};

#endif