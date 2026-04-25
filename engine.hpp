#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "position.hpp"
#include "move.hpp"
#include <utility>
#include <cstdint>

namespace chess
{
  struct SearchNodes
  {
    uint64_t nnodes = 0;
    uint64_t qnodes = 0;
  };

  struct Engine
  {
    std::pair< Move, float> findBestMove(Position& pos, int depth);
    std::pair< Move, float> findBestMove(Position& pos, int depth, SearchNodes& nodes);
    int negamax(Position& pos, int depth, int alpha, int beta, int ply, SearchNodes& nodes);
    int quiescence(Position& pos, int alpha, int beta, int ply, SearchNodes& qnodes);

    void rateMoves(MoveArray& moves, const Position& pos);
    void rateMove(Move& move, const Position& pos);
    void MvBestMoveToBeg(MoveArray& moves, int ind);
  };
};

#endif
