#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP

#include "position.hpp"

namespace chess
{
  struct Evaluator
  {
    static int evaluate(const Position& pos);
    static int relative_eval(const Position& pos);
  };
}

#endif
