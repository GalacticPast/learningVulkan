#include "arenas.hpp"
#include "platform/platform.hpp"

struct arena_system_state
{
    arena_pool pool;
};

static arena_system_state *arena_sys_ptr = nullptr;

bool arena_allocate_arena_pool(u64 size, u32 num_arenas)
{
    void* ptr = platform_virtual_reserve(size,true);

    return true;
}
