#include <iostream>
#include <limits>
#include <cctype>
#include "position.hpp"
#include "move.hpp"
#include "move_generator.hpp"
#include "engine.hpp"

int main(int argc, char** argv)
{
  using namespace chess;

  const int default_depth = 6;
  int depth = 0;
  unsigned threads = 0;
  if (argc == 1)
  {
    depth = default_depth;
  }
  else if (argc > 3)
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
  if (argc == 3)
  {
    int i = 0;
    while (argv[2][i] != '\0')
    {
      if (!std::isdigit(argv[2][i]) || i > 2)
      {
        std::cerr << "Invalid arguments\n";
        return 1;
      }
      threads = threads * 10 + static_cast< unsigned >(argv[2][i] - '0');
      ++i;
    }
  }

  UndoInfo undo;
  Position pos;
  pos.setInitial();
  pos.print();
  MoveArray valid_moves;
  Move curr;
  Engine engine;
  if (threads != 0)
  {
    engine.setThreadCount(threads);
  }
  while (true)
  {
    if (!(std::cin >> curr))
    {
      if (std::cin.eof())
      {
        return 0;
      }
      std::cout << "Invalid notation\n";
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      continue;
    }

    valid_moves = MoveGenerator{}.generateLegalMoves(pos);
    try
    {
      pos.makeMove(getMove(valid_moves, curr), undo);
    }
    catch(const std::exception& e)
    {
      std::cout << e.what() << "\n";
      continue;
    }
    auto ans =  engine.findBestMove(pos, depth);
    pos.makeMove(ans.first, undo);
    pos.print();
    std::cout << "Eval: " << ans.second << "\n";
  }
}
