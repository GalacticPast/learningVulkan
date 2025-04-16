#pragma once

#include "core/application.hpp"
#include "defines.hpp"

bool vulkan_backend_initialize(u64 *vulkan_backend_memory_requirements, application_config *app_config, void *state);
void vulkan_backend_shutdown();
bool vulkan_backend_resize();
bool vulkan_draw_frame(render_data *data);
