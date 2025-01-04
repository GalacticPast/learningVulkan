#include "fileIO.h"
#include "core/logger.h"
#include <stdlib.h>

b8 read_file(const char *filename, char *buffer, b8 is_binary)
{
    char *shader_dir = "./src/shaders/";

    char *full_path;
    sprintf(full_path, "%s%s\n", shader_dir, filename);

    FILE *file_ptr;
    if (is_binary)
    {
        file_ptr = fopen(full_path, "rb");
    }
    else
    {
        file_ptr = fopen(full_path, "r");
    }

    if (file_ptr == NULL)
    {
        ERROR("Failed to read file %s, Maybe the filename is wrong? and make sure it actually exists in the shader dir", filename);
        return false;
    }

    fseek(file_ptr, 0, SEEK_END);
    u32 file_size = ftell(file_ptr);
    fseek(file_ptr, 0, SEEK_SET);

    buffer = malloc(file_size + 1);
    fread(buffer, file_size + 1, 1, file_ptr);

    fclose(file_ptr);

    buffer[file_size] = 0;

    return true;
}
