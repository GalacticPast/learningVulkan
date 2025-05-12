#include "dfreelist.hpp"

#include "core/dasserts.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"

dfreelist *dfreelist_create(u64 *freelist_mem_requirements, u64 memory_size, void *memory)
{
    // TODO: maybe have a state that tracks all the freelists that have been allocated?
    *freelist_mem_requirements = sizeof(dfreelist);
    if (!memory && memory_size == 0)
    {
        return nullptr;
    }
    DASSERT(memory_size != 0);
    dfreelist *free_list   = (dfreelist *)memory;
    free_list->memory      = ((u8 *)memory + sizeof(dfreelist));
    free_list->head        = (dfreelist_node *)free_list->memory;
    free_list->memory_size = memory_size;

    free_list->head->block_size = sizeof(dfreelist_node);
    free_list->head->block      = ((u8 *)memory + sizeof(dfreelist) + sizeof(dfreelist_node));

    // allocte the next free node because we dont want to pollute the head node
    dfreelist_node *head = free_list->head;

    head->next             = (dfreelist_node *)((u8 *)head->block + sizeof(dfreelist_node));
    head->next->block      = ((u8 *)head->block + (sizeof(dfreelist_node) * 2));
    head->next->block_size = memory_size - sizeof(dfreelist) - (sizeof(dfreelist_node) * 2);

    return free_list;
}

bool dfreelist_destroy(dfreelist *dfreelist)
{
    dfreelist->head->next       = nullptr;
    dfreelist->head->block      = nullptr;
    dfreelist->head->block_size = INVALID_ID;

    dzero_memory(dfreelist->memory, dfreelist->memory_size);
    return true;
}

void *dfreelist_allocate(dfreelist *free_list, u64 mem_size)
{
    if (!free_list)
    {
        DERROR("No freelists were provied for allocation. Returning nullptr");
        return nullptr;
    }
    dfreelist_node *node     = free_list->head->next;
    dfreelist_node *next     = node->next;
    dfreelist_node *previous = free_list->head;

    while (node)
    {
        if (node->block_size == mem_size)
        {
            previous->next = next;
            break;
        }
        else if (node->block_size > mem_size)
        {
            dfreelist_node *new_node = (dfreelist_node *)((u8 *)node->block + (sizeof(dfreelist_node) + mem_size));
            new_node->block_size     = node->block_size - (mem_size + sizeof(dfreelist_node));
            new_node->block          = ((u8 *)new_node + sizeof(dfreelist_node));
            previous->next           = new_node;

            node->block_size = mem_size;
            break;
        }
        previous = node;
        node     = node->next;
    }
    if (node == nullptr)
    {
        DERROR("Freelist ran out of space requested %dbytes. Returning nullptr", mem_size);
        return nullptr;
    }
    node->next                                         = nullptr;
    void                              *allocated_block = node->block;
    u64                                block_size      = node->block_size;
    dfreelist_allocated_memory_header *header          = (dfreelist_allocated_memory_header *)node;
    header->block_size                                 = block_size;

    return allocated_block;
}

bool dfreelist_dealocate(dfreelist *free_list, void *ptr)
{
    dfreelist_allocated_memory_header *header =
        (dfreelist_allocated_memory_header *)((u8 *)ptr - sizeof(dfreelist_allocated_memory_header));

    dfreelist_node *previous = nullptr;
    dfreelist_node *curr     = free_list->head;

    while (curr < ptr)
    {
        previous = curr;
        curr     = curr->next;
    }

    if (!previous)
    {
        DERROR("How is this possible!!!");
        return false;
    }
    // TODO: coalese previous and curr ? we're only coealesing previous
    previous->block_size += header->block_size + sizeof(dfreelist_allocated_memory_header);
    dzero_memory(ptr, header->block_size);
    previous->next = curr;

    return true;
}
