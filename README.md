# Memory Management Simulator

A comprehensive simulation of a memory management system, featuring multiple allocation strategies, a 3-level cache hierarchy (L1/L2/L3), and virtual memory with disk access latency simulation.

## Demo

<a href="demo_video.mov" target="_blank">
  <img src="https://img.youtube.com/vi/placeholder/0.jpg" alt="Watch the Demo Video" width="400" />
</a>

## Features

*   **Memory Allocators**:
    *   **First Fit**: Fastest allocation by finding the first available block.
    *   **Best Fit**: Minimizes fragmentation by finding the smallest sufficient block.
    *   **Worst Fit**: Selects the largest block to leave large gaps.
    *   **Buddy System**: Power-of-2 allocation with coalescing.

*   **Cache Hierarchy**:
    *   **3 Levels**: L1 (Direct Mapped), L2 (2-way Set Associative), L3 (8-way Set Associative).
    *   **Replacement Policies**: FIFO (First-In-First-Out), LRU (Least Recently Used), LFU (Least Frequently Used).
    *   **Write Policy**: Write-Allocate / Write-Back (simulated via dirty bits).

*   **Virtual Memory**:
    *   **Paging**: Support for demand paging with configurable page sizes.
    *   **Page Replacement**: 
        *   **FIFO**: Standard queue-based eviction.
        *   **LRU**: Least Recently Used eviction.
        *   **Clock**: Second-chance algorithm using reference bits.
    *   **Disk Latency**: Configurable delay (ms) for page faults to simulate IO.

## Getting Started

### Prerequisites
*   C++ Compiler (g++ or clang++)
*   Make
*   Python 3 (for running tests)

### Compilation
To build the simulator, simply run:

```bash
make
```

This will produce the `memsim_app` executable.

## Usage

Run the interactive shell:
```bash
./memsim_app
```

### Commands

| Command | Arguments | Description |
| :--- | :--- | :--- |
| `init` | `<size>` | Initialize physical memory with `<size>` bytes. |
| `enable_vm` | `<page_size>` | Enable Virtual Memory with specified page size. |
| `malloc` | `<size>` | Allocate `<size>` bytes. |
| `free` | `<address>` | Free memory at physical address `<address>`. |
| `read` | `<address>` | Read from memory address (triggers Cache/VM). |
| `write` | `<address>` | Write to memory address (triggers Cache/VM). |
| `set allocator` | `<strategy>` | Switch strategy: `first fit`, `best fit`, `worst fit`, `buddy`. |
| `set cache policy` | `<policy>` | Set cache eviction: `fifo`, `lru`, `lfu`. |
| `set vm policy` | `<policy>` | Set VM page replacement: `fifo`, `lru`, `clock`. |
| `set vm latency` | `<ms>` | Set disk access latency in milliseconds. |
| `stats` | - | Print current memory, cache, and VM statistics. |
| `dump` | - | Dump the memory map (showing blocks and gaps). |
| `exit` | - | Exit the simulator. |

### Example Session

```bash
minit 1024
set allocator best fit
malloc 100
malloc 50
free 32
set cache policy lru
read 32
stats
```

## Testing

A comprehensive test suite is included in the `tests/` directory.

To run all tests:
```bash
python3 run_tests.py
```

This script will execute all `.in` files in `tests/` and verify that the simulator runs without crashing. Outputs are saved to `outputs/`.

## Project Structure

*   `src/`: Source code (`main.cpp`, `allocator/`, `cache/`, `virtual_memory/`).
*   `include/`: Header files.
*   `tests/`: Test input files.
*   `outputs/`: Test output files.
*   `Makefile`: Build configuration.
