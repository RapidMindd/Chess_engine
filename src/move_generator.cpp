#include "move_generator.hpp"

namespace chess
{
  namespace
  {
    struct AttackTables
    {
      uint64_t knight_[64];
      uint64_t king_[64];
      uint64_t whitePawn_[64];
      uint64_t blackPawn_[64];
      uint64_t rays_[64][8];

      AttackTables()
      {
        const int knight_rows[8] = {2, 1, -1, -2, -2, -1, 1, 2};
        const int knight_cols[8] = {1, 2, 2, 1, -1, -2, -2, -1};
        const int king_rows[8] = {1, 1, 0, -1, -1, -1, 0, 1};
        const int king_cols[8] = {0, 1, 1, 1, 0, -1, -1, -1};
        const int ray_rows[8] = {1, -1, 0, 0, 1, -1, -1, 1};
        const int ray_cols[8] = {0, 0, 1, -1, 1, 1, -1, -1};

        for (int square = 0; square < 64; ++square)
        {
          knight_[square] = 0;
          king_[square] = 0;
          whitePawn_[square] = 0;
          blackPawn_[square] = 0;
          for (int direction = 0; direction < 8; ++direction)
          {
            rays_[square][direction] = 0;
          }

          const int row = square / 8;
          const int col = square % 8;

          for (int i = 0; i < 8; ++i)
          {
            int new_row = row + knight_rows[i];
            int new_col = col + knight_cols[i];
            if (new_row >= 0 && new_row < 8 && new_col >= 0 && new_col < 8)
            {
              knight_[square] |= 1ULL << (new_row * 8 + new_col);
            }

            new_row = row + king_rows[i];
            new_col = col + king_cols[i];
            if (new_row >= 0 && new_row < 8 && new_col >= 0 && new_col < 8)
            {
              king_[square] |= 1ULL << (new_row * 8 + new_col);
            }
          }

          if (row < 7)
          {
            if (col > 0)
            {
              whitePawn_[square] |= 1ULL << ((row + 1) * 8 + col - 1);
            }
            if (col < 7)
            {
              whitePawn_[square] |= 1ULL << ((row + 1) * 8 + col + 1);
            }
          }

          if (row > 0)
          {
            if (col > 0)
            {
              blackPawn_[square] |= 1ULL << ((row - 1) * 8 + col - 1);
            }
            if (col < 7)
            {
              blackPawn_[square] |= 1ULL << ((row - 1) * 8 + col + 1);
            }
          }

          for (int direction = 0; direction < 8; ++direction)
          {
            int new_row = row + ray_rows[direction];
            int new_col = col + ray_cols[direction];
            while (new_row >= 0 && new_row < 8 && new_col >= 0 && new_col < 8)
            {
              rays_[square][direction] |= 1ULL << (new_row * 8 + new_col);
              new_row += ray_rows[direction];
              new_col += ray_cols[direction];
            }
          }
        }
      }
    };

    const AttackTables attacks;
    uint64_t squareBit(int square)
    {
      return 1ULL << square;
    }

    int popLeastSignificantBit(uint64_t& bitboard)
    {
      const int square = __builtin_ctzll(bitboard);
      bitboard &= bitboard - 1;
      return square;
    }

    int firstBlocker(uint64_t blockers, int direction)
    {
      if (direction == 0 || direction == 2 || direction == 4 || direction == 7)
      {
        return __builtin_ctzll(blockers);
      }
      return 63 - __builtin_clzll(blockers);
    }

    uint64_t slidingAttacksFrom(int square, uint64_t occupied, int begin, int end)
    {
      uint64_t result = 0;
      for (int direction = begin; direction < end; ++direction)
      {
        uint64_t ray = attacks.rays_[square][direction];
        const uint64_t blockers = ray & occupied;
        if (blockers != 0)
        {
          ray &= ~attacks.rays_[firstBlocker(blockers, direction)][direction];
        }
        result |= ray;
      }
      return result;
    }

    bool directionUsesRook(int direction)
    {
      return direction < 4;
    }

    bool isSlidingPieceForDirection(Piece piece, int direction)
    {
      if (piece == WHITE_QUEEN || piece == BLACK_QUEEN)
      {
        return true;
      }
      if (directionUsesRook(direction))
      {
        return piece == WHITE_ROOK || piece == BLACK_ROOK;
      }
      return piece == WHITE_BISHOP || piece == BLACK_BISHOP;
    }

    uint64_t lineMaskFromTo(int from, int to, int direction)
    {
      return attacks.rays_[from][direction] & ~attacks.rays_[to][direction];
    }

    uint64_t ownPieces(const Position& pos, int piece)
    {
      return piece > 0 ? pos.getWhitePieces() : pos.getBlackPieces();
    }

    uint64_t enemyPieces(const Position& pos, int piece)
    {
      return piece > 0 ? pos.getBlackPieces() : pos.getWhitePieces();
    }

    void addRayMoves(const Position& pos, Square square, MoveArray& moves, int row_step, int col_step, bool captures_only)
    {
      const int piece = pos.getPiece(square);
      const uint64_t own = ownPieces(pos, piece);
      const uint64_t enemy = enemyPieces(pos, piece);
      const uint64_t occupied = pos.getOccupied();
      int row = square / 8 + row_step;
      int col = square % 8 + col_step;

      while (row >= 0 && row < 8 && col >= 0 && col < 8)
      {
        const int dest_square = row * 8 + col;
        const uint64_t dest = squareBit(dest_square);
        if ((own & dest) != 0)
        {
          break;
        }
        if (!captures_only || (enemy & dest) != 0)
        {
          moves.push({square, static_cast< Square >(dest_square)});
        }
        if ((occupied & dest) != 0)
        {
          break;
        }
        row += row_step;
        col += col_step;
      }
    }

