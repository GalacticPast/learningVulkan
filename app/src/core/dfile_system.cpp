#include "dfile_system.hpp"
#include "core/dasserts.hpp"
#include "core/logger.hpp"
#include "dstring.hpp"

#include <fstream>

bool file_open_and_read(const char *file_name, u64 *buffer_size_requirements, char *buffer, bool is_binary)
{
    u32 io_stream_flags = std::ios::ate;
    if (is_binary)
    {
        io_stream_flags |= std::ios::binary;
    }

    std::ifstream file(file_name, (std::ios_base::openmode)io_stream_flags);

    if (!file.is_open())
    {
        DFATAL("Failed to open file %s", file_name);
        return false;
    }
    u64 file_size = file.tellg();
    if (*buffer_size_requirements == INVALID_ID_64 || *buffer_size_requirements == 0)
    {
        *buffer_size_requirements = file_size;
        return true;
    }

    file.seekg(0);

    // read entire file into a buffer
    file.read(buffer, file_size);

    file.close();

    return true;
}
