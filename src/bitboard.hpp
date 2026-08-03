#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include <cstdint>

#if defined(__BMI2__)
  #include <immintrin.h>
  #define CHESS_USE_PEXT 1
#endif

namespace chess
{
  using Bitboard = uint64_t;

  constexpr Bitboard FILE_A_BB = 0x0101010101010101ULL;
  constexpr Bitboard FILE_H_BB = FILE_A_BB << 7;
  constexpr Bitboard RANK_1_BB = 0x00000000000000FFULL;
  constexpr Bitboard RANK_8_BB = RANK_1_BB << 56;

  inline Bitboard squareBB(int square) noexcept
  {
    return 1ULL << square;
  }

  inline int lsb(Bitboard bitboard) noexcept
  {
    return __builtin_ctzll(bitboard);
  }

  inline int msb(Bitboard bitboard) noexcept
  {
    return 63 - __builtin_clzll(bitboard);
  }

  inline int popcount(Bitboard bitboard) noexcept
  {
    return __builtin_popcountll(bitboard);
  }

  inline int popLsb(Bitboard& bitboard) noexcept
  {
    const int square = lsb(bitboard);
    bitboard &= bitboard - 1;
    return square;
  }

  inline bool moreThanOne(Bitboard bitboard) noexcept
  {
    return (bitboard & (bitboard - 1)) != 0;
  }

  /// north/south shifts that never wrap around the board edges
  inline Bitboard shiftNorth(Bitboard b) noexcept { return b << 8; }
  inline Bitboard shiftSouth(Bitboard b) noexcept { return b >> 8; }
  inline Bitboard shiftNorthEast(Bitboard b) noexcept { return (b & ~FILE_H_BB) << 9; }
  inline Bitboard shiftNorthWest(Bitboard b) noexcept { return (b & ~FILE_A_BB) << 7; }
  inline Bitboard shiftSouthEast(Bitboard b) noexcept { return (b & ~FILE_H_BB) >> 7; }
  inline Bitboard shiftSouthWest(Bitboard b) noexcept { return (b & ~FILE_A_BB) >> 9; }

  extern Bitboard knight_attacks_bb[64];
  extern Bitboard king_attacks_bb[64];
  /// pawn_attacks_bb[0] - squares attacked by a white pawn, [1] - by a black pawn
  extern Bitboard pawn_attacks_bb[2][64];
  /// squares strictly between two aligned squares (0 when they are not aligned)
  extern Bitboard between_bb[64][64];
  /// whole line going through two aligned squares (0 when they are not aligned)
  extern Bitboard line_bb[64][64];
  extern Bitboard file_bb[8];
  extern Bitboard rank_bb[8];
  extern Bitboard adjacent_files_bb[8];
  /// squares in front of a pawn on its own and both neighbour files
  extern Bitboard passed_pawn_span_bb[2][64];
  /// squares in front of a square on its own file
  extern Bitboard forward_file_bb[2][64];
  extern Bitboard king_ring_bb[64];

  struct Magic
  {
    Bitboard mask;
    Bitboard multiplier;
    const Bitboard* attacks;
    unsigned shift;

    unsigned index(Bitboard occupied) const noexcept
    {
#ifdef CHESS_USE_PEXT
      return static_cast< unsigned >(_pext_u64(occupied, mask));
#else
      return static_cast< unsigned >(((occupied & mask) * multiplier) >> shift);
#endif
    }
  };

  extern Magic rook_magics[64];
  extern Magic bishop_magics[64];

  inline Bitboard rookAttacks(int square, Bitboard occupied) noexcept
  {
    const Magic& magic = rook_magics[square];
    return magic.attacks[magic.index(occupied)];
  }

  inline Bitboard bishopAttacks(int square, Bitboard occupied) noexcept
  {
    const Magic& magic = bishop_magics[square];
    return magic.attacks[magic.index(occupied)];
  }

  inline Bitboard queenAttacks(int square, Bitboard occupied) noexcept
  {
    return rookAttacks(square, occupied) | bishopAttacks(square, occupied);
  }

  void initBitboards();

  namespace detail
  {
    /// Schwarz counter: guarantees the tables are ready before any other
    /// translation unit runs its dynamic initialization.
    struct BitboardsInitializer
    {
      BitboardsInitializer()
      {
        initBitboards();
      }
    };
    static BitboardsInitializer bitboards_initializer;
  }
}

#endif
