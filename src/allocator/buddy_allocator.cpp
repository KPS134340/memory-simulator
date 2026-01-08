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

  // Ensure size is power of 2
  // If not, we take largest power of 2 that fits
  // But PDF says "memory size must be a power of two". We assume input is or we
  // truncate.

  max_order = get_order(size);
  // Double check if exact match, if not, warn?
  if (get_size_from_order(max_order) > size) {
    max_order--; // Use the smaller power of 2
  }
  this->total_size = get_size_from_order(max_order); // Adjust real usage

  min_order = get_order(MIN_BLOCK_SIZE);

  // Initial State: One big free block of max_order
  BlockHeader *root = reinterpret_cast<BlockHeader *>(memory_start);
  root->size = this->total_size -
               sizeof(BlockHeader); // Logic varies: in Buddy, usually size is
                                    // total block size including header.
  // Let's stick to: BlockHeader describes the Payload?
  // Actually standard buddy implementation tracks BLOCK SIZE (Power of 2).
  // The header is embedding inside.
  // Payload = BlockSize - sizeof(Header).

  root->is_free = true;
  root->next = nullptr;
  root->prev = nullptr;
  root->id = 0;

  // We mark the 'size' field potentially differently.
  // To match BlockHeader struct usage:
  // root->size should be payload size.
  // But for buddy calculation we need strict power of 2.
  // Let's store payload size in root->size, but calculate using order.

  root->size = get_size_from_order(max_order) - sizeof(BlockHeader);

  free_lists[max_order] = root;

  std::cout << "Buddy Allocator Initialized. Total Size: " << this->total_size
            << " (Order " << max_order << ")" << std::endl;
}

BlockHeader *BuddyAllocator::get_block(int order) {
  // 1. Check if we have a block at this order
  if (order > max_order)
    return nullptr;

  if (free_lists[order] != nullptr) {
    // Remove from list
    BlockHeader *block = free_lists[order];
    free_lists[order] = block->next;
    if (free_lists[order])
      free_lists[order]->prev = nullptr;

    block->next = nullptr;
    block->prev = nullptr;
    block->is_free = false;
    return block;
  }

  // 2. No block, try to split a larger one
  BlockHeader *larger = get_block(order + 1);
  if (!larger)
    return nullptr;

  // Split 'larger' into two buddies of 'order'
  // Address arithmetic
  size_t size = get_size_from_order(order);
  BlockHeader *buddy =
      reinterpret_cast<BlockHeader *>(reinterpret_cast<char *>(larger) + size);

  // Setup Buddy
  buddy->is_free = true;
  buddy->size = size - sizeof(BlockHeader);
  buddy->next = free_lists[order]; // Should be null technically if we just
                                   // emptied it, but safe
  buddy->prev = nullptr;
  if (free_lists[order])
    free_lists[order]->prev = buddy;
  free_lists[order] = buddy; // Add buddy to free list

  // Return 'larger' (the left buddy)
  larger->size = size - sizeof(BlockHeader);
  larger->is_free = false;
  return larger;
}

void *BuddyAllocator::malloc(size_t size) {
  // 1. Calculate required size + header
  size_t total_needed = size + sizeof(BlockHeader);

  // 2. Round up to next power of 2 (Order)
  int order = get_order(total_needed);
  if (order < min_order)
    order = min_order;

  // 3. Get Block
  BlockHeader *block = get_block(order);
  if (!block) {
    std::cout << "Buddy Allocator: No memory available." << std::endl;
    return nullptr;
  }

  block->is_free = false;
  // We don't set ID here, caller might? Or we assume Buddy handles its own IDs
  // for now? MemoryManager expects to set ID.

  std::cout << "Buddy Alloc: Order " << order << " ("
            << get_size_from_order(order) << " bytes)" << std::endl;
  return reinterpret_cast<char *>(block) + sizeof(BlockHeader);
}

void BuddyAllocator::free(void *ptr) {
  if (!ptr)
    return;

  BlockHeader *block = reinterpret_cast<BlockHeader *>(
      reinterpret_cast<char *>(ptr) - sizeof(BlockHeader));

  // Calculate Order from size
  // Note: block->size is payload. Total block size = payload + header.
  // We need to restore the full power-of-2 size conceptually.
  // If we only store payload, we might lose the "true" power of 2 if payload
  // was small? Actually in malloc we set block->size = (1<<order) - header. So
  // (block->size + header) should be 1<<order.

  size_t total_size = block->size + sizeof(BlockHeader);
  int order = get_order(total_size);

  // Coalescing Loop
  char *block_addr = reinterpret_cast<char *>(block);

  while (order < max_order) {
    size_t buddy_size = get_size_from_order(order);

    // Calculate Buddy Address
    // Buddy Addr = Base + ((BlockAddr - Base) XOR BlockSize)
    size_t relative_offset = (size_t)(block_addr - memory_start);
    size_t buddy_offset = relative_offset ^ buddy_size;
    char *buddy_addr = memory_start + buddy_offset;

    BlockHeader *buddy = reinterpret_cast<BlockHeader *>(buddy_addr);

    // Check if buddy is free AND is actually at this order
    // (Buddy might be split into smaller chunks, in which case it's not "free"
    // at this level)

    bool buddy_is_free_at_level = false;

    // Traverse free list at this order to check if buddy is in it?
    // O(N) search is slow but robust.
    // Optimization: Check buddy->is_free and buddy->size.
    // But simply checking is_free is dangerous if buddy is split.
    // If buddy is split, the "buddy header" is the header of the *first*
    // sub-block. If that sub-block is allocated, buddy is false. If that
    // sub-block is free, it might be free at a LOWER order. Correct check:
    // buddy->is_free && (buddy->size total == 1<<order)

    size_t buddy_current_total = buddy->size + sizeof(BlockHeader);
    if (buddy->is_free && buddy_current_total == buddy_size) {
      buddy_is_free_at_level = true;
    }

    if (buddy_is_free_at_level) {
      // Coalesce!
      std::cout << "Merging with buddy at " << buddy_offset << " (Order "
                << order << ")" << std::endl;

      // Remove buddy from free list
      if (buddy->prev)
        buddy->prev->next = buddy->next;
      if (buddy->next)
        buddy->next->prev = buddy->prev;
      if (free_lists[order] == buddy)
        free_lists[order] = buddy->next;

      // Normalize: The resulting block starts at min(block_addr, buddy_addr)
      if (buddy_addr < block_addr) {
        block = buddy;
        block_addr = buddy_addr;
      }

      // Move up
      order++;
      block->size = get_size_from_order(order) - sizeof(BlockHeader);
    } else {
      // Buddy not free or split. Stop.
      break;
    }
  }

  // Add merged block to free list at 'order'
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
