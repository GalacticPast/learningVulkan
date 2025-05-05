
#include "core/application.hpp"
#include "core/dclock.hpp"
#include "core/dmemory.hpp"
#include "core/input.hpp"
#include "core/logger.hpp"

#include "containers/darray.hpp"

#include "defines.hpp"

#include "math/dmath.hpp"

#include "platform/platform.hpp"

#include "resources/geometry_system.hpp"
#include "resources/material_system.hpp"
#include "resources/resource_types.hpp"
#include "resources/texture_system.hpp"

//// #include "../tests/containers/hashtable_test.hpp"
// #include "../tests/containers/dhashtable_test.hpp"
// #include "../tests/events/event_system_test.hpp"
// #include "../tests/linear_allocator/linear_allocator_test.hpp"
#include "../tests/test_manager.hpp"

void update_camera(uniform_buffer_object *ubo, f64 start_time);

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

    u32 s_width;
    u32 s_height;

    platform_get_window_dimensions(&s_width, &s_height);

    f32 fov_rad      = 45 * D_DEG2RAD_MULTIPLIER;
    f32 aspect_ratio = (f32)s_width / s_height;

    uniform_buffer_object global_ubo{};
    global_ubo.view       = mat4_look_at({2, 2, 2}, {0, 0, 0}, {0, 0, 1.0f});
    global_ubo.projection = mat4_perspective(fov_rad, aspect_ratio, 0.01f, 1000.0f);

    const char *geometry_names[6] = {"cube.obj",   "torus.obj",    "ico_sphere.obj",
                                     "sphere.obj", "cylinder.obj", "cone.obj"};

    geometry_config plane_config =
        geometry_system_generate_plane_config(10, 5, 5, 5, 5, 5, "its_a_plane", DEFAULT_MATERIAL_HANDLE);

    render_data triangle{};

    geometry *sponza_geo     = nullptr;
    u32       geometry_count = INVALID_ID;
    geometry_system_get_sponza_geometries(&sponza_geo, &geometry_count);
    triangle.test_geometry  = sponza_geo;
    triangle.geometry_count = geometry_count;

    triangle.global_ubo = global_ubo;

    f64 start_time = 0;
    f64 end_time   = 0;

    f64 req_frame_time = (f64)(1.0f / 240);
    clock_start(&clock);

    u32 index = 0;
    while (app_state.is_running)
    {

        clock_update(&clock);
        start_time = clock.time_elapsed;

        update_camera(&triangle.global_ubo, start_time);

        application_run(&triangle);

        bool a = input_is_key_up(KEY_T);
        bool b = input_was_key_down(KEY_T);
        if (b && a)
        {

            // triangle.test_geometry = geometry_system_get_geometry(&conf);
            // index++;
            // index %= 6;
        }

        clock_update(&clock);
        end_time = clock.time_elapsed - start_time;

        if (!f32_compare(req_frame_time, end_time, 0.0001))
        {
            f64 sleep = req_frame_time - end_time;
            if (sleep > 0)
            {
                u64 sleep_ms = (u64)(sleep * D_SEC_TO_MS_MULTIPLIER);
                platform_sleep(sleep_ms);
            }
        }
        input_update(0);
    }
    application_shutdown();
}

void update_camera(uniform_buffer_object *ubo, f64 start_time)
{
    // ubo->model = mat4_euler_z((start_time * (90.0f * D_DEG2RAD_MULTIPLIER)));
    u32 s_width;
    u32 s_height;
    platform_get_window_dimensions(&s_width, &s_height);

    f32 fov_rad      = 45 * D_DEG2RAD_MULTIPLIER;
    f32 aspect_ratio = (f32)s_width / s_height;

    vec3 velocity = vec3();
    f32  step     = 0.005f;

    // HACK:
    static f32 z     = 0.01f;
    z               += step;
    ubo->projection  = mat4_perspective(fov_rad, aspect_ratio, 0.01f, 1000.0f);
    ubo->model       = mat4_euler_y(z);
    ubo->model       = mat4();

    static vec3 camera_pos   = vec3(0, 0, 6);
    static vec3 camera_euler = vec3(0, 0, 0);

    if (input_is_key_down(KEY_A) || input_is_key_down(KEY_LEFT))
    {
        camera_euler.y += step;
    }

    if (input_is_key_down(KEY_D) || input_is_key_down(KEY_RIGHT))
    {
        camera_euler.y -= step;
    }

    if (input_is_key_down(KEY_UP))
    {
        camera_euler.x += step;
    }

    if (input_is_key_down(KEY_DOWN))
    {
        camera_euler.x -= step;
    }

    if (input_is_key_down(KEY_W))
    {
        vec3 forward  = mat4_forward(ubo->view);
        velocity     += forward;
    }
    if (input_is_key_down(KEY_S))
    {

        vec3 backward  = mat4_backward(ubo->view);
        velocity      += backward;
    }

    vec3  z_axis          = vec3();
    float temp_move_speed = step;
    if (!vec3_compare(z_axis, velocity, 0.0002f))
    {
        // Be sure to normalize the velocity before applying speed.
        velocity.normalize();
        camera_pos.x += velocity.x * temp_move_speed * start_time;
        camera_pos.y += velocity.y * temp_move_speed * start_time;
        camera_pos.z += velocity.z * temp_move_speed * start_time;
    }

    mat4 rotation    = mat4_euler_xyz(camera_euler.x, camera_euler.y, camera_euler.z);
    mat4 translation = mat4_translation(camera_pos);

    ubo->view = rotation * translation;
    ubo->view = mat4_inverse(ubo->view);
}
