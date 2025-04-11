#pragma once

#include "defines.hpp"

#define MAX_FILE_NAME_PATH_SIZE 256

bool file_open_and_read(const char *file_name, u64 *buffer_size_requirements, char *buffer, bool is_binary);
