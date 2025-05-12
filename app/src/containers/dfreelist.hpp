#pragma once
#include "defines.hpp"
// this will return the memory requirement for the freelist itself it does not return how much memory it will manage,
// so you have to allocate a memory block the size of reutrned memory_requirement + the amount of memory you want to
// manage this is up the to the caller to decide. the freelist will be laid out in memory like this:
//          ----------------------------------
//        | freelist header                 |
//        | memory that you want to manage  |
//        |--------------------------------|

// block header should be the same size as freelist_node
struct dfreelist_allocated_memory_header
{
    u64 padding[2];
    u64 block_size = INVALID_ID;
};

struct dfreelist_node
{
    dfreelist_node *next       = nullptr;
    void           *block      = nullptr;
    u64             block_size = INVALID_ID;
};

struct dfreelist
{
    dfreelist_node *head        = nullptr;
    void           *memory      = nullptr;
    u64             memory_size = INVALID_ID;
};

dfreelist *dfreelist_create(u64 *dfreelist_mem_requirements, u64 memory_size, void *memory);
bool       dfreelist_destroy(dfreelist *freelist);

void *dfreelist_allocate(dfreelist *dfree_list, u64 mem_size);
bool  dfreelist_dealocate(dfreelist *dfree_list, void *ptr);
