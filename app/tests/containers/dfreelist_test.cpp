#include "dfreelist_test.hpp"

#include "../expect.hpp"

#include "platform/platform.hpp"

#include "containers/dfreelist.hpp"

bool dfreelist_create_and_destroy_test()
{
    u64 freelist_mem_requirements = INVALID_ID_64;
    dfreelist_create(&freelist_mem_requirements, 0, 0);

    void      *mem      = platform_allocate(GB(1), false);
    dfreelist *freelist = dfreelist_create(&freelist_mem_requirements, GB(1), mem);

    u64 one_gib = GB(1);
    expect_should_be(one_gib, freelist->memory_size);

    dfreelist_destroy(freelist);

    return true;
}

static u32 fib_sequence(u32 *array, u32 index)
{
    if (index == 0 || index == 1)
        return 1;
    return array[index - 1] + array[index - 2];
}

bool dfreelist_allocate_and_deallocate_test()
{
    u64 freelist_mem_requirements = INVALID_ID_64;
    dfreelist_create(&freelist_mem_requirements, 0, 0);

    void      *mem      = platform_allocate(GB(1), false);
    dfreelist *freelist = dfreelist_create(&freelist_mem_requirements, GB(1), mem);

    darray<u32 *> blocks;
    blocks.c_init();

    u32 array_size      = 10000;
    u32 array_byte_size = array_size * sizeof(u32);

    u32 count = GB(1) / array_byte_size - 10000;

    u32 *fib_sec = (u32 *)dfreelist_allocate(freelist, array_byte_size);

    for (u32 i = 0; i < array_size; i++)
    {
        fib_sec[i] = fib_sequence(fib_sec, i);
    }
    DTRACE("count %d", count);

    while (count--)
    {
        u32 *array = (u32 *)dfreelist_allocate(freelist, array_byte_size);

        for (u32 i = 0; i < array_size; i++)
        {
            array[i] = fib_sequence(array, i);
        }

        blocks.push_back(array);
    }

    count = GB(1) / array_byte_size - 10000;
    while (count--)
    {
        u32                               *array = blocks[count];
        dfreelist_allocated_memory_header *header =
            (dfreelist_allocated_memory_header *)((u8 *)(array) - sizeof(dfreelist_allocated_memory_header));
        expect_should_be_msg(array_byte_size, header->block_size, count);

        for (u32 i = 0; i < array_size; i++)
        {
            expect_should_be(fib_sec[i], array[i]);
        }
    }

    count = GB(1) / array_byte_size - 10000;
    while (count--)
    {
        u32 *block = blocks[count];
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
