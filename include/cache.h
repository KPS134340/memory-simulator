#ifndef CACHE_H
#define CACHE_H
#include <cstddef>
#include <deque>
#include <iostream>
#include <vector>


enum class CacheReplacementPolicy { FIFO, LRU, LFU };

struct CacheBlock {
  bool valid = false;
  bool dirty = false;
  size_t tag = 0;
  size_t last_access_time = 0;
  size_t access_count = 0;
};

struct CacheSet {
  std::vector<CacheBlock> blocks;
  int fifo_next_victim = 0;
};

class CacheLevel {

private:
  int level_id;
  size_t size;
  size_t block_size;
  size_t associativity;
  size_t num_sets;
  std::vector<CacheSet> sets;
  size_t hits = 0;
  size_t misses = 0;
  CacheReplacementPolicy policy = CacheReplacementPolicy::FIFO;
  size_t timer = 0;

public:
  CacheLevel(int id, size_t size, size_t block_size, size_t associativity);
  bool access(size_t address, bool is_write);
  void set_policy(CacheReplacementPolicy p);
  void reset_stats();
  size_t get_hits() const { return hits; }
  size_t get_misses() const { return misses; }
  double get_hit_rate() const;
  void print_stats() const;
};

class CacheHierarchy {

private:
  CacheLevel *l1;
  CacheLevel *l2;
  CacheLevel *l3;

public:
  CacheHierarchy();
  ~CacheHierarchy();
  void init(size_t l1_size, size_t l1_block_size, size_t l1_assoc,
            size_t l2_size, size_t l2_block_size, size_t l2_assoc,
            size_t l3_size, size_t l3_block_size, size_t l3_assoc);
  void set_policy(CacheReplacementPolicy p);
  void access(size_t address, char type);
  void print_stats();
};

#endif