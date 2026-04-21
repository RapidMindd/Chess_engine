#include <iostream>
#include "position.hpp"
#include "move.hpp"
#include "move_generator.hpp"
#include "engine.hpp"

int main()
{
  using namespace chess;
  Position pos;
  pos.setInitial();
  UndoInfo undo;
  MoveArray moves = MoveGenerator{}.generateLegalMoves(pos);
  pos.makeMove(getMove(moves, E2, E4), undo);
  moves = MoveGenerator{}.generateLegalMoves(pos);
  pos.makeMove(getMove(moves, D7, D5), undo);
  auto res = Engine{}.findBestMove(pos, 8);
  std::cout << res.first << " " << res.second << "\n";
  // pos.makeMove(res.first, undo);
  // res = Engine{}.findBestMove(pos, 6);
  // std::cout << res.first << " " << res.second << "\n";
  // pos.makeMove(res.first, undo);
  // res = Engine{}.findBestMove(pos, 6);
  // std::cout << res.first << " " << res.second << "\n";
}
