#include "bitboard.hpp"

namespace chess
{
  Bitboard knight_attacks_bb[64];
  Bitboard king_attacks_bb[64];
  Bitboard pawn_attacks_bb[2][64];
  Bitboard between_bb[64][64];
  Bitboard line_bb[64][64];
  Bitboard file_bb[8];
  Bitboard rank_bb[8];
  Bitboard adjacent_files_bb[8];
  Bitboard passed_pawn_span_bb[2][64];
  Bitboard forward_file_bb[2][64];
  Bitboard king_ring_bb[64];

  Magic rook_magics[64];
  Magic bishop_magics[64];

  namespace
  {
    Bitboard rook_table[102400];
    Bitboard bishop_table[5248];

    const int rook_directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    const int bishop_directions[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    bool onBoard(int row, int col)
    {
      return row >= 0 && row < 8 && col >= 0 && col < 8;
    }

    Bitboard slidingAttacks(int square, Bitboard occupied, const int directions[4][2])
    {
      Bitboard result = 0;
      const int row = square / 8;
      const int col = square % 8;
      for (int i = 0; i < 4; ++i)
      {
        int r = row + directions[i][0];
        int c = col + directions[i][1];
        while (onBoard(r, c))
        {
          const Bitboard bit = squareBB(r * 8 + c);
          result |= bit;
          if ((occupied & bit) != 0)
          {
            break;
          }
          r += directions[i][0];
          c += directions[i][1];
        }
      }
      return result;
    }

    /// relevant occupancy bits: the ray without its last square, since a piece
    /// standing on the edge never blocks anything behind it
    Bitboard slidingMask(int square, const int directions[4][2])
    {
      Bitboard result = 0;
      const int row = square / 8;
      const int col = square % 8;
      for (int i = 0; i < 4; ++i)
      {
        int r = row + directions[i][0];
        int c = col + directions[i][1];
        while (onBoard(r + directions[i][0], c + directions[i][1]))
        {
          result |= squareBB(r * 8 + c);
          r += directions[i][0];
          c += directions[i][1];
        }
      }
      return result;
    }

    struct SplitMix64
    {
      uint64_t state;

      uint64_t operator()()
      {
        state += 0x9E3779B97F4A7C15ULL;
        uint64_t z = state;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
      }

      uint64_t sparse()
      {
        return (*this)() & (*this)() & (*this)();
      }
    };

    void initMagics(Magic* magics, Bitboard* table, const int directions[4][2])
    {
      Bitboard occupancies[4096];
      Bitboard references[4096];
      unsigned epoch[4096] = {};
      unsigned current_epoch = 0;
      SplitMix64 rng{0x246C0D1B2E4F5A73ULL};
      size_t offset = 0;

      for (int square = 0; square < 64; ++square)
      {
        Magic& magic = magics[square];
        magic.mask = slidingMask(square, directions);
        const int bits = popcount(magic.mask);
        const size_t size = size_t(1) << bits;
        magic.shift = 64 - bits;
        magic.attacks = table + offset;

        Bitboard subset = 0;
        for (size_t i = 0; i < size; ++i)
        {
          occupancies[i] = subset;
          references[i] = slidingAttacks(square, subset, directions);
          subset = (subset - magic.mask) & magic.mask;
        }

#ifdef CHESS_USE_PEXT
        magic.multiplier = 0;
        for (size_t i = 0; i < size; ++i)
        {
          table[offset + _pext_u64(occupancies[i], magic.mask)] = references[i];
        }
#else
        while (true)
        {
          magic.multiplier = rng.sparse();
          if (popcount((magic.mask * magic.multiplier) >> 56) < 6)
          {
            continue;
          }
          ++current_epoch;
          bool ok = true;
          for (size_t i = 0; i < size && ok; ++i)
          {
            const unsigned index = magic.index(occupancies[i]);
            if (epoch[index] != current_epoch)
            {
              epoch[index] = current_epoch;
              table[offset + index] = references[i];
            }
            else if (table[offset + index] != references[i])
            {
              ok = false;
            }
          }
          if (ok)
          {
            break;
          }
        }
#endif
        offset += size;
      }
      (void) rng;
      (void) epoch;
      (void) current_epoch;
    }

    bool initialized = false;
  }

