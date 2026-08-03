#include <boost/test/unit_test.hpp>

#include <cstdlib>

#include "engine.hpp"
#include "move.hpp"
#include "move_generator.hpp"
#include "position.hpp"
#include "transposition_table.hpp"
#include "zobrist.hpp"

using namespace chess;

namespace
{
  SearchResult searchToDepth(const char* fen, int depth, int threads = 1)
  {
    Position pos = fen;
    Engine engine;
    engine.setHashSizeMb(8);
    engine.setThreads(threads);
    SearchLimits limits;
    limits.depth = depth;
    return engine.search(pos, limits);
  }

  int mateDistance(int score)
  {
    return (VALUE_MATE - std::abs(score) + 1) / 2;
  }

  Move findMove(const Position& pos, const char* uci)
  {
    MoveArray moves;
    MoveGenerator::generateLegalMoves(pos, moves);
    for (int i = 0; i < moves.size(); ++i)
    {
      if (moveToUci(moves.get(i)) == uci)
      {
        return moves.get(i);
      }
    }
    return null_move;
  }
}

BOOST_AUTO_TEST_CASE(search_finds_mate_in_one)
{
  const SearchResult result = searchToDepth("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", 6);
  BOOST_TEST(moveToUci(result.best) == "a1a8");
  BOOST_TEST(result.centipawns >= VALUE_MATE_IN_MAX_PLY);
  BOOST_TEST(mateDistance(result.centipawns) == 1);
}

BOOST_AUTO_TEST_CASE(search_finds_mate_in_two)
{
  const SearchResult result =
    searchToDepth("2bqkbn1/2pppp2/np2N3/r3P1p1/p2N2B1/5Q2/PPPPKPP1/RNB2r2 w - - 0 1", 8);
  BOOST_TEST(moveToUci(result.best) == "f3f7");
  BOOST_TEST(mateDistance(result.centipawns) == 2);
}

/// the losing side must report the mate as negative and still return a move
BOOST_AUTO_TEST_CASE(search_reports_being_mated)
{
  const SearchResult result = searchToDepth("7k/5KQ1/8/8/8/8/8/8 b - - 0 1", 4);
  BOOST_TEST(result.best == null_move);
  BOOST_TEST(result.centipawns == -VALUE_MATE);
}

BOOST_AUTO_TEST_CASE(search_returns_a_principal_variation)
{
  const SearchResult result =
    searchToDepth("r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5Q2/PPPP1PPP/RNB1K1NR w KQkq - 4 4", 8);
  BOOST_TEST(result.pvLength > 0);
  BOOST_TEST(result.pv[0] == result.best);
}

BOOST_AUTO_TEST_CASE(search_avoids_stalemating_a_won_position)
{
  /// white is winning easily and must not walk into a stalemate
  const SearchResult result = searchToDepth("7k/8/6QK/8/8/8/8/8 w - - 0 1", 6);
  Position pos = "7k/8/6QK/8/8/8/8/8 w - - 0 1";
  UndoInfo undo;
  pos.makeMove(result.best, undo);
  BOOST_TEST(!MoveGenerator::isStaleMate(pos));
}

BOOST_AUTO_TEST_CASE(search_with_several_threads_agrees)
{
  const SearchResult single = searchToDepth("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", 6, 1);
  const SearchResult parallel = searchToDepth("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", 6, 4);
  BOOST_TEST(moveToUci(single.best) == moveToUci(parallel.best));
}

BOOST_AUTO_TEST_CASE(search_respects_a_time_limit)
{
  Position pos;
  pos.setInitial();
  Engine engine;
  SearchLimits limits;
  limits.timeMs = 50;
  const SearchResult result = engine.search(pos, limits);
  BOOST_TEST(result.best != null_move);
  BOOST_TEST(result.seconds < 1.0);
}

BOOST_AUTO_TEST_CASE(search_respects_a_node_limit)
{
  Position pos;
  pos.setInitial();
  Engine engine;
  SearchLimits limits;
  limits.maxNodes = 20000;
  const SearchResult result = engine.search(pos, limits);
  BOOST_TEST(result.best != null_move);
  BOOST_TEST(result.nodes < 200000ull);
}

BOOST_AUTO_TEST_CASE(see_values_simple_exchanges)
{
  /// a free pawn
  Position pos = "4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1";
  BOOST_TEST(Engine::seeCapture(pos, findMove(pos, "e4d5")) == 100);

  /// pawn takes a pawn defended by another pawn: the exchange is a wash
  Position defended = "4k3/8/2p5/3p4/4P3/8/8/4K3 w - - 0 1";
  BOOST_TEST(Engine::seeCapture(defended, findMove(defended, "e4d5")) == 0);

  /// queen takes a pawn defended by a pawn: a pawn won for a queen lost
  Position losing = "4k3/8/2p5/3p4/8/8/8/3QK3 w - - 0 1";
  BOOST_TEST(Engine::seeCapture(losing, findMove(losing, "d1d5")) == weights[PAWN] - weights[QUEEN]);
  BOOST_TEST(!Engine::seeGreaterOrEqual(losing, findMove(losing, "d1d5"), 0));

  /// the same capture is fine once the defender is pinned out of the picture
  Position supported = "4k3/8/2p5/3p4/8/3B4/8/3QK3 w - - 0 1";
  BOOST_TEST(Engine::seeGreaterOrEqual(supported, findMove(supported, "d3d5"), 0));
}

