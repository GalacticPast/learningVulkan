#pragma once
#include <defines.hpp>

#include <string>
#include <vector>

// function pointer for tests.
typedef bool (*test_func_ptr)();

struct test_manager
{
    u64 passed;
    u64 failed;

    std::vector<test_func_ptr> test_func_ptrs;
    std::vector<std::string>   test_func_descriptions;
};
