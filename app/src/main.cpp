#include "core/application.hpp"
#include "core/dasserts.hpp"
#include "core/logger.hpp"
#include "defines.hpp"
#include "platform/platform.hpp"

#include "../tests/linear_allocator/linear_allocator_test.hpp"
#include "../tests/test_manager.hpp"

void run_tests()
{
    u64 test_manager_mem_requirements = 0;

    test_manager_initialize(&test_manager_mem_requirements, nullptr);
    test_manager test_instance = *(test_manager *)malloc(test_manager_mem_requirements);
    test_manager_initialize(&test_manager_mem_requirements, &test_instance);

    linear_allocator_register_tests();

    test_manager_run_tests();
}

int main()
{
    application_config app_config;
    app_config.x                = 0;
    app_config.y                = 0;
    app_config.height           = 1280;
    app_config.width            = 720;
    app_config.application_name = "Learning Vulkan";

    application_state app_state;
    bool              result = application_initialize(&app_state, &app_config);
    if (!result)
    {
        DFATAL("Application initialization failed");
        return 2;
    }

    while (true)
    {
        platform_pump_messages();
    }
}
