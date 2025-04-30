#pragma once

#include "resource_types.hpp"

bool material_system_initialize(u64 *material_system_mem_requirements, void *state);
bool material_system_shutdown(void *state);

material *material_system_acquire_from_file(dstring *file_base_name);
material *material_system_acquire(u32 id);
