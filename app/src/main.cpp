#include "containers/darray.hpp"
#include "core/application.hpp"
#include "core/dasserts.hpp"
#include "core/dclock.hpp"
#include "core/logger.hpp"
#include "defines.hpp"
#include "math/dmath.hpp"
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

    vertex a{.position = {-0.5f, -0.5f}, .color = {1.0f, 0.0f, 0.0f}};
    vertex b{.position = {0.5f, -0.5f}, .color = {1.0f, 1.0f, 0.0f}};
    vertex c{.position = {0.5f, 0.5f}, .color = {1.0f, 0.0f, 1.0f}};
    vertex d{.position = {-0.5f, 0.5f}, .color = {0.0f, 0.0f, 1.0f}};

    darray<vertex> vertices;
    vertices.push_back(a);
    vertices.push_back(b);
    vertices.push_back(c);
    vertices.push_back(d);

    darray<u32> indices(6);

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 2;
    indices[4] = 3;
    indices[5] = 0;

    u32 s_width;
    u32 s_height;

    platform_get_window_dimensions(&s_width, &s_height);

    f32 fov_rad      = 45 * D_DEG2RAD_MULTIPLIER;
    f32 aspect_ratio = (f32)s_width / (f32)s_height;

    uniform_buffer_object global_ubo{};
    global_ubo.view       = mat4_look_at({2, 2, 2}, {0, 0, 0}, {0, 0, 1.0f});
    global_ubo.projection = mat4_perspective(fov_rad, aspect_ratio, 0.01f, 1000.0f);

    render_data triangle{};
    triangle.vertices = &vertices;
    triangle.indices  = &indices;

    f64 start_time      = 0;
    f64 curr_frame_time = 0;

    f64 req_frame_time = 1.0f / 60;
    clock_start(&clock);

    while (app_state.is_running)
    {

        clock_update(&clock);
        start_time = clock.time_elapsed;

        global_ubo.model    = mat4_euler_z(start_time * (90.0f * D_DEG2RAD_MULTIPLIER));
        triangle.global_ubo = global_ubo;

        application_run(&triangle);

        clock_update(&clock);
        curr_frame_time = clock.time_elapsed - start_time;

        if (f32_compare(req_frame_time, curr_frame_time, req_frame_time))
        {
            f64 sleep = fabs(req_frame_time - curr_frame_time);
            platform_sleep(sleep * D_SEC_TO_MS_MULTIPLIER);
        }
    }
    application_shutdown();
}
