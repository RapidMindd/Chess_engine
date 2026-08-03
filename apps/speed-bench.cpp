#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>

#include "engine.hpp"
#include "move.hpp"
#include "move_generator.hpp"
#include "position.hpp"

namespace
{
  /// a spread of openings, middlegames and endgames so that one slow phase
  /// cannot hide behind a fast one
  const char* const positions[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r2qr1k1/1bp2pp1/p1nbpn1p/3P2N1/1p5P/P1N1P3/BPQB1PP1/2KR3R b - - 0 15",
    "r3rbk1/1bqn1ppp/p1pp1n2/1p2p3/P2PP1N1/1PN3PP/1BPQ1PB1/R3R1K1 b - - 3 15",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "4rrk1/pp1n1ppp/3bp3/3p4/3P4/1BP1PN2/PP3PPP/R4RK1 w - - 0 15",
    "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1",
    "6k1/5ppp/8/8/1P6/P1P5/5PPP/6K1 w - - 0 30"
  };

  constexpr int position_count = sizeof(positions) / sizeof(positions[0]);

  int parsePositiveInt(const char* text, int fallback)
  {
    if (text == nullptr)
    {
      return fallback;
    }
    char* end = nullptr;
    const long value = std::strtol(text, &end, 10);
    if (end == text || *end != '\0' || value <= 0)
    {
      return -1;
    }
    return static_cast< int >(value);
  }
}

int main(int argc, char** argv)
{
  using namespace chess;

  if (argc > 4)
  {
    std::cerr << "usage: benchmark [depth] [threads] [hash mb]\n";
    return 1;
  }

  const int depth = parsePositiveInt(argc > 1 ? argv[1] : nullptr, 10);
  const int threads = parsePositiveInt(argc > 2 ? argv[2] : nullptr, 1);
  const int hash_mb = parsePositiveInt(argc > 3 ? argv[3] : nullptr, 128);
  if (depth < 0 || threads < 0 || hash_mb < 0)
  {
    std::cerr << "Invalid arguments\n";
    return 1;
  }

  Engine engine;
  engine.setHashSizeMb(static_cast< uint64_t >(hash_mb));
  engine.setThreads(threads);

  SearchNodes nodes;
  SearchLimits limits;
  limits.depth = depth;

  std::cout << "depth " << depth << ", threads " << engine.getThreads()
    << ", hash " << hash_mb << " MB\n";
  std::cout << std::fixed << std::setprecision(2);

  const auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < position_count; ++i)
  {
    Position pos = positions[i];
    /// each position is independent, the table must not leak between them
    engine.newGame();
    const SearchResult result = engine.search(pos, limits, &nodes);
    std::cout << "  [" << i + 1 << "] " << moveToUci(result.best)
      << "  eval " << result.score
      << "  depth " << result.depth << "/" << result.selDepth
      << "  " << result.nodes << " nodes"
      << "  " << result.seconds << "s\n";
  }
  const auto end = std::chrono::steady_clock::now();

  const double seconds = std::chrono::duration< double >(end - start).count();
  const uint64_t total_nodes = nodes.nnodes + nodes.qnodes;

  std::cout << "\ntotal time: " << seconds << "s\n";
  std::cout << "nodes: " << total_nodes
    << " (negamax " << nodes.nnodes << ", quiescence " << nodes.qnodes << ")\n";
  std::cout << "NPS: " << static_cast< uint64_t >(seconds > 0 ? total_nodes / seconds : 0) << "\n";
  return 0;
}
