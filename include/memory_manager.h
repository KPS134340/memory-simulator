#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H
#include "block.h"
#include <cstddef>  
#include <iostream>
#include <vector>
#include "buddy_allocator.h"
#include "cache.h"
#include "virtual_memory.h"


enum class AllocationStrategy { FIRST_FIT, BEST_FIT, WORST_FIT, BUDDY };

class MemoryManager {

private:
  std::vector<char> memory;  
  BlockHeader *head;         
  size_t total_size;
  int next_alloc_id;  
  size_t align(size_t n);  
  int get_next_available_id();
  size_t total_alloc_requests = 0;
  size_t successful_allocs = 0;
  AllocationStrategy current_strategy = AllocationStrategy::FIRST_FIT;
  CacheHierarchy cache_system;
  BuddyAllocator buddy_system;
  VirtualMemoryManager vm_system;
  bool use_virtual_memory = false;
  BlockHeader *find_first_fit(size_t size);
  BlockHeader *find_best_fit(size_t size);
  BlockHeader *find_worst_fit(size_t size);

public:
  void init(size_t size);
  void dump_memory();
  void print_stats();
  void *malloc(size_t size);
  void free(void *ptr);
  void free_by_id(int id);
  void free_smart(int value);
  void enable_vm(size_t page_size);
  void access(size_t address, char rw);
  void *get_ptr_from_offset(size_t offset);
  size_t get_offset_from_ptr(void *ptr);
  void set_strategy(AllocationStrategy strategy);
  void set_cache_policy(CacheReplacementPolicy policy);
  void set_vm_policy(ReplacementPolicy policy);
  void set_vm_latency(int ms);

  BlockHeader *get_head() { return head; }
};

#endif