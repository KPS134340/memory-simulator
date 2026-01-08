#include "../../include/memory_manager.h"
#include <algorithm>
#include <alloca.h>
#include <cstddef>
#include <iostream>
#include <ostream>

// Helper: Round up 'n' to the nearest multiple of 8
// Example: align(1) -> 8, align(7) -> 8, align(9) -> 16
size_t MemoryManager::align(size_t n) { return (n + 7) & ~7; }

// Helper: Get the pointer relative to the start of memory
void *MemoryManager::get_ptr_from_offset(size_t offset) {
  // Safety check: Don't allow offsets beyond the memory size
  if (offset >= total_size)
    return nullptr;

  // The "base" is the start of the vector
  char *base = memory.data();

  // Return base + offset
  return reinterpret_cast<void *>(base + offset);
}

// Helper: Get the offset of a pointer relative to the start
size_t MemoryManager::get_offset_from_ptr(void *ptr) {
  char *base = memory.data();
  char *current = static_cast<char *>(ptr);
  return static_cast<size_t>(current - base);
}

void MemoryManager::set_strategy(AllocationStrategy strategy) {
  if (strategy == AllocationStrategy::BUDDY &&
      current_strategy != AllocationStrategy::BUDDY) {
    std::cout << "Warning: Switching to Buddy System at runtime. Initializing "
                 "Buddy Allocator..."
              << std::endl;
    // We must initialize the buddy system with existing memory
    // WARNING: This assumes ANY existing allocations in First/Best/Worst fit
    // are invalid or ignored by Buddy which is true (Buddy manages its own
    // lists). But physical memory is shared. Ideally we should clear memory or
    // fail if not empty. For simulation, we'll just init.
    buddy_system.init(memory.data(), total_size);
  }
  current_strategy = strategy;
}

void MemoryManager::set_cache_policy(CacheReplacementPolicy policy) {
  cache_system.set_policy(policy);
}

void MemoryManager::set_vm_policy(ReplacementPolicy policy) {
  vm_system.set_policy(policy);
}

void MemoryManager::set_vm_latency(int ms) { vm_system.set_disk_latency(ms); }

BlockHeader *MemoryManager::find_first_fit(size_t size) {
  BlockHeader *current = head;
  while (current != nullptr) {
    if (current->is_free && current->size >= size) {
      return current;
    }
    current = current->next;
  }
  return nullptr;
}

BlockHeader *MemoryManager::find_best_fit(size_t size) {
  BlockHeader *current = head;
  BlockHeader *best_block = nullptr;
  size_t smallest_diff = static_cast<size_t>(-1); // Max possible size_t

  while (current != nullptr) {
    if (current->is_free && current->size >= size) {
      size_t diff = current->size - size;
      if (diff < smallest_diff) {
        smallest_diff = diff;
        best_block = current;

        // Optimization: If exact match, stop searching
        if (diff == 0)
          return current;
      }
    }
    current = current->next;
  }
  return best_block;
}

BlockHeader *MemoryManager::find_worst_fit(size_t size) {
  BlockHeader *current = head;
  BlockHeader *worst_block = nullptr;
  size_t largest_size = 0;

  while (current != nullptr) {
    if (current->is_free && current->size >= size) {
      if (current->size > largest_size) {
        largest_size = current->size;
        worst_block = current;
      }
    }
    current = current->next;
  }
  return worst_block;
}

