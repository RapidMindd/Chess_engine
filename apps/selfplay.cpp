#include <cstdlib>
#include <iostream>

#include "engine.hpp"
#include "move.hpp"
#include "move_generator.hpp"
#include "piece.hpp"
#include "position.hpp"

int main(int argc, char** argv)
{
  using namespace chess;

  if (argc > 4)
  {
    std::cerr << "usage: selfplay [depth] [threads] [move time ms]\n";
    return 1;
  }

  const int depth = argc > 1 ? std::atoi(argv[1]) : 8;
  const int threads = argc > 2 ? std::atoi(argv[2]) : 1;
  const int move_time = argc > 3 ? std::atoi(argv[3]) : 0;
  if (depth <= 0 || threads <= 0 || move_time < 0)
  {
    std::cerr << "Invalid arguments\n";
    return 1;
  }

  Engine engine;
  engine.setThreads(threads);

  Position pos;
  pos.setInitial();
  UndoInfo undo;

  SearchLimits limits;
  limits.depth = move_time > 0 ? 0 : depth;
  limits.timeMs = move_time;

  const char* result = "unfinished";
  for (int ply = 0; ply < 400; ++ply)
  {
    MoveArray legal;
    MoveGenerator::generateLegalMoves(pos, legal);
    if (legal.empty())
    {
      result = MoveGenerator::isCheck(pos)
        ? (pos.isWhiteToMove() ? "0-1 (black mates)" : "1-0 (white mates)")
        : "1/2-1/2 (stalemate)";
      break;
    }
    if (pos.isFiftyMoveDraw())
    {
      result = "1/2-1/2 (fifty move rule)";
      break;
    }
    if (pos.isRepetition())
    {
      result = "1/2-1/2 (repetition)";
      break;
    }
    if (pos.isInsufficientMaterial())
    {
      result = "1/2-1/2 (insufficient material)";
      break;
    }

    const SearchResult search = engine.search(pos, limits);
    if (search.best == null_move)
    {
      break;
    }

    if (pos.isWhiteToMove())
    {
      std::cout << ply / 2 + 1 << ". ";
    }
    printMove(search.best, pos);
    std::cout << (pos.isWhiteToMove() ? " " : "\n") << std::flush;
    pos.makeMove(search.best, undo);
  }

  std::cout << "\n\n" << pos << "\n" << result << "\n";
  return 0;
}