    Move findSlidingAttacker(const Position& pos, int square, Piece piece, const int* directions, int count)
    {
      const uint64_t occupied = pos.getOccupied();
      const uint64_t attackers = pos.getBitboard(piece);

      for (int i = 0; i < count; ++i)
      {
        const int direction = directions[i];
        const uint64_t blockers = attacks.rays_[square][direction] & occupied;
        if (blockers == 0)
        {
          continue;
        }
        const int from = firstBlocker(blockers, direction);
        if ((attackers & squareBit(from)) != 0)
        {
          return Move{static_cast< Square >(from), static_cast< Square >(square)};
        }
      }

      return null_move;
    }

    bool isSquareAttackedOnOccupancy(const Position& pos, int square, bool byWhite, uint64_t occupied,
      uint64_t ignoredAttackers = 0)
    {
      if (square < A1 || square > H8)
      {
        return false;
      }

      if (byWhite)
      {
        if ((attacks.blackPawn_[square] & (pos.getBitboard(WHITE_PAWN) & ~ignoredAttackers)) != 0) return true;
        if ((attacks.knight_[square] & (pos.getBitboard(WHITE_KNIGHT) & ~ignoredAttackers)) != 0) return true;
        if ((attacks.king_[square] & (pos.getBitboard(WHITE_KING) & ~ignoredAttackers)) != 0) return true;
      }
      else
      {
        if ((attacks.whitePawn_[square] & (pos.getBitboard(BLACK_PAWN) & ~ignoredAttackers)) != 0) return true;
        if ((attacks.knight_[square] & (pos.getBitboard(BLACK_KNIGHT) & ~ignoredAttackers)) != 0) return true;
        if ((attacks.king_[square] & (pos.getBitboard(BLACK_KING) & ~ignoredAttackers)) != 0) return true;
      }

      const uint64_t rook_attackers = byWhite
        ? (pos.getBitboard(WHITE_ROOK) | pos.getBitboard(WHITE_QUEEN)) & ~ignoredAttackers
        : (pos.getBitboard(BLACK_ROOK) | pos.getBitboard(BLACK_QUEEN)) & ~ignoredAttackers;
      const uint64_t bishop_attackers = byWhite
        ? (pos.getBitboard(WHITE_BISHOP) | pos.getBitboard(WHITE_QUEEN)) & ~ignoredAttackers
        : (pos.getBitboard(BLACK_BISHOP) | pos.getBitboard(BLACK_QUEEN)) & ~ignoredAttackers;

      for (int direction = 0; direction < 4; ++direction)
      {
        const uint64_t blockers = attacks.rays_[square][direction] & occupied;
        if (blockers != 0 && (rook_attackers & squareBit(firstBlocker(blockers, direction))) != 0)
        {
          return true;
        }
      }

      for (int direction = 4; direction < 8; ++direction)
      {
        const uint64_t blockers = attacks.rays_[square][direction] & occupied;
        if (blockers != 0 && (bishop_attackers & squareBit(firstBlocker(blockers, direction))) != 0)
        {
          return true;
        }
      }

      return false;
    }

    struct LegalInfo
    {
      bool white_;
      int side_;
      int kingSquare_;
      uint64_t own_;
      uint64_t enemy_;
      uint64_t occupied_;
      uint64_t checkers_;
      uint64_t checkMask_;
      uint64_t pinned_;
      uint64_t pinLine_[64];
      int checkCount_;
      bool inCheck_;
    };

    LegalInfo buildLegalInfo(const Position& pos)
    {
      LegalInfo info;
      info.white_ = pos.isWhiteToMove();
      info.side_ = info.white_ ? 1 : -1;
      info.kingSquare_ = info.white_ ? pos.getWhiteKingSquare() : pos.getBlackKingSquare();
      info.own_ = pos.getSidePieces(info.white_);
      info.enemy_ = pos.getSidePieces(!info.white_);
      info.occupied_ = pos.getOccupied();
      info.checkers_ = 0;
      info.checkMask_ = ~0ULL;
      info.pinned_ = 0;
      info.checkCount_ = 0;
      info.inCheck_ = false;

      if (info.kingSquare_ < A1 || info.kingSquare_ > H8)
      {
        return info;
      }

      const uint64_t enemy_pawns = info.white_ ? pos.getBitboard(BLACK_PAWN) : pos.getBitboard(WHITE_PAWN);
      const uint64_t enemy_knights = info.white_ ? pos.getBitboard(BLACK_KNIGHT) : pos.getBitboard(WHITE_KNIGHT);
      const uint64_t pawn_attackers = (info.white_ ? attacks.whitePawn_[info.kingSquare_] : attacks.blackPawn_[info.kingSquare_]) & enemy_pawns;
      const uint64_t knight_attackers = attacks.knight_[info.kingSquare_] & enemy_knights;

      info.checkers_ |= pawn_attackers | knight_attackers;

      for (int direction = 0; direction < 8; ++direction)
      {
        uint64_t blockers = attacks.rays_[info.kingSquare_][direction] & info.occupied_;
        if (blockers == 0)
        {
          continue;
        }

        const int first = firstBlocker(blockers, direction);
        const uint64_t first_bit = squareBit(first);
        const Piece first_piece = static_cast< Piece >(pos.getPiece(first));
        if ((info.enemy_ & first_bit) != 0)
        {
          if (isSlidingPieceForDirection(first_piece, direction))
          {
            info.checkers_ |= first_bit;
          }
          continue;
        }

        blockers &= ~first_bit;
        blockers &= attacks.rays_[first][direction];
        if (blockers == 0)
        {
          continue;
        }

        const int second = firstBlocker(blockers, direction);
        const uint64_t second_bit = squareBit(second);
        const Piece second_piece = static_cast< Piece >(pos.getPiece(second));
        if ((info.enemy_ & second_bit) != 0 && isSlidingPieceForDirection(second_piece, direction))
        {
          info.pinned_ |= first_bit;
          info.pinLine_[first] = lineMaskFromTo(info.kingSquare_, second, direction);
        }
      }

      info.checkCount_ = __builtin_popcountll(info.checkers_);

      info.inCheck_ = info.checkCount_ != 0;
      if (info.checkCount_ == 1)
      {
        const int checker = __builtin_ctzll(info.checkers_);
        info.checkMask_ = squareBit(checker);
        const Piece checker_piece = static_cast< Piece >(pos.getPiece(checker));
        if (checker_piece == WHITE_BISHOP || checker_piece == BLACK_BISHOP
          || checker_piece == WHITE_ROOK || checker_piece == BLACK_ROOK
          || checker_piece == WHITE_QUEEN || checker_piece == BLACK_QUEEN)
        {
          for (int direction = 0; direction < 8; ++direction)
          {
            if ((attacks.rays_[info.kingSquare_][direction] & squareBit(checker)) != 0)
            {
              info.checkMask_ = lineMaskFromTo(info.kingSquare_, checker, direction);
              break;
            }
          }
        }
      }
      else if (info.checkCount_ > 1)
      {
        info.checkMask_ = 0;
      }

      return info;
    }