BOOST_AUTO_TEST_CASE(transposition_table_round_trip)
{
  TranspositionTable table(1);
  const Move best = {E2, E4};
  table.store(0x1234567890ABCDEFull, 7, 42, 11, EXACT, best);

  TTEntry entry;
  BOOST_TEST(table.probe(0x1234567890ABCDEFull, true, entry));
  BOOST_TEST(entry.depth_ == 7);
  BOOST_TEST(entry.eval_ == 42);
  BOOST_TEST(entry.staticEval_ == 11);
  BOOST_TEST(entry.type_ == EXACT);
  BOOST_TEST(entry.bestMove_ == best);

  BOOST_TEST(!table.probe(0x1234567890ABCDEEull, true, entry));

  table.clear();
  BOOST_TEST(!table.probe(0x1234567890ABCDEFull, true, entry));
}

BOOST_AUTO_TEST_CASE(transposition_table_keeps_special_moves)
{
  TranspositionTable table(1);
  const Move castling = {E1, G1, EMPTY, false, true};
  const Move promotion = {A7, A8, WHITE_QUEEN};
  const Move en_passant = {D5, E6, EMPTY, true};

  table.store(1ull, 1, 0, 0, EXACT, castling);
  table.store(2ull, 1, 0, 0, EXACT, promotion);
  table.store(3ull, 1, 0, 0, EXACT, en_passant);

  TTEntry entry;
  BOOST_TEST(table.probe(1ull, true, entry));
  BOOST_TEST(entry.bestMove_ == castling);
  BOOST_TEST(table.probe(2ull, true, entry));
  BOOST_TEST(entry.bestMove_ == promotion);
  BOOST_TEST(table.probe(3ull, true, entry));
  BOOST_TEST(entry.bestMove_ == en_passant);
}

BOOST_AUTO_TEST_CASE(position_detects_repetition)
{
  Position pos = "4k3/8/8/8/8/8/8/R3K3 w - - 0 1";
  UndoInfo undo[4];
  /// shuffling back and forth reaches the starting position again after 4 plies
  const Move moves[4] = {{A1, A2}, {E8, E7}, {A2, A1}, {E7, E8}};

  for (int i = 0; i < 4; ++i)
  {
    BOOST_TEST(!pos.isRepetition());
    pos.makeMove(moves[i], undo[i]);
  }
  BOOST_TEST(pos.isRepetition());

  for (int i = 3; i >= 0; --i)
  {
    pos.undoMove(moves[i], undo[i]);
  }
  BOOST_TEST(!pos.isRepetition());

  /// a null move cuts the window, nothing before it can repeat any more
  UndoInfo null_undo;
  for (int i = 0; i < 4; ++i)
  {
    pos.makeMove(moves[i], undo[i]);
  }
  BOOST_TEST(pos.isRepetition());
  pos.makeNullMove(null_undo);
  BOOST_TEST(!pos.isRepetition());
  pos.undoNullMove(null_undo);
  BOOST_TEST(pos.isRepetition());
}

BOOST_AUTO_TEST_CASE(position_detects_the_fifty_move_rule)
{
  Position pos = "4k3/8/8/8/8/8/8/R3K3 w - - 99 60";
  BOOST_TEST(!pos.isFiftyMoveDraw());
  UndoInfo undo;
  pos.makeMove({A1, A2}, undo);
  BOOST_TEST(pos.isFiftyMoveDraw());

  /// a pawn move or a capture resets the counter
  Position with_pawn = "4k3/8/8/8/8/8/P7/4K3 w - - 99 60";
  with_pawn.makeMove({A2, A3}, undo);
  BOOST_TEST(!with_pawn.isFiftyMoveDraw());
}

BOOST_AUTO_TEST_CASE(position_detects_insufficient_material)
{
  BOOST_TEST(Position("4k3/8/8/8/8/8/8/4K3 w - - 0 1").isInsufficientMaterial());
  BOOST_TEST(Position("4k3/8/8/8/8/8/8/3BK3 w - - 0 1").isInsufficientMaterial());
  BOOST_TEST(Position("4k3/8/8/8/8/8/8/3NK3 w - - 0 1").isInsufficientMaterial());
  BOOST_TEST(!Position("4k3/8/8/8/8/8/P7/4K3 w - - 0 1").isInsufficientMaterial());
  BOOST_TEST(!Position("4k3/8/8/8/8/8/8/3RK3 w - - 0 1").isInsufficientMaterial());
}

BOOST_AUTO_TEST_CASE(incremental_hash_matches_a_full_recomputation)
{
  Position pos = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
  MoveArray moves;
  MoveGenerator::generateLegalMoves(pos, moves);
  UndoInfo undo;
  for (int i = 0; i < moves.size(); ++i)
  {
    const uint64_t before = pos.hash();
    const uint64_t predicted = incrementZobristHash(before, pos, moves.get(i));
    pos.makeMove(moves.get(i), undo);
    BOOST_TEST(pos.hash() == zobristHash(pos));
    BOOST_TEST(pos.hash() == predicted);
    BOOST_TEST(pos.pawnHash() == zobristPawnHash(pos));
    pos.undoMove(moves.get(i), undo);
    BOOST_TEST(pos.hash() == before);
  }
}

BOOST_AUTO_TEST_CASE(fen_round_trip)
{
  const char* fens[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"
  };
  for (const char* fen : fens)
  {
    Position pos = fen;
    Position round_trip = pos.toFen().c_str();
    BOOST_TEST(pos == round_trip);
    BOOST_TEST(pos.hash() == round_trip.hash());
  }
}
