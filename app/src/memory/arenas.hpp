#pragma once

#include "defines.hpp"
// size of the pool  --> has to be multiple of 2
// num of arenas for the pool  --> has to be a multiple of 2

struct arena
{
    bool  is_free        = true;
    u64   size           = INVALID_ID_64;
    u64   allocated_size = INVALID_ID_64;
    void *data           = nullptr;
};

struct arena_pool
{
    const void *start_ptr  = nullptr;
    u64         pool_size  = INVALID_ID_64;
    u32         num_arenas = INVALID_ID;
    arena      *arenas;
};

// I will only call this once at the start of the application. So there will
// be only one pool.
bool arena_allocate_arena_pool(u64 size, u32 num_arenas);
