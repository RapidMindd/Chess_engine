#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <sstream>
#include <utility>

#include "datastructures/robinHoodTable.hpp"
#include "engine.hpp"
#include "game.hpp"
#include "move_generator.hpp"

using games_t = tarasenko::RobinHoodTable< std::string, chess::Game >;

struct SearchParams
{
  int depth = 6;
  chess::EvaluationStrategy strategy = chess::EvaluationStrategy::BALANCED;
};

std::string readName(std::istream& in)
{
  std::istream::sentry s(in);
  if (!s)
  {
    throw std::logic_error("invalid arguments");
  }
  std::string name;
  in >> name;
  if (!in)
  {
    throw std::logic_error("invalid arguments");
  }
  return name;
}

bool isSpace(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool isNumber(const std::string& str)
{
  if (str.empty())
  {
    return false;
  }
  for (size_t i = 0; i < str.size(); ++i)
  {
    if (str[i] < '0' || str[i] > '9')
    {
      return false;
    }
  }
  return true;
}

int toInt(const std::string& str)
{
  int ans = 0;
  for (size_t i = 0; i < str.size(); ++i)
  {
    ans = ans * 10 + str[i] - '0';
  }
  return ans;
}

std::pair< std::string, std::string > readFenAndName(std::istream& in)
{
  std::string line;
  std::getline(in, line);
  size_t end = line.find_last_not_of(" \t\r\n");
  if (end == std::string::npos)
  {
    throw std::logic_error("invalid arguments");
  }
  size_t name_start = end;
  while (name_start > 0 && !isSpace(line[name_start - 1]))
  {
    --name_start;
  }
  if (name_start == 0)
  {
    throw std::logic_error("invalid arguments");
  }
  size_t fen_end = line.find_last_not_of(" \t\r\n", name_start - 1);
  if (fen_end == std::string::npos)
  {
    throw std::logic_error("invalid arguments");
  }
  std::string fen = line.substr(0, fen_end + 1);
  std::string name = line.substr(name_start, end - name_start + 1);
  return {fen, name};
}

chess::Game& getGame(games_t& games, const std::string& name)
{
  if (games.count(name) == 0)
  {
    throw std::logic_error("no board with this name");
  }
  return games.at(name);
}

chess::Move readMove(std::istream& in)
{
  std::istream::sentry s(in);
  if (!s)
  {
    throw std::logic_error("invalid move");
  }
  chess::Move move;
  in >> move;
  if (!in)
  {
    throw std::logic_error("invalid move");
  }
  return move;
}

SearchParams readSearchParams(std::istream& in)
{
  SearchParams params;
  std::string line;
  std::getline(in, line);
  std::stringstream stream(line);
  std::string word;
  for (int i = 0; stream >> word; ++i)
  {
    if (i == 2)
    {
      throw std::logic_error("invalid arguments");
    }
    if (isNumber(word))
    {
      params.depth = toInt(word);
    }
    else
    {
      params.strategy = chess::getEvaluationStrategy(word);
    }
  }
  if (params.depth <= 0)
  {
    throw std::logic_error("invalid arguments");
  }
  return params;
}

void newGame(std::istream& in, std::ostream&, games_t& games)
{
  std::string name = readName(in);
  if (games.count(name))
  {
    throw std::logic_error("board with this name already exist");
  }
  games.insert({name, chess::Game()});
}

void setFen(std::istream& in, std::ostream&, games_t& games)
{
  auto parsed = readFenAndName(in);
  std::string fen = parsed.first;
  std::string name = parsed.second;
  if (games.count(name))
  {
    throw std::logic_error("board with this name already exist");
  }
  games.insert({name, chess::Game(chess::Position(fen.c_str()))});
}

void printGame(std::istream& in, std::ostream& out, games_t& games)
{
  std::string name = readName(in);
  getGame(games, name).print(out);
}

void flipGame(std::istream& in, std::ostream&, games_t& games)
{
  std::string name = readName(in);
  getGame(games, name).flip();
}

void makeMove(std::istream& in, std::ostream&, games_t& games)
{
  std::string name = readName(in);
  chess::Game& game = getGame(games, name);
  chess::Move move = readMove(in);
  chess::MoveArray moves = chess::MoveGenerator::generateLegalMoves(game.getPosition());
  try
  {
    game.makeMove(chess::getMove(moves, move));
  }
  catch (const std::exception&)
  {
    throw std::logic_error("invalid move");
  }
}

void undoMove(std::istream& in, std::ostream&, games_t& games)
{
  std::string name = readName(in);
  getGame(games, name).undoMove();
}

void nextMove(std::istream& in, std::ostream&, games_t& games)
{
  std::string name = readName(in);
  getGame(games, name).nextMove();
}

void prevMove(std::istream& in, std::ostream&, games_t& games)
{
  std::string name = readName(in);
  getGame(games, name).prevMove();
}

void bestMove(std::istream& in, std::ostream& out, games_t& games)
{
  std::string name = readName(in);
  chess::Game& game = getGame(games, name);

  SearchParams params = readSearchParams(in);

  chess::Position pos = game.getPosition();
  chess::MoveArray moves = chess::MoveGenerator::generateLegalMoves(pos);
  if (moves.empty())
  {
    throw std::logic_error("no legal moves");
  }

  chess::Engine engine(chess::getEvaluationCoefficients(params.strategy));
  out << engine.findBestMove(pos, params.depth).first << "\n";
}

int main()
{
  games_t games;

  using cmd_t = void(*)(std::istream&, std::ostream&, games_t&);
  tarasenko::RobinHoodTable< std::string, cmd_t > cmds;
  cmds["new"] = newGame;
  cmds["set_fen"] = setFen;
  cmds["print"] = printGame;
  cmds["flip"] = flipGame;
  cmds["make_move"] = makeMove;
  cmds["undo_move"] = undoMove;
  cmds["next_move"] = nextMove;
  cmds["prev_move"] = prevMove;
  cmds["best_move"] = bestMove;

  std::string cmd;
  while (std::cin >> cmd)
  {
    try
    {
      if (cmds.count(cmd) == 0)
      {
        throw std::logic_error("unknown command");
      }
      cmds.at(cmd)(std::cin, std::cout, games);
    }
    catch (const std::exception& e)
    {
      std::cout << "<INVALID COMMAND: " << e.what() << ">\n";
      auto toignore = std::numeric_limits< std::streamsize >::max();
      std::cin.clear();
      if (std::cin.peek() == ' ' || std::cin.peek() == '\t')
      {
        std::cin.ignore(toignore, '\n');
      }
    }
  }
}
