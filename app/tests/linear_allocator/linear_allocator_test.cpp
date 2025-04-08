#include "linear_allocator_test.hpp"
#include "../expect.hpp"
#include "memory/linear_allocator.hpp"

bool linear_allocator_allocate_and_destroy_test()
{
    linear_allocator test_allocator;

    u64 size = 64 * 1024 * 1024; // 64 mbs
    linear_allocator_create(&test_allocator, size);

    expect_should_be(size, test_allocator.total_size);
    expect_should_be(0, test_allocator.total_allocated);
    expect_should_be(0, test_allocator.num_allocations);
    expect_should_be(test_allocator.memory, test_allocator.current_free_mem_ptr);

    linear_allocator_destroy(&test_allocator);
    expect_should_be(0, test_allocator.total_size);
    expect_should_be(0, test_allocator.total_allocated);
    expect_should_be(0, test_allocator.num_allocations);
    expect_should_be(0, test_allocator.current_free_mem_ptr);
    expect_should_be(0, test_allocator.memory);

    return true;
}

bool linear_allocator_allocate_overallocate_space_test()
{
    linear_allocator test_allocator;

    u64 size = 64 * 1024 * 1024; // 64 mbs
    linear_allocator_create(&test_allocator, size);

    DWARN("Intentionlly casusing the following error.");
    void *memory = linear_allocator_allocate(&test_allocator, size + size);

    expect_should_not_be(size + size, test_allocator.total_allocated);
    expect_should_be(memory, nullptr);

    linear_allocator_destroy(&test_allocator);

    return true;
}

bool linear_allocator_allocate_all_space_in_single_test()
{
    linear_allocator test_allocator;

    u64 size = 64 * 1024 * 1024; // 64 mbs
    linear_allocator_create(&test_allocator, size);

    void *memory = (u8 *)(linear_allocator_allocate(&test_allocator, size));

    expect_should_be(size, test_allocator.total_size);
    expect_should_be(size, test_allocator.total_allocated);
    expect_should_be(memory, (u8 *)test_allocator.current_free_mem_ptr - size);

    linear_allocator_destroy(&test_allocator);

    return true;
}

bool linear_allocator_allocate_all_test()
{
    linear_allocator test_allocator;

    u64 size = 64 * 1024 * 1024; // 64 mbs
    linear_allocator_create(&test_allocator, size);

    u64 chunk_length = 1024;
    u64 length       = size / chunk_length;

    u8 *mem_ptrs[65536];

    void *expected = test_allocator.memory;
    for (u64 i = 0; i < length; i++)
    {
        mem_ptrs[i] = (u8 *)(linear_allocator_allocate(&test_allocator, chunk_length));
        expect_should_be(expected, mem_ptrs[i]);
        expected = (u8 *)expected + chunk_length;
    }

    expect_should_be(size, test_allocator.total_size);
    expect_should_be(size, test_allocator.total_allocated);
    expect_should_be(length, test_allocator.num_allocations);
    expect_should_be((u8 *)test_allocator.memory + size, test_allocator.current_free_mem_ptr);

    linear_allocator_destroy(&test_allocator);

    return true;
}

void linear_allocator_register_tests()
{
    test_manager_register_tests(linear_allocator_allocate_and_destroy_test, "Linear allocator and destroy test");
    test_manager_register_tests(linear_allocator_allocate_all_test, "Linear allocator all test");
    test_manager_register_tests(linear_allocator_allocate_all_space_in_single_test, "Linear allocator all space single test");
    test_manager_register_tests(linear_allocator_allocate_overallocate_space_test, "Linear allocator over allocate space test");
}
