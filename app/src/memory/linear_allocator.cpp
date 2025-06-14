#include "linear_allocator.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"

bool linear_allocator_create(arena* arena, linear_allocator *out_allocator, u64 size)
{
    out_allocator->total_size      = size;
    out_allocator->total_allocated = 0;
    out_allocator->num_allocations = 0;

    out_allocator->memory               = dallocate(arena, size, MEM_TAG_LINEAR_ALLOCATOR);
    out_allocator->current_free_mem_ptr = out_allocator->memory;

    return true;
}

void linear_allocator_destroy(linear_allocator *allocator)
{

    if (allocator->memory)
    {
        dfree(allocator->memory, allocator->total_size, MEM_TAG_LINEAR_ALLOCATOR);
        allocator->total_allocated      = 0;
        allocator->num_allocations      = 0;
        allocator->total_size           = 0;
        allocator->total_allocated      = 0;
        allocator->num_allocations      = 0;
        allocator->memory               = 0;
        allocator->current_free_mem_ptr = 0;
    }
}

void *linear_allocator_allocate(linear_allocator *allocator, u64 size_in_bytes)
{
    if (!allocator)
    {
        DERROR("Provided allocator is nullptr");
        return nullptr;
    }

    if (allocator->total_allocated + size_in_bytes > allocator->total_size)
    {
        DERROR("The linear allocator provided has not enought size to accomadate allocation. Returning nullptr");
        return nullptr;
    }

    void *block = allocator->current_free_mem_ptr;
    allocator->current_free_mem_ptr =
        reinterpret_cast<void *>(reinterpret_cast<u8 *>(allocator->current_free_mem_ptr) + size_in_bytes);
    allocator->total_allocated += size_in_bytes;
    allocator->num_allocations++;

    return block;
}
void linear_allocator_free_all(linear_allocator *allocator)
{
    if (!allocator)
    {
        DERROR("Provided allocator is nullptr");
        return;
    }

    if (allocator->memory)
    {
        dzero_memory(allocator->memory, allocator->total_size);
        allocator->total_allocated = 0;
    }
}
