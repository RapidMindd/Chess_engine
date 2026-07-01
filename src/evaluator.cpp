#include "evaluator.hpp"
#include "move_generator.hpp"
#include "piece.hpp"
#include "piece_square_tables.hpp"
#include "zobrist.hpp"

namespace chess
{
  namespace
  {
    int popLeastSignificantBit(uint64_t& bitboard)
    {
      const int square = __builtin_ctzll(bitboard);
      bitboard &= bitboard - 1;
      return square;
    }
  }

  int Evaluator::evaluate(const Position& pos)
  {
    int eval = 0;

    int white_pawn_cols[8] = {};
    int black_pawn_cols[8] = {};

    uint64_t pieces = pos.getBitboard(WHITE_PAWN);
    while (pieces != 0)
    {
      const int i = popLeastSignificantBit(pieces);
      eval += weights[WHITE_PAWN] + pawn_table[i];
      ++white_pawn_cols[i % 8];
    }

    pieces = pos.getBitboard(BLACK_PAWN);
    while (pieces != 0)
    {
      const int i = popLeastSignificantBit(pieces);
      eval -= weights[WHITE_PAWN] + pawn_table[i ^ 56];
      ++black_pawn_cols[i % 8];
    }

    pieces = pos.getBitboard(WHITE_KNIGHT);
    while (pieces != 0)
    {
      const int i = popLeastSignificantBit(pieces);
      eval += weights[WHITE_KNIGHT] + knight_table[i]
        + MoveGenerator::countPseudoLegalKnightMoves(pos, static_cast< Square >(i)) * 4;
    }

    pieces = pos.getBitboard(BLACK_KNIGHT);
    while (pieces != 0)
    {
      const int i = popLeastSignificantBit(pieces);
      eval -= weights[WHITE_KNIGHT] + knight_table[i ^ 56]
        + MoveGenerator::countPseudoLegalKnightMoves(pos, static_cast< Square >(i)) * 4;
    }

    pieces = pos.getBitboard(WHITE_BISHOP);
    while (pieces != 0)
    {
      const int i = popLeastSignificantBit(pieces);
      eval += weights[WHITE_BISHOP] + bishop_table[i]
        + MoveGenerator::countPseudoLegalBishopMoves(pos, static_cast< Square >(i)) * 5;
    }

    pieces = pos.getBitboard(BLACK_BISHOP);
    while (pieces != 0)
    {
      const int i = popLeastSignificantBit(pieces);
      eval -= weights[WHITE_BISHOP] + bishop_table[i ^ 56]
        + MoveGenerator::countPseudoLegalBishopMoves(pos, static_cast< Square >(i)) * 5;
    }

    pieces = pos.getBitboard(WHITE_ROOK);
    while (pieces != 0)
    {
      const int i = popLeastSignificantBit(pieces);
      eval += weights[WHITE_ROOK]
        + MoveGenerator::countPseudoLegalRookMoves(pos, static_cast< Square >(i)) * 2;
    }

    pieces = pos.getBitboard(BLACK_ROOK);
    while (pieces != 0)
    {
      const int i = popLeastSignificantBit(pieces);
      eval -= weights[WHITE_ROOK]
        + MoveGenerator::countPseudoLegalRookMoves(pos, static_cast< Square >(i)) * 2;
    }

    pieces = pos.getBitboard(WHITE_QUEEN);
    while (pieces != 0)
    {
      const int i = popLeastSignificantBit(pieces);
      eval += weights[WHITE_QUEEN] + queen_table[i]
        + MoveGenerator::countPseudoLegalQueenMoves(pos, static_cast< Square >(i));
    }

    pieces = pos.getBitboard(BLACK_QUEEN);
    while (pieces != 0)
    {
      const int i = popLeastSignificantBit(pieces);
      eval -= weights[WHITE_QUEEN] + queen_table[i ^ 56]
        + MoveGenerator::countPseudoLegalQueenMoves(pos, static_cast< Square >(i));
    }

    pieces = pos.getBitboard(WHITE_KING);
    while (pieces != 0)
    {
      const int i = popLeastSignificantBit(pieces);
      eval += king_table[i];
    }

    pieces = pos.getBitboard(BLACK_KING);
    while (pieces != 0)
    {
      const int i = popLeastSignificantBit(pieces);
      eval -= king_table[i ^ 56];
    }

    pawn_structure_eval(white_pawn_cols, black_pawn_cols, eval);
    king_safety(pos, eval);

    return eval;
  }

  int Evaluator::relative_eval(const Position& pos)
  {
    int eval = evaluate(pos);
    return pos.isWhiteToMove() ? eval : -eval;
  }

  void Evaluator::material(Piece piece, int& eval)
  {
    if (piece > 0)
    {
      eval += weights[piece];
    }
    else if (piece < 0)
    {
      eval -= weights[-piece];
    }
  }

