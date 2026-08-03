#include "move_generator.hpp"

namespace chess
{
  namespace
  {
    void pushPromotions(MoveArray& moves, int from, int to, bool white)
    {
      moves.push({static_cast< Square >(from), static_cast< Square >(to), makePiece(QUEEN, white)});
      moves.push({static_cast< Square >(from), static_cast< Square >(to), makePiece(KNIGHT, white)});
      moves.push({static_cast< Square >(from), static_cast< Square >(to), makePiece(ROOK, white)});
      moves.push({static_cast< Square >(from), static_cast< Square >(to), makePiece(BISHOP, white)});
    }

    void pushTargets(MoveArray& moves, int from, Bitboard targets)
    {
      while (targets != 0)
      {
        moves.push({static_cast< Square >(from), static_cast< Square >(popLsb(targets))});
      }
    }

    /// squares the enemy attacks when our king is transparent, so that the king
    /// never steps backwards along the line of a checking slider
    Bitboard dangerSquares(const Position& pos, bool white, Bitboard occupied_without_king)
    {
      const bool enemy_white = !white;
      Bitboard danger = 0;

      Bitboard pawns = pos.getPieces(PAWN, enemy_white);
      danger |= enemy_white
        ? (shiftNorthEast(pawns) | shiftNorthWest(pawns))
        : (shiftSouthEast(pawns) | shiftSouthWest(pawns));

      Bitboard knights = pos.getPieces(KNIGHT, enemy_white);
      while (knights != 0)
      {
        danger |= knight_attacks_bb[popLsb(knights)];
      }

      Bitboard bishops = pos.getPieces(BISHOP, enemy_white) | pos.getPieces(QUEEN, enemy_white);
      while (bishops != 0)
      {
        danger |= bishopAttacks(popLsb(bishops), occupied_without_king);
      }

      Bitboard rooks = pos.getPieces(ROOK, enemy_white) | pos.getPieces(QUEEN, enemy_white);
      while (rooks != 0)
      {
        danger |= rookAttacks(popLsb(rooks), occupied_without_king);
      }

      const Bitboard king = pos.getPieces(KING, enemy_white);
      if (king != 0)
      {
        danger |= king_attacks_bb[lsb(king)];
      }

      return danger;
    }

    bool isLegalEnPassant(const Position& pos, const LegalInfo& info, int from, int to)
    {
      /// hand built positions in the tests may have no king at all
      if (info.kingSquare_ < 0)
      {
        return true;
      }

      const int captured = to - 8 * info.side_;
      Bitboard occupied = info.occupied_;
      occupied &= ~squareBB(from);
      occupied &= ~squareBB(captured);
      occupied |= squareBB(to);

      const bool enemy_white = !info.white_;
      const Bitboard rooks = pos.getPieces(ROOK, enemy_white) | pos.getPieces(QUEEN, enemy_white);
      const Bitboard bishops = pos.getPieces(BISHOP, enemy_white) | pos.getPieces(QUEEN, enemy_white);
      if ((rookAttacks(info.kingSquare_, occupied) & rooks) != 0)
      {
        return false;
      }
      return (bishopAttacks(info.kingSquare_, occupied) & bishops) == 0;
    }

