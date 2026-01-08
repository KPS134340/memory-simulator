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

  // Stats for Replacement Policies
  size_t last_access_time = 0; // For LRU
  size_t access_count = 0;     // For LFU
};

struct CacheSet {
  std::vector<CacheBlock> blocks;
  // For FIFO: we keep track of which index to evict next
  int fifo_next_victim = 0;
};

class CacheLevel {
private:
  int level_id;
  size_t size;          // Total size in bytes
  size_t block_size;    // Block size in bytes
  size_t associativity; // Number of ways

  size_t num_sets;

  std::vector<CacheSet> sets;

  // Stats
  size_t hits = 0;
  size_t misses = 0;

  // Policy State
  CacheReplacementPolicy policy = CacheReplacementPolicy::FIFO;
  size_t timer = 0; // Global timer for this level to track LRU

public:
  CacheLevel(int id, size_t size, size_t block_size, size_t associativity);

  // Returns true if hit, false if miss.
  // If miss, it installs the block (evicting if necessary)
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

  // Perform a memory access through the hierarchy
  void access(size_t address, char type); // type: 'R' or 'W'

  void print_stats();
};

#endif
