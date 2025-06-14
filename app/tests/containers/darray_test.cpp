#include "darray_test.hpp"
#include "../expect.hpp"
#include "../src/containers/darray.hpp"

static void print_array(darray<s32> &array)
{
    s32 length = static_cast<s32>(array.size());

    for (s32 i = 0; i < length; i++)
    {
        printf("%d  ", array[i]);
    }
    printf("\n");
    printf("-------------------------------------------\n");
}

bool darray_create_and_destroy(arena* arena)
{
    darray<s32> array;

    u64         huge_size = 64 * 1024 * 1024;
    darray<s32> huge_array;
    huge_array.c_init(arena, huge_size);

    expect_should_be(array.size(), 0);
    expect_should_be(huge_array.size(), huge_size);

    return true;
}

bool darray_create_and_resize(arena* arena)
{
    darray<s32> array;
    array.reserve(arena);

    s32         size = 100000;
    for (s32 i = 0; i < size; i++)
    {
        array.push_back(i);
    }

    int j = 0;
    for (s32 i = 0; i < size; i++)
    {
        j = array[i];
        expect_should_be(i, j);
    }
    return true;
}

bool darray_pop_back_test(arena* arena)
{
    darray<s32> array;
    s32         size = 100000;
    for (s32 i = 0; i < size; i++)
    {
        array.push_back(i);
    }

    int j = 0;
    for (s32 i = size - 1; i >= 0; i--)
    {
        j = array.pop_back();
        expect_should_be(i, j);
    }
    return true;
}

bool darray_pop_at_test(arena* arena)
{
    darray<s32> array;
    array.c_init(arena, 10);
    for (s32 i = 0; i < 10; i++)
    {
        array[i] = i;
    }

    s32 i = array.pop_at(5);
    expect_should_be(5, i);

    print_array(array);

    i = array.pop_at(8);
    expect_should_be(9, i);
    print_array(array);

    i = array.pop_at(7);
    expect_should_be(8, i);
    print_array(array);

    i = array.pop_at(3);
    expect_should_be(3, i);
    print_array(array);

    i = array.pop_at(3);
    expect_should_be(4, i);
    print_array(array);

    i = array.pop_at(3);
    expect_should_be(6, i);
    print_array(array);

    i = array.pop_at(3);
    expect_should_be(7, i);
    print_array(array);

    i = array.pop_at(1);
    expect_should_be(1, i);
    print_array(array);

    i = array.pop_at(0);
    expect_should_be(0, i);
    print_array(array);

    i = array.pop_at(0);
    expect_should_be(2, i);
    print_array(array);

    return true;
}

void darray_register_tests()
{
    test_manager_register_tests(darray_create_and_destroy, "darray_create");
    test_manager_register_tests(darray_create_and_resize, "darray create and resize");
    test_manager_register_tests(darray_pop_back_test, "darray pop back test");
    test_manager_register_tests(darray_pop_at_test, "darray pop at test");
}
