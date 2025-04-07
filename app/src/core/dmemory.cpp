#include "dmemory.hpp"
#include "core/logger.hpp"
#include "platform/platform.hpp"
#include <cstdio>
#include <cstring>

static memory_system *memory_system_ptr;

static const char *memory_tag_strings[MEM_TAG_MAX_TAGS] = {"UNKNOWN    ", "LINEAR_ALLC", "APPLICATION", "RENDERER   "};

bool memory_system_startup(u64 *memory_system_memory_requirements, void *state)
{
    *memory_system_memory_requirements = sizeof(memory_system);
    if (!state)
    {
        return true;
    }
    memory_system_ptr = (memory_system *)state;

    dzero_memory(memory_system_ptr, sizeof(memory_system));

    return true;
}

void memory_system_destroy()
{
    memory_system_ptr = 0;
}

void *dallocate(u64 mem_size, memory_tags tag)
{
    if (tag == MEM_TAG_UNKNOWN)
    {
        DWARN("dallocate() called with MEM_TAG_UNKNOWN. Classify the allocation.");
    }
    if (memory_system_ptr)
    {
        memory_system_ptr->stats.tagged_allocations[tag] += mem_size;
        memory_system_ptr->stats.total_allocated++;
    }
    return platform_allocate(mem_size, false);
}
void dfree(void *block, u64 mem_size, memory_tags tag)
{
    if (tag == MEM_TAG_UNKNOWN)
    {
        DWARN("dfree() called with MEM_TAG_UNKNOWN. Classify the deallocation.");
    }
    if (memory_system_ptr)
    {
        memory_system_ptr->stats.tagged_allocations[tag] -= mem_size;
        memory_system_ptr->stats.total_allocated--;
    }
    return platform_free(block, false);
}
void dset_memory_value(void *block, s64 value, u64 size)
{
    platform_set_memory(block, value, size);
}
void dzero_memory(void *block, u64 size)
{
    platform_set_memory(block, 0, size);
}
void dcopy_memory(void *dest, void *source, u64 size)
{
    memcpy(dest, source, size);
}
char *get_memory_usg_str()
{
    const u64 gib = 1024 * 1024 * 1024;
    const u64 mib = 1024 * 1024;
    const u64 kib = 1024;

    char buffer[8000] = "System memory use (tagged):\n";
    u64  offset       = strlen(buffer);
    for (u32 i = 0; i < MEM_TAG_MAX_TAGS; ++i)
    {
        char unit[4] = "XiB";
        f32  amount  = 1.0f;
        if (memory_system_ptr->stats.tagged_allocations[i] >= gib)
        {
            unit[0] = 'G';
            amount  = (f32)memory_system_ptr->stats.tagged_allocations[i] / (f32)gib;
        }
        else if (memory_system_ptr->stats.tagged_allocations[i] >= mib)
        {
            unit[0] = 'M';
            amount  = (f32)memory_system_ptr->stats.tagged_allocations[i] / (f32)mib;
        }
        else if (memory_system_ptr->stats.tagged_allocations[i] >= kib)
        {
            unit[0] = 'K';
            amount  = (f32)memory_system_ptr->stats.tagged_allocations[i] / (f32)kib;
        }
        else
        {
            unit[0] = 'B';
            unit[1] = 0;
            amount  = (float)memory_system_ptr->stats.tagged_allocations[i];
        }

        s32 length = snprintf(buffer + offset, 8000, "  %s: %.2f%s\n", memory_tag_strings[i], (f64)amount, unit);
        offset += length;
    }
    char *out_string = strdup(buffer);
    return out_string;
}