    bool canMovePinnedPiece(const LegalInfo& info, int from, int to)
    {
      return (info.pinned_ & squareBit(from)) == 0 || (info.pinLine_[from] & squareBit(to)) != 0;
    }

    bool canMoveNonKing(const LegalInfo& info, int from, int to)
    {
      if (info.checkCount_ > 1)
      {
        return false;
      }
      const uint64_t to_bit = squareBit(to);
      return (info.checkMask_ & to_bit) != 0 && canMovePinnedPiece(info, from, to);
    }

    void pushPromotions(MoveArray& moves, Square from, Square to, int side)
    {
      moves.push({from, to, static_cast< Piece >(WHITE_QUEEN * side)});
      moves.push({from, to, static_cast< Piece >(WHITE_KNIGHT * side)});
      moves.push({from, to, static_cast< Piece >(WHITE_ROOK * side)});
      moves.push({from, to, static_cast< Piece >(WHITE_BISHOP * side)});
    }

    bool isLegalEnPassant(const Position& pos, const LegalInfo& info, int from, int to)
    {
      const int captured = to - 8 * info.side_;
      uint64_t occupied = info.occupied_;
      occupied &= ~squareBit(from);
      occupied &= ~squareBit(captured);
      occupied |= squareBit(to);
      return !isSquareAttackedOnOccupancy(pos, info.kingSquare_, !info.white_, occupied, squareBit(captured));
    }

    void addLegalKingMoves(const Position& pos, const LegalInfo& info, MoveArray& moves, bool captures_only)
    {
      if (info.kingSquare_ < A1 || info.kingSquare_ > H8)
      {
        return;
      }

      const int row = info.kingSquare_ / 8;
      const int col = info.kingSquare_ % 8;
      const int row_offset[8] = {1, 1, 0, -1, -1, -1, 0, 1};
      const int col_offset[8] = {0, 1, 1, 1, 0, -1, -1, -1};
      const uint64_t king_bit = squareBit(info.kingSquare_);

      for (int i = 0; i < 8; ++i)
      {
        const int new_row = row + row_offset[i];
        const int new_col = col + col_offset[i];
        if (new_row < 0 || new_row >= 8 || new_col < 0 || new_col >= 8)
        {
          continue;
        }
        const int to = new_row * 8 + new_col;
        const uint64_t to_bit = squareBit(to);
        if ((info.own_ & to_bit) != 0)
        {
          continue;
        }
        const int target_piece = pos.getPiece(to);
        if (target_piece == WHITE_KING || target_piece == BLACK_KING)
        {
          continue;
        }
        if (captures_only && (info.enemy_ & to_bit) == 0)
        {
          continue;
        }

        uint64_t occupied = info.occupied_ & ~king_bit;
        occupied |= to_bit;
        uint64_t ignored = (info.enemy_ & to_bit) != 0 ? to_bit : 0;
        if (!isSquareAttackedOnOccupancy(pos, to, !info.white_, occupied, ignored))
        {
          moves.push({static_cast< Square >(info.kingSquare_), static_cast< Square >(to)});
        }
      }
    }

    void addLegalCastlingMoves(const Position& pos, const LegalInfo& info, MoveArray& moves)
    {
      if (info.inCheck_ || info.kingSquare_ < A1 || info.kingSquare_ > H8)
      {
        return;
      }

      Castling rights = pos.getCastling();
      const int square = info.kingSquare_;

      if (rights.king_ && pos.getPiece(square + 1) == EMPTY && pos.getPiece(square + 2) == EMPTY
        && !isSquareAttackedOnOccupancy(pos, square + 1, !info.white_, info.occupied_)
        && !isSquareAttackedOnOccupancy(pos, square + 2, !info.white_, info.occupied_))
      {
        moves.push({static_cast< Square >(square), static_cast< Square >(square + 2), EMPTY, 0, 1});
      }

      if (rights.queen_ && pos.getPiece(square - 1) == EMPTY && pos.getPiece(square - 2) == EMPTY
        && pos.getPiece(square - 3) == EMPTY
        && !isSquareAttackedOnOccupancy(pos, square - 1, !info.white_, info.occupied_)
        && !isSquareAttackedOnOccupancy(pos, square - 2, !info.white_, info.occupied_))
      {
        moves.push({static_cast< Square >(square), static_cast< Square >(square - 2), EMPTY, 0, 1});
      }
    }

    void addLegalRayMoves(const LegalInfo& info, Square from, MoveArray& moves,
      int row_step, int col_step, bool captures_only)
    {
      int row = from / 8 + row_step;
      int col = from % 8 + col_step;

      while (row >= 0 && row < 8 && col >= 0 && col < 8)
      {
        const int to = row * 8 + col;
        const uint64_t to_bit = squareBit(to);
        if ((info.own_ & to_bit) != 0)
        {
          break;
        }
        if ((!captures_only || (info.enemy_ & to_bit) != 0) && canMoveNonKing(info, from, to))
        {
          moves.push({from, static_cast< Square >(to)});
        }
        if ((info.occupied_ & to_bit) != 0)
        {
          break;
        }
        row += row_step;
        col += col_step;
      }
    }

