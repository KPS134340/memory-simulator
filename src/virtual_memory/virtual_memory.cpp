#include "../../include/virtual_memory.h"
#include <chrono>  
#include <thread>

void VirtualMemoryManager::init(size_t page_size, size_t virtual_size,
                                size_t physical_memory_size) {
  this->page_size = page_size;
  size_t num_pages = virtual_size / page_size;
  this->total_frames = physical_memory_size / page_size;
  page_table.clear();
  page_table.resize(num_pages);
  frame_table.clear();
  frame_table.assign(total_frames, -1);  
  fifo_pages.clear();
  page_faults = 0;
  page_hits = 0;
  access_counter = 0;
  clock_hand = 0;
  std::cout << "VM Initialized: Page Size=" << page_size
            << ", Virtual Pages=" << num_pages
            << ", Physical Frames=" << total_frames << std::endl;
}

int VirtualMemoryManager::find_free_frame() {

  for (size_t i = 0; i < total_frames; ++i) {

    if (frame_table[i] == -1) {
      return i;
    }
  }

  return -1;  
}

int VirtualMemoryManager::evict_page() {
  size_t victim_page_idx = -1;

  if (policy == ReplacementPolicy::FIFO) {

    if (!fifo_pages.empty()) {
      victim_page_idx = fifo_pages.front();
      fifo_pages.pop_front();
    }

  } else if (policy == ReplacementPolicy::LRU) {
    size_t min_time = static_cast<size_t>(-1);
    int victim_frame = -1;

    for (size_t i = 0; i < total_frames; ++i) {
      int p_idx = frame_table[i];

      if (p_idx != -1) {

        if (page_table[p_idx].last_access_time < min_time) {
          min_time = page_table[p_idx].last_access_time;
          victim_page_idx = p_idx;
          victim_frame = i;
        }
      }
    }

  } else if (policy == ReplacementPolicy::CLOCK) {
    int loops = 0;

    while (loops < 2) {
      int p_idx = frame_table[clock_hand];

      if (p_idx != -1) {

        if (page_table[p_idx].reference_bit) {
          page_table[p_idx].reference_bit = false;
        } else {
          victim_page_idx = p_idx;
          break;
        }
      }

      clock_hand = (clock_hand + 1) % total_frames;
      if (clock_hand == 0)
        loops++;
    }

    if (victim_page_idx != static_cast<size_t>(-1)) {
      clock_hand = (clock_hand + 1) % total_frames;
    }
  }

  if (victim_page_idx != static_cast<size_t>(-1)) {
    int frame = page_table[victim_page_idx].frame_number;
    page_table[victim_page_idx].valid = false;
    page_table[victim_page_idx].frame_number = -1;
    frame_table[frame] = -1;
    std::cout << "  Evicting Page " << victim_page_idx << " from Frame "
              << frame << std::endl;
    return frame;
  }

  return -1;
}

bool VirtualMemoryManager::translate(size_t v_addr, size_t &p_addr) {
  if (page_size == 0)
    return false;
  access_counter++;
  size_t page_idx = v_addr / page_size;
  size_t offset = v_addr % page_size;

  if (page_idx >= page_table.size()) {
    std::cout << "SegFault: Virtual Address " << v_addr << " out of bounds."
              << std::endl;
    return false;
  }

  if (page_table[page_idx].valid) {
    page_hits++;
    page_table[page_idx].last_access_time = access_counter;
    page_table[page_idx].reference_bit = true;
    int frame = page_table[page_idx].frame_number;
    p_addr = (frame * page_size) + offset;
    return true;
  }

  page_faults++;
  std::cout << "  Page Fault at address " << v_addr << " (Page " << page_idx
            << ")" << std::endl;

  if (disk_latency_ms > 0) {
    std::cout << "  (Simulating Disk I/O: " << disk_latency_ms << "ms)..."
              << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(disk_latency_ms));
  }

  int frame = find_free_frame();

  if (frame == -1) {
    frame = evict_page();
  }

  if (frame == -1) {
    std::cout << "Critical Error: Could not resolve page fault (Memory full "
                 "and eviction failed?)"
              << std::endl;
    return false;
  }

  page_table[page_idx].valid = true;
  page_table[page_idx].frame_number = frame;
  page_table[page_idx].last_access_time = access_counter;
  page_table[page_idx].reference_bit = true;
  frame_table[frame] = page_idx;

  if (policy == ReplacementPolicy::FIFO) {
    fifo_pages.push_back(page_idx);
  }

  p_addr = (frame * page_size) + offset;
  return true;
}

void VirtualMemoryManager::print_stats() {
  std::cout << "\n=== Virtual Memory Statistics ===" << std::endl;
  std::cout << "  Page Faults: " << page_faults << std::endl;
  std::cout << "  Page Hits:   " << page_hits << std::endl;
  double rate = (page_hits + page_faults > 0)
                    ? (double)page_hits / (page_hits + page_faults) * 100.0
                    : 0.0;
  std::cout << "  Hit Rate:    " << rate << "%" << std::endl;

  if (disk_latency_ms > 0) {
    std::cout << "  Disk Latency per Fault: " << disk_latency_ms << "ms"
              << std::endl;
  }

  std::cout << "=================================\n" << std::endl;
}