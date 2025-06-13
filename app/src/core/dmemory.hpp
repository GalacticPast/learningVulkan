#pragma once

#include "defines.hpp"
#include "memory/arenas.hpp"

enum memory_tags
{
    MEM_TAG_UNKNOWN,
    MEM_TAG_LINEAR_ALLOCATOR,
    MEM_TAG_DSTRING,
    MEM_TAG_DHASHTABLE,
    MEM_TAG_DARRAY,
    MEM_TAG_APPLICATION,
    MEM_TAG_RENDERER,
    MEM_TAG_GEOMETRY,
    MEM_TAG_MAX_TAGS,
};


bool memory_system_startup(u64 *memory_system_memory_requirements, void *state);
void memory_system_shutdown(void *state);

void *dallocate(arena* arena, u64 mem_size, memory_tags tag);
void  dfree(void *block, u64 size, memory_tags tag);

void dset_memory_value(void *block, u64 value, u64 size);
void dzero_memory(void *block, u64 size);

void dcopy_memory(void *dest, const void *source, u64 size);

void get_memory_usg_str(u64 *buffer_usg_mem_requirements, char *out_buffer);

// WARN: this is a hack because we initialize the systems linear allocator before the memory sub system and thats why it
// doesnt report correctly for the first time.
void set_memory_stats_for_tag(u64 size_in_bytes, memory_tags tag);