    void addPawnMoves(const Position& pos, const LegalInfo& info, MoveArray& moves,
      bool quiet, bool captures)
    {
      const bool white = info.white_;
      const int up = 8 * info.side_;
      const Bitboard pawns = pos.getPieces(PAWN, white);
      if (pawns == 0)
      {
        return;
      }

      const Bitboard promotion_rank = white ? RANK_8_BB : RANK_1_BB;
      const Bitboard double_rank = white ? rank_bb[3] : rank_bb[4];
      const Bitboard empty = ~info.occupied_;

      if (quiet || (captures && (pawns & (white ? rank_bb[6] : rank_bb[1])) != 0))
      {
        Bitboard single = (white ? shiftNorth(pawns) : shiftSouth(pawns)) & empty;
        Bitboard doubles = (white ? shiftNorth(single) : shiftSouth(single)) & empty & double_rank
          & info.checkMask_;
        Bitboard pushes = single & info.checkMask_;

        Bitboard promotions = pushes & promotion_rank;
        Bitboard quiet_pushes = pushes & ~promotion_rank;

        if (!captures)
        {
          promotions = 0;
        }
        if (!quiet)
        {
          quiet_pushes = 0;
          doubles = 0;
        }

        while (promotions != 0)
        {
          const int to = popLsb(promotions);
          const int from = to - up;
          if ((info.pinned_ & squareBB(from)) == 0 || (line_bb[info.kingSquare_][from] & squareBB(to)) != 0)
          {
            pushPromotions(moves, from, to, white);
          }
        }
        while (quiet_pushes != 0)
        {
          const int to = popLsb(quiet_pushes);
          const int from = to - up;
          if ((info.pinned_ & squareBB(from)) == 0 || (line_bb[info.kingSquare_][from] & squareBB(to)) != 0)
          {
            moves.push({static_cast< Square >(from), static_cast< Square >(to)});
          }
        }
        while (doubles != 0)
        {
          const int to = popLsb(doubles);
          const int from = to - 2 * up;
          if ((info.pinned_ & squareBB(from)) == 0 || (line_bb[info.kingSquare_][from] & squareBB(to)) != 0)
          {
            moves.push({static_cast< Square >(from), static_cast< Square >(to)});
          }
        }
      }

      if (!captures)
      {
        return;
      }

      const Bitboard targets = info.enemy_ & info.checkMask_;
      Bitboard east = (white ? shiftNorthEast(pawns) : shiftSouthEast(pawns)) & targets;
      Bitboard west = (white ? shiftNorthWest(pawns) : shiftSouthWest(pawns)) & targets;
      const int east_offset = white ? 9 : -7;
      const int west_offset = white ? 7 : -9;

      for (int i = 0; i < 2; ++i)
      {
        Bitboard capture_set = i == 0 ? east : west;
        const int offset = i == 0 ? east_offset : west_offset;
        while (capture_set != 0)
        {
          const int to = popLsb(capture_set);
          const int from = to - offset;
          if ((info.pinned_ & squareBB(from)) != 0 && (line_bb[info.kingSquare_][from] & squareBB(to)) == 0)
          {
            continue;
          }
          if ((squareBB(to) & promotion_rank) != 0)
          {
            pushPromotions(moves, from, to, white);
          }
          else
          {
            moves.push({static_cast< Square >(from), static_cast< Square >(to)});
          }
        }
      }

      const int en_passant = pos.getEnPassantSquare();
      if (en_passant >= 0)
      {
        Bitboard candidates = pawn_attacks_bb[white ? 1 : 0][en_passant] & pawns;
        while (candidates != 0)
        {
          const int from = popLsb(candidates);
          const int captured = en_passant - up;
          /// the pawn that can be taken is the checker itself when we are in check
          if (info.checkCount_ != 0 && (info.checkMask_ & squareBB(en_passant)) == 0
            && (info.checkers_ & squareBB(captured)) == 0)
          {
            continue;
          }
          if ((info.pinned_ & squareBB(from)) != 0
            && (line_bb[info.kingSquare_][from] & squareBB(en_passant)) == 0)
          {
            continue;
          }
          if (isLegalEnPassant(pos, info, from, en_passant))
          {
            moves.push({static_cast< Square >(from), static_cast< Square >(en_passant), EMPTY, true});
          }
        }
      }
    }

    void addPieceMoves(const Position& pos, const LegalInfo& info, MoveArray& moves, Bitboard allowed)
    {
      const bool white = info.white_;

      Bitboard knights = pos.getPieces(KNIGHT, white) & ~info.pinned_;
      while (knights != 0)
      {
        const int from = popLsb(knights);
        pushTargets(moves, from, knight_attacks_bb[from] & allowed);
      }

      Bitboard bishops = pos.getPieces(BISHOP, white) | pos.getPieces(QUEEN, white);
      while (bishops != 0)
      {
        const int from = popLsb(bishops);
        Bitboard targets = bishopAttacks(from, info.occupied_) & allowed;
        if ((info.pinned_ & squareBB(from)) != 0)
        {
          targets &= line_bb[info.kingSquare_][from];
        }
        pushTargets(moves, from, targets);
      }

      Bitboard rooks = pos.getPieces(ROOK, white) | pos.getPieces(QUEEN, white);
      while (rooks != 0)
      {
        const int from = popLsb(rooks);
        Bitboard targets = rookAttacks(from, info.occupied_) & allowed;
        if ((info.pinned_ & squareBB(from)) != 0)
        {
          targets &= line_bb[info.kingSquare_][from];
        }
        pushTargets(moves, from, targets);
      }
    }

