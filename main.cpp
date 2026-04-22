#include <iostream>
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
  MoveArray valid_moves;
  Move curr;
  while (std::cin >> curr)
  {
    if (std::cin.fail())
    {
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
    auto ans =  Engine{}.findBestMove(pos, 6);
    pos.makeMove(ans.first, undo);
    pos.print();
  }
}
