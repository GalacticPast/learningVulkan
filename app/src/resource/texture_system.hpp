#pragma once
#include "core/dstring.hpp"
#include "defines.hpp"

struct texture
{
    dstring name;
    u32     id           = INVALID_ID;
    u32     tex_width    = INVALID_ID;
    u32     tex_height   = INVALID_ID;
    u32     num_channels = INVALID_ID;

    u8 **pixels;

    void *vulkan_texture_state;
};

bool texture_system_initialize(u64 *texture_system_mem_requirements, void *state);

bool texture_system_shutdown(void *state);
void texture_system_get_default_texture(texture *out_texture);
bool texture_system_create_texture(dstring *file_base_name);
