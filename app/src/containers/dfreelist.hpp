#pragma once
#include "defines.hpp"
// this will return the memory requirement for the freelist itself it does not return how much memory it will manage,
// so you have to allocate a memory block the size of reutrned memory_requirement + the amount of memory you want to
// manage this is up the to the caller to decide. the freelist will be laid out in memory like this:
//          ----------------------------------
//        | freelist header                 |
//        | memory that you want to manage  |
//        |--------------------------------|

struct freelist_node
{
    bool           initialized = false;
    freelist_node *next        = nullptr;
    freelist_node *prev        = nullptr;
    void          *block       = nullptr;
    u64            block_size  = INVALID_ID;
};

struct freelist
{
    freelist_node *head        = nullptr;
    void          *memory      = nullptr;
    u64            memory_size = INVALID_ID;
};

freelist *dfreelist_create(u64 *freelist_mem_requirements, u64 memory_size, void *memory);
bool      dfreelist_destroy(freelist *freelist);

void *dfreelist_allocate(freelist *free_list, u64 mem_size);
bool  dfreelist_dealocate(freelist *free_list, void *ptr, u64 mem_size);