    void addLegalKnightMoves(const LegalInfo& info, Square from, MoveArray& moves, bool captures_only)
    {
      if ((info.pinned_ & squareBit(from)) != 0 || info.checkCount_ > 1)
      {
        return;
      }

      const int row = from / 8;
      const int col = from % 8;
      const int row_offset[8] = {2, 1, -1, -2, -2, -1, 1, 2};
      const int col_offset[8] = {1, 2, 2, 1, -1, -2, -2, -1};

      for (int i = 0; i < 8; ++i)
      {
        const int new_row = row + row_offset[i];
        const int new_col = col + col_offset[i];
        if (new_row < 0 || new_row >= 8 || new_col < 0 || new_col >= 8)
        {
          continue;
        }
        const int to = new_row * 8 + new_col;
        const uint64_t to_bit = squareBit(to);
        if ((info.own_ & to_bit) != 0 || (captures_only && (info.enemy_ & to_bit) == 0))
        {
          continue;
        }
        if ((info.checkMask_ & to_bit) != 0)
        {
          moves.push({from, static_cast< Square >(to)});
        }
      }
    }

    void addLegalPawnMoves(const Position& pos, const LegalInfo& info, Square from, MoveArray& moves,
      bool captures_only)
    {
      if (info.checkCount_ > 1)
      {
        return;
      }

      const int row = from / 8;
      const int col = from % 8;
      const int displacement = info.white_ ? 8 : -8;
      const int start_row = info.white_ ? 1 : 6;
      const int promotion_row = info.white_ ? 6 : 1;
      const int en_passant_row = info.white_ ? 4 : 3;

      if (!captures_only)
      {
        const int to = from + displacement;
        if (to >= A1 && to <= H8 && (info.occupied_ & squareBit(to)) == 0)
        {
          if (canMoveNonKing(info, from, to))
          {
            if (row == promotion_row)
            {
              pushPromotions(moves, from, static_cast< Square >(to), info.side_);
            }
            else
            {
              moves.push({from, static_cast< Square >(to)});
            }
          }

          const int double_to = from + displacement * 2;
          if (row == start_row && (info.occupied_ & squareBit(double_to)) == 0
            && canMoveNonKing(info, from, double_to))
          {
            moves.push({from, static_cast< Square >(double_to)});
          }
        }
      }

      const int take_displacements[2] = {9 * info.side_, 7 * info.side_};
      const int corner_col_for_take[2] = {info.white_ ? 7 : 0, info.white_ ? 0 : 7};

      for (int i = 0; i < 2; ++i)
      {
        if (col == corner_col_for_take[i])
        {
          continue;
        }
        const int to = from + take_displacements[i];
        const uint64_t to_bit = squareBit(to);
        if ((info.enemy_ & to_bit) != 0 && canMoveNonKing(info, from, to))
        {
          if (row == promotion_row)
          {
            pushPromotions(moves, from, static_cast< Square >(to), info.side_);
          }
          else
          {
            moves.push({from, static_cast< Square >(to)});
          }
        }
        else if (row == en_passant_row && pos.getEnPassantSquare() == to
          && isLegalEnPassant(pos, info, from, to))
        {
          moves.push({from, static_cast< Square >(to), EMPTY, true});
        }
      }
    }

    void addLegalPieceMoves(const Position& pos, const LegalInfo& info, Square from, MoveArray& moves,
      bool captures_only)
    {
      const int piece = pos.getPiece(from);
      const int abs_piece = piece > 0 ? piece : -piece;

      switch (abs_piece)
      {
        case WHITE_KNIGHT:
          addLegalKnightMoves(info, from, moves, captures_only);
          break;
        case WHITE_BISHOP:
          addLegalRayMoves(info, from, moves, 1, 1, captures_only);
          addLegalRayMoves(info, from, moves, -1, 1, captures_only);
          addLegalRayMoves(info, from, moves, -1, -1, captures_only);
          addLegalRayMoves(info, from, moves, 1, -1, captures_only);
          break;
        case WHITE_QUEEN:
          addLegalRayMoves(info, from, moves, 1, 0, captures_only);
          addLegalRayMoves(info, from, moves, -1, 0, captures_only);
          addLegalRayMoves(info, from, moves, 0, 1, captures_only);
          addLegalRayMoves(info, from, moves, 0, -1, captures_only);
          addLegalRayMoves(info, from, moves, 1, 1, captures_only);
          addLegalRayMoves(info, from, moves, -1, 1, captures_only);
          addLegalRayMoves(info, from, moves, -1, -1, captures_only);
          addLegalRayMoves(info, from, moves, 1, -1, captures_only);
          break;
        case WHITE_PAWN:
          addLegalPawnMoves(pos, info, from, moves, captures_only);
          break;
        case WHITE_ROOK:
          addLegalRayMoves(info, from, moves, 1, 0, captures_only);
          addLegalRayMoves(info, from, moves, -1, 0, captures_only);
          addLegalRayMoves(info, from, moves, 0, 1, captures_only);
          addLegalRayMoves(info, from, moves, 0, -1, captures_only);
          break;
        case WHITE_KING:
          addLegalKingMoves(pos, info, moves, captures_only);
          if (!captures_only)
          {
            addLegalCastlingMoves(pos, info, moves);
          }
          break;
      }
    }
  }

  void MoveGenerator::generateKingMoves(const Position& pos, Square square, MoveArray& moves)
  {
    constexpr int possible_moves = 8;
    const int piece = pos.getPiece(square);
    const uint64_t own = ownPieces(pos, piece);
    const int row = square / 8;
    const int col = square % 8;

    int row_offset[possible_moves] = {1, 1, 0, -1, -1, -1, 0, 1};
    int col_offset[possible_moves] = {0, 1, 1, 1, 0, -1, -1, -1};

    for (int i = 0; i < possible_moves; ++i)
    {
      int new_row = row + row_offset[i];
      int new_col = col + col_offset[i];
      if (new_row >= 0 && new_row < 8 && new_col >= 0 && new_col < 8)
      {
        int dest_square = new_row * 8 + new_col;
        if ((own & squareBit(dest_square)) == 0)
        {
          moves.push({square, static_cast< Square >(dest_square)});
        }
      }
    }
  }

