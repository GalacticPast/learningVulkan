#pragma once

#include "resource_types.hpp"

bool geometry_system_initialize(u64 *geometry_system_mem_requirements, void *state);
bool geometry_system_shutdowm(void *state);

geometry *geometry_system_get_geometry(geometry_config *config);
geometry *geometry_system_get_geometry_by_name(dstring geometry_name);
geometry *geometry_system_get_default_geometry();
