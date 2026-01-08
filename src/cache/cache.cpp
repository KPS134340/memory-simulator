#include "../../include/cache.h"
#include <iomanip>
CacheLevel::CacheLevel(int id, size_t size, size_t block_size,
                       size_t associativity)
    : level_id(id), size(size), block_size(block_size),
      associativity(associativity) {
  if (block_size == 0 || associativity == 0) {
    this->block_size = 32;
    this->associativity = 1;
  }
  num_sets = size / (this->block_size * this->associativity);
  if (num_sets == 0)
    num_sets = 1;  
  sets.resize(num_sets);
  for (auto &set : sets) {
    set.blocks.resize(this->associativity);
    set.fifo_next_victim = 0;
  }
}
void CacheLevel::set_policy(CacheReplacementPolicy p) {
  policy = p;
}
bool CacheLevel::access(size_t address, bool is_write) {
  timer++;
  size_t index = (address / block_size) % num_sets;
  size_t tag = address / (block_size * num_sets);
  CacheSet &set = sets[index];
  for (size_t i = 0; i < set.blocks.size(); ++i) {
    if (set.blocks[i].valid && set.blocks[i].tag == tag) {
      hits++;
      set.blocks[i].last_access_time = timer;
      set.blocks[i].access_count++;
      if (is_write) {
        set.blocks[i].dirty = true;
      }
      return true;
    }
  }
  misses++;
  int victim_idx = -1;
  for (size_t i = 0; i < set.blocks.size(); ++i) {
    if (!set.blocks[i].valid) {
      victim_idx = i;
      break;
    }
  }
  if (victim_idx == -1) {
    if (policy == CacheReplacementPolicy::FIFO) {
      victim_idx = set.fifo_next_victim;
      set.fifo_next_victim = (set.fifo_next_victim + 1) % associativity;
    } else if (policy == CacheReplacementPolicy::LRU) {
      size_t min_time = -1;
      for (size_t i = 0; i < set.blocks.size(); ++i) {
        if (set.blocks[i].last_access_time < min_time) {
          min_time = set.blocks[i].last_access_time;
          victim_idx = i;
        }
      }
    } else if (policy == CacheReplacementPolicy::LFU) {
      size_t min_count = -1;
      for (size_t i = 0; i < set.blocks.size(); ++i) {
        if (set.blocks[i].access_count < min_count) {
          min_count = set.blocks[i].access_count;
          victim_idx = i;
        } else if (set.blocks[i].access_count == min_count) {
          if (set.blocks[i].last_access_time <
              set.blocks[victim_idx].last_access_time) {
            victim_idx = i;
          }
        }
      }
    }
  }
  CacheBlock &victim = set.blocks[victim_idx];
  victim.valid = true;
  victim.tag = tag;
  victim.dirty = is_write;
  victim.last_access_time = timer;
  victim.access_count = 1;
  return false;
}
double CacheLevel::get_hit_rate() const {
  size_t total = hits + misses;
  if (total == 0)
    return 0.0;
  return (double)hits / total * 100.0;
}
void CacheLevel::print_stats() const {
  std::cout << "L" << level_id << " Cache Stats:" << std::endl;
  std::cout << "  Hits: " << hits << std::endl;
  std::cout << "  Misses: " << misses << std::endl;
  std::cout << "  Hit Rate: " << std::fixed << std::setprecision(2)
            << get_hit_rate() << "%" << std::endl;
}
void CacheLevel::reset_stats() {
  hits = 0;
  misses = 0;
}
CacheHierarchy::CacheHierarchy() : l1(nullptr), l2(nullptr), l3(nullptr) {}
CacheHierarchy::~CacheHierarchy() {
  if (l1)
    delete l1;
  if (l2)
    delete l2;
  if (l3)
    delete l3;
}
void CacheHierarchy::init(size_t l1_size, size_t l1_block_size, size_t l1_assoc,
                          size_t l2_size, size_t l2_block_size, size_t l2_assoc,
                          size_t l3_size, size_t l3_block_size,
                          size_t l3_assoc) {
  if (l1)
    delete l1;
  if (l2)
    delete l2;
  if (l3)
    delete l3;
  l1 = new CacheLevel(1, l1_size, l1_block_size, l1_assoc);
  l2 = new CacheLevel(2, l2_size, l2_block_size, l2_assoc);
  l3 = new CacheLevel(3, l3_size, l3_block_size, l3_assoc);
  std::cout << "Cache System Initialized:" << std::endl;
  std::cout << "  L1: " << l1_size << "B, Block " << l1_block_size << "B, "
            << l1_assoc << "-way" << std::endl;
  std::cout << "  L2: " << l2_size << "B, Block " << l2_block_size << "B, "
            << l2_assoc << "-way" << std::endl;
  std::cout << "  L3: " << l3_size << "B, Block " << l3_block_size << "B, "
            << l3_assoc << "-way" << std::endl;
}
void CacheHierarchy::set_policy(CacheReplacementPolicy p) {
  if (l1)
    l1->set_policy(p);
  if (l2)
    l2->set_policy(p);
  if (l3)
    l3->set_policy(p);
  std::cout << "Cache Policy set to ";
  if (p == CacheReplacementPolicy::FIFO)
    std::cout << "FIFO";
  else if (p == CacheReplacementPolicy::LRU)
    std::cout << "LRU";
  else if (p == CacheReplacementPolicy::LFU)
    std::cout << "LFU";
  std::cout << std::endl;
}
void CacheHierarchy::access(size_t address, char type) {
  if (!l1 || !l2 || !l3)
    return;
  bool is_write = (type == 'W' || type == 'w');
  bool l1_hit = l1->access(address, is_write);
  if (l1_hit)
    return;
  bool l2_hit = l2->access(address, is_write);
  if (l2_hit)
    return;
  l3->access(address, is_write);
}
void CacheHierarchy::print_stats() {
  std::cout << "\n=== Cache Statistics ===" << std::endl;
  if (l1)
    l1->print_stats();
  if (l2)
    l2->print_stats();
  if (l3)
    l3->print_stats();
  std::cout << "========================\n" << std::endl;
}
