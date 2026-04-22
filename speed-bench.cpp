#include <iostream>
#include <chrono>
#include "position.hpp"
#include "move.hpp"
#include "move_generator.hpp"
#include "engine.hpp"

int main()
{
  using namespace chess;
  Position pos = "r2qr1k1/1bp2pp1/p1nbpn1p/3P2N1/1p5P/P1N1P3/BPQB1PP1/2KR3R b - - 0 15";

  auto start = std::chrono::high_resolution_clock::now();

  Engine{}.findBestMove(pos, 8);

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;

  std::cout << duration.count() << "\n";
}
