#ifndef PIECE_SQUARE_TABLES_HPP
#define PIECE_SQUARE_TABLES_HPP

#include <cstdint>

namespace chess
{
  /// Tapered material values (midgame / endgame), in centipawns.
  extern const int midgame_values[7];
  extern const int endgame_values[7];

  /// Game phase contribution of every piece type; a full board is 24.
  extern const int phase_values[7];
  constexpr int MAX_PHASE = 24;

  /// Ready to use tables indexed by [dense piece index][square]:
  /// 0..5 are white pawn..king, 6..11 are black pawn..king. Material value is
  /// already folded in and black entries are negative, so a position score is
  /// just the sum of the entries of every piece on the board.
  extern int16_t midgame_table[12][64];
  extern int16_t endgame_table[12][64];

  /// Same tables without the colour sign, from the point of view of the moving
  /// side; used for cheap move ordering deltas.
  extern int16_t midgame_psqt[7][2][64];

  void initPieceSquareTables();

  namespace detail
  {
    struct PieceSquareTablesInitializer
    {
      PieceSquareTablesInitializer()
      {
        initPieceSquareTables();
      }
    };
    static PieceSquareTablesInitializer piece_square_tables_initializer;
  }
}

#endif