    void addKingMoves(const LegalInfo& info, MoveArray& moves, Bitboard allowed)
    {
      if (info.kingSquare_ < 0)
      {
        return;
      }
      pushTargets(moves, info.kingSquare_, king_attacks_bb[info.kingSquare_] & allowed & ~info.danger_);
    }

    void addCastlingMoves(const Position& pos, const LegalInfo& info, MoveArray& moves)
    {
      if (info.inCheck_ || info.kingSquare_ < 0)
      {
        return;
      }

      const Castling rights = pos.getCastling();
      const int square = info.kingSquare_;

      if (rights.king_
        && (info.occupied_ & (squareBB(square + 1) | squareBB(square + 2))) == 0
        && (info.danger_ & (squareBB(square + 1) | squareBB(square + 2))) == 0)
      {
        moves.push({static_cast< Square >(square), static_cast< Square >(square + 2), EMPTY, false, true});
      }

      if (rights.queen_
        && (info.occupied_ & (squareBB(square - 1) | squareBB(square - 2) | squareBB(square - 3))) == 0
        && (info.danger_ & (squareBB(square - 1) | squareBB(square - 2))) == 0)
      {
        moves.push({static_cast< Square >(square), static_cast< Square >(square - 2), EMPTY, false, true});
      }
    }

    Bitboard ownPieces(const Position& pos, int piece)
    {
      return piece > 0 ? pos.getWhitePieces() : pos.getBlackPieces();
    }

    Bitboard enemyPieces(const Position& pos, int piece)
    {
      return piece > 0 ? pos.getBlackPieces() : pos.getWhitePieces();
    }