  void MoveGenerator::generateCastlingMoves(const Position& pos, Square square, MoveArray& moves)
  {
    Castling rights = pos.getCastling();
    UndoInfo undo;
    Position half_king_move_pos = pos;
    bool king_castle = false;
    if (rights.king_ && pos.getPiece(square + 1) == EMPTY && pos.getPiece(square + 2) == EMPTY)
    {
      half_king_move_pos.makeMove(Move{square, static_cast< Square >(square + 1)}, undo);
      king_castle = true;
      if (!isSquareAttackedQuick(pos, static_cast< Square >(square), !pos.isWhiteToMove())
      && !isSquareAttackedQuick(half_king_move_pos, static_cast< Square >(square + 1), !pos.isWhiteToMove()))
      {
        moves.push({square, static_cast< Square >(square + 2), EMPTY, 0, 1});
      }
    }
    if (rights.queen_ && pos.getPiece(square - 1) == EMPTY
    && pos.getPiece(square - 2) == EMPTY && pos.getPiece(square - 3) == EMPTY)
    {
      if (king_castle)
      {
        half_king_move_pos.undoMove(Move{square, static_cast< Square >(square + 1)}, undo);
      }
      half_king_move_pos.makeMove(Move{square, static_cast< Square >(square - 1)}, undo);
      if (!isSquareAttackedQuick(pos, static_cast< Square >(square), !pos.isWhiteToMove())
      && !isSquareAttackedQuick(half_king_move_pos, static_cast< Square >(square - 1), !pos.isWhiteToMove()))
      {
        moves.push({square, static_cast< Square >(square - 2), EMPTY, 0, 1});
      }
    }
  }


  void MoveGenerator::generateQueenMoves(const Position& pos, Square square, MoveArray& moves)
  {
    generateRookMoves(pos, square, moves);
    generateBishopMoves(pos, square, moves);
  }

  void MoveGenerator::generateKnightMoves(const Position& pos, Square square, MoveArray& moves)
  {
    constexpr int possible_moves = 8;
    const int piece = pos.getPiece(square);
    const uint64_t own = ownPieces(pos, piece);
    const int row = square / 8;
    const int col = square % 8;

    int row_offset[possible_moves] = {2, 1, -1, -2, -2, -1, 1, 2};
    int col_offset[possible_moves] = {1, 2, 2, 1, -1, -2, -2, -1};

    for (int i = 0; i < possible_moves; ++i)
    {
      int new_row = row + row_offset[i];
      int new_col = col + col_offset[i];
      if (new_row >= 0 && new_row < 8 && new_col >= 0 && new_col < 8)
      {
        int dest_square = new_row * 8 + new_col;
        if ((own & squareBit(dest_square)) == 0)
        {
          moves.push({square, static_cast< Square >(dest_square)});
        }
      }
    }
  }

  void MoveGenerator::generateBishopMoves(const Position& pos, Square square, MoveArray& moves)
  {
    addRayMoves(pos, square, moves, 1, 1, false);
    addRayMoves(pos, square, moves, -1, 1, false);
    addRayMoves(pos, square, moves, -1, -1, false);
    addRayMoves(pos, square, moves, 1, -1, false);
  }

  void MoveGenerator::generateRookMoves(const Position& pos, Square square, MoveArray& moves)
  {
    addRayMoves(pos, square, moves, 1, 0, false);
    addRayMoves(pos, square, moves, -1, 0, false);
    addRayMoves(pos, square, moves, 0, 1, false);
    addRayMoves(pos, square, moves, 0, -1, false);
  }

  void MoveGenerator::generatePawnMoves(const Position& pos, Square square, MoveArray& moves)
  {
    const int is_white_piece = pos.getPiece(square) > 0 ? 1 : -1;
    const int displacement = is_white_piece == 1 ? 8 : -8;
    const int start_row = is_white_piece == 1 ? 1 : 6;
    const int promotion_row = is_white_piece == 1 ? 6 : 1;
    const int enPassant_row = is_white_piece == 1 ? 4 : 3;

    const int row = square / 8;
    const int col = square % 8;
    const uint64_t occupied = pos.getOccupied();
    const uint64_t enemy = is_white_piece == 1 ? pos.getBlackPieces() : pos.getWhitePieces();

    auto promote = [&moves, square, is_white_piece](int displacement){
        moves.push({square, static_cast< Square >(square + displacement), static_cast< Piece >(WHITE_QUEEN * is_white_piece)});
        moves.push({square, static_cast< Square >(square + displacement), static_cast< Piece >(WHITE_KNIGHT * is_white_piece)});
        moves.push({square, static_cast< Square >(square + displacement), static_cast< Piece >(WHITE_ROOK * is_white_piece)});
        moves.push({square, static_cast< Square >(square + displacement), static_cast< Piece >(WHITE_BISHOP * is_white_piece)});
    };

    if ((occupied & squareBit(square + displacement)) == 0)
    {
      if (row == promotion_row)
      {
        promote(displacement);
      }
      else
      {
        moves.push({square, static_cast< Square >(square + displacement)});
        if (row == start_row && (occupied & squareBit(square + displacement * 2)) == 0)
        {
          moves.push({square, static_cast< Square >(square + displacement * 2)});
        }
      }
    }

    const int take_displacements[2] = {9 * is_white_piece, 7 * is_white_piece};
    const int corner_col_for_take[2] = {is_white_piece == 1 ? 7 : 0, is_white_piece == 1 ? 0 : 7};

    for (size_t i = 0; i < 2; ++i)
    {
      if (col != corner_col_for_take[i])
      {
        const int dest_square = square + take_displacements[i];
        if ((enemy & squareBit(dest_square)) != 0)
        {
          if (row == promotion_row)
          {
            promote(take_displacements[i]);
          }
          else
          {
            moves.push({square, static_cast< Square >(dest_square)});
          }
        }
        else if (row == enPassant_row && pos.getEnPassantSquare() == dest_square)
        {
          moves.push({square, static_cast< Square >(dest_square), EMPTY, true});
        }
      }
    }
  }

