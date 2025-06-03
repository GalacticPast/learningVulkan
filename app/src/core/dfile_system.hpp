#pragma once

#include "core/dstring.hpp"
#include "defines.hpp"
#include <fstream>

#define MAX_FILE_NAME_PATH_SIZE 256

bool file_open(dstring file_name, std::ifstream* out_file_handle, bool is_binary);
bool file_close(std::ifstream* f);
bool file_get_line(std::ifstream& f, dstring* out_line);

bool file_open_and_read(const char *file_name, u64 *buffer_size_requirements, char *buffer, bool is_binary);
