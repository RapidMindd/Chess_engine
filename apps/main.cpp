#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include "datastructures/robinHoodTable.hpp"
#include "game.hpp"

using games_t = tarasenko::RobinHoodTable< std::string, chess::Game >;

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

void newGame(std::istream& in, std::ostream&, games_t& games)
{
  std::string name = readName(in);
  if (games.count(name))
  {
    throw std::logic_error("board with this name already exist");
  }
  games.insert({name, chess::Game()});
}

void printGame(std::istream& in, std::ostream& out, games_t& games)
{
  std::string name = readName(in);
  if (games.count(name) == 0)
  {
    throw std::logic_error("no board with this name");
  }
  games.at(name).print(out);
}

int main()
{
  games_t games;

  using cmd_t = void(*)(std::istream&, std::ostream&, games_t&);
  tarasenko::RobinHoodTable< std::string, cmd_t > cmds;
  cmds["new"] = newGame;
  cmds["print"] = printGame;

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
      std::cin.ignore(toignore, '\n');
    }
  }
}