  bool MoveGenerator::isSquareAttacked(const Position& pos, Square square)
  {
    const int row = square / 8;
    const int col = square % 8;
    const int is_white_move = pos.isWhiteToMove() ? 1 : -1;
    MoveArray moves;
    int i = 0;

    generateRookMoves(pos, square, moves);
    for (; i < moves.size(); ++i)
    {
      if (pos.getPiece(moves.get(i).to_) == WHITE_ROOK * is_white_move) return true;
      if (pos.getPiece(moves.get(i).to_) == WHITE_QUEEN * is_white_move) return true;
    }

    generateBishopMoves(pos, square, moves);
    for (; i < moves.size(); ++i)
    {
      if (pos.getPiece(moves.get(i).to_) == WHITE_BISHOP * is_white_move) return true;
      if (pos.getPiece(moves.get(i).to_) == WHITE_QUEEN * is_white_move) return true;
    }

    generateKnightMoves(pos, square, moves);
    for (; i < moves.size(); ++i)
    {
      if (pos.getPiece(moves.get(i).to_) == WHITE_KNIGHT * is_white_move) return true;
    }

    generateKingMoves(pos, square, moves);
    for (; i < moves.size(); ++i)
    {
      if (pos.getPiece(moves.get(i).to_) == WHITE_KING * is_white_move) return true;
    }

    if (is_white_move == 1)
    {
      if (row > 0 && col > 0 && pos.getPiece(square - 9) == WHITE_PAWN) return true;
      if (row > 0 && col < 7 && pos.getPiece(square - 7) == WHITE_PAWN) return true;
    }
    else
    {
      if (row < 7 && col > 0 && pos.getPiece(square + 7) == BLACK_PAWN) return true;
      if (row < 7 && col < 7 && pos.getPiece(square + 9) == BLACK_PAWN) return true;
    }

    return false;
  }

  bool MoveGenerator::isSquareAttackedQuick(const Position& pos, Square square, bool byWhite)
  {
    return isSquareAttackedOnOccupancy(pos, square, byWhite, pos.getOccupied());
  }

  MoveArray MoveGenerator::generatePseudoLegalMoves(const Position& pos, bool castling)
  {
    MoveArray moves;
    uint64_t pieces = pos.getSidePieces(pos.isWhiteToMove());
    while (pieces != 0)
    {
      const int i = popLeastSignificantBit(pieces);
      const int piece = pos.getPiece(i);
      const int is_white_piece = piece > 0 ? 1 : -1;
      const int abs_piece = piece * is_white_piece;
      switch (abs_piece)
      {
        case EMPTY:
          break;
        case WHITE_KNIGHT:
          generateKnightMoves(pos, static_cast< Square >(i), moves);
          break;
        case WHITE_BISHOP:
          generateBishopMoves(pos, static_cast< Square >(i), moves);
          break;
        case WHITE_QUEEN:
          generateQueenMoves(pos, static_cast< Square >(i), moves);
          break;
        case WHITE_PAWN:
          generatePawnMoves(pos, static_cast< Square >(i), moves);
          break;
        case WHITE_ROOK:
          generateRookMoves(pos, static_cast< Square >(i), moves);
          break;
        case WHITE_KING:
          generateKingMoves(pos, static_cast< Square >(i), moves);
          if (castling)
          {
            generateCastlingMoves(pos, static_cast< Square >(i), moves);
          }
          break;
      }
    }
    return moves;
  }

  MoveArray MoveGenerator::generateLegalMoves(const Position& pos)
  {
    MoveArray moves;
    LegalInfo info = buildLegalInfo(pos);
    uint64_t pieces = info.own_;
    if (!info.inCheck_ && info.pinned_ == 0)
    {
      while (pieces != 0)
      {
        const int from = popLeastSignificantBit(pieces);
        const int piece = pos.getPiece(from);
        const int abs_piece = piece > 0 ? piece : -piece;
        switch (abs_piece)
        {
          case WHITE_KNIGHT:
            generateKnightMoves(pos, static_cast< Square >(from), moves);
            break;
          case WHITE_BISHOP:
            generateBishopMoves(pos, static_cast< Square >(from), moves);
            break;
          case WHITE_QUEEN:
            generateQueenMoves(pos, static_cast< Square >(from), moves);
            break;
          case WHITE_PAWN:
            addLegalPawnMoves(pos, info, static_cast< Square >(from), moves, false);
            break;
          case WHITE_ROOK:
            generateRookMoves(pos, static_cast< Square >(from), moves);
            break;
          case WHITE_KING:
            addLegalPieceMoves(pos, info, static_cast< Square >(from), moves, false);
            break;
        }
      }
      return moves;
    }

    while (pieces != 0)
    {
      const int from = popLeastSignificantBit(pieces);
      addLegalPieceMoves(pos, info, static_cast< Square >(from), moves, false);
    }
    return moves;
  }

  bool MoveGenerator::isMate(const Position& pos)
  {
    if (generateLegalMoves(pos).empty()
      && isCheck(pos))
    {
      return true;
    }
    return false;
  }

  bool MoveGenerator::isStaleMate(const Position& pos)
  {
    if (generateLegalMoves(pos).empty()
      && !isSquareAttackedQuick(pos, static_cast< Square >(pos.getCurentColourKingSquare()), !pos.isWhiteToMove()))
    {
      return true;
    }
    return false;
  }

  bool MoveGenerator::isCheck(const Position& pos)
  {
    if (isSquareAttackedQuick(pos, static_cast< Square >(pos.getCurentColourKingSquare()), !pos.isWhiteToMove()))
    {
      return true;
    }
    return false;
  }

