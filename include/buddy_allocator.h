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
  BlockHeader *free_lists[MAX_LEVELS];
  char *memory_start;  
  size_t total_size;
  int min_order;  
  int max_order;  
  int get_order(size_t size);
  size_t get_size_from_order(int order);
  BlockHeader *get_block(int order);
public:
  BuddyAllocator();
  void init(char *memory, size_t size);
  void *malloc(size_t size);
  void free(void *ptr);
  void debug_lists();
};
#endif
