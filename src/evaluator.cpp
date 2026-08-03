#include "evaluator.hpp"

#include <algorithm>

#include "bitboard.hpp"
#include "move_generator.hpp"
#include "piece.hpp"
#include "piece_square_tables.hpp"

namespace chess
{
  namespace
  {
    /// mobility bonus per reachable square, midgame / endgame
    constexpr int mobility_mg[7] = {0, 0, 4, 5, 2, 1, 0};
    constexpr int mobility_eg[7] = {0, 0, 4, 5, 4, 2, 0};

    constexpr int BISHOP_PAIR_MG = 30;
    constexpr int BISHOP_PAIR_EG = 50;
    constexpr int ROOK_OPEN_FILE = 25;
    constexpr int ROOK_SEMI_OPEN_FILE = 12;
    constexpr int DOUBLED_PAWN_MG = -10;
    constexpr int DOUBLED_PAWN_EG = -20;
    constexpr int ISOLATED_PAWN_MG = -14;
    constexpr int ISOLATED_PAWN_EG = -18;
    constexpr int BACKWARD_PAWN_MG = -8;
    constexpr int BACKWARD_PAWN_EG = -10;
    constexpr int TEMPO = 12;

    /// passed pawn bonus indexed by the rank the pawn stands on, from its side
    constexpr int passed_mg[8] = {0, 5, 10, 20, 35, 60, 100, 0};
    constexpr int passed_eg[8] = {0, 10, 20, 35, 60, 100, 160, 0};

    /// attack weight of a piece that reaches the ring around the enemy king
    constexpr int king_attack_weight[7] = {0, 0, 20, 20, 40, 80, 0};
    constexpr int king_attack_scale[8] = {0, 0, 50, 75, 88, 94, 97, 99};

    constexpr int PAWN_SHIELD = 12;
    constexpr int OPEN_FILE_NEAR_KING = -20;

    struct EvalTerms
    {
      int midgame = 0;
      int endgame = 0;
    };

    void evaluateSide(const Position& pos, bool white, EvalTerms& terms)
    {
      const Bitboard occupied = pos.getOccupied();
      const Bitboard own = pos.getSidePieces(white);
      const Bitboard own_pawns = pos.getPieces(PAWN, white);
      const Bitboard enemy_pawns = pos.getPieces(PAWN, !white);
      const int enemy_king = white ? pos.getBlackKingSquare() : pos.getWhiteKingSquare();
      const Bitboard enemy_ring = enemy_king >= 0 ? king_ring_bb[enemy_king] : 0;

      /// squares attacked by enemy pawns are not real mobility
      const Bitboard enemy_pawn_attacks = white
        ? (shiftSouthEast(enemy_pawns) | shiftSouthWest(enemy_pawns))
        : (shiftNorthEast(enemy_pawns) | shiftNorthWest(enemy_pawns));
      const Bitboard mobility_area = ~own & ~enemy_pawn_attacks;

      int attack_units = 0;
      int attacker_count = 0;

      Bitboard knights = pos.getPieces(KNIGHT, white);
      while (knights != 0)
      {
        const int square = popLsb(knights);
        const Bitboard attacks = knight_attacks_bb[square];
        const int count = popcount(attacks & mobility_area);
        terms.midgame += count * mobility_mg[KNIGHT];
        terms.endgame += count * mobility_eg[KNIGHT];
        if ((attacks & enemy_ring) != 0)
        {
          attack_units += king_attack_weight[KNIGHT];
          ++attacker_count;
        }
      }

      Bitboard bishops = pos.getPieces(BISHOP, white);
      if (moreThanOne(bishops))
      {
        terms.midgame += BISHOP_PAIR_MG;
        terms.endgame += BISHOP_PAIR_EG;
      }
      while (bishops != 0)
      {
        const int square = popLsb(bishops);
        const Bitboard attacks = bishopAttacks(square, occupied);
        const int count = popcount(attacks & mobility_area);
        terms.midgame += count * mobility_mg[BISHOP];
        terms.endgame += count * mobility_eg[BISHOP];
        if ((attacks & enemy_ring) != 0)
        {
          attack_units += king_attack_weight[BISHOP];
          ++attacker_count;
        }
      }

      Bitboard rooks = pos.getPieces(ROOK, white);
      while (rooks != 0)
      {
        const int square = popLsb(rooks);
        const Bitboard attacks = rookAttacks(square, occupied);
        const int count = popcount(attacks & mobility_area);
        terms.midgame += count * mobility_mg[ROOK];
        terms.endgame += count * mobility_eg[ROOK];

        const Bitboard file = file_bb[fileOf(square)];
        if ((file & own_pawns) == 0)
        {
          terms.midgame += (file & enemy_pawns) == 0 ? ROOK_OPEN_FILE : ROOK_SEMI_OPEN_FILE;
        }
        if ((attacks & enemy_ring) != 0)
        {
          attack_units += king_attack_weight[ROOK];
          ++attacker_count;
        }
      }

      Bitboard queens = pos.getPieces(QUEEN, white);
      while (queens != 0)
      {
        const int square = popLsb(queens);
        const Bitboard attacks = queenAttacks(square, occupied);
        const int count = popcount(attacks & mobility_area);
        terms.midgame += count * mobility_mg[QUEEN];
        terms.endgame += count * mobility_eg[QUEEN];
        if ((attacks & enemy_ring) != 0)
        {
          attack_units += king_attack_weight[QUEEN];
          ++attacker_count;
        }
      }

      /// a single attacker rarely means anything, several of them do
      const int scale = king_attack_scale[std::min(attacker_count, 7)];
      terms.midgame += attack_units * scale / 100;

      /// pawn shelter of our own king
      const int king_square = white ? pos.getWhiteKingSquare() : pos.getBlackKingSquare();
      if (king_square >= 0)
      {
        const Bitboard shield = king_attacks_bb[king_square] & own_pawns;
        terms.midgame += popcount(shield) * PAWN_SHIELD;

        const int king_file = fileOf(king_square);
        const int first = std::max(0, king_file - 1);
        const int last = std::min(7, king_file + 1);
        for (int file = first; file <= last; ++file)
        {
          if ((file_bb[file] & own_pawns) == 0)
          {
            terms.midgame += OPEN_FILE_NEAR_KING;
          }
        }
      }
    }
  }

