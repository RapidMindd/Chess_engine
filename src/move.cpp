#include "move.hpp"

#include <iostream>
#include <stdexcept>

#include "move_generator.hpp"
#include "piece.hpp"
#include "position.hpp"

namespace chess
{
  void MoveArray::print() const
  {
    if (!empty())
    {
      std::cout << moves_[0];
    }
    for (int i = 1; i < size(); ++i)
    {
      std::cout << " " << moves_[i];
    }
    std::cout << "\n";
  }

  bool operator==(const Move& move1, const Move& move2)
  {
    return move1.from_ == move2.from_ && move1.to_ == move2.to_
      && move1.promotionPiece_ == move2.promotionPiece_
      && move1.isCastling_ == move2.isCastling_
      && move1.isEnPassant_ == move2.isEnPassant_;
  }

  bool operator!=(const Move& move1, const Move& move2)
  {
    return !(move1 == move2);
  }

  std::ostream& operator<<(std::ostream& out, const Move& move)
  {
    return out << move.from_ << "-" << move.to_;
  }

  std::ostream& operator<<(std::ostream& out, Square square)
  {
    const char col = static_cast< char >('a' + fileOf(square));
    const char row = static_cast< char >('1' + rankOf(square));
    return out << col << row;
  }

  std::istream& operator>>(std::istream& in, Move& move)
  {
    char char_from = 0;
    int row_from = 0;
    in >> char_from >> row_from;
    char separator = 0;
    in >> separator;
    char char_to = 0;
    int row_to = 0;
    in >> char_to >> row_to;

    if (!in)
    {
      return in;
    }

    const int col_from = char_from - 'a';
    const int col_to = char_to - 'a';
    if (col_from < 0 || col_from > 7 || col_to < 0 || col_to > 7
      || row_from < 1 || row_from > 8 || row_to < 1 || row_to > 8 || separator != '-')
    {
      in.setstate(std::ios::failbit);
      return in;
    }
    move = {static_cast< Square >((row_from - 1) * 8 + col_from),
      static_cast< Square >((row_to - 1) * 8 + col_to)};
    return in;
  }

  std::string moveToUci(const Move& move)
  {
    std::string result;
    result += static_cast< char >('a' + fileOf(move.from_));
    result += static_cast< char >('1' + rankOf(move.from_));
    result += static_cast< char >('a' + fileOf(move.to_));
    result += static_cast< char >('1' + rankOf(move.to_));
    if (move.promotionPiece_ != EMPTY)
    {
      const char names[7] = {' ', 'p', 'n', 'b', 'r', 'q', 'k'};
      result += names[typeOf(move.promotionPiece_)];
    }
    return result;
  }

  bool containsMove(const MoveArray& moves, Move move)
  {
    for (int i = 0; i < moves.size(); ++i)
    {
      if (moves.get(i) == move)
      {
        return true;
      }
    }
    return false;
  }

  bool isEqualArraysUnordered(const MoveArray& moves1, const MoveArray& moves2)
  {
    if (moves1.size() != moves2.size())
    {
      return false;
    }

    for (int i = 0; i < moves1.size(); ++i)
    {
      if (!containsMove(moves2, moves1.get(i)))
      {
        return false;
      }
    }
    return true;
  }

  Move getMove(const MoveArray& moves, Square from, Square to)
  {
    for (int i = 0; i < moves.size(); ++i)
    {
      if (moves.get(i).from_ == from && moves.get(i).to_ == to)
      {
        return moves.get(i);
      }
    }
    return null_move;
  }

  Move getMove(const MoveArray& moves, Move move)
  {
    const Move returned = getMove(moves, move.from_, move.to_);
    if (returned != null_move)
    {
      return returned;
    }
    throw std::logic_error("Illegal move");
  }

  std::string moveToAlgebraic(const Move& move, const Position& pos)
  {
    const int piece = pos.getPiece(move.from_);
    const int piece_type = typeOf(piece);

    Position after = pos;
    UndoInfo undo;
    after.makeMove(move, undo);
    const bool is_check = MoveGenerator::isCheck(after);
    const bool is_mate = is_check && MoveGenerator::isMate(after);
    const char* suffix = is_mate ? "#" : (is_check ? "+" : "");

    if (move.isCastling_)
    {
      return (move.to_ > move.from_ ? std::string("0-0") : std::string("0-0-0")) + suffix;
    }

    const bool is_capture = pos.getPiece(move.to_) != EMPTY || move.isEnPassant_;
    std::string result;

    if (piece_type == PAWN)
    {
      if (is_capture)
      {
        result += static_cast< char >('a' + fileOf(move.from_));
        result += 'x';
      }
    }
    else
    {
      result += pieceToChar(static_cast< Piece >(piece_type));

      /// only spell out the departure square when another identical piece
      /// could go to the same square
      bool same_file = false;
      bool same_rank = false;
      bool ambiguous = false;
      MoveArray legal_moves;
      MoveGenerator::generateLegalMoves(pos, legal_moves);
      for (int i = 0; i < legal_moves.size(); ++i)
      {
        const Move& other = legal_moves.get(i);
        if (other.from_ == move.from_ || other.to_ != move.to_
          || pos.getPiece(other.from_) != piece)
        {
          continue;
        }
        ambiguous = true;
        same_file = same_file || fileOf(other.from_) == fileOf(move.from_);
        same_rank = same_rank || rankOf(other.from_) == rankOf(move.from_);
      }
      if (ambiguous)
      {
        if (!same_file)
        {
          result += static_cast< char >('a' + fileOf(move.from_));
        }
        else if (!same_rank)
        {
          result += static_cast< char >('1' + rankOf(move.from_));
        }
        else
        {
          result += static_cast< char >('a' + fileOf(move.from_));
          result += static_cast< char >('1' + rankOf(move.from_));
        }
      }
      if (is_capture)
      {
        result += 'x';
      }
    }

    result += static_cast< char >('a' + fileOf(move.to_));
    result += static_cast< char >('1' + rankOf(move.to_));
    if (move.promotionPiece_ != EMPTY)
    {
      result += '=';
      result += pieceToChar(static_cast< Piece >(typeOf(move.promotionPiece_)));
    }
    result += suffix;
    return result;
  }

  void printMove(const Move& move, const Position& pos)
  {
    std::cout << moveToAlgebraic(move, pos);
  }
}
