#include "zobrist.hpp"

#include "move.hpp"
#include "piece.hpp"
#include "position.hpp"

namespace chess
{
  uint64_t zobrist_board[12][64];
  uint64_t zobrist_side;
  uint64_t zobrist_castling[16];
  uint64_t zobrist_enpassant[64];

  namespace
  {
    bool initialized = false;

    /// splitmix64: a fixed, self contained generator keeps the keys reproducible
    /// across compilers and standard library versions
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
    };
  }

  void initZobristHash()
  {
    if (initialized)
    {
      return;
    }
    initialized = true;

    SplitMix64 rng{67};
    for (int piece = 0; piece < 12; ++piece)
    {
      for (int square = 0; square < 64; ++square)
      {
        zobrist_board[piece][square] = rng();
      }
    }
    zobrist_side = rng();
    for (int i = 0; i < 16; ++i)
    {
      zobrist_castling[i] = rng();
    }
    for (int i = 0; i < 64; ++i)
    {
      zobrist_enpassant[i] = rng();
    }
  }

  uint64_t zobristHash(const Position& pos)
  {
    uint64_t hash = 0;
    Bitboard occupied = pos.getOccupied();
    while (occupied != 0)
    {
      const int square = popLsb(occupied);
      hash ^= zobrist_board[pieceIndexOf(pos.getPiece(square))][square];
    }
    if (pos.isWhiteToMove())
    {
      hash ^= zobrist_side;
    }
    hash ^= zobrist_castling[pos.getCastlingRights()];
    const int en_passant = pos.getEnPassantSquare();
    if (en_passant != -1)
    {
      hash ^= zobrist_enpassant[en_passant];
    }
    return hash;
  }

  uint64_t zobristPawnHash(const Position& pos)
  {
    uint64_t hash = 0;
    Bitboard pawns = pos.getBitboard(WHITE_PAWN) | pos.getBitboard(BLACK_PAWN);
    while (pawns != 0)
    {
      const int square = popLsb(pawns);
      hash ^= zobrist_board[pieceIndexOf(pos.getPiece(square))][square];
    }
    return hash;
  }

  uint64_t incrementZobristHash(uint64_t hash, const Position& pos, const Move& move)
  {
    const int moving_piece = pos.getPiece(move.from_);
    const int is_white_piece = pos.isWhiteToMove() ? 1 : -1;
    const int old_en_passant = pos.getEnPassantSquare();
    const int old_castling_rights = pos.getCastlingRights();

    hash ^= zobrist_board[pieceIndexOf(moving_piece)][move.from_];

    if (move.isEnPassant_)
    {
      const int captured_square = move.to_ - (8 * is_white_piece);
      const int captured_piece = WHITE_PAWN * -is_white_piece;
      hash ^= zobrist_board[pieceIndexOf(captured_piece)][captured_square];
    }
    else
    {
      const int captured_piece = pos.getPiece(move.to_);
      if (captured_piece != EMPTY)
      {
        hash ^= zobrist_board[pieceIndexOf(captured_piece)][move.to_];
      }
    }

    const int placed_piece = move.promotionPiece_ != EMPTY ? move.promotionPiece_ : moving_piece;
    hash ^= zobrist_board[pieceIndexOf(placed_piece)][move.to_];

    if (move.isCastling_)
    {
      const int rook_piece = WHITE_ROOK * is_white_piece;
      if (move.to_ - move.from_ == 2)
      {
        hash ^= zobrist_board[pieceIndexOf(rook_piece)][move.to_ + 1];
        hash ^= zobrist_board[pieceIndexOf(rook_piece)][move.to_ - 1];
      }
      else if (move.from_ - move.to_ == 2)
      {
        hash ^= zobrist_board[pieceIndexOf(rook_piece)][move.to_ - 2];
        hash ^= zobrist_board[pieceIndexOf(rook_piece)][move.to_ + 1];
      }
    }

    hash ^= zobrist_side;

    hash ^= zobrist_castling[old_castling_rights];
    int new_castling_rights = old_castling_rights;
    if (move.to_ == H1 || move.from_ == H1)
    {
      new_castling_rights &= ~WHITE_KING_SIDE;
    }
    if (move.to_ == A1 || move.from_ == A1)
    {
      new_castling_rights &= ~WHITE_QUEEN_SIDE;
    }
    if (move.to_ == H8 || move.from_ == H8)
    {
      new_castling_rights &= ~BLACK_KING_SIDE;
    }
    if (move.to_ == A8 || move.from_ == A8)
    {
      new_castling_rights &= ~BLACK_QUEEN_SIDE;
    }
    if (move.from_ == E1)
    {
      new_castling_rights &= ~(WHITE_KING_SIDE | WHITE_QUEEN_SIDE);
    }
    if (move.from_ == E8)
    {
      new_castling_rights &= ~(BLACK_KING_SIDE | BLACK_QUEEN_SIDE);
    }
    hash ^= zobrist_castling[new_castling_rights];

    if (old_en_passant != -1)
    {
      hash ^= zobrist_enpassant[old_en_passant];
    }

    if (move.to_ == move.from_ + (16 * is_white_piece)
      && moving_piece == WHITE_PAWN * is_white_piece)
    {
      hash ^= zobrist_enpassant[move.from_ + (8 * is_white_piece)];
    }

    return hash;
  }

  int pieceIndex(int piece)
  {
    return pieceIndexOf(piece);
  }
}
