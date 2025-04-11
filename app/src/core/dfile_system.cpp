#include "dfile_system.hpp"
#include "core/dasserts.hpp"
#include "core/logger.hpp"
#include "dstring.hpp"

#include <fstream>

bool file_open_and_read(const char *file_name, u64 *buffer_size_requirements, char *buffer, bool is_binary)
{
    const char *file_path_prefix;
    if (is_binary)
    {
        file_path_prefix = "";
    }
    else
    {
        file_path_prefix = "../assets/shaders/";
    }

    char full_file_path[MAX_FILE_NAME_PATH_SIZE] = {};
    string_copy_format(full_file_path, "%s%s", 0, file_path_prefix, file_name);

    u32 io_stream_flags = std::ios::ate;
    if (is_binary)
    {
        io_stream_flags |= std::ios::binary;
    }

    std::ifstream file(full_file_path, io_stream_flags);

    if (!file.is_open())
    {
        DFATAL("Failed to open file %s", file_name);
        return false;
    }
    u64 file_size = file.tellg();
    if (*buffer_size_requirements == INVALID_ID_64)
    {
        *buffer_size_requirements = file_size;
        return true;
    }

    file.seekg(0);
    file.read(buffer, file_size);

    file.close();

    return true;
}
