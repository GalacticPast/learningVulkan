#include "application.hpp"
#include "defines.hpp"

#include <iostream>
#include <memory>

#include "../tests/memory/memory_system_test.hpp"
#include "../tests/test_manager.hpp"

void run_tests()
{
    u64 test_manager_mem_requirements = 0;

    test_manager_initialize(&test_manager_mem_requirements, nullptr);
    test_manager test_instance = *(test_manager *)malloc(test_manager_mem_requirements);
    test_manager_initialize(&test_manager_mem_requirements, &test_instance);

    memory_system_register_tests(&test_instance);

    test_manager_run_tests();
}

int main()
{
    application_state app_state = {};

    run_tests();
}
