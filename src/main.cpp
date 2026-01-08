#include "../include/memory_manager.h"
#include <iostream>
#include <sstream>
#include <string>


int main() {
  MemoryManager mem;
  bool initialized = false;
  std::string command;
  std::cout << "Welcome to MemSim. Type 'help' for commands." << std::endl;

  while (true) {
    std::cout << "> ";
    std::getline(std::cin, command);
    std::stringstream ss(command);
    std::string action;
    ss >> action;

    if (action == "exit") {
      break;
    } else if (action == "init") {
      size_t size;

      if (ss >> size) {
        mem.init(size);
        initialized = true;
      } else {
        std::cout << "Usage: init <size>" << std::endl;
      }
    }

    else if (action == "help") {
      std::cout << "Commands:\n";
      std::cout << "  init <size>          - Initialize memory" << std::endl;
      std::cout << "  enable_vm <page_size>- Enable Virtual Memory"
                << std::endl;
      std::cout << "  malloc <size>        - Allocate bytes" << std::endl;
      std::cout << "  free <addr>          - Free bytes at relative address"
                << std::endl;
      std::cout << "  read <addr>          - Read from address (Cache Test)"
                << std::endl;
      std::cout << "  write <addr> <val>   - Write to address (Cache Test)"
                << std::endl;
      std::cout << "  dump                 - Show memory map" << std::endl;
      std::cout << "  stats                - Show usage stats" << std::endl;
      std::cout << "  exit                 - Quit program" << std::endl;
    }

    else if (!initialized) {
      std::cout << "Error: Memory not initialized. Run 'init <size>' first."
                << std::endl;
      continue;
    } else if (action == "malloc") {
      size_t size;

      if (ss >> size) {
        void *ptr = mem.malloc(size);

        if (ptr) {
          size_t offset = mem.get_offset_from_ptr(ptr);
          std::cout << "Allocated at address: " << offset << std::endl;
        } else {
          std::cout << "Allocation failed (Not enough memory)" << std::endl;
        }
      }

    } else if (action == "free") {
      int value;

      if (ss >> value) {
        mem.free_smart(value);
      } else {
        std::cout << "Usage: free <block_id> OR free <address>" << std::endl;
      }

    } else if (action == "dump") {
      mem.dump_memory();
    } else if (action == "stats") {
      mem.print_stats();
    } else if (action == "read") {
      size_t addr;

      if (ss >> addr) {
        mem.access(addr, 'R');
        std::cout << "Read from address " << addr << std::endl;
      } else {
        std::cout << "Usage: read <address>" << std::endl;
      }

    } else if (action == "write") {
      size_t addr;
      int value;  

      if (ss >> addr >> value) {
        mem.access(addr, 'W');
        std::cout << "Wrote " << value << " to address " << addr << std::endl;
      } else {
        std::cout << "Usage: write <address> <value>" << std::endl;
      }
    }

    else if (action == "set") {
      std::string target, strategy_name, extra;
      ss >> target >> strategy_name;

      if (target == "allocator") {

        if (ss >> extra) {
          strategy_name += " " + extra;
        }

        if (strategy_name == "first fit") {
          mem.set_strategy(AllocationStrategy::FIRST_FIT);
          std::cout << "Strategy changed to First Fit." << std::endl;
        } else if (strategy_name == "best fit") {
          mem.set_strategy(AllocationStrategy::BEST_FIT);
          std::cout << "Strategy changed to Best Fit." << std::endl;
        } else if (strategy_name == "worst fit") {
          mem.set_strategy(AllocationStrategy::WORST_FIT);
          std::cout << "Strategy changed to Worst Fit." << std::endl;
        } else if (strategy_name == "buddy") {
          mem.set_strategy(AllocationStrategy::BUDDY);
          std::cout << "Strategy changed to Buddy Allocator." << std::endl;
        } else {
          std::cout
              << "Unknown strategy. Use: first fit, best fit, worst fit, buddy."
              << std::endl;
        }

      } else if (target == "cache" && strategy_name == "policy") {
        std::string policy_str;

        if (ss >> policy_str) {

          if (policy_str == "fifo") {
            mem.set_cache_policy(CacheReplacementPolicy::FIFO);
            std::cout << "Cache Policy set to FIFO" << std::endl;
          } else if (policy_str == "lru") {
            mem.set_cache_policy(CacheReplacementPolicy::LRU);
            std::cout << "Cache Policy set to LRU" << std::endl;
          } else if (policy_str == "lfu") {
            mem.set_cache_policy(CacheReplacementPolicy::LFU);
            std::cout << "Cache Policy set to LFU" << std::endl;
          } else {
            std::cout << "Unknown policy. Use: fifo, lru, lfu" << std::endl;
          }

        } else {
          std::cout << "Usage: set cache policy <fifo|lru|lfu>" << std::endl;
        }

      } else if (target == "vm") {

        if (strategy_name == "policy") {
          std::string policy_str;

          if (ss >> policy_str) {

            if (policy_str == "fifo") {
              mem.set_vm_policy(ReplacementPolicy::FIFO);
              std::cout << "VM Policy set to FIFO" << std::endl;
            } else if (policy_str == "lru") {
              mem.set_vm_policy(ReplacementPolicy::LRU);
              std::cout << "VM Policy set to LRU" << std::endl;
            } else if (policy_str == "clock") {
              mem.set_vm_policy(ReplacementPolicy::CLOCK);
              std::cout << "VM Policy set to CLOCK" << std::endl;
            } else {
              std::cout << "Unknown policy. Use: fifo, lru, clock" << std::endl;
            }

          } else {
            std::cout << "Usage: set vm policy <fifo|lru|clock>" << std::endl;
          }

        } else if (strategy_name == "latency") {
          int ms;

          if (ss >> ms) {
            mem.set_vm_latency(ms);
            std::cout << "VM Disk Latency set to " << ms << "ms" << std::endl;
          } else {
            std::cout << "Usage: set vm latency <ms>" << std::endl;
          }

        } else {
          std::cout << "Unknown VM setting. Use: policy, latency" << std::endl;
        }
      }

    } else if (action == "enable_vm") {
      size_t page_size;

      if (ss >> page_size) {
        mem.enable_vm(page_size);
      } else {
        std::cout << "Usage: enable_vm <page_size>" << std::endl;
      }
    }
  }

  return 0;
}