    void addRayMoves(const Position& pos, Square square, MoveArray& moves, int row_step, int col_step,
      bool captures_only)
    {
      const int piece = pos.getPiece(square);
      const Bitboard own = ownPieces(pos, piece);
      const Bitboard enemy = enemyPieces(pos, piece);
      const Bitboard occupied = pos.getOccupied();
      int row = rankOf(square) + row_step;
      int col = fileOf(square) + col_step;

      while (row >= 0 && row < 8 && col >= 0 && col < 8)
      {
        const int dest_square = row * 8 + col;
        const Bitboard dest = squareBB(dest_square);
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
  }

  LegalInfo MoveGenerator::buildLegalInfo(const Position& pos)
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
    info.danger_ = 0;
    info.checkCount_ = 0;
    info.inCheck_ = false;

    if (info.kingSquare_ < 0)
    {
      return info;
    }

    const int king_square = info.kingSquare_;
    const bool enemy_white = !info.white_;

    info.checkers_ = (pawn_attacks_bb[info.white_ ? 0 : 1][king_square] & pos.getPieces(PAWN, enemy_white))
      | (knight_attacks_bb[king_square] & pos.getPieces(KNIGHT, enemy_white));

    const Bitboard enemy_rooks = pos.getPieces(ROOK, enemy_white) | pos.getPieces(QUEEN, enemy_white);
    const Bitboard enemy_bishops = pos.getPieces(BISHOP, enemy_white) | pos.getPieces(QUEEN, enemy_white);

    /// sliders that would attack the king if every own piece disappeared: each
    /// of them either gives check or pins whatever single piece is in between
    Bitboard snipers = (rookAttacks(king_square, info.enemy_) & enemy_rooks)
      | (bishopAttacks(king_square, info.enemy_) & enemy_bishops);
    while (snipers != 0)
    {
      const int sniper = popLsb(snipers);
      const Bitboard blockers = between_bb[king_square][sniper] & info.occupied_;
      if (blockers == 0)
      {
        info.checkers_ |= squareBB(sniper);
      }
      else if (!moreThanOne(blockers) && (blockers & info.own_) != 0)
      {
        info.pinned_ |= blockers;
      }
    }

    info.checkCount_ = popcount(info.checkers_);
    info.inCheck_ = info.checkCount_ != 0;
    if (info.checkCount_ == 1)
    {
      const int checker = lsb(info.checkers_);
      info.checkMask_ = between_bb[king_square][checker] | info.checkers_;
    }
    else if (info.checkCount_ > 1)
    {
      info.checkMask_ = 0;
    }

    info.danger_ = dangerSquares(pos, info.white_, info.occupied_ ^ squareBB(king_square));

    /// tables that make givesCheck() a couple of bitboard tests per move
    const int enemy_king = info.white_ ? pos.getBlackKingSquare() : pos.getWhiteKingSquare();
    info.enemyKingSquare_ = enemy_king;
    info.discoveryCandidates_ = 0;
    for (int i = 0; i < 7; ++i)
    {
      info.checkSquares_[i] = 0;
    }
    if (enemy_king >= 0)
    {
      const Bitboard bishop_checks = bishopAttacks(enemy_king, info.occupied_);
      const Bitboard rook_checks = rookAttacks(enemy_king, info.occupied_);
      info.checkSquares_[PAWN] = pawn_attacks_bb[info.white_ ? 1 : 0][enemy_king];
      info.checkSquares_[KNIGHT] = knight_attacks_bb[enemy_king];
      info.checkSquares_[BISHOP] = bishop_checks;
      info.checkSquares_[ROOK] = rook_checks;
      info.checkSquares_[QUEEN] = bishop_checks | rook_checks;

      const Bitboard own_rooks = pos.getPieces(ROOK, info.white_) | pos.getPieces(QUEEN, info.white_);
      const Bitboard own_bishops = pos.getPieces(BISHOP, info.white_) | pos.getPieces(QUEEN, info.white_);
      Bitboard own_snipers = (rookAttacks(enemy_king, 0) & own_rooks)
        | (bishopAttacks(enemy_king, 0) & own_bishops);
      while (own_snipers != 0)
      {
        const int sniper = popLsb(own_snipers);
        const Bitboard blockers = between_bb[enemy_king][sniper] & info.occupied_;
        if (blockers != 0 && !moreThanOne(blockers))
        {
          info.discoveryCandidates_ |= blockers & info.own_;
        }
      }
    }

    return info;
  }

  bool MoveGenerator::givesCheck(const Position& pos, const LegalInfo& info, const Move& move)
  {
    const int enemy_king = info.enemyKingSquare_;
    if (enemy_king < 0)
    {
      return false;
    }

    /// these move two pieces or change the piece type, so the cached tables,
    /// which were built for the current occupancy, do not apply
    if (move.promotionPiece_ != EMPTY || move.isEnPassant_ || move.isCastling_)
    {
      return givesCheck(pos, move);
    }

    const Bitboard to_bit = squareBB(move.to_);
    if ((info.checkSquares_[typeOf(pos.getPiece(move.from_))] & to_bit) != 0)
    {
      return true;
    }
    return (info.discoveryCandidates_ & squareBB(move.from_)) != 0
      && (line_bb[enemy_king][move.from_] & to_bit) == 0;
  }

  void MoveGenerator::generateLegalMoves(const Position& pos, const LegalInfo& info, MoveArray& moves)
  {
    addKingMoves(info, moves, ~info.own_);
    if (info.checkCount_ > 1)
    {
      return;
    }
    addCastlingMoves(pos, info, moves);
    addPieceMoves(pos, info, moves, ~info.own_ & info.checkMask_);
    addPawnMoves(pos, info, moves, true, true);
  }

  void MoveGenerator::generateLegalMoves(const Position& pos, MoveArray& moves)
  {
    generateLegalMoves(pos, buildLegalInfo(pos), moves);
  }

  MoveArray MoveGenerator::generateLegalMoves(const Position& pos)
  {
    MoveArray moves;
    generateLegalMoves(pos, moves);
    return moves;
  }

