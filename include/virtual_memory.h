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
  bool reference_bit = false;  // For Clock Algorithm
  size_t last_access_time = 0; // For LRU
};

enum class ReplacementPolicy { FIFO, LRU, CLOCK };

class VirtualMemoryManager {
private:
  size_t page_size;
  std::vector<PageTableEntry> page_table;

  // Frame Table acts as the "Physical Memory" manager for pages.
  // frame_table[frame_id] = page_number (Reverse Mapping)
  // If frame_table[frame_id] == -1, frame is free.
  std::vector<int> frame_table;
  size_t total_frames;

  ReplacementPolicy policy = ReplacementPolicy::FIFO;

  // FIFO Queue
  std::deque<int>
      fifo_queue; // Stores Frame Numbers (or Page Numbers?) -> Frame Numbers is
                  // easier for "victim frame" selection, but for FIFO/LRU we
                  // usually tracking specific loaded Pages.
  // Let's track loaded pages for replacement logic.
  std::deque<size_t> fifo_pages;

  size_t access_counter = 0; // For LRU

  // Clock Algorithm State
  size_t clock_hand = 0; // Index into frame_table (circular buffer)

  // Disk Latency Simulation
  int disk_latency_ms = 0;

  // Stats
  size_t page_faults = 0;
  size_t page_hits = 0;

  int find_free_frame();
  int evict_page();

public:
  void init(size_t page_size, size_t virtual_size, size_t physical_memory_size);

  // Returns true if hit, false if fault (and handles fault internally or
  // returns false to let caller handle?) Plan: handle fault internally to
  // simulate transparent OS behavior But we might want to return "was_fault"
  // for reporting.
  bool translate(size_t v_addr, size_t &p_addr);

  void print_stats();
  void set_policy(ReplacementPolicy p) { policy = p; }
  void set_disk_latency(int ms) { disk_latency_ms = ms; }
};

#endif
