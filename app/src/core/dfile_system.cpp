#include "dfile_system.hpp"

#include "core/dasserts.hpp"
#include "core/logger.hpp"
#include "dstring.hpp"


bool file_open(dstring file_name, std::fstream *out_file_handle, bool for_writing, bool is_binary)
{
    DASSERT(out_file_handle);

    u32 io_stream_flags;

    if (for_writing)
    {
        io_stream_flags = std::ios::out;
    }
    else
    {
        io_stream_flags = std::ios::in;
    }

    if (is_binary)
    {
        io_stream_flags |= std::ios::binary;
    }

    out_file_handle->open(file_name.string, static_cast<std::ios_base::openmode>(io_stream_flags));

    if (!out_file_handle->is_open())
    {
        DFATAL("Failed to open file %s", file_name.c_str());
        return false;
    }
    return true;
}

bool file_exists(dstring *path)
{
    DASSERT(path);
    std::ifstream file(path->c_str());
    return file.good();
}

bool file_close(std::fstream *f)
{
    DASSERT(f);
    f->close();
    return true;
}

bool file_get_line(std::fstream &f, dstring *out_line)
{
    DASSERT(f);
    DASSERT(out_line);

    f.getline(out_line->string, MAX_STRING_LENGTH);
    out_line->str_len = f.gcount();

    if (f.fail())
    {
        std::ios_base::iostate state = f.rdstate();

        // Check for specific error bits
        if (state & std::ios_base::eofbit)
        {
            DDEBUG("End of file reached.");
            return false;
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
    auto io_stream_flags = std::ios::ate;

    if (is_binary)
    {
        io_stream_flags |= std::ios::binary;
    }
//NOTE: because the /r/n endings are causing the buffer size to mismatch
#if DPLATFORM_WINDOWS
        io_stream_flags |= std::ios::binary;
#endif
    std::ifstream file(file_name, io_stream_flags);

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

    file.seekg(0, std::ios::beg);

    // read entire file into a buffer
    file.read(buffer, file_size);

    file.close();

    return true;
}

bool file_write(std::fstream *f, const char *buffer, u64 size)
{
    DASSERT_MSG(f, "Provided file handle is null ptr");
    DASSERT_MSG(buffer, "Provided buffer is null ptr");

    f->write(buffer, size);

    return true;
}
