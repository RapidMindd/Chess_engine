#ifndef ZOBRIST_HPP
#define ZOBRIST_HPP

#include <cstdint>

namespace chess
{
  struct Position;
  struct Move;

  /// indexed by [dense piece index][square], see pieceIndexOf()
  extern uint64_t zobrist_board[12][64];
  extern uint64_t zobrist_side;
  extern uint64_t zobrist_castling[16];
  extern uint64_t zobrist_enpassant[64];

  void initZobristHash();

  /// full recomputation, used when a position is built from scratch
  uint64_t zobristHash(const Position& pos);
  uint64_t zobristPawnHash(const Position& pos);
  /// key of the position reached after `move`, computed from the current key
  uint64_t incrementZobristHash(uint64_t hash, const Position& pos, const Move& move);

  int pieceIndex(int piece);

  namespace detail
  {
    struct ZobristInitializer
    {
      ZobristInitializer()
      {
        initZobristHash();
      }
    };
    static ZobristInitializer zobrist_initializer;
  }
}

#endif