  Evaluator::Evaluator():
    pawns_(new PawnHashTable())
  {
    for (int i = 0; i < PawnHashTable::SIZE; ++i)
    {
      pawns_->entries[i] = PawnHashTable::Entry();
    }
  }

  void Evaluator::pawnStructure(const Position& pos, int& midgame, int& endgame)
  {
    const uint64_t key = pos.pawnHash();
    PawnHashTable::Entry& slot = pawns_->entries[key & (PawnHashTable::SIZE - 1)];
    if (slot.used && slot.key == key)
    {
      midgame += slot.midgame;
      endgame += slot.endgame;
      return;
    }

    int mg = 0;
    int eg = 0;
    for (int side = 0; side < 2; ++side)
    {
      const bool white = side == 0;
      const int sign = white ? 1 : -1;
      const Bitboard own_pawns = pos.getPieces(PAWN, white);
      const Bitboard enemy_pawns = pos.getPieces(PAWN, !white);

      Bitboard pawns = own_pawns;
      while (pawns != 0)
      {
        const int square = popLsb(pawns);
        const int file = fileOf(square);
        const int relative_rank = white ? rankOf(square) : 7 - rankOf(square);

        if ((forward_file_bb[white ? 0 : 1][square] & own_pawns) != 0)
        {
          mg += sign * DOUBLED_PAWN_MG;
          eg += sign * DOUBLED_PAWN_EG;
        }
        if ((adjacent_files_bb[file] & own_pawns) == 0)
        {
          mg += sign * ISOLATED_PAWN_MG;
          eg += sign * ISOLATED_PAWN_EG;
        }
        else if ((passed_pawn_span_bb[white ? 1 : 0][square] & adjacent_files_bb[file] & own_pawns) == 0)
        {
          /// no friendly pawn behind on a neighbour file: cannot be defended
          mg += sign * BACKWARD_PAWN_MG;
          eg += sign * BACKWARD_PAWN_EG;
        }
        if ((passed_pawn_span_bb[white ? 0 : 1][square] & enemy_pawns) == 0)
        {
          mg += sign * passed_mg[relative_rank];
          eg += sign * passed_eg[relative_rank];
        }
      }
    }

    slot.key = key;
    slot.midgame = mg;
    slot.endgame = eg;
    slot.used = true;
    midgame += mg;
    endgame += eg;
  }

  int Evaluator::evaluate(const Position& pos)
  {
    int midgame = pos.midgameScore();
    int endgame = pos.endgameScore();

    EvalTerms white_terms;
    EvalTerms black_terms;
    evaluateSide(pos, true, white_terms);
    evaluateSide(pos, false, black_terms);

    midgame += white_terms.midgame - black_terms.midgame;
    endgame += white_terms.endgame - black_terms.endgame;

    pawnStructure(pos, midgame, endgame);

    const int phase = std::min< int >(pos.phase(), MAX_PHASE);
    int score = (midgame * phase + endgame * (MAX_PHASE - phase)) / MAX_PHASE;

    /// a bare king and a minor piece cannot win, do not report an advantage
    if (pos.isInsufficientMaterial())
    {
      score /= 8;
    }

    score += pos.isWhiteToMove() ? TEMPO : -TEMPO;
    return score;
  }

  int Evaluator::relativeEval(const Position& pos)
  {
    const int score = evaluate(pos);
    return pos.isWhiteToMove() ? score : -score;
  }

  int Evaluator::staticEvaluate(const Position& pos)
  {
    static thread_local Evaluator evaluator;
    return evaluator.evaluate(pos);
  }

  int Evaluator::relative_eval(const Position& pos)
  {
    static thread_local Evaluator evaluator;
    return evaluator.relativeEval(pos);
  }
}
