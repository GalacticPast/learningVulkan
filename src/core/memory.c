#include "memory.h"
#include "core/logger.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct allocated_memory_sizes
{
    u64 sizes[TYPE_MAX_TAGS];
} allocated_memory_sizes;

allocated_memory_sizes memory = {0};

void *__allocate_memory(u64 size, memory_tags tag)
{
    if (tag == TYPE_UNKNOWN)
    {
        WARN("Unkown type allocated. Reclass this allocation.");
    }
    memory.sizes[tag] += size;

    return calloc(size, 1);
}

void __free_memory(void *block, u64 block_size, memory_tags tag)
{
    memory.sizes[tag] -= block_size;
    free(block);
}

void print_memory_sizes()
{

    DEBUG("Memory allocated sizes: ");
    char  buffer[4096];
    char *tag[TYPE_MAX_TAGS] = {"[Platform]: ", " [Renderer]: ", " [Arrays]:   ", " [Unknown]:  "};

    s32 offset = 0;

    u32 gb = 1024 * 1024 * 1024;
    u32 mb = 1024 * 1024;
    u32 kb = 1024;

    for (u32 i = 0; i < TYPE_MAX_TAGS; i++)
    {
        if (memory.sizes[i] >= gb)
        {
            offset += sprintf(buffer + offset, "%s%.2f GB\n", tag[i], ((f64)memory.sizes[i] / gb));
        }
        else if (memory.sizes[i] >= mb)
        {
            offset += sprintf(buffer + offset, "%s%.2f MB\n", tag[i], ((f64)memory.sizes[i] / mb));
        }
        else if (memory.sizes[i] >= kb)
        {
            offset += sprintf(buffer + offset, "%s%.2f KB\n", tag[i], ((f64)memory.sizes[i] / kb));
        }
        else
        {
            offset += sprintf(buffer + offset, "%s%.2f \n", tag[i], ((f64)memory.sizes[i] / 1));
        }
    }
    platform_log_message(buffer, LOG_LEVEL_DEBUG, 4096);
}
