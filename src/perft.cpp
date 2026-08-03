#include "perft.hpp"

#include <iostream>

#include "move.hpp"
#include "move_generator.hpp"

namespace chess
{
  uint64_t perft(Position& pos, int depth)
  {
    if (depth == 0)
    {
      return 1;
    }

    MoveArray moves;
    MoveGenerator::generateLegalMoves(pos, moves);
    if (depth == 1)
    {
      return static_cast< uint64_t >(moves.size());
    }

    uint64_t nodes = 0;
    UndoInfo undo;
    for (int i = 0; i < moves.size(); ++i)
    {
      pos.makeMove(moves.get(i), undo);
      nodes += perft(pos, depth - 1);
      pos.undoMove(moves.get(i), undo);
    }
    return nodes;
  }

  void perftDivide(Position& pos, int depth)
  {
    MoveArray moves;
    MoveGenerator::generateLegalMoves(pos, moves);
    uint64_t total = 0;
    UndoInfo undo;
    for (int i = 0; i < moves.size(); ++i)
    {
      pos.makeMove(moves.get(i), undo);
      const uint64_t nodes = perft(pos, depth - 1);
      pos.undoMove(moves.get(i), undo);
      total += nodes;
      std::cout << moves.get(i) << ": " << nodes << "\n";
    }
    std::cout << "total: " << total << "\n";
  }
}
