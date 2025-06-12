#pragma once
#include "defines.hpp"

// 24 bytes
struct dfreelist_allocation_header
{
    u64 block_size;
    u64 allocation_id;
    u64 padding;
};

struct dfreelist_node
{
    dfreelist_node *next;
    u64             block_size;
    u64             padding;
};

struct dfreelist
{
    void *data;
    u64   size;
    u64   used;

    dfreelist_node *head;
};

//// this will return the memory requirement for the freelist itself it does not return how much memory it will manage,
//// so you have to allocate a memory block the size of reutrned memory_requirement + the amount of memory you want to
//// manage this is up the to the caller to decide. the freelist will be laid out in memory like this:
////          ----------------------------------
////        | dfreelist header                 |
////        | memory that you want to manage  |
////        |--------------------------------|
//// dfreelist_allocated_memory_header will be the same size as freelist_node

dfreelist *dfreelist_create(u64 *dfreelist_mem_requirements, u64 memory_size, void *memory);
bool       dfreelist_destroy(dfreelist *freelist);

void *dfreelist_allocate(dfreelist *dfreelist, u64 mem_size);
bool  dfreelist_dealocate(dfreelist *dfreelist, void *ptr);
