#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "position.hpp"
#include "move.hpp"
#include <utility>

namespace chess
{
  struct Engine
  {
    std::pair< Move, int > findBestMove(Position& pos, int depth);
    int negamax(Position& pos, int depth, int alpha, int beta, int ply);
    int quiescence(Position& pos, int alpha, int beta, int ply);

    void rateMoves(MoveArray& moves, const Position& pos);
    void rateMove(Move& move, const Position& pos);
    void MvBestMoveToBeg(MoveArray& moves, int ind);
  };
};

#endif
