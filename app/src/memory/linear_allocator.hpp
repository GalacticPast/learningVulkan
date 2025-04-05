#pragma once
#include "defines.hpp"

struct linear_allocator
{
    u64 total_size;
    u64 total_allocated;
    u64 num_allocations;

    void *memory;
    u64  *mem_ptr;
};

bool linear_allocator_create(linear_allocator *out_allocator, u64 size);
void linear_allocator_destroy(linear_allocator *out_allocator);
