#include <boost/test/unit_test.hpp>

#include "perft.hpp"
#include "position.hpp"

using namespace chess;

namespace
{
  uint64_t nodes(const char* fen, int depth)
  {
    Position pos = fen;
    return perft(pos, depth);
  }
}

BOOST_AUTO_TEST_CASE(perft_initial_position)
{
  const char* fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  BOOST_TEST(nodes(fen, 1) == 20ull);
  BOOST_TEST(nodes(fen, 2) == 400ull);
  BOOST_TEST(nodes(fen, 3) == 8902ull);
  BOOST_TEST(nodes(fen, 4) == 197281ull);
  BOOST_TEST(nodes(fen, 5) == 4865609ull);
}

/// "kiwipete": castling, pins and en passant all at once
BOOST_AUTO_TEST_CASE(perft_kiwipete)
{
  const char* fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
  BOOST_TEST(nodes(fen, 1) == 48ull);
  BOOST_TEST(nodes(fen, 2) == 2039ull);
  BOOST_TEST(nodes(fen, 3) == 97862ull);
  BOOST_TEST(nodes(fen, 4) == 4085603ull);
}

/// a pawn endgame full of en passant pins along the fourth rank
BOOST_AUTO_TEST_CASE(perft_en_passant_pins)
{
  const char* fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
  BOOST_TEST(nodes(fen, 1) == 14ull);
  BOOST_TEST(nodes(fen, 2) == 191ull);
  BOOST_TEST(nodes(fen, 3) == 2812ull);
  BOOST_TEST(nodes(fen, 4) == 43238ull);
  BOOST_TEST(nodes(fen, 5) == 674624ull);
}

BOOST_AUTO_TEST_CASE(perft_promotions)
{
  const char* fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
  BOOST_TEST(nodes(fen, 1) == 6ull);
  BOOST_TEST(nodes(fen, 2) == 264ull);
  BOOST_TEST(nodes(fen, 3) == 9467ull);
  BOOST_TEST(nodes(fen, 4) == 422333ull);
}

BOOST_AUTO_TEST_CASE(perft_mirrored_position)
{
  const char* fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
  BOOST_TEST(nodes(fen, 1) == 44ull);
  BOOST_TEST(nodes(fen, 2) == 1486ull);
  BOOST_TEST(nodes(fen, 3) == 62379ull);
  BOOST_TEST(nodes(fen, 4) == 2103487ull);
}

BOOST_AUTO_TEST_CASE(perft_middlegame)
{
  const char* fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";
  BOOST_TEST(nodes(fen, 1) == 46ull);
  BOOST_TEST(nodes(fen, 2) == 2079ull);
  BOOST_TEST(nodes(fen, 3) == 89890ull);
  BOOST_TEST(nodes(fen, 4) == 3894594ull);
}
