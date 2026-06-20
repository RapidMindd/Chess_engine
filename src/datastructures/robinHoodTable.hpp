#ifndef ROBIN_HOOD_TABLE_HPP
#define ROBIN_HOOD_TABLE_HPP

#include <functional>
#include <cstddef>
#include <utility>
#include "hmac_hash.hpp"
#include "vector.hpp"

namespace tarasenko
{
  template< class Key, class Value, class Hash = HmacHash< Key >, class Equal = std::equal_to< Key > >
  struct RobinHoodTable
  {
    RobinHoodTable();
    explicit RobinHoodTable(size_t bucket_count);
    RobinHoodTable(size_t bucket_count, const Hash& hash, const Equal& equal = Equal());

    bool empty() const noexcept;
    size_t size() const noexcept;
    size_t bucket_count() const noexcept;
    double load_factor() const noexcept;
    double max_load_factor() const noexcept;
    void max_load_factor(double ml);

    void clear();
    void reserve(size_t new_capacity);
    void rehash(size_t new_bucket_count);

    bool insert(const std::pair< const Key, Value >& value);
    size_t erase(const Key& key);
    size_t count(const Key& key) const;

    Value* find(const Key& key) noexcept;
    const Value* find(const Key& key) const noexcept;

    Value& at(const Key& key);
    const Value& at(const Key& key) const;
    Value& operator[](const Key& key);

  private:
    struct Bucket
    {
      Key key;
      Value value;
      size_t distance;
      bool occupied;
    };

    Vector< Bucket > buckets_;
    size_t size_;
    Hash hash_;
    Equal equal_;
    double max_load_factor_;

  };
}

#endif
