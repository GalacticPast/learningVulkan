#pragma once

#include "defines.hpp"
// size of the pool  --> has to be multiple of 2
// num of arenas for the pool  --> has to be a multiple of 2

struct arena
{
    u64   total_size = INVALID_ID_64;
    u64   allocated  = INVALID_ID_64;
    void *start_ptr  = nullptr;
    void *free_ptr   = nullptr;
};

struct arena_pool
{
    u64         pool_size        = INVALID_ID_64; // 8
    u32         arena_size_bytes = INVALID_ID;    // 4
    u32         arena_size_pages = INVALID_ID;    // 4
    // if they are not free that means we have to commit them.
    u32 num_arenas               = INVALID_ID; // 4

    // this will hold the starting ptr's for the arenas at total_pool_size / num_arenas internval
    // if pool_size is m, where m must be a multiple of the page size, and num_arenas is n, where n is a multiple of 2.
    // then each arena will have m/n size;
    arena *arenas;
};

// INFO: I will only call this once at the start of the application. So there will
//  be only one pool.
bool arena_allocate_arena_pool(u64 size, u32 num_arenas);
bool arena_free_arena_pool();

arena *arena_get_arena();
void   arena_free_arena(arena *in_arena);

void *arena_allocate_block(arena *in_arena, u64 mem_size);

void arena_reset_arena(arena *in_arena);
