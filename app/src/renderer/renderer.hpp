#pragma once

#include "defines.hpp"

bool renderer_system_startup(u64 *renderer_system_memory_requirements, struct application_config *app_config, struct linear_allocator *linear_systems_allocator, void *state);
void renderer_system_shutdown();
