#include <cctype>
#include <cmath>
#include <iostream>
#include <chrono>
#include "position.hpp"
#include "move.hpp"
#include "move_generator.hpp"
#include "engine.hpp"

int main(int argc, char** argv)
{
  using namespace chess;

  const int default_depth = 6;
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

  SearchNodes nodes;
  constexpr int size = 3;
  const char* pos[size] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r2qr1k1/1bp2pp1/p1nbpn1p/3P2N1/1p5P/P1N1P3/BPQB1PP1/2KR3R b - - 0 15",
    "r3rbk1/1bqn1ppp/p1pp1n2/1p2p3/P2PP1N1/1PN3PP/1BPQ1PB1/R3R1K1 b - - 3 15"
  };

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < size; ++i)
  {
    Position cur = pos[i];
    auto ans = Engine{}.findBestMove(cur, depth, &nodes);
    std::cout << ans.first << " " << ans.second << "\n";
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;
  uint64_t total_nodes = nodes.nnodes + nodes.qnodes;
  double nps = total_nodes / duration.count();

  std::cout << duration.count() << "s\n";
  std::cout << "NPS: " << nps << "; negamax nodes: " << nodes.nnodes
  << "; quiecsence nodes: " << nodes.qnodes << "\n";
}
