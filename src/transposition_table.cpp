#include "transposition_table.hpp"

#include <algorithm>

#include "piece.hpp"

namespace chess
{
  namespace
  {
    constexpr int GENERATION_CYCLE = 64;

    /// A move squeezed into 16 bits: from, to, promoted piece and a flag telling
    /// promotion / en passant / castling apart.
    uint16_t packMove(const Move& move)
    {
      unsigned special = 0;
      unsigned promotion = 0;
      if (move.promotionPiece_ != EMPTY)
      {
        special = 1;
        promotion = static_cast< unsigned >(typeOf(move.promotionPiece_) - KNIGHT) & 3u;
      }
      else if (move.isEnPassant_)
      {
        special = 2;
      }
      else if (move.isCastling_)
      {
        special = 3;
      }
      return static_cast< uint16_t >(move.from_ | (move.to_ << 6) | (promotion << 12) | (special << 14));
    }

    Move unpackMove(uint16_t packed, bool white)
    {
      Move move = {};
      move.from_ = static_cast< Square >(packed & 63);
      move.to_ = static_cast< Square >((packed >> 6) & 63);
      const unsigned special = (packed >> 14) & 3;
      if (special == 1)
      {
        move.promotionPiece_ = makePiece(KNIGHT + static_cast< int >((packed >> 12) & 3), white);
      }
      else if (special == 2)
      {
        move.isEnPassant_ = true;
      }
      else if (special == 3)
      {
        move.isCastling_ = true;
      }
      return move;
    }

    uint64_t packData(int depth, int eval, int static_eval, TTEntryType type, uint16_t move, uint8_t generation)
    {
      const uint64_t eval_bits = static_cast< uint16_t >(static_cast< int16_t >(eval));
      const uint64_t static_bits = static_cast< uint16_t >(static_cast< int16_t >(static_eval));
      const uint64_t depth_bits = static_cast< uint8_t >(std::max(0, std::min(255, depth + 1)));
      return eval_bits
        | (static_bits << 16)
        | (static_cast< uint64_t >(move) << 32)
        | (depth_bits << 48)
        | (static_cast< uint64_t >(type & 3) << 56)
        | (static_cast< uint64_t >(generation & 63) << 58);
    }

    int16_t dataEval(uint64_t data) { return static_cast< int16_t >(data & 0xFFFF); }
    int16_t dataStaticEval(uint64_t data) { return static_cast< int16_t >((data >> 16) & 0xFFFF); }
    uint16_t dataMove(uint64_t data) { return static_cast< uint16_t >((data >> 32) & 0xFFFF); }
    int dataDepth(uint64_t data) { return static_cast< int >((data >> 48) & 0xFF) - 1; }
    TTEntryType dataType(uint64_t data) { return static_cast< TTEntryType >((data >> 56) & 3); }
    int dataGeneration(uint64_t data) { return static_cast< int >((data >> 58) & 63); }

    uint64_t roundDownToPowerOfTwo(uint64_t value)
    {
      uint64_t result = 1;
      while (result * 2 <= value)
      {
        result *= 2;
      }
      return result;
    }
  }

  TranspositionTable::TranspositionTable():
    TranspositionTable(64)
  {}

  TranspositionTable::TranspositionTable(uint64_t megabytes)
  {
    resize(megabytes);
  }

  void TranspositionTable::resize(uint64_t megabytes)
  {
    if (megabytes < 1)
    {
      megabytes = 1;
    }
    const uint64_t bytes_per_bucket = sizeof(Slot) * BUCKET_SIZE;
    const uint64_t buckets = roundDownToPowerOfTwo(std::max< uint64_t >(1, megabytes * 1024 * 1024 / bytes_per_bucket));

    bucketCount_ = buckets;
    bucketMask_ = buckets - 1;
    slots_.reset(new Slot[buckets * BUCKET_SIZE]);
    clear();
  }

  void TranspositionTable::resizeEntries(uint64_t entries)
  {
    const uint64_t buckets = roundDownToPowerOfTwo(std::max< uint64_t >(BUCKET_SIZE, entries) / BUCKET_SIZE);
    bucketCount_ = buckets;
    bucketMask_ = buckets - 1;
    slots_.reset(new Slot[buckets * BUCKET_SIZE]);
    clear();
  }

