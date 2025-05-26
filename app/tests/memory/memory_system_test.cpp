#include "../expect.hpp"
#include "../test_manager.hpp"
#include "core/dmemory.hpp"

bool memory_system_test()
{
    void *test_inst;

    u64 memory_system_mem_requirements = 0;

    memory_system_startup(&memory_system_mem_requirements, 0);
    test_inst = malloc(memory_system_mem_requirements);
    memory_system_startup(&memory_system_mem_requirements, test_inst);

    u64 buffer_usg_mem_requirements = 0;
    get_memory_usg_str(&buffer_usg_mem_requirements, static_cast<char *>(0));
    darray<char *> buffer(buffer_usg_mem_requirements);
    get_memory_usg_str(&buffer_usg_mem_requirements, reinterpret_cast<char *>(buffer.data));

    return true;
}

void memory_system_register_tests(test_manager *instance)
{
    test_manager_register_tests(memory_system_test, "memory_system_allocation_and_deallocation_test");
}
