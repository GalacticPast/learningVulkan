#pragma once
#include "containers/darray.hpp"
#include "core/application.hpp"
#include "texture_system.hpp"

#define GEOMETRY_NAME_MAX_LENGTH 512

struct geometry_config
{
    darray<vertex> vertices;
    darray<u32>    indices;
    char           name[GEOMETRY_NAME_MAX_LENGTH];
    char           texture_name[TEXTURE_NAME_MAX_LENGTH];
};

bool geometry_system_initialize(u64 *geometry_system_mem_requirements, void *state);
bool geometry_system_shutdowm(void *state);
