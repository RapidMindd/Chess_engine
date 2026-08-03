#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "perft.hpp"
#include "position.hpp"

namespace
{
  struct PerftCase
  {
    const char* fen;
    uint64_t expected[7];
  };

  /// the classic perft suite, expected[i] is the node count at depth i
  const PerftCase cases[] = {
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
      {1, 20, 400, 8902, 197281, 4865609, 119060324}},
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
      {1, 48, 2039, 97862, 4085603, 193690690, 0}},
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
      {1, 14, 191, 2812, 43238, 674624, 11030083}},
    {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
      {1, 6, 264, 9467, 422333, 15833292, 0}},
    {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
      {1, 44, 1486, 62379, 2103487, 89941194, 0}},
    {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
      {1, 46, 2079, 89890, 3894594, 164075551, 0}}
  };

  constexpr int case_count = sizeof(cases) / sizeof(cases[0]);
}

int main(int argc, char** argv)
{
  using namespace chess;

  if (argc > 1 && std::strcmp(argv[1], "divide") == 0)
  {
    if (argc < 4)
    {
      std::cerr << "usage: perft divide <depth> <fen>\n";
      return 1;
    }
    std::string fen = argv[3];
    for (int i = 4; i < argc; ++i)
    {
      fen += ' ';
      fen += argv[i];
    }
    Position pos = fen.c_str();
    perftDivide(pos, std::atoi(argv[2]));
    return 0;
  }

  const int max_depth = argc > 1 ? std::atoi(argv[1]) : 5;
  if (max_depth < 1 || max_depth > 6)
  {
    std::cerr << "depth must be between 1 and 6\n";
    return 1;
  }

  int failures = 0;
  uint64_t total_nodes = 0;
  const auto start = std::chrono::steady_clock::now();

  for (int i = 0; i < case_count; ++i)
  {
    std::cout << cases[i].fen << "\n";
    for (int depth = 1; depth <= max_depth; ++depth)
    {
      const uint64_t expected = cases[i].expected[depth];
      if (expected == 0)
      {
        continue;
      }
      Position pos = cases[i].fen;
      const uint64_t nodes = perft(pos, depth);
      total_nodes += nodes;
      const bool ok = nodes == expected;
      failures += ok ? 0 : 1;
      std::cout << (ok ? "  ok   " : "  FAIL ") << "depth " << depth
        << ": " << nodes << " (expected " << expected << ")\n";
    }
  }

  const double seconds = std::chrono::duration< double >(
    std::chrono::steady_clock::now() - start).count();
  std::cout << "\n" << total_nodes << " nodes in " << seconds << "s ("
    << static_cast< uint64_t >(seconds > 0 ? total_nodes / seconds : 0) << " nps)\n";
  std::cout << (failures == 0 ? "all positions match\n" : "FAILURES\n");
  return failures == 0 ? 0 : 1;
}
