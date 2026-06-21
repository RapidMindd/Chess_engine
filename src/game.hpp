#ifndef GAME_HPP
#define GAME_HPP

#include <cstddef>
#include <iosfwd>

#include "datastructures/vector.hpp"
#include "move.hpp"
#include "position.hpp"

namespace chess
{
  struct GameMove
  {
    Move move_;
    UndoInfo undo_;
  };

  struct Game
  {
    Game();
    explicit Game(const Position& start);

    const Position& getPosition() const noexcept;
    const Position& getStartPosition() const noexcept;
    size_t getCurrentMove() const noexcept;
    size_t getMovesCount() const noexcept;

    void makeMove(const Move& move);
    void undoMove();
    void prevMove();
    void nextMove();

    void print(std::ostream& out) const;

  private:
    Position startPosition_;
    Position currentPosition_;
    tarasenko::Vector< GameMove > moves_;
    size_t currentMove_;
  };
}

#endif
