#pragma once

#include "defines.hpp"

enum memory_tags
{
    LINEAR_ALLOCATOR,
    APPLICATION,
    RENDERER,
    MAX_MEMORY_TAGS,
};

struct memory_system_stats
{
};

class memory_system
{
  public:
    bool initialize(void *state, u64 *memory_system_requirements);
    void shutdown();

  private:
    memory_system_stats stats;
};

void *dallocate(u64 mem_size, memory_tags tag);
void  dfree(void *block, memory_tags tag);

void dset_memory_value(void *block, u64 value, u64 size);
void dzero_memory(void *block, u64 size);