void MemoryManager::print_stats() {
  BlockHeader *current = head;

  // Metrics Variables
  size_t total_free_mem = 0;
  size_t total_used_mem = 0;      // Includes padding
  size_t total_internal_frag = 0; // Sum of all padding
  size_t largest_free_block = 0;

  while (current != nullptr) {
    if (current->is_free) {
      total_free_mem += current->size;
      if (current->size > largest_free_block) {
        largest_free_block = current->size;
      }
    } else {
      total_used_mem += current->size;
      total_internal_frag += current->padding;
    }
    current = current->next;
  }

  std::cout << "\n=== Memory System Statistics ===" << std::endl;

  // 1. Memory Utilization
  // Formula: (Used Memory / Total Memory) * 100
  // Note: Some definitions subtract internal fragmentation from 'Used',
  // but usually 'Used' means "unavailable to others". We will use that.
  double utilization =
      (total_size > 0)
          ? (static_cast<double>(total_used_mem) / total_size) * 100.0
          : 0.0;
  std::cout << "Memory Utilization: " << utilization << "% (" << total_used_mem
            << "/" << total_size << " bytes)" << std::endl;

  // 2. Internal Fragmentation
  // The total bytes wasted inside allocated blocks
  std::cout << "Internal Fragmentation: " << total_internal_frag << " bytes"
            << std::endl;

  // 3. External Fragmentation
  // Formula: 1 - (Largest Free Block / Total Free Memory)
  // If we have 100 bytes free, but largest chunk is 10, fragmentation is 0.9
  // (90%).
  double ext_frag = 0.0;
  if (total_free_mem > 0) {
    ext_frag = 1.0 - (static_cast<double>(largest_free_block) / total_free_mem);
  }
  std::cout << "External Fragmentation: " << (ext_frag * 100.0) << "%"
            << std::endl;

  // 4. Success/Failure Rate
  std::cout << "Allocation Requests: " << total_alloc_requests << std::endl;
  std::cout << "Successful Allocs:   " << successful_allocs << std::endl;

  double success_rate =
      (total_alloc_requests > 0)
          ? (static_cast<double>(successful_allocs) / total_alloc_requests) *
                100.0
          : 0.0;

  std::cout << "Success Rate:        " << success_rate << "%" << std::endl;
  std::cout << "==============================\n" << std::endl;

  cache_system.print_stats();

  if (use_virtual_memory) {
    vm_system.print_stats();
  }
}

int MemoryManager::get_next_available_id() {
  std::vector<int> used_ids;

  // 1. Collect all currently used IDs
  BlockHeader *current = head;
  while (current != nullptr) {
    if (!current->is_free && current->id > 0) {
      used_ids.push_back(current->id);
    }
    current = current->next;
  }

  // 2. Sort them to easily find the first gap
  // Example: [5, 1, 2] -> [1, 2, 5]
  std::sort(used_ids.begin(), used_ids.end());

  // 3. Find the smallest missing integer starting from 1
  int candidate = 1;
  for (int id : used_ids) {
    if (id == candidate) {
      // ID exists, so this candidate is taken. Try next number.
      candidate++;
    } else if (id > candidate) {
      // Found a gap!
      // Example: ids match 1, 2. Next id is 5. Candidate is 3.
      // Since 5 > 3, we know 3 is missing.
      return candidate;
    }
    // If id < candidate (duplicate logic), ignore it.
  }

  // If we went through the whole list and found no gaps,
  // the next ID is simply one greater than the last one found.
  return candidate;
}

void MemoryManager::init(size_t size) {

  this->total_size = size;
  this->next_alloc_id = 1;
  this->total_alloc_requests = 0;
  this->successful_allocs = 0;

  this->successful_allocs = 0;

  // 1. Allocate the "physical" memory
  memory.resize(size);

  if (current_strategy == AllocationStrategy::BUDDY) {
    buddy_system.init(memory.data(), size);
    // We also need to initialize head/standard structure just in case?
    // Or we completely bypass? Let's bypass head init to avoid confusion,
    // but 'head' pointer might be used by dump/stats?
    // Actually standard dump/stats iterate simple linked list.
    // Buddy has complex list structure. We need separate dump/stats for Buddy.
    head = nullptr; // Standard list is empty/invalid

    // Initialize Cache (should work fine)
    // L1: 64B, L2: 256B, L3: 1024B (1MB effectively for simulation scale? No,
    // 1KB to match scale) Let's use 1024B for L3.
    cache_system.init(64, 8, 1, 256, 8, 2, 1024, 64, 8);
    return;
  }

  // 2. Initialize the first block at the start of memory
  // We cast the raw char* address to a BlockHeader*
  head = reinterpret_cast<BlockHeader *>(memory.data());

  // 3. Set up the metadata
  // Available size is Total Size minus the size of the Header itself
  head->size = size - sizeof(BlockHeader);
  head->is_free = true;
  head->next = nullptr;
  head->prev = nullptr;
  head->padding = 0;

  std::cout << "Memory initialized with " << size << " bytes." << std::endl;
  std::cout << "Initial Free Block Size: " << head->size << " bytes."
            << std::endl;

  // Initialize Cache with default config
  // L1: 64B size, 8B block, 1-way (Direct Mapped)
  // L2: 256B size, 8B block, 2-way Set Associative
  // L3: 1024B size, 64B block, 8-way Set Associative
  cache_system.init(64, 8, 1, 256, 8, 2, 1024, 64, 8);
}

