#pragma once
#include "defines.h"

typedef enum memory_tags
{
    TYPE_PLATFORM,
    TYPE_RENDERER,
    TYPE_ARRAYS,
    TYPE_UNKNOWN,
    TYPE_MAX_TAGS,
} memory_tags;

#define ALLOCATE_MEMORY_RENDERER(size) __allocate_memory(size, TYPE_RENDERER)
#define ALLOCATE_MEMORY_ARRAY(size) __allocate_memory(size, TYPE_ARRAYS)
#define ALLOCATE_MEMORY_PLATFORM(size) __allocate_memory(size, TYPE_PLATFORM)

#define FREE_MEMORY_RENDERER(ptr_to_block, block_size) __free_memory(ptr_to_block, block_size, TYPE_RENDERER)
#define FREE_MEMORY_ARRAY(ptr_to_block, block_size) __free_memory(ptr_to_block, block_size, TYPE_ARRAYS)
#define FREE_MEMORY_PLATFORM(ptr_to_block, block_size) __free_memory(ptr_to_block, block_size, TYPE_PLATFORM)

void *__allocate_memory(u64 size, memory_tags tag);
void  __free_memory(void *block, u64 block_size, memory_tags tag);

void zero_memory(void *block, u64 block_size);

void print_memory_sizes();
