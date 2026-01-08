#ifndef BLOCK_H
#define BLOCK_H
#include <cstddef>  


struct BlockHeader {
    size_t size;         
    size_t padding;      
    bool is_free;
    BlockHeader* next;
    BlockHeader* prev;
    int id;
};

#endif