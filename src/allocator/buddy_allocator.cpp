#include "../../include/buddy_allocator.h"

BuddyAllocator::BuddyAllocator()
    : memory_start(nullptr), total_size(0), min_order(0), max_order(0) {

  for (int i = 0; i < MAX_LEVELS; ++i) {
    free_lists[i] = nullptr;
  }
}

int BuddyAllocator::get_order(size_t size) {
  int order = 0;
  size_t s = 1;

  while (s < size) {
    s <<= 1;
    order++;
  }

  return order;
}

size_t BuddyAllocator::get_size_from_order(int order) {
  return (size_t)1 << order;
}

void BuddyAllocator::init(char *memory, size_t size) {
  this->memory_start = memory;
  this->total_size = size;
  max_order = get_order(size);

  if (get_size_from_order(max_order) > size) {
    max_order--;  
  }

  this->total_size = get_size_from_order(max_order);  
  min_order = get_order(MIN_BLOCK_SIZE);
  BlockHeader *root = reinterpret_cast<BlockHeader *>(memory_start);
  root->size = this->total_size -
               sizeof(BlockHeader);  
  root->is_free = true;
  root->next = nullptr;
  root->prev = nullptr;
  root->id = 0;
  root->size = get_size_from_order(max_order) - sizeof(BlockHeader);
  free_lists[max_order] = root;
  std::cout << "Buddy Allocator Initialized. Total Size: " << this->total_size
            << " (Order " << max_order << ")" << std::endl;
}

BlockHeader *BuddyAllocator::get_block(int order) {
  if (order > max_order)
    return nullptr;

  if (free_lists[order] != nullptr) {
    BlockHeader *block = free_lists[order];
    free_lists[order] = block->next;
    if (free_lists[order])
      free_lists[order]->prev = nullptr;
    block->next = nullptr;
    block->prev = nullptr;
    block->is_free = false;
    return block;
  }

  BlockHeader *larger = get_block(order + 1);
  if (!larger)
    return nullptr;
  size_t size = get_size_from_order(order);
  BlockHeader *buddy =
      reinterpret_cast<BlockHeader *>(reinterpret_cast<char *>(larger) + size);
  buddy->is_free = true;
  buddy->size = size - sizeof(BlockHeader);
  buddy->next = free_lists[order];  
  buddy->prev = nullptr;
  if (free_lists[order])
    free_lists[order]->prev = buddy;
  free_lists[order] = buddy;  
  larger->size = size - sizeof(BlockHeader);
  larger->is_free = false;
  return larger;
}

void *BuddyAllocator::malloc(size_t size) {
  size_t total_needed = size + sizeof(BlockHeader);
  int order = get_order(total_needed);
  if (order < min_order)
    order = min_order;
  BlockHeader *block = get_block(order);

  if (!block) {
    std::cout << "Buddy Allocator: No memory available." << std::endl;
    return nullptr;
  }

  block->is_free = false;
  std::cout << "Buddy Alloc: Order " << order << " ("
            << get_size_from_order(order) << " bytes)" << std::endl;
  return reinterpret_cast<char *>(block) + sizeof(BlockHeader);
}

void BuddyAllocator::free(void *ptr) {
  if (!ptr)
    return;
  BlockHeader *block = reinterpret_cast<BlockHeader *>(
      reinterpret_cast<char *>(ptr) - sizeof(BlockHeader));
  size_t total_size = block->size + sizeof(BlockHeader);
  int order = get_order(total_size);
  char *block_addr = reinterpret_cast<char *>(block);

  while (order < max_order) {
    size_t buddy_size = get_size_from_order(order);
    size_t relative_offset = (size_t)(block_addr - memory_start);
    size_t buddy_offset = relative_offset ^ buddy_size;
    char *buddy_addr = memory_start + buddy_offset;
    BlockHeader *buddy = reinterpret_cast<BlockHeader *>(buddy_addr);
    bool buddy_is_free_at_level = false;
    size_t buddy_current_total = buddy->size + sizeof(BlockHeader);

    if (buddy->is_free && buddy_current_total == buddy_size) {
      buddy_is_free_at_level = true;
    }

    if (buddy_is_free_at_level) {
      std::cout << "Merging with buddy at " << buddy_offset << " (Order "
                << order << ")" << std::endl;
      if (buddy->prev)
        buddy->prev->next = buddy->next;
      if (buddy->next)
        buddy->next->prev = buddy->prev;
      if (free_lists[order] == buddy)
        free_lists[order] = buddy->next;

      if (buddy_addr < block_addr) {
        block = buddy;
        block_addr = buddy_addr;
      }

      order++;
      block->size = get_size_from_order(order) - sizeof(BlockHeader);
    } else {
      break;
    }
  }

  block->is_free = true;
  block->next = free_lists[order];
  block->prev = nullptr;
  if (free_lists[order])
    free_lists[order]->prev = block;
  free_lists[order] = block;
}

void BuddyAllocator::debug_lists() {
  std::cout << "--- Buddy Free Lists ---" << std::endl;

  for (int i = min_order; i <= max_order; ++i) {
    int count = 0;
    BlockHeader *curr = free_lists[i];

    while (curr) {
      count++;
      curr = curr->next;
    }

    if (count > 0) {
      std::cout << "Order " << i << " (" << get_size_from_order(i) << " or "
                << (1 << i) << "): " << count << " blocks" << std::endl;
    }
  }

  std::cout << "------------------------" << std::endl;
}