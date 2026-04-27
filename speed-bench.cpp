#include <iostream>
#include <chrono>
#include "position.hpp"
#include "move.hpp"
#include "move_generator.hpp"
#include "engine.hpp"

int main()
{
  using namespace chess;

  SearchNodes nodes;
  Position pos = "r2qr1k1/1bp2pp1/p1nbpn1p/3P2N1/1p5P/P1N1P3/BPQB1PP1/2KR3R b - - 0 15";
  // Position pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

  auto start = std::chrono::high_resolution_clock::now();

  Engine{}.findBestMove(pos, 8, nodes);

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;
  uint64_t total_nodes = nodes.nnodes + nodes.qnodes;
  double nps = total_nodes / duration.count();

  std::cout << duration.count() << "s\n";
  std::cout << "NPS: " << nps << "; negamax nodes: " << nodes.nnodes
  << "; quiecsence nodes: " << nodes.qnodes << "\n";
}
