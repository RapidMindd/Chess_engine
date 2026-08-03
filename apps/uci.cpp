/// Minimal UCI front end, enough to plug the engine into a GUI or into
/// cutechess-cli for strength testing.
#include <iostream>
#include <sstream>
#include <thread>
#include <string>
#include <vector>

#include "engine.hpp"
#include "move.hpp"
#include "move_generator.hpp"
#include "perft.hpp"
#include "position.hpp"

using namespace chess;

namespace
{
  const char* const START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

  Move parseUciMove(const Position& pos, const std::string& text)
  {
    MoveArray moves;
    MoveGenerator::generateLegalMoves(pos, moves);
    for (int i = 0; i < moves.size(); ++i)
    {
      if (moveToUci(moves.get(i)) == text)
      {
        return moves.get(i);
      }
    }
    return null_move;
  }

  void applyMoves(Position& pos, std::istringstream& stream)
  {
    std::string token;
    UndoInfo undo;
    while (stream >> token)
    {
      const Move move = parseUciMove(pos, token);
      if (move == null_move)
      {
        break;
      }
      pos.makeMove(move, undo);
    }
  }

  int timeForMove(int remaining, int increment, int moves_to_go)
  {
    if (remaining <= 0)
    {
      return 0;
    }
    const int moves = moves_to_go > 0 ? moves_to_go : 30;
    int budget = remaining / moves + increment * 3 / 4;
    /// never risk running into the flag
    budget = std::min(budget, remaining - 30);
    return std::max(budget, 5);
  }
}

int main()
{
  std::ios::sync_with_stdio(false);

  Engine engine;
  engine.setInfoOutput(true);
  Position pos = START_FEN;

  std::string line;
  while (std::getline(std::cin, line))
  {
    std::istringstream stream(line);
    std::string command;
    stream >> command;

    if (command == "uci")
    {
      std::cout << "id name Chess_engine\n";
      std::cout << "id author Yaroslav Tarasenko\n";
      std::cout << "option name Hash type spin default 64 min 1 max 4096\n";
      std::cout << "option name Threads type spin default 1 max 512\n";
      std::cout << "uciok" << std::endl;
    }
    else if (command == "isready")
    {
      std::cout << "readyok" << std::endl;
    }
    else if (command == "setoption")
    {
      std::string token;
      std::string name;
      std::string value;
      stream >> token;
      while (stream >> token && token != "value")
      {
        name += name.empty() ? token : " " + token;
      }
      while (stream >> token)
      {
        value += value.empty() ? token : " " + token;
      }
      if (name == "Hash")
      {
        engine.setHashSizeMb(static_cast< uint64_t >(std::max(1, std::atoi(value.c_str()))));
      }
      else if (name == "Threads")
      {
        engine.setThreads(std::max(1, std::atoi(value.c_str())));
      }
    }
    else if (command == "ucinewgame")
    {
      engine.newGame();
      pos = START_FEN;
    }
    else if (command == "position")
    {
      std::string token;
      stream >> token;
      if (token == "startpos")
      {
        pos = START_FEN;
        stream >> token;
      }
      else if (token == "fen")
      {
        std::string fen;
        while (stream >> token && token != "moves")
        {
          fen += fen.empty() ? token : " " + token;
        }
        pos = fen.c_str();
      }
      if (token == "moves")
      {
        applyMoves(pos, stream);
      }
    }
    else if (command == "go")
    {
      SearchLimits limits;
      int wtime = 0;
      int btime = 0;
      int winc = 0;
      int binc = 0;
      int movestogo = 0;
      std::string token;
      while (stream >> token)
      {
        if (token == "depth") stream >> limits.depth;
        else if (token == "movetime") stream >> limits.timeMs;
        else if (token == "nodes") stream >> limits.maxNodes;
        else if (token == "infinite") limits.infinite = true;
        else if (token == "wtime") stream >> wtime;
        else if (token == "btime") stream >> btime;
        else if (token == "winc") stream >> winc;
        else if (token == "binc") stream >> binc;
        else if (token == "movestogo") stream >> movestogo;
        else if (token == "perft")
        {
          int depth = 1;
          stream >> depth;
          perftDivide(pos, depth);
          limits.depth = 0;
          limits.infinite = false;
        }
      }
      if (limits.timeMs == 0 && !limits.infinite && (wtime != 0 || btime != 0))
      {
        limits.timeMs = timeForMove(pos.isWhiteToMove() ? wtime : btime,
          pos.isWhiteToMove() ? winc : binc, movestogo);
      }
      if (limits.depth == 0 && limits.timeMs == 0 && limits.maxNodes == 0 && !limits.infinite)
      {
        limits.depth = 8;
      }

      if (limits.infinite)
      {
        /// search in the background so that "stop" can still be read
        SearchResult result;
        std::thread searcher([&]() { result = engine.search(pos, limits); });
        std::string command_while_searching;
        while (std::getline(std::cin, command_while_searching))
        {
          if (command_while_searching == "stop" || command_while_searching == "quit")
          {
            engine.stop();
            break;
          }
          if (command_while_searching == "isready")
          {
            std::cout << "readyok" << std::endl;
          }
        }
        searcher.join();
        std::cout << "bestmove " << moveToUci(result.best) << std::endl;
        if (command_while_searching == "quit")
        {
          break;
        }
      }
      else
      {
        const SearchResult result = engine.search(pos, limits);
        std::cout << "bestmove " << moveToUci(result.best) << std::endl;
      }
    }
    else if (command == "d")
    {
      pos.print();
      std::cout << pos.toFen() << "\n";
    }
    else if (command == "quit")
    {
      break;
    }
  }
  return 0;
}
