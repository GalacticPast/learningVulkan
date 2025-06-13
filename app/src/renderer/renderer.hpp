#pragma once

#include "defines.hpp"
#include "memory/arenas.hpp"

bool renderer_system_startup(arena* arena, u64 *renderer_system_memory_requirements, struct application_config *app_config,
                             struct linear_allocator *linear_systems_allocator, void *state);
void renderer_system_shutdown();
bool renderer_resize();

void renderer_draw_frame(struct render_data *data);
