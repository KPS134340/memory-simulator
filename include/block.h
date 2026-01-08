#ifndef BLOCK_H
#define BLOCK_H

#include <cstddef> // for size_t

struct BlockHeader {
    size_t size;        // Usable space (aligned)
    size_t padding;     // Internal fragmentation
    bool is_free;
    BlockHeader* next;
    BlockHeader* prev;
    int id;
};

#endif