  void MoveGenerator::generateActiveMoves(const Position& pos, const LegalInfo& info, MoveArray& moves)
  {
    if (info.inCheck_)
    {
      generateLegalMoves(pos, info, moves);
      return;
    }
    addKingMoves(info, moves, info.enemy_);
    addPieceMoves(pos, info, moves, info.enemy_);
    addPawnMoves(pos, info, moves, false, true);
  }

  MoveArray MoveGenerator::generateActiveMoves(const Position& pos)
  {
    MoveArray moves;
    generateActiveMoves(pos, buildLegalInfo(pos), moves);
    return moves;
  }

  void MoveGenerator::generateQuietMoves(const Position& pos, const LegalInfo& info, MoveArray& moves)
  {
    if (info.inCheck_)
    {
      return;
    }
    const Bitboard empty = ~info.occupied_;
    addKingMoves(info, moves, empty);
    addCastlingMoves(pos, info, moves);
    addPieceMoves(pos, info, moves, empty);
    addPawnMoves(pos, info, moves, true, false);
  }

  Bitboard MoveGenerator::attackersTo(const Position& pos, int square, Bitboard occupied)
  {
    return (pawn_attacks_bb[1][square] & pos.getBitboard(WHITE_PAWN))
      | (pawn_attacks_bb[0][square] & pos.getBitboard(BLACK_PAWN))
      | (knight_attacks_bb[square] & (pos.getBitboard(WHITE_KNIGHT) | pos.getBitboard(BLACK_KNIGHT)))
      | (king_attacks_bb[square] & (pos.getBitboard(WHITE_KING) | pos.getBitboard(BLACK_KING)))
      | (rookAttacks(square, occupied) & (pos.getBitboard(WHITE_ROOK) | pos.getBitboard(BLACK_ROOK)
        | pos.getBitboard(WHITE_QUEEN) | pos.getBitboard(BLACK_QUEEN)))
      | (bishopAttacks(square, occupied) & (pos.getBitboard(WHITE_BISHOP) | pos.getBitboard(BLACK_BISHOP)
        | pos.getBitboard(WHITE_QUEEN) | pos.getBitboard(BLACK_QUEEN)));
  }

  bool MoveGenerator::isSquareAttackedQuick(const Position& pos, Square square, bool byWhite)
  {
    if (square > H8)
    {
      return false;
    }
    const Bitboard occupied = pos.getOccupied();
    if ((pawn_attacks_bb[byWhite ? 0 : 1][square] & pos.getPieces(PAWN, byWhite)) != 0) return true;
    if ((knight_attacks_bb[square] & pos.getPieces(KNIGHT, byWhite)) != 0) return true;
    if ((king_attacks_bb[square] & pos.getPieces(KING, byWhite)) != 0) return true;
    if ((rookAttacks(square, occupied)
      & (pos.getPieces(ROOK, byWhite) | pos.getPieces(QUEEN, byWhite))) != 0) return true;
    return (bishopAttacks(square, occupied)
      & (pos.getPieces(BISHOP, byWhite) | pos.getPieces(QUEEN, byWhite))) != 0;
  }

  bool MoveGenerator::isSquareAttacked(const Position& pos, Square square)
  {
    return isSquareAttackedQuick(pos, square, pos.isWhiteToMove());
  }

  bool MoveGenerator::isCheck(const Position& pos)
  {
    const int king_square = pos.getCurentColourKingSquare();
    if (king_square < 0)
    {
      return false;
    }
    return isSquareAttackedQuick(pos, static_cast< Square >(king_square), !pos.isWhiteToMove());
  }

  bool MoveGenerator::isMate(const Position& pos)
  {
    const LegalInfo info = buildLegalInfo(pos);
    if (!info.inCheck_)
    {
      return false;
    }
    MoveArray moves;
    generateLegalMoves(pos, info, moves);
    return moves.empty();
  }

  bool MoveGenerator::isStaleMate(const Position& pos)
  {
    const LegalInfo info = buildLegalInfo(pos);
    if (info.inCheck_)
    {
      return false;
    }
    MoveArray moves;
    generateLegalMoves(pos, info, moves);
    return moves.empty();
  }

