#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP

#include "position.hpp"

namespace chess
{
  struct Evaluator
  {
    static int evaluate(const Position& pos);
    static int relative_eval(const Position& pos);

    static void material(const Position& pos, int& eval);
    static void mobility(const Position& pos, int& eval);
    static void piece_square_tables(const Position& pos, int& eval);
  };
}

#endif