  void initBitboards()
  {
    if (initialized)
    {
      return;
    }
    initialized = true;

    for (int i = 0; i < 8; ++i)
    {
      file_bb[i] = FILE_A_BB << i;
      rank_bb[i] = RANK_1_BB << (8 * i);
    }
    for (int i = 0; i < 8; ++i)
    {
      adjacent_files_bb[i] = 0;
      if (i > 0)
      {
        adjacent_files_bb[i] |= file_bb[i - 1];
      }
      if (i < 7)
      {
        adjacent_files_bb[i] |= file_bb[i + 1];
      }
    }

    const int knight_offsets[8][2] = {{2, 1}, {1, 2}, {-1, 2}, {-2, 1}, {-2, -1}, {-1, -2}, {1, -2}, {2, -1}};
    const int king_offsets[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};

    for (int square = 0; square < 64; ++square)
    {
      const int row = square / 8;
      const int col = square % 8;
      knight_attacks_bb[square] = 0;
      king_attacks_bb[square] = 0;
      pawn_attacks_bb[0][square] = 0;
      pawn_attacks_bb[1][square] = 0;

      for (int i = 0; i < 8; ++i)
      {
        int r = row + knight_offsets[i][0];
        int c = col + knight_offsets[i][1];
        if (onBoard(r, c))
        {
          knight_attacks_bb[square] |= squareBB(r * 8 + c);
        }
        r = row + king_offsets[i][0];
        c = col + king_offsets[i][1];
        if (onBoard(r, c))
        {
          king_attacks_bb[square] |= squareBB(r * 8 + c);
        }
      }

      pawn_attacks_bb[0][square] = shiftNorthEast(squareBB(square)) | shiftNorthWest(squareBB(square));
      pawn_attacks_bb[1][square] = shiftSouthEast(squareBB(square)) | shiftSouthWest(squareBB(square));

      Bitboard forward_white = 0;
      for (int r = row + 1; r < 8; ++r)
      {
        forward_white |= squareBB(r * 8 + col);
      }
      Bitboard forward_black = 0;
      for (int r = row - 1; r >= 0; --r)
      {
        forward_black |= squareBB(r * 8 + col);
      }
      forward_file_bb[0][square] = forward_white;
      forward_file_bb[1][square] = forward_black;

      Bitboard span_white = forward_white;
      Bitboard span_black = forward_black;
      for (int r = row + 1; r < 8; ++r)
      {
        if (col > 0) span_white |= squareBB(r * 8 + col - 1);
        if (col < 7) span_white |= squareBB(r * 8 + col + 1);
      }
      for (int r = row - 1; r >= 0; --r)
      {
        if (col > 0) span_black |= squareBB(r * 8 + col - 1);
        if (col < 7) span_black |= squareBB(r * 8 + col + 1);
      }
      passed_pawn_span_bb[0][square] = span_white;
      passed_pawn_span_bb[1][square] = span_black;
    }

    /// the king ring keeps the king inside the board even in the corners
    for (int square = 0; square < 64; ++square)
    {
      int row = square / 8;
      int col = square % 8;
      if (row == 0) row = 1;
      if (row == 7) row = 6;
      if (col == 0) col = 1;
      if (col == 7) col = 6;
      const int center = row * 8 + col;
      king_ring_bb[square] = king_attacks_bb[center] | squareBB(center);
    }

    initMagics(rook_magics, rook_table, rook_directions);
    initMagics(bishop_magics, bishop_table, bishop_directions);

    for (int a = 0; a < 64; ++a)
    {
      for (int b = 0; b < 64; ++b)
      {
        between_bb[a][b] = 0;
        line_bb[a][b] = 0;
      }
    }

    for (int a = 0; a < 64; ++a)
    {
      for (int b = 0; b < 64; ++b)
      {
        if (a == b)
        {
          continue;
        }
        const Bitboard b_bit = squareBB(b);
        if ((rookAttacks(a, 0) & b_bit) != 0)
        {
          between_bb[a][b] = rookAttacks(a, b_bit) & rookAttacks(b, squareBB(a));
          line_bb[a][b] = (rookAttacks(a, 0) & rookAttacks(b, 0)) | squareBB(a) | b_bit;
        }
        else if ((bishopAttacks(a, 0) & b_bit) != 0)
        {
          between_bb[a][b] = bishopAttacks(a, b_bit) & bishopAttacks(b, squareBB(a));
          line_bb[a][b] = (bishopAttacks(a, 0) & bishopAttacks(b, 0)) | squareBB(a) | b_bit;
        }
      }
    }
  }
}