  Move MoveGenerator::findPawnAttacker(const Position& pos, int square, int side)
  {
    const int row = square / 8;
    const int col = square % 8;

    if (side == 1)
    {
      if (row > 0 && col > 0 && (pos.getBitboard(WHITE_PAWN) & squareBit(square - 9)) != 0)
      {
        return Move{static_cast< Square >(square - 9), static_cast< Square >(square)};
      }
      if (row > 0 && col < 7 && (pos.getBitboard(WHITE_PAWN) & squareBit(square - 7)) != 0)
      {
        return Move{static_cast< Square >(square - 7), static_cast< Square >(square)};
      }
    }
    else
    {
      if (row < 7 && col > 0 && (pos.getBitboard(BLACK_PAWN) & squareBit(square + 7)) != 0)
      {
        return Move{static_cast< Square >(square + 7), static_cast< Square >(square)};
      }
      if (row < 7 && col < 7 && (pos.getBitboard(BLACK_PAWN) & squareBit(square + 9)) != 0)
      {
        return Move{static_cast< Square >(square + 9), static_cast< Square >(square)};
      }
    }

    return null_move;
  }

  Move MoveGenerator::findKnightAttacker(const Position& pos, int square, int side)
  {
    const int row = square / 8;
    const int col = square % 8;
    const int knight = WHITE_KNIGHT * (side > 0 ? 1 : -1);
    const uint64_t knights = pos.getBitboard(static_cast< Piece >(knight));
    const int row_offset[8] = {2, 1, -1, -2, -2, -1, 1, 2};
    const int col_offset[8] = {1, 2, 2, 1, -1, -2, -2, -1};

    for (int i = 0; i < 8; ++i)
    {
      int cur_row = row + row_offset[i];
      int cur_col = col + col_offset[i];
      if (cur_row >= 0 && cur_row < 8 && cur_col >= 0 && cur_col < 8)
      {
        int from = cur_row * 8 + cur_col;
        if ((knights & squareBit(from)) != 0)
        {
          return Move{static_cast< Square >(from), static_cast< Square >(square)};
        }
      }
    }

    return null_move;
  }

  Move MoveGenerator::findBishopAttacker(const Position& pos, int square, int side)
  {
    const int directions[4] = {4, 5, 6, 7};
    return findSlidingAttacker(pos, square, static_cast< Piece >(WHITE_BISHOP * (side > 0 ? 1 : -1)), directions, 4);
  }

  Move MoveGenerator::findRookAttacker(const Position& pos, int square, int side)
  {
    const int directions[4] = {0, 1, 2, 3};
    return findSlidingAttacker(pos, square, static_cast< Piece >(WHITE_ROOK * (side > 0 ? 1 : -1)), directions, 4);
  }

