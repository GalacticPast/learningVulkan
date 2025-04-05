#include "dmemory.hpp"

bool memory_system::initialize(void *state, u64 *memory_system_requirements)
{
    return false;
}

void memory_system::shutdown()
{
}

void *dallocate(u64 mem_size, memory_tags tag)
{
}
void dfree(void *block, memory_tags tag);

void dset_memory_value(void *block, u64 value, u64 size);
void dzero_memory(void *block, u64 size);
