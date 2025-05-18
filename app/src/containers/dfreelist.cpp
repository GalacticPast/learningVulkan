#include "dfreelist.hpp"

#include "core/dasserts.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"
#include "defines.hpp"

#define DFREELIST_PADDING 8
#define DFREELIST_MINIMUM_ALLOCATION_BLOCK_SIZE 24

static void dfreelist_add_node(dfreelist_node *previous, dfreelist_node *node, dfreelist_node *next)
{
    if (previous)
    {
        previous->next = node;
    }
    node->next = next;
}

static void dfreelist_remove_node(dfreelist_node *previous, dfreelist_node *node, dfreelist_node *next)
{
    if (!previous || !node)
    {
        DERROR("Provided node is nullptr for dfreelist_remove_node");
        return;
    }
    previous->next = next;

    node->next       = 0;
    node->block      = 0;
    node->block_size = 0;
}

dfreelist *dfreelist_create(u64 *freelist_mem_requirements, u64 memory_size, void *memory)
{
    // TODO: maybe have a state that tracks all the freelists that have been allocated?
    *freelist_mem_requirements = sizeof(dfreelist);
    if (!memory && memory_size == 0)
    {
        return nullptr;
    }
    DASSERT(memory_size != 0);
    dfreelist *free_list       = (dfreelist *)memory;
    free_list->memory_commited = 0;

    void *raw_freelist_memory_ptr = (char *)free_list + sizeof(dfreelist);
    free_list->memory             = (void *)DALIGN_UP((uintptr_t)raw_freelist_memory_ptr, DFREELIST_PADDING);
    u32 free_list_memory_offset   = (uintptr_t)free_list->memory - (uintptr_t)raw_freelist_memory_ptr;

    free_list->memory_size = memory_size - (sizeof(dfreelist) + free_list_memory_offset);

    // HACK:
    DASSERT(free_list->memory_size >= GB(1));

    void *raw_head         = free_list->memory;
    free_list->head        = (dfreelist_node *)DALIGN_UP(raw_head, DFREELIST_PADDING);
    free_list->head->block = (char *)free_list->head + sizeof(dfreelist_node);

    dfreelist_node *head = free_list->head;

    head->next       = (dfreelist_node *)((char *)head->block + sizeof(dfreelist_node));
    head->block_size = sizeof(dfreelist_node);

    dfreelist_node *next = free_list->head->next;
    next->next           = nullptr;
    next->block          = (char *)next + sizeof(dfreelist_node);
    next->block_size     = memory_size - (head->block_size + (sizeof(dfreelist_node) * 2));

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

    dfreelist_node *node     = free_list->head;
    dfreelist_node *previous = nullptr;

    dfreelist_allocated_memory_header *header          = nullptr;
    void                              *allocated_block = nullptr;

    if (mem_size < DFREELIST_MINIMUM_ALLOCATION_BLOCK_SIZE)
    {
        mem_size = DFREELIST_MINIMUM_ALLOCATION_BLOCK_SIZE;
    }

    s64 required_size = mem_size + sizeof(dfreelist_allocated_memory_header);
    DASSERT((s64)(free_list->memory_commited + required_size) < free_list->memory_size);

    while (node)
    {
        DASSERT(node->block_size < free_list->memory_size);
        if (node->block_size == required_size)
        {
            allocated_block = node->block;
            header          = (dfreelist_allocated_memory_header *)node;

            dfreelist_remove_node(previous, node, node->next);
            break;
        }
        else if (node->block_size > required_size)
        {
            void           *raw_node_ptr = (char *)node->block + required_size;
            dfreelist_node *new_node     = (dfreelist_node *)DALIGN_UP((uintptr_t)raw_node_ptr, DFREELIST_PADDING);

            u64 ptr_offset       = (uintptr_t)new_node - (uintptr_t)raw_node_ptr;
            new_node->block_size = node->block_size - (required_size + ptr_offset);
            new_node->block      = (char *)new_node + sizeof(dfreelist_node);

            dfreelist_add_node(previous, new_node, node->next);

            allocated_block = node->block;
            header          = (dfreelist_allocated_memory_header *)node;

            break;
        }
        previous = node;
        node     = node->next;
    }

    dzero_memory(header, sizeof(dfreelist_allocated_memory_header));

    // check for overrides
    header->padding[0] = INVALID_ID_64;
    header->padding[1] = INVALID_ID_64;
    //

    header->block_size          = mem_size;
    free_list->memory_commited += mem_size;

    return allocated_block;
}

bool dfreelist_dealocate(dfreelist *free_list, void *ptr)
{
    dfreelist_allocated_memory_header *header =
        (dfreelist_allocated_memory_header *)((u8 *)ptr - sizeof(dfreelist_allocated_memory_header));

    DASSERT(header->padding[0] == INVALID_ID_64);
    DASSERT(header->padding[1] == INVALID_ID_64);

    u64 block_size = header->block_size;
    DASSERT(block_size < GB(1));
    DASSERT(block_size > 0);

    dzero_memory(header, sizeof(dfreelist_allocated_memory_header));

    dfreelist_node *new_node = (dfreelist_node *)header;
    new_node->block_size     = block_size;
    new_node->next           = nullptr;
    new_node->block          = ptr;
    dzero_memory(new_node->block, new_node->block_size);

    dfreelist_node *previous = nullptr;
    dfreelist_node *node     = free_list->head;

    while (node)
    {
        if (node > ptr)
        {
            dfreelist_add_node(previous, new_node, node);
            break;
        }
        previous = node;
        node     = node->next;
    }

    free_list->memory_commited -= new_node->block_size;

    DASSERT(free_list->memory_commited >= 0);

    {
        // coalese prev and next blocks

        void *prev_ptr = (void *)((u8 *)previous->block + previous->block_size + sizeof(dfreelist_node));
        void *next_ptr = (void *)((u8 *)new_node->block + new_node->block_size + sizeof(dfreelist_node));

        if (prev_ptr == new_node)
        {
            previous->block_size += new_node->block_size + sizeof(dfreelist_node);
            prev_ptr              = (void *)((u8 *)previous->block + previous->block_size);
            if (prev_ptr == new_node->next)
            {
                previous->block_size += new_node->next->block_size + sizeof(dfreelist_node);
                dfreelist_remove_node(new_node, node, node->next);
            }
            dfreelist_remove_node(previous, new_node, node);
        }
        else if (next_ptr == node)
        {
            new_node->block_size += node->block_size + sizeof(dfreelist_node);
            dfreelist_remove_node(new_node, node, node->next);
        }
    }

    return true;
}
