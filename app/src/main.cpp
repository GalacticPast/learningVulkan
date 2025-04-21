#include "containers/darray.hpp"
#include "core/application.hpp"
#include "core/dasserts.hpp"
#include "core/dclock.hpp"
#include "core/dstring.hpp"
#include "core/input.hpp"
#include "core/logger.hpp"

#include "defines.hpp"

#include "math/dmath.hpp"

#include "platform/platform.hpp"

#include "resource/texture_system.hpp"

//// #include "../tests/containers/hashtable_test.hpp"
// #include "../tests/containers/dhashtable_test.hpp"
// #include "../tests/events/event_system_test.hpp"
// #include "../tests/linear_allocator/linear_allocator_test.hpp"
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
    // darray_register_tests();
    // dhashtable_register_tests();

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

    darray<vertex> vertices;
    vertices.push_back(a);
    vertices.push_back(b);
    vertices.push_back(c);
    vertices.push_back(d);

    darray<u32> indices(6);
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 3;
    indices[3] = 3;
    indices[4] = 1;
    indices[5] = 2;

    u32 s_width;
    u32 s_height;

    platform_get_window_dimensions(&s_width, &s_height);

    f32 fov_rad      = 45 * D_DEG2RAD_MULTIPLIER;
    f32 aspect_ratio = 16.0f / 9.0f;

    uniform_buffer_object global_ubo{};
    global_ubo.view       = mat4_look_at({2, 2, 2}, {0, 0, 0}, {0, 0, 1.0f});
    global_ubo.projection = mat4_perspective(fov_rad, aspect_ratio, 0.01f, 1000.0f);

    const char *texture_names[5] = {"DEFAULT_TEXTURE", "texture.jpg", "paving.png", "paving2.png", "cobblestone.png"};

    dstring file_name;
    file_name = texture_names[1];
    texture_system_create_texture(&file_name);
    file_name = texture_names[2];
    texture_system_create_texture(&file_name);
    file_name = texture_names[3];
    texture_system_create_texture(&file_name);
    file_name = texture_names[4];
    texture_system_create_texture(&file_name);

    render_data triangle{};
    triangle.vertices = vertices;
    triangle.indices  = indices;
    triangle.texture  = new texture();

    f64 start_time = 0;
    f64 end_time   = 0;

    f64 req_frame_time = (f64)(1.0f / 60);
    clock_start(&clock);

    u32 index = 0;
    while (app_state.is_running)
    {

        clock_update(&clock);
        start_time = clock.time_elapsed;

        global_ubo.model    = mat4_euler_z((start_time * (90.0f * D_DEG2RAD_MULTIPLIER)));
        triangle.global_ubo = global_ubo;

        application_run(&triangle);

        bool a = input_is_key_up(KEY_T);
        bool b = input_was_key_down(KEY_T);
        if (b && a)
        {
            texture_system_get_texture(texture_names[index], triangle.texture);
            index++;
            index %= 5;
        }

        clock_update(&clock);
        end_time = clock.time_elapsed - start_time;

        if (!f32_compare(req_frame_time, end_time, 0.0001))
        {
            f64 sleep    = req_frame_time - end_time;
            u64 sleep_ms = (u64)(sleep * D_SEC_TO_MS_MULTIPLIER);
            if (sleep_ms > 0)
            {
                platform_sleep(sleep_ms);
            }
        }
        input_update(0);
    }
    application_shutdown();
}
