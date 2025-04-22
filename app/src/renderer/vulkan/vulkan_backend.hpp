#pragma once

#include "core/application.hpp"
#include "defines.hpp"
#include "resource/texture_system.hpp"

bool vulkan_backend_initialize(u64 *vulkan_backend_memory_requirements, application_config *app_config, void *state);
void vulkan_backend_shutdown();
bool vulkan_backend_resize();
bool vulkan_draw_frame(render_data *data);

bool vulkan_create_texture(texture *in_texture, u8 *pixels);
bool vulkan_destroy_texture(texture *in_texture);
