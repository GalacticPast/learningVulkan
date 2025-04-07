#include "test_manager.hpp"
#include "core/logger.hpp"
#include <core/dmemory.hpp>

static test_manager *instance_ptr;

bool test_manager_initialize(u64 *test_manager_memory_requirements, void *state)
{
    *test_manager_memory_requirements = sizeof(test_manager);
    if (!state)
    {
        return true;
    }
    instance_ptr        = (test_manager *)state;
    instance_ptr->tests = std::vector<test>();

    return true;
}

void test_manager_register_tests(bool (*test_func_ptr)(), const char *func_description)
{
    test new_test;
    new_test.test = test_func_ptr;
    new_test.desc = func_description;

    instance_ptr->tests.push_back(new_test);
}

void test_manager_run_tests()
{
    u64 test_func_length = instance_ptr->tests.size();

    const char *status[2] = {"FAILED", "PASSED"};
    for (u64 i = 0; i < test_func_length; i++)
    {
        bool result = instance_ptr->tests[i].test();
        DDEBUG("Desc: %s, result: %s", instance_ptr->tests[i].desc, status[result]);
    }
}
