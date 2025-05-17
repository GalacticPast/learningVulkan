#include "dfreelist.hpp"

#include "core/dasserts.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"
#include "defines.hpp"

static void dfreelist_add_node(dfreelist_node *previous, dfreelist_node *node, dfreelist_node *next)
{
    if (!previous || !node)
    {
        DERROR("Provided node is nullptr dfreelist_add_node");
        return;
    }
    previous->next = node;
    node->next     = next;
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
    dfreelist *free_list = (dfreelist *)memory;
    free_list->memory    = ((u8 *)memory + sizeof(dfreelist));

    void *raw_head             = free_list->memory;
    free_list->head            = (dfreelist_node *)DALIGN_UP(raw_head, 8);
    free_list->memory_size     = memory_size;
    free_list->memory_commited = 0;

    free_list->head->block_size = sizeof(dfreelist_node);
    free_list->head->block      = ((u8 *)memory + sizeof(dfreelist) + sizeof(dfreelist_node));

    // allocte the next free node because we dont want to pollute the head node
    dfreelist_node *head     = free_list->head;
    void           *raw_next = ((u8 *)head->block + sizeof(dfreelist_node));
    head->next               = (dfreelist_node *)DALIGN_UP(raw_next, 8);
    head->next->block        = ((u8 *)head->block + (sizeof(dfreelist_node)));
    head->next->block_size   = memory_size - sizeof(dfreelist) - (sizeof(dfreelist_node) * 2);

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
    u64                                block_size      = INVALID_ID;

    while (node)
    {
        if (node->block_size == mem_size)
        {
            allocated_block = node->block;
            block_size      = node->block_size;
            header          = (dfreelist_allocated_memory_header *)node;

            dfreelist_remove_node(previous, node, node->next);

            break;
        }
        else if (node->block_size > (mem_size + sizeof(dfreelist_node)))
        {
            void           *raw_ptr  = (dfreelist_node *)((u8 *)node->block + mem_size);
            dfreelist_node *new_node = (dfreelist_node *)DALIGN_UP(raw_ptr, 8);

            new_node->block_size = node->block_size - (mem_size + sizeof(dfreelist_node));
            new_node->block      = ((u8 *)new_node + sizeof(dfreelist_node));

            header = (dfreelist_allocated_memory_header *)node;

            dfreelist_add_node(previous, new_node, node->next);

            allocated_block = node->block;
            block_size      = mem_size;
            node            = nullptr;

            break;
        }
        previous = node;
        node     = node->next;
    }
    if (block_size == INVALID_ID || !allocated_block || !header)
    {
        DERROR("No block of size %lld was found returning nullptr", mem_size);
    }

    header->block_size          = block_size;
    free_list->memory_commited += mem_size;

    return allocated_block;
}

bool dfreelist_dealocate(dfreelist *free_list, void *ptr)
{
    dfreelist_allocated_memory_header *header =
        (dfreelist_allocated_memory_header *)((u8 *)ptr - sizeof(dfreelist_allocated_memory_header));

    u64 block_size = header->block_size;

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

    {
        // coalese prev and next blocks

        void *prev_ptr = (void *)((u8 *)previous->block + previous->block_size);
        void *next_ptr = (void *)((u8 *)new_node->block + new_node->block_size);

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
