#include "texture_system.hpp"
#include "containers/dhashtable.hpp"

#define MAX_TEXTURES_LOADED 1024

struct texture
{
};

struct texture_system_state
{
    dhashtable<texture> textures_hashtable;
};

static texture_system_state *tex_sys_state_ptr;

bool texture_system_initialize(u64 *texture_system_mem_requirements, void *state)
{
    *texture_system_mem_requirements = sizeof(texture_system_state);
    if (!state)
    {
        return true;
    }
    tex_sys_state_ptr = (texture_system_state *)state;
    {

        tex_sys_state_ptr->textures_hashtable = new dhashtable(MAX_TEXURES_LOADED);
    }
}

bool texture_system_shutdown(void *state);
