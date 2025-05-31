#pragma once

#include "core/application.hpp"
#include "defines.hpp"
#include "renderer/vulkan/vulkan_types.hpp"

bool vulkan_backend_initialize(u64 *vulkan_backend_memory_requirements, application_config *app_config, void *state);
void vulkan_backend_shutdown();
bool vulkan_backend_resize();
bool vulkan_draw_frame(render_data *data);

bool vulkan_initialize_shader(shader_config *config, shader *in_shader);
// bool vulkan_add_uniforms(shader *in_shader, shader_stage stage, shader_scope scope, dstring *uniform_name, u32
// sizeof_uniform,
//                          u32 num_binding, void *data);
// bool vulkan_add_attributes(shader *in_shader, shader_stage stage, shader_scope scope, dstring *attribute_name, u32
// sizeof_attribute,
//                            u32 location, void *data);


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

bool vulkan_update_global_descriptor_sets(vulkan_shader *shader, scene_global_uniform_buffer_object *scene_global,
                                          light_global_uniform_buffer_object *light_global);

bool vulkan_create_graphics_command_pool(vulkan_context *vk_context);
bool vulkan_create_descriptor_command_pools(vulkan_context *vk_context);
bool vulkan_update_materials_descriptor_set(vulkan_shader* shader, material *material);

