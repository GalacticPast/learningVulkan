#pragma once
#include "resource_types.hpp"

bool texture_system_initialize(u64 *texture_system_mem_requirements, void *state);
bool texture_system_shutdown(void *state);

bool     texture_system_create_texture(dstring *file_base_name);
texture *texture_system_get_texture(const char *texture_name);
texture *texture_system_get_default_texture();
