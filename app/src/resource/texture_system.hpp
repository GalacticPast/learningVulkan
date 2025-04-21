#pragma once
#include "defines.hpp"

bool texture_system_initialize(u64 *texture_system_mem_requirements, void *state);

bool texture_system_shutdown(void *state);
