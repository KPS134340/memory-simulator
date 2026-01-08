#ifndef VIRTUAL_MEMORY_H
#define VIRTUAL_MEMORY_H
#include <cstddef>
#include <deque>
#include <iostream>
#include <map>
#include <vector>
struct PageTableEntry {
  int frame_number = -1;
  bool valid = false;
  bool dirty = false;
  bool reference_bit = false;   
  size_t last_access_time = 0;  
};
enum class ReplacementPolicy { FIFO, LRU, CLOCK };
class VirtualMemoryManager {
private:
  size_t page_size;
  std::vector<PageTableEntry> page_table;
  std::vector<int> frame_table;
  size_t total_frames;
  ReplacementPolicy policy = ReplacementPolicy::FIFO;
  std::deque<int>
      fifo_queue;  
  std::deque<size_t> fifo_pages;
  size_t access_counter = 0;  
  size_t clock_hand = 0;  
  int disk_latency_ms = 0;
  size_t page_faults = 0;
  size_t page_hits = 0;
  int find_free_frame();
  int evict_page();
public:
  void init(size_t page_size, size_t virtual_size, size_t physical_memory_size);
  bool translate(size_t v_addr, size_t &p_addr);
  void print_stats();
  void set_policy(ReplacementPolicy p) { policy = p; }
  void set_disk_latency(int ms) { disk_latency_ms = ms; }
};
#endif
