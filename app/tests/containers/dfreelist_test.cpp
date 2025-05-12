#include "dfreelist_test.hpp"

#include "../expect.hpp"

#include "platform/platform.hpp"

#include "containers/dfreelist.hpp"

bool dfreelist_create_and_destroy_test()
{
    u64 freelist_mem_requirements = INVALID_ID_64;
    dfreelist_create(&freelist_mem_requirements, 0, 0);

    void      *mem      = platform_allocate(GIGA(1), false);
    dfreelist *freelist = dfreelist_create(&freelist_mem_requirements, GIGA(1), mem);

    u64 one_gib = GIGA(1);
    expect_should_be(one_gib, freelist->memory_size);

    dfreelist_destroy(freelist);

    return true;
}

bool dfreelist_allocate_and_deallocate_test()
{
    u64 freelist_mem_requirements = INVALID_ID_64;
    dfreelist_create(&freelist_mem_requirements, 0, 0);

    void      *mem      = platform_allocate(GIGA(1), false);
    dfreelist *freelist = dfreelist_create(&freelist_mem_requirements, GIGA(1), mem);

    darray<void *> blocks;
    blocks.c_init();

    u32 count = 1000;
    while (count--)
    {
        void *block = dfreelist_allocate(freelist, 10000);
        blocks.push_back(block);
    }
    count = 1000;
    while (count--)
    {
        void                              *block = blocks[count];
        dfreelist_allocated_memory_header *header =
            (dfreelist_allocated_memory_header *)((u8 *)block - sizeof(dfreelist_allocated_memory_header));
        expect_should_be(10000, header->block_size);
    }

    count = 1000;
    while (count--)
    {
        void *block = blocks[count];
        dfreelist_dealocate(freelist, block);
    }

    dfreelist_destroy(freelist);

    return true;
}

bool dfreelist_over_allocate_test()
{
    return true;
}

void dfreelist_register_tests()
{
    test_manager_register_tests(dfreelist_create_and_destroy_test, "Freelist create and destroy test");
    test_manager_register_tests(dfreelist_allocate_and_deallocate_test, "Freelist allocate and deallocate test");
    // test_manager_register_tests(dfreelist_create_and_destroy_test, "Freelist create and destroy test");
}
