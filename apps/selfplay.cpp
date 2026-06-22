#include "engine.hpp"
#include "move.hpp"
#include "piece.hpp"
#include "position.hpp"
#include <iostream>

int main(int argc, char** argv)
{
  using namespace chess;

  const int default_depth = 7;
  int depth = 0;
  if (argc == 1)
  {
    depth = default_depth;
  }
  else if (argc > 2)
  {
    std::cerr << "Invalid arguments\n";
    return 1;
  }
  else
  {
    int i = 0;
    while (argv[1][i] != '\0')
    {
      if (!std::isdigit(argv[1][i]) || i > 1)
      {
        std::cerr << "Invalid arguments\n";
        return 1;
      }
      depth = depth * 10 + (argv[1][i] - '0');
      ++i;
    }
  }

  UndoInfo undo;
  Engine engine;
  Position pos;
  pos.setInitial();
  int moves_without_activity = 0;
  for (int i = 0; i < 200; ++i)
  {
    auto next = engine.findBestMove(pos, depth);
    if (next.first == null_move)
    {
      std::cout << next.second << "\n";
      std::cout << pos << "\n";
      break;
    }
    if (pos.getPiece(next.first.to_) == EMPTY
      && pos.getPiece(next.first.from_) != WHITE_PAWN * (pos.isWhiteToMove() ? 1 : -1))
    {
      ++moves_without_activity;
    }
    else
    {
      moves_without_activity = 0;
    }
    if (moves_without_activity >= 30)
    {
      std::cout << next.second << "\n";
      std::cout << pos << "\n";
      break;
    }
    printMove(next.first, pos, std::cout);
    pos.makeMove(next.first, undo);
    i % 2 ? std::cout << "\n" : std::cout << " ";
  }
}
