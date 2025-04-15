#include "containers/darray.hpp"
#include "core/application.hpp"
#include "core/dasserts.hpp"
#include "core/dclock.hpp"
#include "core/logger.hpp"
#include "defines.hpp"
#include "platform/platform.hpp"

#include "../tests/containers/darray_test.hpp"
#include "../tests/events/event_system_test.hpp"
#include "../tests/linear_allocator/linear_allocator_test.hpp"
#include "../tests/test_manager.hpp"

void run_tests()
{
    u64 test_manager_mem_requirements = 0;

    test_manager_initialize(&test_manager_mem_requirements, nullptr);
    test_manager *test_instance = (test_manager *)malloc(test_manager_mem_requirements);
    darray<test>  test_array;
    test_instance->tests = &test_array;
    test_manager_initialize(&test_manager_mem_requirements, test_instance);

    // linear_allocator_register_tests();
    // event_system_register_tests();
    darray_register_tests();

    test_manager_run_tests();
    test_instance->tests = 0;
    test_instance        = 0;
}

int main()
{
    application_config app_config;
    app_config.width            = INVALID_ID;
    app_config.height           = INVALID_ID;
    app_config.x                = INVALID_ID;
    app_config.y                = INVALID_ID;
    app_config.application_name = "Learning Vulkan";

    dclock clock{};

    application_state app_state;

    bool result = application_initialize(&app_state, &app_config);
    if (!result)
    {
        DFATAL("Application initialization failed");
        return 2;
    }

    f64 start_time      = 0;
    f64 curr_frame_time = 0;
    f64 end_time        = 0;

    f64 req_frame_time  = 1.0f / 60;

    while (app_state.is_running)
    {

        clock_update(&clock);
        start_time = clock.start_time;
        application_run();
        clock_update(&clock);
        end_time        = clock.start_time;

        curr_frame_time = end_time - start_time;

        if (curr_frame_time < req_frame_time)
        {
            f64 sleep_ns = req_frame_time - curr_frame_time - 0.0000000001;
            platform_sleep(sleep_ns);
        }
    }

    application_shutdown();
}
