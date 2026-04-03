#include <iostream>
#include "position.hpp"

// TODO: generate moves
// TODO: castling and enPassant logic

int main()
{
  chess::Position pos;
  pos.setInitial();
  pos.print();
}
