#include "containers/dhashtable.hpp"
#include "defines.hpp"
#include "dfreelist.hpp"

#define MAX_FREELIST_ALLOCATED_IDS 16384
#define DEFAULT_FREELIST_ALIGNMENT 8

struct dfreelist_state
{
    dhashtable<u32> allocated_ids;
};

static dfreelist_state *dfreelist_state_ptr = nullptr;

inline void *align_up(void *ptr, u32 alignment)
{
    DASSERT((alignment & (alignment - 1)) == 0); // alignment must be power of 2
    uintptr_t raw     = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t aligned = (raw + alignment - 1) & ~(uintptr_t(alignment) - 1);
    return reinterpret_cast<void *>(aligned);
}

void _dfreelist_add_node(dfreelist *dfreelist, dfreelist_node *prev, dfreelist_node *new_node, dfreelist_node *next);
void _dfreelist_remove_node(dfreelist *dfreelist, dfreelist_node *prev, dfreelist_node *node_to_remove,
                            dfreelist_node *next);

dfreelist *dfreelist_create(u64 *dfreelist_mem_requirements, u64 memory_size, void *memory)
{
    DASSERT_MSG(
        !dfreelist_state_ptr,
        "Dfreelist state ptr is already initialized. If you want to allocate another freelist then you have to change "
        "the code to support multiple freelists. Man just learn arenas, why do you need a second freelist??.");
    *dfreelist_mem_requirements = sizeof(dfreelist_state);
    if (!memory)
    {
        return nullptr;
    }
    DASSERT(memory_size != INVALID_ID_64);

    dfreelist_state_ptr = static_cast<dfreelist_state *>(memory);
    u64 hashtable_size  = dfreelist_state_ptr->allocated_ids.get_size_requirements(MAX_FREELIST_ALLOCATED_IDS);

    dfreelist_state *ptr = dfreelist_state_ptr;

    dfreelist_allocation_header* hashtable_mem_header = reinterpret_cast<dfreelist_allocation_header *>(static_cast<u8 *>(memory) + sizeof(dfreelist_state));
    hashtable_mem_header->block_size = hashtable_size;
    hashtable_mem_header->padding = INVALID_ID_64;

    u8   *hashtable_mem = reinterpret_cast<u8 *>(hashtable_mem_header) + sizeof(dfreelist_allocation_header);
    void *mem           = align_up(hashtable_mem, DEFAULT_FREELIST_ALIGNMENT);

    s32 diff_bytes = reinterpret_cast<uintptr_t>(mem) - reinterpret_cast<uintptr_t>(hashtable_mem);
    DASSERT(diff_bytes >= 0);

    ptr->allocated_ids.c_init(mem, MAX_FREELIST_ALLOCATED_IDS);
    ptr->allocated_ids.is_non_resizable = true;

    void      *raw_ptr = reinterpret_cast<u8 *>(mem) + hashtable_size;
    dfreelist *list    = reinterpret_cast<dfreelist *>(align_up(raw_ptr, DEFAULT_FREELIST_ALIGNMENT));

    diff_bytes += reinterpret_cast<uintptr_t>(list) - reinterpret_cast<uintptr_t>(raw_ptr);

    list->data = reinterpret_cast<void *>(reinterpret_cast<u8 *>(list) + sizeof(dfreelist));
    list->size = memory_size - hashtable_size - diff_bytes - sizeof(dfreelist_state);

    list->used = 0;
    list->head = static_cast<dfreelist_node *>(list->data);

    list->head->next       = nullptr;
    list->head->block_size = list->size - sizeof(dfreelist) - sizeof(dfreelist_node);

    return list;
}

bool dfreelist_destroy(dfreelist *freelist)
{
    u64 size = 0;
    u64 chain_length = 0;
    dfreelist_node* curr = freelist->head;

    while(curr != nullptr)
    {
        size += curr->block_size;
        curr = curr->next;
        chain_length++;
    }
    // should check if the final size is the same

    u64 num_elements = dfreelist_state_ptr->allocated_ids.num_elements();
    DASSERT(num_elements != INVALID_ID_64);
    if(num_elements > 0)
    {
        DERROR("Still havent freed %lld allocations", num_elements);
    }
    dfreelist_state_ptr = nullptr;
    return true;
}

