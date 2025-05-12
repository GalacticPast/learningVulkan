#include "dfreelist.hpp"
#include "core/dasserts.hpp"

freelist *dfreelist_create(u64 *freelist_mem_requirements, u64 memory_size, void *memory)
{
    // TODO: maybe have a state that tracks all the freelists that have been allocated?
    *freelist_mem_requirements = sizeof(freelist);
    if (!memory && memory_size == 0)
    {
        return nullptr;
    }
    DASSERT(memory_size != 0);
    freelist *free_list    = (freelist *)memory;
    free_list->head        = (freelist_node *)((u8 *)memory + sizeof(freelist));
    free_list->memory      = memory;
    free_list->memory_size = memory_size;

    free_list->head->initialized = true;
    free_list->head->next        = nullptr;
    free_list->head->prev        = nullptr;
    free_list->head->block_size  = memory_size - sizeof(freelist) - sizeof(freelist_node);
    free_list->head->block       = ((u8 *)memory + sizeof(freelist) + sizeof(freelist_node));

    return free_list;
}

bool dfreelist_destroy(freelist *freelist)
{
    freelist->head->initialized = false;
    freelist->head->next        = nullptr;
    freelist->head->prev        = nullptr;
    freelist->head->block       = nullptr;
    freelist->head->block_size  = INVALID_ID;

    return true;
}

void *dfreelist_allocate(freelist *free_list, u64 mem_size)
{
}
bool dfreelist_dealocate(freelist *free_list, void *ptr, u64 mem_size)
{
    return true;
}
