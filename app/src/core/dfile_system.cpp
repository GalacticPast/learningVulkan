#include "dfile_system.hpp"

#include "core/dasserts.hpp"
#include "core/logger.hpp"
#include "dstring.hpp"
#include <string>

bool file_open(dstring file_name, std::ifstream* out_file_handle, bool is_binary)
{
    DASSERT(out_file_handle);

    u32 io_stream_flags = std::ios::in;
    if (is_binary)
    {
        io_stream_flags |= std::ios::binary;
    }

    out_file_handle->open(file_name.string, static_cast<std::ios_base::openmode>(io_stream_flags));

    if (!out_file_handle->is_open())
    {
        DFATAL("Failed to open file %s", file_name);
        return false;
    }
    return true;
}

bool file_close(std::ifstream* f)
{
    DASSERT(f);
    f->close();
    return true;
}

bool file_get_line(std::ifstream& f, dstring* out_line)
{
    DASSERT(f);
    DASSERT(out_line);

    f.getline(out_line->string, MAX_STRING_LENGTH);

    if(f.fail())
    {
        std::ios_base::iostate state = f.rdstate();

        // Check for specific error bits
        if (state & std::ios_base::eofbit)
        {
            DERROR("End of file reached.");
        }
        if (state & std::ios_base::failbit)
        {
            DERROR("Non-fatal I/O error occurred.");
        }
        if (state & std::ios_base::badbit)
        {
            DERROR("Fatal I/O error occurred.");
        }
        return false;
    }

    return true;
}


bool file_open_and_read(const char *file_name, u64 *buffer_size_requirements, char *buffer, bool is_binary)
{
    u32 io_stream_flags = std::ios::in;
    if (is_binary)
    {
        io_stream_flags |= std::ios::binary;
    }

    std::ifstream file(file_name, static_cast<std::ios_base::openmode>(io_stream_flags));

    if (!file.is_open())
    {
        DFATAL("Failed to open file %s", file_name);
        return false;
    }
    u64 file_size = file.tellg();
    if (!buffer)
    {
        *buffer_size_requirements = file_size;
        file.close();
        return true;
    }

    file.seekg(0);

    // read entire file into a buffer
    file.read(buffer, file_size);

    file.close();

    return true;
}