  bool MoveGenerator::isPseudoLegal(const Position& pos, const Move& move)
  {
    if (move.from_ == move.to_)
    {
      return false;
    }
    const int piece = pos.getPiece(move.from_);
    if (piece == EMPTY || (piece > 0) != pos.isWhiteToMove())
    {
      return false;
    }
    const int target = pos.getPiece(move.to_);
    if (target != EMPTY && (target > 0) == pos.isWhiteToMove())
    {
      return false;
    }

    const bool white = pos.isWhiteToMove();
    const Bitboard occupied = pos.getOccupied();
    const int type = typeOf(piece);

    if (move.isCastling_)
    {
      if (type != KING)
      {
        return false;
      }
      MoveArray castles;
      addCastlingMoves(pos, buildLegalInfo(pos), castles);
      return containsMove(castles, move);
    }

    if (type == PAWN)
    {
      const int up = white ? 8 : -8;
      if (move.isEnPassant_)
      {
        return pos.getEnPassantSquare() == move.to_
          && (pawn_attacks_bb[white ? 0 : 1][move.from_] & squareBB(move.to_)) != 0;
      }
      const bool promotion_rank = rankOf(move.to_) == (white ? 7 : 0);
      if ((move.promotionPiece_ != EMPTY) != promotion_rank)
      {
        return false;
      }
      if (target != EMPTY)
      {
        return (pawn_attacks_bb[white ? 0 : 1][move.from_] & squareBB(move.to_)) != 0;
      }
      if (move.to_ == move.from_ + up)
      {
        return true;
      }
      return move.to_ == move.from_ + 2 * up && rankOf(move.from_) == (white ? 1 : 6)
        && pos.getPiece(move.from_ + up) == EMPTY;
    }

    if (move.promotionPiece_ != EMPTY || move.isEnPassant_)
    {
      return false;
    }

    switch (type)
    {
      case KNIGHT: return (knight_attacks_bb[move.from_] & squareBB(move.to_)) != 0;
      case BISHOP: return (bishopAttacks(move.from_, occupied) & squareBB(move.to_)) != 0;
      case ROOK: return (rookAttacks(move.from_, occupied) & squareBB(move.to_)) != 0;
      case QUEEN: return (queenAttacks(move.from_, occupied) & squareBB(move.to_)) != 0;
      case KING: return (king_attacks_bb[move.from_] & squareBB(move.to_)) != 0;
    }
    return false;
  }

  bool MoveGenerator::givesCheck(const Position& pos, const Move& move)
  {
    const bool white = pos.isWhiteToMove();
    const int enemy_king = white ? pos.getBlackKingSquare() : pos.getWhiteKingSquare();
    if (enemy_king < 0)
    {
      return false;
    }

    const int piece = pos.getPiece(move.from_);
    const int type = move.promotionPiece_ != EMPTY ? typeOf(move.promotionPiece_) : typeOf(piece);
    Bitboard occupied = (pos.getOccupied() ^ squareBB(move.from_)) | squareBB(move.to_);
    if (move.isEnPassant_)
    {
      occupied ^= squareBB(move.to_ - (white ? 8 : -8));
    }

    switch (type)
    {
      case PAWN:
        if ((pawn_attacks_bb[white ? 0 : 1][move.to_] & squareBB(enemy_king)) != 0) return true;
        break;
      case KNIGHT:
        if ((knight_attacks_bb[move.to_] & squareBB(enemy_king)) != 0) return true;
        break;
      case BISHOP:
        if ((bishopAttacks(move.to_, occupied) & squareBB(enemy_king)) != 0) return true;
        break;
      case ROOK:
        if ((rookAttacks(move.to_, occupied) & squareBB(enemy_king)) != 0) return true;
        break;
      case QUEEN:
        if ((queenAttacks(move.to_, occupied) & squareBB(enemy_king)) != 0) return true;
        break;
      default:
        break;
    }

    /// discovered check
    const Bitboard rooks = (pos.getPieces(ROOK, white) | pos.getPieces(QUEEN, white)) & ~squareBB(move.from_);
    const Bitboard bishops = (pos.getPieces(BISHOP, white) | pos.getPieces(QUEEN, white)) & ~squareBB(move.from_);
    if ((rookAttacks(enemy_king, occupied) & rooks & ~squareBB(move.to_)) != 0) return true;
    if ((bishopAttacks(enemy_king, occupied) & bishops & ~squareBB(move.to_)) != 0) return true;

    if (move.isCastling_)
    {
      const int rook_to = move.to_ > move.from_ ? move.to_ - 1 : move.to_ + 1;
      const Bitboard after = (occupied ^ squareBB(move.to_ > move.from_ ? move.to_ + 1 : move.to_ - 2))
        | squareBB(rook_to);
      if ((rookAttacks(rook_to, after) & squareBB(enemy_king)) != 0) return true;
    }

    return false;
  }