  void TranspositionTable::clear()
  {
    const uint64_t total = bucketCount_ * BUCKET_SIZE;
    for (uint64_t i = 0; i < total; ++i)
    {
      slots_[i].check.store(0, std::memory_order_relaxed);
      slots_[i].data.store(0, std::memory_order_relaxed);
    }
    generation_ = 0;
  }

  void TranspositionTable::newSearch()
  {
    generation_ = static_cast< uint8_t >((generation_ + 1) % GENERATION_CYCLE);
  }

  size_t TranspositionTable::bucketIndex(uint64_t key) const noexcept
  {
    return static_cast< size_t >(key & bucketMask_) * BUCKET_SIZE;
  }

  void TranspositionTable::prefetch(uint64_t key) const
  {
    __builtin_prefetch(&slots_[bucketIndex(key)]);
  }

  bool TranspositionTable::probe(uint64_t key, bool white_to_move, TTEntry& entry) const
  {
    const size_t base = bucketIndex(key);
    for (int i = 0; i < BUCKET_SIZE; ++i)
    {
      const uint64_t data = slots_[base + i].data.load(std::memory_order_relaxed);
      const uint64_t check = slots_[base + i].check.load(std::memory_order_relaxed);
      if (data == 0 || (check ^ data) != key)
      {
        continue;
      }
      entry.key_ = key;
      entry.eval_ = dataEval(data);
      entry.staticEval_ = dataStaticEval(data);
      entry.depth_ = dataDepth(data);
      entry.type_ = dataType(data);
      entry.used_ = true;
      entry.bestMove_ = unpackMove(dataMove(data), white_to_move);
      return true;
    }
    entry.used_ = false;
    return false;
  }

  void TranspositionTable::store(uint64_t key, int depth, int eval, int static_eval,
    TTEntryType type, const Move& best)
  {
    const size_t base = bucketIndex(key);
    size_t victim = base;
    int worst = 1 << 30;

    for (int i = 0; i < BUCKET_SIZE; ++i)
    {
      const uint64_t data = slots_[base + i].data.load(std::memory_order_relaxed);
      const uint64_t check = slots_[base + i].check.load(std::memory_order_relaxed);
      if (data == 0)
      {
        victim = base + i;
        worst = -(1 << 30);
        break;
      }
      if ((check ^ data) == key)
      {
        /// an entry for this very position: keep the deeper of the two searches
        if (dataDepth(data) > depth + 3 && dataType(data) == EXACT && type != EXACT)
        {
          return;
        }
        victim = base + i;
        worst = -(1 << 30);
        break;
      }
      int age = generation_ - dataGeneration(data);
      if (age < 0)
      {
        age += GENERATION_CYCLE;
      }
      const int value = dataDepth(data) - 4 * age;
      if (value < worst)
      {
        worst = value;
        victim = base + i;
      }
    }

    uint16_t packed_move = packMove(best);
    if (best == null_move)
    {
      /// do not throw away a usable move when this node had none
      const uint64_t data = slots_[victim].data.load(std::memory_order_relaxed);
      const uint64_t check = slots_[victim].check.load(std::memory_order_relaxed);
      if (data != 0 && (check ^ data) == key)
      {
        packed_move = dataMove(data);
      }
    }

    const uint64_t data = packData(depth, eval, static_eval, type, packed_move, generation_);
    slots_[victim].check.store(key ^ data, std::memory_order_relaxed);
    slots_[victim].data.store(data, std::memory_order_relaxed);
  }

  int TranspositionTable::hashfull() const
  {
    int used = 0;
    const int sample = 1000;
    for (int i = 0; i < sample; ++i)
    {
      const uint64_t data = slots_[i].data.load(std::memory_order_relaxed);
      if (data != 0 && dataGeneration(data) == generation_)
      {
        ++used;
      }
    }
    return used;
  }

  uint64_t TranspositionTable::entryCount() const
  {
    return bucketCount_ * BUCKET_SIZE;
  }
}
