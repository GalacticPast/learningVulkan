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

    vertex a{.position = {-0.5f, -0.5f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coord = {0.0f, 0.0f}}; // Red
    vertex b{.position = {0.5f, -0.5f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coord = {1.0f, 0.0f}};  // Green
    vertex c{.position = {0.5f, 0.5f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coord = {1.0f, 1.0f}};   // Blue
    vertex d{.position = {-0.5f, 0.5f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coord = {0.0f, 1.0f}};  // Yellow

    vertex e{.position = {-0.5f, -0.5f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coord = {0.0f, 0.0f}}; // Magenta
    vertex f{.position = {0.5f, -0.5f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coord = {1.0f, 0.0f}};  // Cyan
    vertex g{.position = {0.5f, 0.5f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coord = {1.0f, 1.0f}};   // Orange
    vertex h{.position = {-0.5f, 0.5f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .tex_coord = {0.0f, 1.0f}};  // Purple

    darray<vertex> vertices;
    vertices.push_back(a);
    vertices.push_back(b);
    vertices.push_back(c);
    vertices.push_back(d);
    vertices.push_back(e);
    vertices.push_back(f);
    vertices.push_back(g);
    vertices.push_back(h);

    darray<u32> indices(36);
    indices[0] = 0;
    indices[1] = 3;
    indices[2] = 1;
    indices[3] = 3;
    indices[4] = 2;
    indices[5] = 1;

    indices[6]  = 1;
    indices[7]  = 2;
    indices[8]  = 5;
    indices[9]  = 2;
    indices[10] = 6;
    indices[11] = 5;

    indices[12] = 5;
    indices[13] = 6;
    indices[14] = 4;
    indices[15] = 6;
    indices[16] = 7;
    indices[17] = 4;

    indices[18] = 4;
    indices[19] = 7;
    indices[20] = 0;
    indices[21] = 7;
    indices[22] = 3;
    indices[23] = 0;

    indices[24] = 3;
    indices[25] = 7;
    indices[26] = 2;
    indices[27] = 7;
    indices[28] = 6;
    indices[29] = 2;

    indices[30] = 4;
    indices[31] = 0;
    indices[32] = 5;
    indices[33] = 0;
    indices[34] = 1;
    indices[35] = 5;

    u32 s_width;
    u32 s_height;

    platform_get_window_dimensions(&s_width, &s_height);

    f32 fov_rad      = 45 * D_DEG2RAD_MULTIPLIER;
    f32 aspect_ratio = 16.0f / 9.0f;

    uniform_buffer_object global_ubo{};
    global_ubo.view       = mat4_look_at({2, 2, 2}, {0, 0, 0}, {0, 0, 1.0f});
    global_ubo.projection = mat4_perspective(fov_rad, aspect_ratio, 0.01f, 1000.0f);

    render_data triangle{};
    triangle.vertices = vertices;
    triangle.indices  = indices;

    f64 start_time      = 0;
    f64 curr_frame_time = 0;

    f64 req_frame_time = (f64)(1.0f / 240);
    clock_start(&clock);

    while (app_state.is_running)
    {

        clock_update(&clock);
        start_time = clock.time_elapsed;

        global_ubo.model    = mat4_euler_z((start_time * (90.0f * D_DEG2RAD_MULTIPLIER)));
        triangle.global_ubo = global_ubo;

        application_run(&triangle);

        clock_update(&clock);
        curr_frame_time = clock.time_elapsed - start_time;

        if (f32_compare((f32)req_frame_time, (f32)curr_frame_time, (f32)req_frame_time))
        {
            f64 sleep = fabs(req_frame_time - curr_frame_time);
            platform_sleep((u64)sleep * D_SEC_TO_MS_MULTIPLIER);
        }
    }
    application_shutdown();
}
