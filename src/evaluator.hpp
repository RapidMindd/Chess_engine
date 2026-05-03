#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP

#include "position.hpp"

namespace chess
{
  struct Evaluator
  {
    static int evaluate(const Position& pos);
    static int relative_eval(const Position& pos);

    static void material(Piece piece, int& eval);
    static void mobility(const Position& pos, int square, Piece piece, int& eval);
    static void piece_square_tables(int square, Piece piece, int& eval);
    static void pawn_structure_fill(Piece piece, int square, int* white, int* black);
    static void pawn_structure_eval(int* white, int* black, int& eval);
    static void king_safety(const Position& pos, int& eval);
  };
}

#endif
