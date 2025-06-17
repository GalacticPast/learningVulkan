#include "core/dstring.hpp"
#include "resources/resource_types.hpp"

bool shader_system_startup(arena *arena, u64 *shader_system_mem_requirements, void *state);
bool shader_system_shutdown(void *state);

bool shader_system_create_shader(dstring *shader_config_file_path, shader *out_shader, u64 *out_shader_id);

bool shader_system_bind_shader(u64 shader_id);

bool shader_system_create_default_shaders(u64 *material_shader_id, u64 *skybox_shader_id, u64 *grid_shader_id,
                                          u64 *ui_shader_id);

shader *shader_system_get_shader(u64 id);

u64 shader_system_get_default_material_shader_id();
u64 shader_system_get_default_skybox_shader_id();
u64 shader_system_get_default_grid_shader_id();

// so in the end we need a dynamic buffer for each stage or just have one big buffer for all 3
bool shader_system_update_per_frame(scene_global_uniform_buffer_object *scene_global,
                                    light_global_uniform_buffer_object *light_global);
bool shader_system_update_per_group(u64 offset, void *data);
bool shader_system_update_per_object(u64 offset, void *data);
