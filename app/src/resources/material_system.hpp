#pragma once

#include "resource_types.hpp"

bool material_system_initialize(u64 *material_system_mem_requirements, void *state);
bool material_system_shutdown(void *state);

material *material_system_get_default_material();
material *material_system_acquire_from_config_file(dstring *file_base_name);
material *material_system_acquire_from_id(u32 id);
bool      material_system_parse_mtl_file(dstring *mtl_file_name);

material *material_system_acquire_from_name(dstring *material_name);

bool material_system_parse_mtl_file(dstring *mtl_file_name);
