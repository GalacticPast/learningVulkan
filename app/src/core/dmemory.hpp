#pragma once

#include "defines.hpp"

enum memory_tags
{
    MEM_TAG_UNKNOWN,
    MEM_TAG_LINEAR_ALLOCATOR,
    MEM_TAG_APPLICATION,
    MEM_TAG_RENDERER,
    MEM_TAG_MAX_TAGS,
};

struct memory_system_stats
{
    u64 total_allocated;
    u64 tagged_allocations[MEM_TAG_MAX_TAGS];
};

struct memory_system
{
    memory_system_stats stats;
};

bool memory_system_initialze(void *state, u64 *memory_system_memory_requirements);
void memory_system_destroy();

void *dallocate(u64 mem_size, memory_tags tag);
void  dfree(void *block, u64 size, memory_tags tag);

void dset_memory_value(void *block, u64 value, u64 size);
void dzero_memory(void *block, u64 size);

void dcopy_memory(void *dest, void *source, u64 size);

char *get_memory_usg_str();
