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

    linear_allocator_free_all(&test_allocator);
    expect_should_be(0, test_allocator.total_size);
    expect_should_be(0, test_allocator.total_allocated);
    expect_should_be(0, test_allocator.num_allocations);
    expect_should_be(0, test_allocator.current_free_mem_ptr);
    expect_should_be(0, test_allocator.memory);

    return true;
}

bool linear_allocator_allocator_full_test()
{
    linear_allocator test_allocator;

    u64 size = 64 * 1024 * 1024; // 64 mbs
    linear_allocator_create(&test_allocator, size);

    u64 length = size / 1000;

    u8 mem_ptrs[length];

    for (int i = 0; i < length; i++)
    {
    }

    expect_should_be(size, test_allocator.total_size);
    expect_should_be(0, test_allocator.total_allocated);
    expect_should_be(0, test_allocator.num_allocations);
    expect_should_be(test_allocator.memory, test_allocator.current_free_mem_ptr);

    return true;
}

void linear_allocator_register_tests()
{
    test_manager_register_tests(linear_allocator_allocate_and_destroy_test, "Linear allocator and destroy test.");
}
