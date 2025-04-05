#include "linear_allocator.hpp"
#include "core/dmemory.hpp"

bool linear_allocator_create(linear_allocator *out_allocator, u64 size)
{
    out_allocator->total_size      = size;
    out_allocator->total_allocated = 0;
    out_allocator->num_allocations = 0;
}

void linear_allocator_destroy(linear_allocator *out_allocator)
{
}
