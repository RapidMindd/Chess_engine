#include "zobrist.hpp"
#include "piece.hpp"
#include <cstdint>
#include <random>
#include <sys/types.h>

namespace chess
{
  uint64_t zobrist_board[64 * 12];
  uint64_t zobrist_side;
  uint64_t zobrist_castling[16];
  uint64_t zobrist_enpassant[64];

  void initZobristHash()
  {
    std::mt19937_64 rng(67);
    for (int i = 0; i < 64; i++)
    {
      for (int j = 0; j < 12; j++)
      {
        zobrist_board[i * 12 + j] = rng();
      }
    }
    zobrist_side = rng();
    for (int i = 0; i < 16; i++)
    {
      zobrist_castling[i] = rng();
    }
    for (int i = 0; i < 64; i++)
    {
      zobrist_enpassant[i] = rng();
    }
  }

  uint64_t zobristHash(const Position &pos)
  {
    uint64_t hash = 0;
    for (int i = 0; i < 64; i++)
    {
      int cur = pos.getPiece(i);
      if (cur != 0)
      {
        hash ^= zobrist_board[i * 12 + pieceIndex(cur)];
      }
    }
    hash ^= zobrist_side * (pos.isWhiteToMove() ? 1 : 0);
    hash ^= zobrist_castling[pos.getCastlingRights()];
    int enPassant = pos.getEnPassantSquare();
    if (enPassant != -1)
    {
      hash ^= zobrist_enpassant[enPassant];
    }
    return hash;
  }

  uint64_t incrementZobristHash(uint64_t hash, const Position &pos, const Move &move)
  {
    int from = pos.getPiece(move.from_);
    hash ^= zobrist_board[move.from_ * 12 + pieceIndex(from)];

    int to = pos.getPiece(move.to_);
    if (to != EMPTY)
    {
      hash ^= zobrist_board[move.to_ * 12 + pieceIndex(to)];
    }

    hash ^= zobrist_board[move.to_ * 12 + pieceIndex(from)];

    hash ^= zobrist_side * (pos.isWhiteToMove() ? 1 : 0);
    hash ^= zobrist_castling[pos.getCastlingRights()];
    int enPassant = pos.getEnPassantSquare();
    if (enPassant != -1)
    {
      hash ^= zobrist_enpassant[enPassant];
    }
    return hash;
  }

  int pieceIndex(int piece)
  {
    return piece > 0 ? piece + 5 : piece + 6;
  }
}
