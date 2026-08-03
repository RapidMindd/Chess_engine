#ifndef PERFT_HPP
#define PERFT_HPP

#include <cstdint>
#include "position.hpp"

namespace chess
{
  uint64_t perft(Position& pos, int depth);
  void perftDivide(Position& pos, int depth);
}

#endif
