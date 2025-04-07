#include "../expect.hpp"
#include "../test_manager.hpp"
#include "core/dmemory.hpp"

bool memory_system_test()
{
    void *test_inst;

    u64 memory_system_mem_requirements = 0;

    memory_system_initialze(nullptr, &memory_system_mem_requirements);
    test_inst = malloc(memory_system_mem_requirements);
    memory_system_initialze(test_inst, &memory_system_mem_requirements);

    u64   size  = 64 * 1024 * 1024;
    void *block = dallocate(size, MEM_TAG_UNKNOWN);

    char *buffer = get_memory_usg_str();
    DDEBUG("%s", buffer);
    dfree(block, size, MEM_TAG_UNKNOWN);
    free(buffer);

    return true;
}

void memory_system_register_tests(test_manager *instance)
{
    test_manager_register_tests(memory_system_test, "memory_system_allocation_and_deallocation_test");
}
