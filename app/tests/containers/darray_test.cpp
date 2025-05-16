#include "darray_test.hpp"
#include "../expect.hpp"
#include "../src/containers/darray.hpp"

bool darray_create_and_destroy()
{
    darray<s32> array;

    u64         huge_size = 64 * 1024 * 1024;
    darray<s32> huge_array(huge_size);

    expect_should_be(array.size(), 0);
    expect_should_be(huge_array.size(), huge_size);

    return true;
}

bool darray_create_and_resize()
{
    darray<s32> array;
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

bool darray_pop_back_test()
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

void print_array(darray<s32> &array)
{
    s32 length = (s32)array.size();

    for (s32 i = 0; i < length; i++)
    {
        DDEBUG("%d", array[i]);
    }
}

bool darray_pop_at_test()
{
    darray<s32> array(10);

    for (s32 i = 0; i < 10; i++)
    {
        array[i] = i;
    }

    s32 i = array.pop_at(5);
    expect_should_be(5, i);

    i = array.pop_at(8);
    expect_should_be(9, i);

    i = array.pop_at(7);
    expect_should_be(8, i);

    i = array.pop_at(3);
    expect_should_be(3, i);

    i = array.pop_at(3);
    expect_should_be(4, i);

    i = array.pop_at(3);
    expect_should_be(6, i);

    i = array.pop_at(3);
    expect_should_be(7, i);

    i = array.pop_at(1);
    expect_should_be(1, i);

    i = array.pop_at(0);
    expect_should_be(0, i);

    i = array.pop_at(0);
    expect_should_be(2, i);

    return true;
}

void darray_register_tests()
{
    test_manager_register_tests(darray_create_and_destroy, "darray_create");
    test_manager_register_tests(darray_create_and_resize, "darray create and resize");
    test_manager_register_tests(darray_pop_back_test, "darray pop back test");
    test_manager_register_tests(darray_pop_at_test, "darray pop at test");
}