void *dfreelist_allocate(dfreelist *dfreelist, u64 mem_size)
{
    DASSERT(dfreelist);

    static u64 num = 0;

    dfreelist_node *prev = nullptr;
    dfreelist_node *curr = dfreelist->head;
    DASSERT_MSG(curr, "Head is missing for the freelist");

    u64 required_size = mem_size + sizeof(dfreelist_node);

    while (curr != nullptr && curr->block_size < required_size)
    {
        prev = curr;
        DASSERT(curr->padding == INVALID_ID_64);
        curr = curr->next;
    }
    DASSERT_MSG(curr, "There is no more space in the freelist");
    _dfreelist_remove_node(dfreelist, prev, curr, curr->next);

    dfreelist_node *new_node_raw =
        reinterpret_cast<dfreelist_node *>((reinterpret_cast<uintptr_t>(curr) + required_size));
    dfreelist_node *new_node = static_cast<dfreelist_node *>(align_up(new_node_raw, DEFAULT_FREELIST_ALIGNMENT));

    u32 diff_bytes  = reinterpret_cast<uintptr_t>(new_node) - reinterpret_cast<uintptr_t>(new_node_raw);
    required_size  += diff_bytes;

    // add the node
    new_node->block_size = curr->block_size - required_size;
    new_node->padding    = INVALID_ID_64;
    _dfreelist_add_node(dfreelist, prev, new_node, curr->next);
    //

    dfreelist_allocation_header *header = reinterpret_cast<dfreelist_allocation_header *>(curr);
    header->block_size                  = mem_size;
    header->padding                     = INVALID_ID_64;
    header->allocation_id               = dfreelist_state_ptr->allocated_ids.insert(INVALID_ID_64, num);
    num++;
    void *block = reinterpret_cast<void *>(reinterpret_cast<u8 *>(header) + sizeof(dfreelist_allocation_header));

    dfreelist->used += mem_size;

    return block;
}

bool dfreelist_dealocate(dfreelist *dfreelist, void *ptr)
{
    dfreelist_allocation_header *header =
        reinterpret_cast<dfreelist_allocation_header *>(static_cast<u8 *>(ptr) - sizeof(dfreelist_allocation_header));
    DASSERT(header->padding == INVALID_ID_64);

    u64 header_block_size    = header->block_size;
    u64 allocation_id = header->allocation_id;

    dfreelist_node *new_node = reinterpret_cast<dfreelist_node *>(header);

    dfreelist_node *prev = nullptr;
    dfreelist_node *curr = dfreelist->head;

    while (curr != nullptr && curr < new_node)
    {
        prev = curr;
        curr = curr->next;
    }
    DASSERT_MSG(curr,
                "Coulndt find the block. This could indicate a very serious issue with the freelist implementation");

    new_node->block_size = header_block_size;
    new_node->padding    = INVALID_ID_64;
    _dfreelist_add_node(dfreelist, prev, new_node, curr);
    bool result = dfreelist_state_ptr->allocated_ids.erase(allocation_id);
    DASSERT(result);

    // TODO: coalese blocks
    if (prev)
    {
        uintptr_t new_ptr  = reinterpret_cast<uintptr_t>(prev) + prev->block_size;
        new_ptr           += sizeof(dfreelist_node);
        if (new_ptr == reinterpret_cast<uintptr_t>(new_node))
        {
            prev->block_size  += new_node->block_size;
            prev->block_size  += sizeof(dfreelist_node);
            uintptr_t new_ptr  = reinterpret_cast<uintptr_t>(prev) + prev->block_size;
            new_ptr           += sizeof(dfreelist_node);
            if (new_ptr == reinterpret_cast<uintptr_t>(new_node->next))
            {
                prev->block_size += new_node->next->block_size;
                prev->block_size += sizeof(dfreelist_node);
                //TODO: verify this
                _dfreelist_remove_node(dfreelist, new_node, new_node->next, new_node->next->next);
            }
            _dfreelist_remove_node(dfreelist, prev, new_node, new_node->next);
        }
    }
    else
    {
        uintptr_t new_ptr  = reinterpret_cast<uintptr_t>(new_node) + new_node->block_size;
        new_ptr           += sizeof(dfreelist_node);
        if (new_ptr == reinterpret_cast<uintptr_t>(new_node->next))
        {
            new_node->block_size += new_node->next->block_size;
            new_node->block_size += sizeof(dfreelist_node);
            _dfreelist_remove_node(dfreelist, new_node, new_node->next, new_node->next->next);
        }
    }

    dfreelist->used -= header_block_size;

    return true;
}

void _dfreelist_remove_node(dfreelist *dfreelist, dfreelist_node *prev, dfreelist_node *node_to_remove,
                            dfreelist_node *next)
{
    // this means that this is the new head for the list
    if (prev == nullptr)
    {
        dfreelist->head = next;
    }
    else
    {
        prev->next = next;
    }
}

void _dfreelist_add_node(dfreelist *dfreelist, dfreelist_node *prev, dfreelist_node *new_node, dfreelist_node *next)
{
    if (prev == nullptr)
    {
        dfreelist->head = new_node;
        new_node->next  = next;
    }
    else
    {
        prev->next     = new_node;
        new_node->next = next;
    }
}
