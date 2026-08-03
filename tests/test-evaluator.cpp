#include <boost/test/unit_test.hpp>

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "evaluator.hpp"
#include "move.hpp"
#include "piece_square_tables.hpp"
#include "position.hpp"

using namespace chess;

namespace
{
  char swapCase(char c)
  {
    if (std::islower(static_cast< unsigned char >(c)))
    {
      return static_cast< char >(std::toupper(static_cast< unsigned char >(c)));
    }
    return static_cast< char >(std::tolower(static_cast< unsigned char >(c)));
  }

  /// turns a FEN upside down and swaps the colours, which must leave the
  /// evaluation unchanged apart from its sign
  std::string mirrorFen(const std::string& fen)
  {
    std::istringstream in(fen);
    std::string board;
    std::string side;
    std::string castling;
    std::string en_passant;
    in >> board >> side >> castling >> en_passant;

    std::vector< std::string > ranks;
    std::string current;
    for (size_t i = 0; i < board.size(); ++i)
    {
      if (board[i] == '/')
      {
        ranks.push_back(current);
        current.clear();
      }
      else
      {
        current += board[i];
      }
    }
    ranks.push_back(current);

    std::string mirrored;
    for (size_t i = ranks.size(); i-- > 0;)
    {
      for (size_t j = 0; j < ranks[i].size(); ++j)
      {
        const char c = ranks[i][j];
        mirrored += std::isalpha(static_cast< unsigned char >(c)) ? swapCase(c) : c;
      }
      if (i != 0)
      {
        mirrored += '/';
      }
    }

    std::string rights;
    for (size_t i = 0; i < castling.size(); ++i)
    {
      rights += std::isalpha(static_cast< unsigned char >(castling[i]))
        ? swapCase(castling[i]) : castling[i];
    }
    std::string ep = "-";
    if (en_passant != "-" && en_passant.size() >= 2)
    {
      ep = en_passant;
      ep[1] = static_cast< char >('1' + (7 - (en_passant[1] - '1')));
    }

    return mirrored + " " + (side == "w" ? "b" : "w") + " "
      + (rights.empty() ? "-" : rights) + " " + ep + " 0 1";
  }

  const char* const positions[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r2qr1k1/1bp2pp1/p1nbpn1p/3P2N1/1p5P/P1N1P3/BPQB1PP1/2KR3R b - - 0 15",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "4rrk1/pp1n1ppp/3bp3/3p4/3P4/1BP1PN2/PP3PPP/R4RK1 w - - 0 15",
    "6k1/5ppp/8/8/1P6/P1P5/5PPP/6K1 w - - 0 30",
    "2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1",
    "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1"
  };
}

BOOST_AUTO_TEST_CASE(evaluation_is_colour_symmetric)
{
  Evaluator evaluator;
  for (const char* fen : positions)
  {
    Position original = fen;
    const std::string mirrored_fen = mirrorFen(fen);
    Position mirrored = mirrored_fen.c_str();
    BOOST_TEST(evaluator.evaluate(original) == -evaluator.evaluate(mirrored));
  }
}

BOOST_AUTO_TEST_CASE(evaluation_prefers_more_material)
{
  Evaluator evaluator;
  Position equal = "4k3/8/8/8/8/8/8/4K3 w - - 0 1";
  Position extra_queen = "4k3/8/8/8/8/8/8/3QK3 w - - 0 1";
  Position extra_rook = "4k3/8/8/8/8/8/8/3RK3 w - - 0 1";
  BOOST_TEST(evaluator.evaluate(extra_queen) > evaluator.evaluate(extra_rook));
  BOOST_TEST(evaluator.evaluate(extra_rook) > evaluator.evaluate(equal));
}

BOOST_AUTO_TEST_CASE(incremental_scores_follow_the_pieces)
{
  Position pos;
  pos.setInitial();
  /// a symmetric start has to score zero on both game phases
  BOOST_TEST(pos.midgameScore() == 0);
  BOOST_TEST(pos.endgameScore() == 0);
  BOOST_TEST(pos.phase() == MAX_PHASE);

  UndoInfo undo;
  const int before = pos.midgameScore();
  pos.makeMove({E2, E4}, undo);
  BOOST_TEST(pos.midgameScore() != before);
  pos.undoMove({E2, E4}, undo);
  BOOST_TEST(pos.midgameScore() == before);
}

BOOST_AUTO_TEST_CASE(the_side_to_move_gets_a_tempo)
{
  Position white_to_move = "4k3/8/8/8/8/8/8/4K3 w - - 0 1";
  Position black_to_move = "4k3/8/8/8/8/8/8/4K3 b - - 0 1";
  Evaluator evaluator;
  BOOST_TEST(evaluator.evaluate(white_to_move) > evaluator.evaluate(black_to_move));
  BOOST_TEST(evaluator.relativeEval(white_to_move) == evaluator.relativeEval(black_to_move));
}
