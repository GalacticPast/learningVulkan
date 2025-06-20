#pragma once

#include "math/dmath_types.hpp"
#include "resource_types.hpp"

bool material_system_initialize(arena* system_arena, arena *resource_arena);
bool material_system_shutdown();

material *material_system_get_default_material();
material *material_system_get_from_config_file(dstring *file_base_name);
material *material_system_get_from_id(u32 id);
bool      material_system_parse_mtl_file(dstring *mtl_file_name);
material *material_system_get_from_name(dstring *material_name);

// for which shader do you want the material to be applied to
bool material_system_create_material(material_config* config, u64 shader_id);

bool material_system_load_font(dstring *font_base_name, font_glyph_data **data);

bool material_system_parse_mtl_file(dstring *mtl_file_name);

bool material_system_update_material_shader_globals(scene_global_uniform_buffer_object *sgubo,
                                                    light_global_uniform_buffer_object *lgubo);
bool material_system_update_skybox_shader_globals(scene_global_uniform_buffer_object *sgubo);
