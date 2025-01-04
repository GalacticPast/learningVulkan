#include "fileIO.h"
#include "containers/array.h"
#include "core/asserts.h"
#include "core/logger.h"
#include <stdio.h>
#include <stdlib.h>

char *read_file(const char *filename, b8 is_binary)
{
    char *shader_dir = "/home/galacticpast/Documents/projects/learningVulkan/src/shaders/";
    char *binary_dir = "/home/galacticpast/Documents/projects/learningVulkan/bin/";

    char full_path[100] = {};

    FILE *file_ptr;
    if (is_binary)
    {
        sprintf(full_path, "%s%s", binary_dir, filename);
        file_ptr = fopen(full_path, "rb");
        if (file_ptr == NULL)
        {
            FATAL("Failed to read binary file %s, Maybe the filename is wrong? and make sure it actually exists in the bin dir", full_path);
            debugbreak();
        }
    }
    else
    {
        sprintf(full_path, "%s%s\n", shader_dir, filename);
        file_ptr = fopen(full_path, "r");
        if (file_ptr == NULL)
        {
            FATAL("Failed to read file %s, Maybe the filename is wrong? and make sure it actually exists in the shader dir", filename);
            debugbreak();
        }
    }

    fseek(file_ptr, 0, SEEK_END);
    u32 file_size = ftell(file_ptr);
    fseek(file_ptr, 0, SEEK_SET);

    char *buffer = array_create_with_capacity(char, file_size + 1);
    fread(buffer, file_size + 1, 1, file_ptr);

    fclose(file_ptr);

    buffer[file_size] = 0;
    array_set_length(buffer, file_size);

    return buffer;
}
