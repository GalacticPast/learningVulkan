#include "dfreelist.hpp"

#include "core/dasserts.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"
#include "defines.hpp"

#define DFREELIST_PADDING 8
#define DFREELIST_MINIMUM_ALLOCATION_BLOCK_SIZE 24

void free_list_free_all(dfreelist *fl)
{
    fl->used                   = 0;
    dfreelist_node *first_node = (dfreelist_node *)fl->data;
    first_node->block_size     = fl->size;
    first_node->next           = NULL;
    fl->head                   = first_node;
}

dfreelist *dfreelist_create(u64 *dfreelist_mem_requirements, u64 memory_size, void *memory)
{
    *dfreelist_mem_requirements = sizeof(dfreelist);
    if (!memory)
    {
        return nullptr;
    }
    dfreelist *fl = (dfreelist *)memory;
    fl->data      = (u8 *)memory + sizeof(dfreelist);
    fl->size      = memory_size;
    free_list_free_all(fl);
    return fl;
}

bool dfreelist_destroy(dfreelist *freelist)
{
    free_list_free_all(freelist);
    return true;
}

bool is_power_of_two(uintptr_t x)
{
    return (x & (x - 1)) == 0;
}

size_t calc_padding_with_header(uintptr_t ptr, uintptr_t alignment, size_t header_size)
{
    uintptr_t p, a, modulo, padding, needed_space;

    DASSERT(is_power_of_two(alignment));

    p      = ptr;
    a      = alignment;
    modulo = p & (a - 1); // (p % a) as it assumes alignment is a power of two

    padding      = 0;
    needed_space = 0;

    if (modulo != 0)
    { // Same logic as 'align_forward'
        padding = a - modulo;
    }

    needed_space = (uintptr_t)header_size;

    if (padding < needed_space)
    {
        needed_space -= padding;

        if ((needed_space & (a - 1)) != 0)
        {
            padding += a * (1 + (needed_space / a));
        }
        else
        {
            padding += a * (needed_space / a);
        }
    }

    return (size_t)padding;
}

void free_list_node_insert(dfreelist_node **phead, dfreelist_node *prev_node, dfreelist_node *new_node)
{
    if (prev_node == NULL)
    {
        if (*phead != NULL)
        {
            new_node->next = *phead;
        }
        else
        {
            *phead = new_node;
        }
    }
    else
    {
        if (prev_node->next == NULL)
        {
            prev_node->next = new_node;
            new_node->next  = NULL;
        }
        else
        {
            new_node->next  = prev_node->next;
            prev_node->next = new_node;
        }
    }
}

void free_list_node_remove(dfreelist_node **phead, dfreelist_node *prev_node, dfreelist_node *del_node)
{
    if (prev_node == NULL)
    {
        *phead = del_node->next;
    }
    else
    {
        prev_node->next = del_node->next;
    }
}

dfreelist_node *free_list_find_first(dfreelist *fl, size_t size, size_t alignment, size_t *padding_,
                                     dfreelist_node **prev_node_)
{
    // Iterates the list and finds the first free block with enough space
    dfreelist_node *node      = fl->head;
    dfreelist_node *prev_node = NULL;

    size_t padding = 0;

    while (node != NULL)
    {
        padding = calc_padding_with_header((uintptr_t)node, (uintptr_t)alignment, sizeof(dfreelist_allocation_header));
        size_t required_space = size + padding;
        if (node->block_size >= required_space)
        {
            break;
        }
        prev_node = node;
        node      = node->next;
    }
    if (padding_)
        *padding_ = padding;
    if (prev_node_)
        *prev_node_ = prev_node;
    return node;
}
dfreelist_node *free_list_find_best(dfreelist *fl, size_t size, size_t alignment, size_t *padding_,
                                    dfreelist_node **prev_node_)
{
    // This iterates the entire list to find the best fit
    // O(n)
    size_t smallest_diff = ~(size_t)0;

    dfreelist_node *node      = fl->head;
    dfreelist_node *prev_node = NULL;
    dfreelist_node *best_node = NULL;

    size_t padding = 0;

    while (node != NULL)
    {
        padding = calc_padding_with_header((uintptr_t)node, (uintptr_t)alignment, sizeof(dfreelist_allocation_header));
        size_t required_space = size + padding;
        if (node->block_size >= required_space && (node->block_size - required_space < smallest_diff))
        {
            best_node = node;
        }
        prev_node = node;
        node      = node->next;
    }
    if (padding_)
        *padding_ = padding;
    if (prev_node_)
        *prev_node_ = prev_node;
    return best_node;
}

void *dfreelist_allocate(dfreelist *fl, u64 size)
{
    size_t                       padding   = 0;
    dfreelist_node              *prev_node = NULL;
    dfreelist_node              *node      = NULL;
    size_t                       alignment_padding, required_space, remaining;
    dfreelist_allocation_header *header_ptr;

    if (size < sizeof(dfreelist_node))
    {
        size = sizeof(dfreelist_node);
    }
    u64 alignment = 8;

    node = free_list_find_first(fl, size, alignment, &padding, &prev_node);

    if (node == NULL)
    {
        DASSERT(0 && "Free list has no free memory");
        return NULL;
    }

    alignment_padding = padding - sizeof(dfreelist_allocation_header);
    required_space    = size + padding;
    remaining         = node->block_size - required_space;

    if (remaining > 0)
    {
        uintptr_t       raw_ptr  = (uintptr_t)((char *)node + required_space);
        dfreelist_node *new_node = (dfreelist_node *)DALIGN_UP(raw_ptr, DFREELIST_PADDING);

        new_node->block_size = remaining;
        free_list_node_insert(&fl->head, node, new_node);
    }

    free_list_node_remove(&fl->head, prev_node, node);

    header_ptr             = (dfreelist_allocation_header *)((char *)node + alignment_padding);
    header_ptr->block_size = required_space;
    header_ptr->padding    = alignment_padding;

    fl->used += required_space;

    return (void *)((char *)header_ptr + sizeof(dfreelist_allocation_header));
}

void free_list_coalescence(dfreelist *fl, dfreelist_node *prev_node, dfreelist_node *free_node);

bool dfreelist_dealocate(dfreelist *fl, void *ptr)
{
    dfreelist_allocation_header *header;
    dfreelist_node              *free_node;
    dfreelist_node              *node;
    dfreelist_node              *prev_node = NULL;

    if (ptr == NULL)
    {
        return false;
    }

    header                = (dfreelist_allocation_header *)((char *)ptr - sizeof(dfreelist_allocation_header));
    free_node             = (dfreelist_node *)header;
    free_node->block_size = header->block_size + header->padding;
    free_node->next       = NULL;

    node = fl->head;
    while (node != NULL)
    {
        if (ptr < node)
        {
            free_list_node_insert(&fl->head, prev_node, free_node);
            prev_node = node;
            node      = node->next;
            break;
        }
        prev_node = node;
        node      = node->next;
    }

    fl->used -= free_node->block_size;

    free_list_coalescence(fl, prev_node, free_node);
    return true;
}

void free_list_coalescence(dfreelist *fl, dfreelist_node *prev_node, dfreelist_node *free_node)
{
    if (free_node->next != NULL && (void *)((char *)free_node + free_node->block_size) == free_node->next)
    {
        free_node->block_size += free_node->next->block_size;
        free_list_node_remove(&fl->head, free_node, free_node->next);
    }

    if (prev_node->next != NULL && (void *)((char *)prev_node + prev_node->block_size) == free_node)
    {
        prev_node->block_size += free_node->next->block_size;
        free_list_node_remove(&fl->head, prev_node, free_node);
    }
}
