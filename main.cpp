#include <iostream>
#include "position.hpp"

// TODO: place pieces on board (for tests)
// TODO: print MoveArray (for tests)
// TODO: castling and enPassant logic

int main()
{
  chess::Position pos;
  pos.setInitial();
  pos.print();
}