  void MoveGenerator::generateKingMoves(const Position& pos, Square square, MoveArray& moves)
  {
    constexpr int possible_moves = 8;
    const int piece = pos.getPiece(square);
    const Bitboard own = ownPieces(pos, piece);
    const int row = rankOf(square);
    const int col = fileOf(square);

    const int row_offset[possible_moves] = {1, 1, 0, -1, -1, -1, 0, 1};
    const int col_offset[possible_moves] = {0, 1, 1, 1, 0, -1, -1, -1};

    for (int i = 0; i < possible_moves; ++i)
    {
      const int new_row = row + row_offset[i];
      const int new_col = col + col_offset[i];
      if (new_row >= 0 && new_row < 8 && new_col >= 0 && new_col < 8)
      {
        const int dest_square = new_row * 8 + new_col;
        if ((own & squareBB(dest_square)) == 0)
        {
          moves.push({square, static_cast< Square >(dest_square)});
        }
      }
    }
  }

  void MoveGenerator::generateKnightMoves(const Position& pos, Square square, MoveArray& moves)
  {
    constexpr int possible_moves = 8;
    const int piece = pos.getPiece(square);
    const Bitboard own = ownPieces(pos, piece);
    const int row = rankOf(square);
    const int col = fileOf(square);

    const int row_offset[possible_moves] = {2, 1, -1, -2, -2, -1, 1, 2};
    const int col_offset[possible_moves] = {1, 2, 2, 1, -1, -2, -2, -1};

    for (int i = 0; i < possible_moves; ++i)
    {
      const int new_row = row + row_offset[i];
      const int new_col = col + col_offset[i];
      if (new_row >= 0 && new_row < 8 && new_col >= 0 && new_col < 8)
      {
        const int dest_square = new_row * 8 + new_col;
        if ((own & squareBB(dest_square)) == 0)
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

  void MoveGenerator::generateQueenMoves(const Position& pos, Square square, MoveArray& moves)
  {
    generateRookMoves(pos, square, moves);
    generateBishopMoves(pos, square, moves);
  }

  void MoveGenerator::generatePawnMoves(const Position& pos, Square square, MoveArray& moves)
  {
    const int piece = pos.getPiece(square);
    const int is_white_piece = piece > 0 ? 1 : -1;
    const bool white = is_white_piece == 1;
    const int displacement = 8 * is_white_piece;
    const int start_row = white ? 1 : 6;
    const int promotion_row = white ? 6 : 1;
    const int en_passant_row = white ? 4 : 3;

    const int row = rankOf(square);
    const int col = fileOf(square);
    const Bitboard occupied = pos.getOccupied();
    const Bitboard enemy = enemyPieces(pos, piece);

    if ((occupied & squareBB(square + displacement)) == 0)
    {
      if (row == promotion_row)
      {
        pushPromotions(moves, square, square + displacement, white);
      }
      else
      {
        moves.push({square, static_cast< Square >(square + displacement)});
        if (row == start_row && (occupied & squareBB(square + displacement * 2)) == 0)
        {
          moves.push({square, static_cast< Square >(square + displacement * 2)});
        }
      }
    }

    const int take_displacements[2] = {9 * is_white_piece, 7 * is_white_piece};
    const int corner_col_for_take[2] = {white ? 7 : 0, white ? 0 : 7};

    for (int i = 0; i < 2; ++i)
    {
      if (col == corner_col_for_take[i])
      {
        continue;
      }
      const int dest_square = square + take_displacements[i];
      if ((enemy & squareBB(dest_square)) != 0)
      {
        if (row == promotion_row)
        {
          pushPromotions(moves, square, dest_square, white);
        }
        else
        {
          moves.push({square, static_cast< Square >(dest_square)});
        }
      }
      else if (row == en_passant_row && pos.getEnPassantSquare() == dest_square)
      {
        moves.push({square, static_cast< Square >(dest_square), EMPTY, true});
      }
    }
  }

  void MoveGenerator::generateCastlingMoves(const Position& pos, Square square, MoveArray& moves)
  {
    const Castling rights = pos.getCastling();
    const bool by_white = !pos.isWhiteToMove();
    const Bitboard occupied = pos.getOccupied();

    if (rights.king_ && (occupied & (squareBB(square + 1) | squareBB(square + 2))) == 0
      && !isSquareAttackedQuick(pos, square, by_white)
      && !isSquareAttackedQuick(pos, static_cast< Square >(square + 1), by_white)
      && !isSquareAttackedQuick(pos, static_cast< Square >(square + 2), by_white))
    {
      moves.push({square, static_cast< Square >(square + 2), EMPTY, false, true});
    }

    if (rights.queen_
      && (occupied & (squareBB(square - 1) | squareBB(square - 2) | squareBB(square - 3))) == 0
      && !isSquareAttackedQuick(pos, square, by_white)
      && !isSquareAttackedQuick(pos, static_cast< Square >(square - 1), by_white)
      && !isSquareAttackedQuick(pos, static_cast< Square >(square - 2), by_white))
    {
      moves.push({square, static_cast< Square >(square - 2), EMPTY, false, true});
    }
  }

  MoveArray MoveGenerator::generatePseudoLegalMoves(const Position& pos, bool castling)
  {
    MoveArray moves;
    Bitboard pieces = pos.getSidePieces(pos.isWhiteToMove());
    while (pieces != 0)
    {
      const int from = popLsb(pieces);
      switch (typeOf(pos.getPiece(from)))
      {
        case KNIGHT: generateKnightMoves(pos, static_cast< Square >(from), moves); break;
        case BISHOP: generateBishopMoves(pos, static_cast< Square >(from), moves); break;
        case QUEEN: generateQueenMoves(pos, static_cast< Square >(from), moves); break;
        case PAWN: generatePawnMoves(pos, static_cast< Square >(from), moves); break;
        case ROOK: generateRookMoves(pos, static_cast< Square >(from), moves); break;
        case KING:
          generateKingMoves(pos, static_cast< Square >(from), moves);
          if (castling)
          {
            generateCastlingMoves(pos, static_cast< Square >(from), moves);
          }
          break;
        default: break;
      }
    }
    return moves;
  }

  void MoveGenerator::generatePseudoLegalActiveMoves(const Position& pos, MoveArray& moves)
  {
    generateActiveMoves(pos, buildLegalInfo(pos), moves);
  }

  int MoveGenerator::countPseudoLegalRookMoves(const Position& pos, Square square)
  {
    return popcount(rookAttacks(square, pos.getOccupied()) & ~ownPieces(pos, pos.getPiece(square)));
  }

  int MoveGenerator::countPseudoLegalBishopMoves(const Position& pos, Square square)
  {
    return popcount(bishopAttacks(square, pos.getOccupied()) & ~ownPieces(pos, pos.getPiece(square)));
  }

  int MoveGenerator::countPseudoLegalQueenMoves(const Position& pos, Square square)
  {
    return popcount(queenAttacks(square, pos.getOccupied()) & ~ownPieces(pos, pos.getPiece(square)));
  }

  int MoveGenerator::countPseudoLegalKnightMoves(const Position& pos, Square square)
  {
    return popcount(knight_attacks_bb[square] & ~ownPieces(pos, pos.getPiece(square)));
  }
}
