#pragma once

#include "core/application.hpp"
#include "defines.hpp"
#include "renderer/vulkan/vulkan_types.hpp"
#include "resources/texture_system.hpp"

bool vulkan_backend_initialize(u64 *vulkan_backend_memory_requirements, application_config *app_config, void *state);
void vulkan_backend_shutdown();
bool vulkan_backend_resize();
bool vulkan_draw_frame(render_data *data);

bool vulkan_create_material(material *in_material);
bool vulkan_destroy_material(material *in_material);

bool vulkan_create_texture(texture *in_texture, u8 *pixels);
bool vulkan_destroy_texture(texture *in_texture);

bool vulkan_create_geometry(geometry *out_geometry, u32 vertex_count, vertex *vertices, u32 index_count, u32 *indices);
bool vulkan_destroy_geometry(geometry *geometry);

// not in bytes
u32 vulkan_calculate_index_offset(vulkan_context *vk_context, u32 geometry_id);
// not in bytes
u32 vulkan_calculate_vertex_offset(vulkan_context *vk_context, u32 geometry_id);
