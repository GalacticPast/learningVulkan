#include "darray_test.hpp"
#include "../expect.hpp"
#include "../src/containers/darray.hpp"
#include <vector>

bool darray_create_and_destroy()
{
    darray<s32> array;

    u64         huge_size = 64 * 1024 * 1024;
    darray<s32> huge_array(huge_size);

    expect_should_be(array.size(), 0);
    expect_should_be(huge_array.size(), (huge_size / sizeof(s32)));

    return true;
}

bool darray_create_and_resize()
{
    darray<s32> array;
    u32         size = 100000;
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

bool darray_pop_back_test()
{
    darray<s32> array;
    u32         size = 100000;
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

bool darray_pop_at_test()
{
    darray<s32> array(10);

    for (s32 i = 0; i < 10; i++)
    {
        array[i] = i;
    }

    array.pop_at(0);
    array.pop_at(5);
    array.pop_at(8);
    array.pop_at(7);
    array.pop_at(3);

    s32 array_size = array.size();

    for (s32 i = 0; i < array_size; i++)
    {
        s32 num = array[i];
        DDEBUG("%d", num);
    }

    return true;
}

void darray_register_tests()
{
    test_manager_register_tests(darray_create_and_destroy, "darray_create");
    test_manager_register_tests(darray_create_and_resize, "darray create and resize");
    test_manager_register_tests(darray_pop_back_test, "darray pop back test");
    test_manager_register_tests(darray_pop_at_test, "darray pop at test");
}
