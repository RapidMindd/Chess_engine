#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "position.hpp"
#include "move.hpp"
#include "utility"

namespace chess
{
  struct Engine
  {
    std::pair< Move, int > findBestMove(Position& pos, int depth);
    int negamax(Position& pos, int depth);
  };
};

#endif
