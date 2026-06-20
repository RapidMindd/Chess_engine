#ifndef ROBIN_HOOD_TABLE_HPP
#define ROBIN_HOOD_TABLE_HPP

#include <functional>
#include <cstddef>
#include <iterator>
#include <utility>
#include "hmac_hash.hpp"
#include "vector.hpp"

namespace tarasenko
{
  template< class Key, class Value, class Hash = HmacHash< Key >, class Equal = std::equal_to< Key > >
  struct RobinHoodTable
  {
    struct Iterator
    {
      Iterator() noexcept;

      std::pair< Key, Value >& operator*() const;
      std::pair< Key, Value >* operator->() const;

      Iterator& operator++();
      Iterator operator++(int);

      bool operator==(const Iterator& rhs) const noexcept;
      bool operator!=(const Iterator& rhs) const noexcept;

    private:
      RobinHoodTable* table_;
      size_t index_;

      explicit Iterator(RobinHoodTable* table, size_t index) noexcept;

      friend struct RobinHoodTable< Key, Value, Hash, Equal >;
    };

    struct ConstIterator
    {
      ConstIterator() noexcept;
      ConstIterator(const Iterator& rhs) noexcept;

      const std::pair< Key, Value >& operator*() const;
      const std::pair< Key, Value >* operator->() const;

      ConstIterator& operator++();
      ConstIterator operator++(int);

      bool operator==(const ConstIterator& rhs) const noexcept;
      bool operator!=(const ConstIterator& rhs) const noexcept;

    private:
      const RobinHoodTable* table_;
      size_t index_;

      explicit ConstIterator(const RobinHoodTable* table, size_t index) noexcept;

      friend struct RobinHoodTable< Key, Value, Hash, Equal >;
    };

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

    Iterator find(const Key& key) noexcept;
    ConstIterator find(const Key& key) const noexcept;

    Value& at(const Key& key);
    const Value& at(const Key& key) const;
    Value& operator[](const Key& key);

    Iterator begin() noexcept;
    Iterator end() noexcept;
    ConstIterator begin() const noexcept;
    ConstIterator end() const noexcept;
    ConstIterator cbegin() const noexcept;
    ConstIterator cend() const noexcept;

  private:
    struct Bucket
    {
      std::pair< Key, Value > data;
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
