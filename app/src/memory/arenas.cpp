#include "arenas.hpp"
#include "core/dasserts.hpp"
#include "core/logger.hpp"
#include "defines.hpp"
#include "math/dmath_types.hpp"
#include "platform/platform.hpp"

#define ARENA_DEFAULT_ALIGNMENT 8

struct arena_system_state
{
    arena_pool    pool;
    platform_info info;
};

static arena_system_state *arena_sys_ptr = nullptr;

// I am using the first page as the arena system state. Kind of seems wasteful but its only 4096 bytes.
bool arena_allocate_arena_pool(u64 size, u32 num_arenas)
{
    void *start_ptr = platform_virtual_reserve(size, true);

    u32 page_size = 4096;
    arena_sys_ptr = reinterpret_cast<arena_system_state *>(platform_virtual_commit(start_ptr, 1));
    DASSERT(arena_sys_ptr);

    arena_sys_ptr->info = platform_get_info();

    // check if our assumption was correct. I think I can avoid this by finding a better way but oh well.
    DASSERT(arena_sys_ptr->info.page_size == page_size);
    void *pool_start_ptr = static_cast<u8 *>(start_ptr) + page_size;

    arena_pool *pool       = &arena_sys_ptr->pool;
    // for now allocate only one page
    pool->start_ptr        = platform_virtual_commit(pool_start_ptr, 1);
    pool->pool_size        = size - (2 * page_size);
    pool->num_arenas       = num_arenas;
    pool->arena_size_bytes = pool->pool_size / num_arenas;
    pool->arena_size_pages = floor(pool->arena_size_bytes / arena_sys_ptr->info.page_size);

    void *bool_array_ptr = reinterpret_cast<u8 *>(pool_start_ptr) + sizeof(arena_pool);
    pool->free_arenas    = reinterpret_cast<bool *>(DALIGN_UP(bool_array_ptr, sizeof(bool)));

    void *array_ptr = reinterpret_cast<u8 *>(pool_start_ptr) + (sizeof(bool) * num_arenas) + sizeof(arena_pool);
    pool->arenas    = reinterpret_cast<arena **>(DALIGN_UP(array_ptr, sizeof(arena *)));

    u8 *arena_start_ptr = static_cast<u8 *>(pool_start_ptr) + page_size;
    u64 arena_size      = size / num_arenas;

    for (u32 i = 0; i < num_arenas; i++)
    {
        pool->arenas[i]      = reinterpret_cast<arena *>(arena_start_ptr);
        pool->free_arenas[i] = true;

        arena_start_ptr += arena_size;
    }

    return true;
}

arena *arena_get_arena()
{
    DASSERT_MSG(arena_sys_ptr,
                "Arena systems hasent been initialized!!!. Initialize it first before calling this function");

    u32     num_arenas       = arena_sys_ptr->pool.num_arenas;
    u32     arena_size_pages = arena_sys_ptr->pool.arena_size_pages;
    bool   *free_arenas      = arena_sys_ptr->pool.free_arenas;
    arena **arenas_arr       = arena_sys_ptr->pool.arenas;

    arena *out_arena = nullptr;

    for (u32 i = 0; i < num_arenas; i++)
    {
        if (free_arenas[i])
        {
            arena *start_ptr = arenas_arr[i];
            out_arena        = reinterpret_cast<arena *>(platform_virtual_commit(start_ptr, arena_size_pages));
            DASSERT_MSG(out_arena, "There is something wrong with platform_virtual_commit");

            void *raw_ptr         = reinterpret_cast<void *>(reinterpret_cast<u8 *>(out_arena) + sizeof(arena));
            out_arena->start_ptr  = reinterpret_cast<void *>(DALIGN_UP(raw_ptr, sizeof(arena)));
            out_arena->free_ptr   = out_arena->start_ptr;
            out_arena->total_size = arena_sys_ptr->pool.arena_size_bytes;
            free_arenas[i]        = false;
        }
    }
    DASSERT_MSG(out_arena, "There are no more free arenas.");
    return out_arena;
}

void *arena_allocate_block(arena *in_arena, u64 mem_size)
{
    DASSERT(in_arena);

    uintptr_t next_arena = reinterpret_cast<uintptr_t>(in_arena) + arena_sys_ptr->pool.arena_size_bytes;

    uintptr_t return_ptr = DALIGN_UP(in_arena->free_ptr, ARENA_DEFAULT_ALIGNMENT);

    uintptr_t next_free_ptr    = reinterpret_cast<uintptr_t>(in_arena->free_ptr) + mem_size;
    uintptr_t aligned_free_ptr = DALIGN_UP(next_free_ptr, ARENA_DEFAULT_ALIGNMENT);

// sanity checking
#ifdef DEBUG

    arena **arenas_arr = arena_sys_ptr->pool.arenas;
    u32     num_arenas = arena_sys_ptr->pool.num_arenas;
    bool    found      = false;
    for (u32 i = 0; i < num_arenas; i++)
    {
        uintptr_t arena = reinterpret_cast<uintptr_t>(arenas_arr[i]);
        if (arena == next_arena)
        {
            found = true;
            break;
        }
    }
    DASSERT(found);

#endif

    if (aligned_free_ptr > next_arena)
    {
        DERROR("Arena doesnot have enough remaining space to accomade allocation of size %d.", mem_size);
        return nullptr;
    }
    in_arena->free_ptr   = reinterpret_cast<void *>(aligned_free_ptr);
    in_arena->allocated += mem_size;

    return reinterpret_cast<void *>(return_ptr);
}

void arena_reset_arena(arena *in_arena)
{
    DASSERT(in_arena);
    in_arena->free_ptr = in_arena->start_ptr;
    dzero_memory(in_arena->free_ptr, in_arena->total_size);
    in_arena->allocated = 0;
}

void arena_free_arena(arena *in_arena)
{

    u32     num_arenas       = arena_sys_ptr->pool.num_arenas;
    u32     arena_size_pages = arena_sys_ptr->pool.arena_size_pages;
    u32     arena_size_bytes = arena_sys_ptr->pool.arena_size_bytes;
    bool   *free_arenas      = arena_sys_ptr->pool.free_arenas;
    arena **arenas_arr       = arena_sys_ptr->pool.arenas;

    arena *out_arena = nullptr;

    bool found = false;
    for (u32 i = 0; i < num_arenas; i++)
    {
        if (free_arenas[i] && arenas_arr[i] == in_arena)
        {
            platform_virtual_free(in_arena, arena_size_bytes, true);
            free_arenas[i] = false;
            found          = true;
            break;
        }
    }
    DASSERT_MSG(found, "Passed arena ptr doesnot match the arenas array inside the arena pool.");
    return;
}