  Move MoveGenerator::findQueenAttacker(const Position& pos, int square, int side)
  {
    const int directions[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    return findSlidingAttacker(pos, square, static_cast< Piece >(WHITE_QUEEN * (side > 0 ? 1 : -1)), directions, 8);
  }

  Move MoveGenerator::findKingAttacker(const Position& pos, int square, int side)
  {
    const int row = square / 8;
    const int col = square % 8;
    const int king = WHITE_KING * (side > 0 ? 1 : -1);
    const uint64_t kings = pos.getBitboard(static_cast< Piece >(king));
    const int row_offset[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    const int col_offset[8] = {0, 1, 1, 1, 0, -1, -1, -1};

    for (int i = 0; i < 8; ++i)
    {
      int cur_row = row + row_offset[i];
      int cur_col = col + col_offset[i];
      if (cur_row >= 0 && cur_row < 8 && cur_col >= 0 && cur_col < 8)
      {
        int from = cur_row * 8 + cur_col;
        if ((kings & squareBit(from)) != 0)
        {
          return Move{static_cast< Square >(from), static_cast< Square >(square)};
        }
      }
    }

    return null_move;
  }

  void MoveGenerator::generateRookCaptures(const Position& pos, Square square, MoveArray& moves)
  {
    addRayMoves(pos, square, moves, 1, 0, true);
    addRayMoves(pos, square, moves, -1, 0, true);
    addRayMoves(pos, square, moves, 0, 1, true);
    addRayMoves(pos, square, moves, 0, -1, true);
  }

  void MoveGenerator::generateBishopCaptures(const Position& pos, Square square, MoveArray& moves)
  {
    addRayMoves(pos, square, moves, 1, 1, true);
    addRayMoves(pos, square, moves, -1, 1, true);
    addRayMoves(pos, square, moves, -1, -1, true);
    addRayMoves(pos, square, moves, 1, -1, true);
  }

  void MoveGenerator::generateKnightCaptures(const Position& pos, Square square, MoveArray& moves)
  {
    constexpr int possible_moves = 8;
    const int piece = pos.getPiece(square);
    const uint64_t enemy = enemyPieces(pos, piece);
    const int row = square / 8;
    const int col = square % 8;

    int row_offset[possible_moves] = {2, 1, -1, -2, -2, -1, 1, 2};
    int col_offset[possible_moves] = {1, 2, 2, 1, -1, -2, -2, -1};

    for (int i = 0; i < possible_moves; ++i)
    {
      int new_row = row + row_offset[i];
      int new_col = col + col_offset[i];
      if (new_row >= 0 && new_row < 8 && new_col >= 0 && new_col < 8)
      {
        int dest_square = new_row * 8 + new_col;
        if ((enemy & squareBit(dest_square)) != 0)
        {
          moves.push({square, static_cast< Square >(dest_square)});
        }
      }
    }
  }

  void MoveGenerator::generateKingCaptures(const Position& pos, Square square, MoveArray& moves)
  {
    constexpr int possible_moves = 8;
    const int piece = pos.getPiece(square);
    const uint64_t enemy = enemyPieces(pos, piece);
    const int row = square / 8;
    const int col = square % 8;

    int row_offset[possible_moves] = {1, 1, 0, -1, -1, -1, 0, 1};
    int col_offset[possible_moves] = {0, 1, 1, 1, 0, -1, -1, -1};

    for (int i = 0; i < possible_moves; ++i)
    {
      int new_row = row + row_offset[i];
      int new_col = col + col_offset[i];
      if (new_row >= 0 && new_row < 8 && new_col >= 0 && new_col < 8)
      {
        int dest_square = new_row * 8 + new_col;
        if ((enemy & squareBit(dest_square)) != 0)
        {
          moves.push({square, static_cast< Square >(dest_square)});
        }
      }
    }
  }

  void MoveGenerator::generatePawnCapturesAndPromotions(const Position& pos, Square square, MoveArray& moves)
  {
    const int is_white_piece = pos.getPiece(square) > 0 ? 1 : -1;
    const int displacement = is_white_piece == 1 ? 8 : -8;

    const int promotion_row = is_white_piece == 1 ? 6 : 1;
    const int enPassant_row = is_white_piece == 1 ? 4 : 3;

    const int row = square / 8;
    const int col = square % 8;
    const uint64_t occupied = pos.getOccupied();
    const uint64_t enemy = is_white_piece == 1 ? pos.getBlackPieces() : pos.getWhitePieces();

    auto promote = [&moves, square, is_white_piece](int displacement){
        moves.push({square, static_cast< Square >(square + displacement), static_cast< Piece >(WHITE_QUEEN * is_white_piece)});
        moves.push({square, static_cast< Square >(square + displacement), static_cast< Piece >(WHITE_KNIGHT * is_white_piece)});
        moves.push({square, static_cast< Square >(square + displacement), static_cast< Piece >(WHITE_ROOK * is_white_piece)});
        moves.push({square, static_cast< Square >(square + displacement), static_cast< Piece >(WHITE_BISHOP * is_white_piece)});
    };

    if ((occupied & squareBit(square + displacement)) == 0 && row == promotion_row)
    {
      promote(displacement);
    }

    const int take_displacements[2] = {9 * is_white_piece, 7 * is_white_piece};
    const int corner_col_for_take[2] = {is_white_piece == 1 ? 7 : 0, is_white_piece == 1 ? 0 : 7};

    for (size_t i = 0; i < 2; ++i)
    {
      if (col != corner_col_for_take[i])
      {
        const int dest_square = square + take_displacements[i];
        if ((enemy & squareBit(dest_square)) != 0)
        {
          if (row == promotion_row)
          {
            promote(take_displacements[i]);
          }
          else
          {
            moves.push({square, static_cast< Square >(dest_square)});
          }
        }
        else if (row == enPassant_row && pos.getEnPassantSquare() == dest_square)
        {
          moves.push({square, static_cast< Square >(dest_square), EMPTY, true});
        }
      }
    }
  }

  void MoveGenerator::generateQueenCaptures(const Position& pos, Square square, MoveArray& moves)
  {
    generateRookCaptures(pos, square, moves);
    generateBishopCaptures(pos, square, moves);
  }

  void MoveGenerator::generatePseudoLegalActiveMoves(const Position& pos, MoveArray& moves)
  {
    uint64_t pieces = pos.getSidePieces(pos.isWhiteToMove());
    while (pieces != 0)
    {
      const int i = popLeastSignificantBit(pieces);
      const int piece = pos.getPiece(i);
      const int is_white_piece = piece > 0 ? 1 : -1;
      const int abs_piece = piece * is_white_piece;
      switch (abs_piece)
      {
        case EMPTY:
          break;
        case WHITE_KNIGHT:
          generateKnightCaptures(pos, static_cast< Square >(i), moves);
          break;
        case WHITE_BISHOP:
          generateBishopCaptures(pos, static_cast< Square >(i), moves);
          break;
        case WHITE_QUEEN:
          generateQueenCaptures(pos, static_cast< Square >(i), moves);
          break;
        case WHITE_PAWN:
          generatePawnCapturesAndPromotions(pos, static_cast< Square >(i), moves);
          break;
        case WHITE_ROOK:
          generateRookCaptures(pos, static_cast< Square >(i), moves);
          break;
        case WHITE_KING:
          generateKingCaptures(pos, static_cast< Square >(i), moves);
          break;
      }
    }
  }

  MoveArray MoveGenerator::generateActiveMoves(const Position& pos)
  {
    MoveArray moves;
    LegalInfo info = buildLegalInfo(pos);
    uint64_t pieces = info.own_;
    if (!info.inCheck_ && info.pinned_ == 0)
    {
      while (pieces != 0)
      {
        const int from = popLeastSignificantBit(pieces);
        const int piece = pos.getPiece(from);
        const int abs_piece = piece > 0 ? piece : -piece;
        switch (abs_piece)
        {
          case WHITE_KNIGHT:
            generateKnightCaptures(pos, static_cast< Square >(from), moves);
            break;
          case WHITE_BISHOP:
            generateBishopCaptures(pos, static_cast< Square >(from), moves);
            break;
          case WHITE_QUEEN:
            generateQueenCaptures(pos, static_cast< Square >(from), moves);
            break;
          case WHITE_PAWN:
            addLegalPawnMoves(pos, info, static_cast< Square >(from), moves, true);
            break;
          case WHITE_ROOK:
            generateRookCaptures(pos, static_cast< Square >(from), moves);
            break;
          case WHITE_KING:
            addLegalPieceMoves(pos, info, static_cast< Square >(from), moves, true);
            break;
        }
      }
      return moves;
    }

    while (pieces != 0)
    {
      const int from = popLeastSignificantBit(pieces);
      addLegalPieceMoves(pos, info, static_cast< Square >(from), moves, true);
    }
    return moves;
  }

  int MoveGenerator::countPseudoLegalRookMoves(const Position &pos, Square square)
  {
    return __builtin_popcountll(slidingAttacksFrom(square, pos.getOccupied(), 0, 4)
      & ~ownPieces(pos, pos.getPiece(square)));
  }

  int MoveGenerator::countPseudoLegalBishopMoves(const Position &pos, Square square)
  {
    return __builtin_popcountll(slidingAttacksFrom(square, pos.getOccupied(), 4, 8)
      & ~ownPieces(pos, pos.getPiece(square)));
  }

  int MoveGenerator::countPseudoLegalQueenMoves(const Position &pos, Square square)
  {
    return countPseudoLegalRookMoves(pos, square) + countPseudoLegalBishopMoves(pos, square);
  }

  int MoveGenerator::countPseudoLegalKnightMoves(const Position &pos, Square square)
  {
    return __builtin_popcountll(attacks.knight_[square] & ~ownPieces(pos, pos.getPiece(square)));
  }
}
