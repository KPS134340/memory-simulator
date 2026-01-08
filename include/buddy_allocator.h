#ifndef BUDDY_ALLOCATOR_H
#define BUDDY_ALLOCATOR_H

#include "block.h"
#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>

class BuddyAllocator {
private:
  static const int MIN_BLOCK_SIZE = 32;
  static const int MAX_LEVELS = 32;

  // Array of free lists for each order (0 to 31)
  // free_lists[i] contains blocks of size 2^i
  BlockHeader *free_lists[MAX_LEVELS];

  char *memory_start; // Pointer to the start of memory managed by Buddy
  size_t total_size;
  int min_order; // Order corresponding to MIN_BLOCK_SIZE
  int max_order; // Order corresponding to total_size

  // Helpers
  int get_order(size_t size);
  size_t get_size_from_order(int order);

  // Core recursive alloc
  BlockHeader *get_block(int order);

public:
  BuddyAllocator();

  // Setup the memory for buddy system
  void init(char *memory, size_t size);

  // Core API
  void *malloc(size_t size);
  void free(void *ptr);

  // Debugging
  void debug_lists();
};

#endif