  void Evaluator::mobility(const Position &pos, int square, Piece piece, int &eval)
  {
    const int piece_color = piece > 0 ? 1 : -1;
    int abs_piece = piece * piece_color;
    switch (abs_piece)
    {
      case WHITE_QUEEN:
        eval += MoveGenerator::countPseudoLegalQueenMoves(pos, static_cast< Square >(square)) * piece_color * 1;
        break;
      case WHITE_KNIGHT:
        eval += MoveGenerator::countPseudoLegalKnightMoves(pos, static_cast< Square >(square)) * piece_color * 4;
        break;
      case WHITE_BISHOP:
        eval += MoveGenerator::countPseudoLegalBishopMoves(pos, static_cast< Square >(square)) * piece_color * 5;
        break;
      case WHITE_ROOK:
        eval += MoveGenerator::countPseudoLegalRookMoves(pos, static_cast< Square >(square)) * piece_color * 2;
        break;
    }
  }

  void Evaluator::piece_square_tables(int square, Piece piece, int& eval)
  {
    const int piece_color = piece > 0 ? 1 : -1;
    int abs_piece = piece * piece_color;
    switch (abs_piece)
    {
      case WHITE_KING:
        eval += king_table[piece_color == 1 ? square : square ^ 56] * piece_color;
        break;
      case WHITE_QUEEN:
        eval += queen_table[piece_color == 1 ? square : square ^ 56] * piece_color;
        break;
      case WHITE_KNIGHT:
        eval += knight_table[piece_color == 1 ? square : square ^ 56] * piece_color;
        break;
      case WHITE_BISHOP:
        eval += bishop_table[piece_color == 1 ? square : square ^ 56] * piece_color;
        break;
      case WHITE_PAWN:
        eval += pawn_table[piece_color == 1 ? square : square ^ 56] * piece_color;
        break;
    }
  }

  void Evaluator::pawn_structure_fill(Piece piece, int square, int* white, int* black)
  {
    if (piece == WHITE_PAWN)
    {
      white[square % 8] += 1;
    }
    else if (piece == BLACK_PAWN)
    {
      black[square % 8] += 1;
    }
  }

  void Evaluator::pawn_structure_eval(int *white, int *black, int& eval)
  {
    for (int i = 1; i < 7; ++i)
    {
      if (white[i] >= 2)
      {
        if (white[i + 1] == 0 && white[i - 1] == 0)
        {
          eval -= 40;
        }
        else
        {
          eval -= 15;
        }
      }
      if (black[i] >= 2)
      {
        if (black[i + 1] == 0 && black[i - 1] == 0)
        {
          eval += 40;
        }
        else
        {
          eval += 15;
        }
      }
    }
    if (white[0] >= 2) eval -= 40;
    if (black[0] >= 2) eval += 40;
    if (white[7] >= 2) eval -= 40;
    if (black[7] >= 2) eval += 40;
  }

  void Evaluator::king_safety(const Position &pos, int &eval)
  {
    const int white_king_square = pos.getWhiteKingSquare();
    const int black_king_square = pos.getBlackKingSquare();
    const uint64_t white_pawns = pos.getBitboard(WHITE_PAWN);
    const uint64_t black_pawns = pos.getBitboard(BLACK_PAWN);
    if (white_king_square % 8 != 3 && white_king_square % 8 != 4 && white_king_square % 8 != 5)
    {
      int row = white_king_square / 8;
      int col = white_king_square % 8;
      if (row < 2)
      {
        int c1 = col - 1;
        int c2 = col + 1;

        if (c1 < 0) c1 = 0;
        if (c2 > 7) c2 = 7;

        for (int r = row; r <= row + 1; ++r)
        {
          for (int c = c1; c <= c2; ++c)
          {
            int square = r * 8 + c;
            if (square == white_king_square)
                continue;
            if ((white_pawns & (1ULL << square)) != 0)
            {
              eval += 5;
            }
          }
        }
      }
    }

    if (black_king_square % 8 != 3 && black_king_square % 8 != 4  && black_king_square % 8 != 5)
    {
      int row = black_king_square / 8;
      int col = black_king_square % 8;
      if (row > 5)
      {
        int c1 = col - 1;
        int c2 = col + 1;

        if (c1 < 0) c1 = 0;
        if (c2 > 7) c2 = 7;

        for (int r = row; r >= row - 1; --r)
        {
          for (int c = c1; c <= c2; ++c)
          {
            int square = r * 8 + c;
            if (square == black_king_square)
                continue;
            if ((black_pawns & (1ULL << square)) != 0)
            {
              eval -= 5;
            }
          }
        }
      }
    }
  }
}
