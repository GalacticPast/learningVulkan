#pragma once
#include "containers/darray.hpp"
#include <defines.hpp>

#include <string>

// function pointer for tests.
typedef bool (*test_func_ptr)(arena *arena);

struct test
{
    test_func_ptr test;
    const char   *desc;
};

struct test_manager
{
    u64           passed;
    u64           failed;
    arena        *arena;
    darray<test> *tests;
};

bool test_manager_initialize(arena *arena, u64 *test_manager_memory_requirements, void *state);

void test_manager_register_tests(test_func_ptr, const char *func_description);
void test_manager_run_tests();
