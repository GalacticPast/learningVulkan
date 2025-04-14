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
    for (s32 i = size; i >= 0; i--)
    {
        j = array.pop_back();
        expect_should_be(i, j);
    }
    return true;
}

void darray_register_tests()
{
    test_manager_register_tests(darray_create_and_destroy, "darray_create");
    test_manager_register_tests(darray_create_and_resize, "darray create and resize");
}
