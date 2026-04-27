#include <iostream>
#include <limits>
#include "position.hpp"
#include "move.hpp"
#include "move_generator.hpp"
#include "engine.hpp"

int main()
{
  using namespace chess;

  UndoInfo undo;
  Position pos;
  pos.setInitial();
  pos.print();
  MoveArray valid_moves;
  Move curr;
  Engine engine;
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
    auto ans =  engine.findBestMove(pos, 6);
    pos.makeMove(ans.first, undo);
    pos.print();
    std::cout << "Eval: " << ans.second << "\n";
  }
}
