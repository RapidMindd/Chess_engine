#include <cctype>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

#include "engine.hpp"
#include "move.hpp"
#include "move_generator.hpp"
#include "position.hpp"

namespace
{
  void printHelp()
  {
    std::cout << "enter moves as e2-e4, or one of:\n"
      << "  board    print the position\n"
      << "  moves    list the legal moves\n"
      << "  undo     take back the last full move\n"
      << "  quit     leave\n";
  }
}

int main(int argc, char** argv)
{
  using namespace chess;

  if (argc > 3)
  {
    std::cerr << "usage: main [depth] [move time ms]\n";
    return 1;
  }

  const int depth = argc > 1 ? std::atoi(argv[1]) : 8;
  const int move_time = argc > 2 ? std::atoi(argv[2]) : 0;
  if (depth <= 0 || move_time < 0)
  {
    std::cerr << "Invalid arguments\n";
    return 1;
  }

  Position pos;
  pos.setInitial();
  pos.print();
  printHelp();

  Engine engine;
  SearchLimits limits;
  limits.depth = move_time > 0 ? 0 : depth;
  limits.timeMs = move_time;

  /// keeps the position replayable so that "undo" does not need a move history
  std::vector< Move > played;
  std::vector< UndoInfo > undos;

  std::string line;
  while (true)
  {
    MoveArray legal;
    MoveGenerator::generateLegalMoves(pos, legal);
    if (legal.empty())
    {
      std::cout << (MoveGenerator::isCheck(pos) ? "checkmate\n" : "stalemate\n");
      return 0;
    }

    std::cout << "> " << std::flush;
    if (!(std::cin >> line))
    {
      return 0;
    }

    if (line == "quit")
    {
      return 0;
    }
    if (line == "help")
    {
      printHelp();
      continue;
    }
    if (line == "board")
    {
      pos.print();
      continue;
    }
    if (line == "moves")
    {
      legal.print();
      continue;
    }
    if (line == "undo")
    {
      for (int i = 0; i < 2 && !played.empty(); ++i)
      {
        pos.undoMove(played.back(), undos.back());
        played.pop_back();
        undos.pop_back();
      }
      pos.print();
      continue;
    }

    /// the move itself still comes from the stream, so put the token back
    std::istringstream move_stream(line);
    Move entered = {};
    if (!(move_stream >> entered))
    {
      std::cout << "Invalid notation, expected something like e2-e4\n";
      continue;
    }

    const Move move = getMove(legal, entered.from_, entered.to_);
    if (move == null_move)
    {
      std::cout << "Illegal move\n";
      continue;
    }

    undos.emplace_back();
    pos.makeMove(move, undos.back());
    played.push_back(move);

    MoveArray reply_moves;
    MoveGenerator::generateLegalMoves(pos, reply_moves);
    if (reply_moves.empty())
    {
      pos.print();
      std::cout << (MoveGenerator::isCheck(pos) ? "checkmate, you win\n" : "stalemate\n");
      return 0;
    }

    const SearchResult result = engine.search(pos, limits);
    undos.emplace_back();
    pos.makeMove(result.best, undos.back());
    played.push_back(result.best);

    pos.print();
    std::cout << "engine played " << moveToUci(result.best)
      << ", eval " << result.score
      << ", depth " << result.depth << "\n";
  }
}