void MemoryManager::enable_vm(size_t page_size) {
  use_virtual_memory = true;
  size_t virtual_size = 65536;
  vm_system.init(page_size, virtual_size, total_size);
  std::cout << "Virtual Memory Enabled." << std::endl;
}

void MemoryManager::access(size_t address, char rw) {
  size_t final_addr = address;

  if (use_virtual_memory) {
    size_t p_addr;
    bool result = vm_system.translate(address, p_addr);
    if (result) {
      std::cout << "  Virtual Address " << address << " -> Physical Address "
                << p_addr << std::endl;
      final_addr = p_addr;
    } else {
      return;
    }
  }

  if (final_addr >= total_size) {
    std::cout << "Error: Access violation at physical address " << final_addr
              << std::endl;
    return;
  }
  cache_system.access(final_addr, rw);
}

void MemoryManager::dump_memory() {
  if (current_strategy == AllocationStrategy::BUDDY) {
    buddy_system.debug_lists();
    return;
  }

  std::cout << "\n--- Memory dump ---" << std::endl;
  BlockHeader *current = head;
  size_t offset = 0;

  while (current != nullptr) {
    std::cout << "[" << offset << " - "
              << (offset + sizeof(BlockHeader) + current->size - 1) << "] ";

    if (current->is_free) {
      std::cout << "FREE";
    } else {
      std::cout << "USED (ID=" << current->id << ")";
    }
    std::cout << " | Size: " << current->size << " (+32 header)" << std::endl;

    offset += sizeof(BlockHeader) + current->size;
    current = current->next;
  }
  std::cout << "-------------------\n" << std::endl;
}

void *MemoryManager::malloc(size_t size) {
  total_alloc_requests++;

  if (current_strategy == AllocationStrategy::BUDDY) {
    void *ptr = buddy_system.malloc(size);
    if (ptr)
      successful_allocs++;
    return ptr;
  }

  size_t aligned_size = align(size);
  size_t padding = aligned_size - size;

  BlockHeader *candidate = nullptr;

  switch (current_strategy) {
  case AllocationStrategy::FIRST_FIT:
    candidate = find_first_fit(aligned_size);
    break;
  case AllocationStrategy::BEST_FIT:
    candidate = find_best_fit(aligned_size);
    break;
  case AllocationStrategy::WORST_FIT:
    candidate = find_worst_fit(aligned_size);
    break;
  default:
    break;
  }

  if (candidate == nullptr) {
    return nullptr;
  }

  // Split Logic
  if (candidate->size >= aligned_size + sizeof(BlockHeader) + 1) {
    BlockHeader *new_block =
        reinterpret_cast<BlockHeader *>(reinterpret_cast<char *>(candidate) +
                                        sizeof(BlockHeader) + aligned_size);
    new_block->size = candidate->size - aligned_size - sizeof(BlockHeader);
    new_block->is_free = true;
    new_block->next = candidate->next;
    new_block->prev = candidate;
    new_block->id = 0;
    new_block->padding = 0;

    candidate->size = aligned_size;
    candidate->next = new_block;
    if (new_block->next)
      new_block->next->prev = new_block;
  }

  candidate->is_free = false;
  candidate->id = get_next_available_id();
  candidate->padding = padding;

  successful_allocs++;

  std::cout << "Allocated block id " << candidate->id << " at address "
            << get_offset_from_ptr(reinterpret_cast<char *>(candidate) +
                                   sizeof(BlockHeader))
            << " (Strategy: " << (int)current_strategy << ")" << std::endl;

  return reinterpret_cast<char *>(candidate) + sizeof(BlockHeader);
}

