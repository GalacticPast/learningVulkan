#pragma once
#include "core/dstring.hpp"
#include "defines.hpp"

#define DEFAULT_TEXTURE_HANDLE "DEFAULT_TEXTURE"

struct texture
{

    dstring name;
    u32     id           = INVALID_ID;
    u32     tex_width    = INVALID_ID;
    u32     tex_height   = INVALID_ID;
    u32     num_channels = INVALID_ID;

    void *vulkan_texture_state = nullptr;
};

bool texture_system_initialize(u64 *texture_system_mem_requirements, void *state);

bool texture_system_shutdown(void *state);
void texture_system_get_texture(const char *texture_name, texture *out_texture);
bool texture_system_create_texture(dstring *file_base_name);
