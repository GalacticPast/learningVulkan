#include "dmemory.hpp"
#include "containers/dfreelist.hpp"
#include "core/dasserts.hpp"
#include "core/logger.hpp"

#include "defines.hpp"
#include "platform/platform.hpp"

#include <cstdio>
#include <cstring>

static memory_system *memory_system_ptr;
// clang-format off
static const char *memory_tag_strings[MEM_TAG_MAX_TAGS] = {
    "UNKNOWN    ",
    "LINEAR_ALLC",
    "DSTRING     ",
    "DHASHTABLE ",
    "DARRAY     ",
    "APPLICATION",
    "RENDERER   ",
    "GEOMETRY   "
};
// clang-format on 

bool memory_system_startup(u64 *memory_system_memory_requirements, void *state)
{
    u64 freelist_mem_requirements = INVALID_ID_64; 
    dfreelist_create(&freelist_mem_requirements, 0, 0);
    *memory_system_memory_requirements = sizeof(memory_system) + freelist_mem_requirements + GIGA(1);
    
    if (!state)
    {
        return true;
    }
    DINFO("Starting up memory system...");
    memory_system_ptr = (memory_system *)state;

    dzero_memory(memory_system_ptr, *memory_system_memory_requirements);

    void* freelist_mem = (u8 *)state + sizeof(memory_system);
    memory_system_ptr->dfreelist = dfreelist_create(&freelist_mem_requirements, GIGA(1),freelist_mem);


    return true;
}

void memory_system_shutdown(void *state)
{
    DINFO("Shutting down memory system...");
    memory_system_ptr = 0;
}

void *dallocate(u64 mem_size, memory_tags tag)
{
    if (!memory_system_ptr)
    {
        DFATAL("Memory subsystem hasnt been initialized yet. Initialize it");
        debugBreak();
        return nullptr;
    }
    if (tag == MEM_TAG_UNKNOWN)
    {
        DWARN("dallocate() called with MEM_TAG_UNKNOWN. Classify the allocation.");
    }
    memory_system_ptr->stats.tagged_allocations[tag] += mem_size;
    memory_system_ptr->stats.total_allocated++;
    //void *block = platform_allocate(mem_size, false);
    void *block = dfreelist_allocate(memory_system_ptr->dfreelist, mem_size);
    dzero_memory(block, mem_size);
    return block;
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
    bool result = dfreelist_dealocate(memory_system_ptr->dfreelist, block);
    if(!result)
    {
        DERROR("dfreelist_dealocate failded!!!");
    }
    return;
}
void dset_memory_value(void *block, u64 value, u64 size)
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
void get_memory_usg_str(u64 *buffer_usg_mem_requirements, char *out_buffer)
{
    *buffer_usg_mem_requirements = 8000;
    if (!out_buffer)
    {
        return;
    }

    const u64 gib      = 1024 * 1024 * 1024;
    const u64 mib      = 1024 * 1024;
    const u64 kib      = 1024;

    const char *header = "System memory use (tagged):\n";
    u64         offset = strlen(header);

    dcopy_memory(out_buffer, (void *)header, offset);

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

        s32 length  = snprintf(out_buffer + offset, 8000, "  %s: %.2f%s\n", memory_tag_strings[i], (f64)amount, unit);
        offset     += length;
    }

    return;
}

void set_memory_stats_for_tag(u64 size_in_bytes, memory_tags tag)
{
    memory_system_ptr->stats.tagged_allocations[tag] += size_in_bytes;
    memory_system_ptr->stats.total_allocated++;
}