void MemoryManager::free(void *ptr) {
  if (!ptr)
    return;

  if (current_strategy == AllocationStrategy::BUDDY) {
    buddy_system.free(ptr);
    return;
  }

  // Robustness Fix: Verify pointer
  BlockHeader *current = head;
  bool found = false;

  while (current != nullptr) {
    void *data_ptr = reinterpret_cast<char *>(current) + sizeof(BlockHeader);

    if (data_ptr == ptr) {
      found = true;
      break;
    }
    current = current->next;
  }

  if (!found) {
    std::cout << "Error: Invalid address. Pointer is not the start of an "
                 "allocated block."
              << std::endl;
    return;
  }

  if (current->is_free) {
    std::cout << "Error: Block is already free." << std::endl;
    return;
  }

  std::cout << "Freeing Block ID " << current->id << "..." << std::endl;
  current->is_free = true;
  current->id = 0;

  // Coalesce Next
  if (current->next && current->next->is_free) {
    current->size += sizeof(BlockHeader) + current->next->size;
    current->next = current->next->next;
    if (current->next)
      current->next->prev = current;
  }

  // Coalesce Prev
  if (current->prev && current->prev->is_free) {
    current->prev->size += sizeof(BlockHeader) + current->size;
    current->prev->next = current->next;
    if (current->next)
      current->next->prev = current->prev;
  }
}

void MemoryManager::free_by_id(int id) {
  BlockHeader *current = head;
  while (current != nullptr) {
    if (!current->is_free && current->id == id) {
      void *ptr = reinterpret_cast<char *>(current) + sizeof(BlockHeader);
      free(ptr);
      return;
    }
    current = current->next;
  }
  std::cout << "Error: Block ID " << id << " not found or already freed."
            << std::endl;
}

void MemoryManager::free_smart(int value) {
  BlockHeader *current = head;
  BlockHeader *target = nullptr;

  // Search by ID
  while (current != nullptr) {
    if (!current->is_free && current->id == value) {
      target = current;
      break;
    }
    current = current->next;
  }

  // Search by Address
  if (target == nullptr) {
    void *ptr = get_ptr_from_offset(static_cast<size_t>(value));
    if (ptr) {
      current = head;
      while (current != nullptr) {
        void *data_loc =
            reinterpret_cast<char *>(current) + sizeof(BlockHeader);
        if (data_loc == ptr) {
          if (!current->is_free) {
            target = current;
          }
          break;
        }
        current = current->next;
      }
    }
  }

  if (target != nullptr) {
    std::cout << "Freeing Block ID " << target->id << " (Address "
              << get_offset_from_ptr(reinterpret_cast<char *>(target) +
                                     sizeof(BlockHeader))
              << ")..." << std::endl;

    target->is_free = true;
    target->id = 0;

    // Coalesce Next
    if (target->next && target->next->is_free) {
      target->size += sizeof(BlockHeader) + target->next->size;
      target->next = target->next->next;
      if (target->next)
        target->next->prev = target;
    }

    // Coalesce Prev
    if (target->prev && target->prev->is_free) {
      target->prev->size += sizeof(BlockHeader) + target->size;
      target->prev->next = target->next;
      if (target->next)
        target->next->prev = target->prev;
    }

  } else {
    std::cout << "Error: No allocated block found with ID or Address " << value
              << std::endl;
  }
}
