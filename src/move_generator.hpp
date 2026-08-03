#ifndef MOVE_GENERATOR_HPP
#define MOVE_GENERATOR_HPP

#include "bitboard.hpp"
#include "move.hpp"
#include "position.hpp"

namespace chess
{
  /// Pre-computed information about checks and pins for the side to move.
  /// Building it once per node makes the generated moves legal by construction,
  /// so no make/undo verification is needed.
  struct LegalInfo
  {
    bool white_;
    int side_;
    int kingSquare_;
    Bitboard own_;
    Bitboard enemy_;
    Bitboard occupied_;
    Bitboard checkers_;
    /// squares a non king move must land on to stop the check
    Bitboard checkMask_;
    Bitboard pinned_;
    /// squares attacked by the opponent, king excluded from the blockers
    Bitboard danger_;
    int checkCount_;
    bool inCheck_;

    int enemyKingSquare_;
    /// squares from which a piece of the given type would check the enemy king
    Bitboard checkSquares_[7];
    /// own pieces whose departure would uncover a check
    Bitboard discoveryCandidates_;
  };

  struct MoveGenerator
  {
    static LegalInfo buildLegalInfo(const Position& pos);

    static void generateLegalMoves(const Position& pos, MoveArray& moves);
    static void generateLegalMoves(const Position& pos, const LegalInfo& info, MoveArray& moves);
    static MoveArray generateLegalMoves(const Position& pos);

    /// captures, queen promotions and, when in check, every evasion
    static void generateActiveMoves(const Position& pos, const LegalInfo& info, MoveArray& moves);
    static MoveArray generateActiveMoves(const Position& pos);

    /// quiet checks and other non capturing moves, used after generateActiveMoves
    static void generateQuietMoves(const Position& pos, const LegalInfo& info, MoveArray& moves);

    static bool isPseudoLegal(const Position& pos, const Move& move);
    static bool givesCheck(const Position& pos, const Move& move);
    /// same answer as givesCheck, but reuses the tables built for this node
    static bool givesCheck(const Position& pos, const LegalInfo& info, const Move& move);

    static Bitboard attackersTo(const Position& pos, int square, Bitboard occupied);
    static bool isSquareAttacked(const Position& pos, Square square);
    static bool isSquareAttackedQuick(const Position& pos, Square square, bool byWhite);

    static bool isMate(const Position& pos);
    static bool isCheck(const Position& pos);
    static bool isStaleMate(const Position& pos);

    /// legacy pseudo legal helpers, kept for the tests and for tooling
    static MoveArray generatePseudoLegalMoves(const Position& pos, bool castling = 1);
    static void generatePseudoLegalActiveMoves(const Position& pos, MoveArray& moves);
    static void generateKingMoves(const Position& pos, Square square, MoveArray& moves);
    static void generateQueenMoves(const Position& pos, Square square, MoveArray& moves);
    static void generateKnightMoves(const Position& pos, Square square, MoveArray& moves);
    static void generateBishopMoves(const Position& pos, Square square, MoveArray& moves);
    static void generateRookMoves(const Position& pos, Square square, MoveArray& moves);
    static void generatePawnMoves(const Position& pos, Square square, MoveArray& moves);
    static void generateCastlingMoves(const Position& pos, Square square, MoveArray& moves);

    static int countPseudoLegalQueenMoves(const Position& pos, Square square);
    static int countPseudoLegalBishopMoves(const Position& pos, Square square);
    static int countPseudoLegalRookMoves(const Position& pos, Square square);
    static int countPseudoLegalKnightMoves(const Position& pos, Square square);
  };
}

#endif
