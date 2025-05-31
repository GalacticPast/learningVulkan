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

#include "../tests/containers/darray_test.hpp"
#include "../tests/containers/dfreelist_test.hpp"
#include "../tests/containers/dhashtable_test.hpp"
#include "../tests/events/event_system_test.hpp"
#include "../tests/linear_allocator/linear_allocator_test.hpp"
#include "../tests/test_manager.hpp"

void update_camera(scene_global_uniform_buffer_object *ubo, f64 start_time);

void run_tests()
{
    u64 test_manager_mem_requirements = 0;
    u64 memory_mem_requirements       = 0;

    memory_system_startup(&memory_mem_requirements, 0);
    void *block = platform_allocate(memory_mem_requirements, false);
    memory_system_startup(&memory_mem_requirements, block);

    test_manager_initialize(&test_manager_mem_requirements, nullptr);
    test_manager *test_instance = static_cast<test_manager *>(malloc(test_manager_mem_requirements));
    darray<test>  test_array;
    test_instance->tests = &test_array;
    test_manager_initialize(&test_manager_mem_requirements, test_instance);

    linear_allocator_register_tests();
    event_system_register_tests();
    darray_register_tests();
    dhashtable_register_tests();
    dfreelist_register_tests();

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
    f32 aspect_ratio = static_cast<f32>(s_width) / static_cast<f32>(s_height);

    scene_global_uniform_buffer_object scene_ubo{};
    scene_ubo.view       = mat4_look_at({2, 2, 2}, {0, 0, 0}, {0, 0, 1.0f});
    scene_ubo.projection = mat4_perspective(fov_rad, aspect_ratio, 0.01f, 1000.0f);

    light_global_uniform_buffer_object light_ubo{};
    light_ubo.position = {5.0f, 4.0f, 0.0f};
    light_ubo.color    = {1.0f, 1.0f, 1.0f};

    geometry_config plane_config =
        geometry_system_generate_plane_config(10, 5, 5, 5, 5, 5, "its_a_plane", DEFAULT_MATERIAL_HANDLE);

    render_data triangle{};

    {
        //const char *obj_file_name  = "sponza.obj";
        //const char *mtl_file_name  = "sponza.mtl";
        //u32         geometry_count = INVALID_ID;
        //geometry  **geos           = nullptr;
        //geometry_system_get_geometries_from_file(obj_file_name, mtl_file_name, &geos, &geometry_count);
    }

    geometry_config parent_config = geometry_system_generate_cube_config();
    geometry_config child_config{};

    geometry_system_copy_config(&child_config, &parent_config);

    scale_geometries(&child_config, {0.5f, 0.5f, 0.5f});

    u64 cube_id1 = geometry_system_create_geometry(&parent_config, false);
    u64 cube_id2 = geometry_system_create_geometry(&child_config, false);

    u32        geometry_count = 2;
    geometry **geos = static_cast<geometry **>(dallocate(sizeof(geometry) * geometry_count, MEM_TAG_GEOMETRY));

    geos[0] = geometry_system_get_geometry(cube_id1);
    geos[1] = geometry_system_get_geometry(cube_id2);

    math::vec3 left    = {-4.0f, 0, 0};
    geos[1]->ubo.model = mat4_translation(left);

    dstring mat_file  = "orange_lines_512";
    geos[1]->material = material_system_acquire_from_config_file(&mat_file);

    triangle.test_geometry  = geos;
    triangle.geometry_count = geometry_count;

    triangle.scene_ubo = scene_ubo;
    triangle.light_ubo = light_ubo;

    f64 frame_start_time   = 0;
    f64 frame_end_time     = 0;
    f64 frame_elapsed_time = 0;

    f64 req_frame_time = static_cast<f64>((1.0f / 240));
    clock_start(&clock);

    u32 index = 0;
    f32 z     = 0.1f;
    while (app_state.is_running)
    {
        clock_update(&clock);
        frame_start_time = clock.time_elapsed;
        update_camera(&triangle.scene_ubo, frame_elapsed_time);

        // geos[1]->ubo.model *= mat4_euler_y(z);
        z = sinf(frame_elapsed_time);

        application_run(&triangle);

        clock_update(&clock);
        frame_end_time     = clock.time_elapsed;
        frame_elapsed_time = frame_end_time - frame_start_time;

        if (!f32_compare(req_frame_time, frame_elapsed_time, 0.0001))
        {
            f64 sleep = req_frame_time - frame_elapsed_time;
            if (sleep > 0)
            {
                u64 sleep_ms = static_cast<u64>(sleep * D_SEC_TO_MS_MULTIPLIER);
                platform_sleep(sleep_ms);
            }
        }
        input_update(0);
        clock_update(&clock);
    }
    application_shutdown();
}

void update_camera(scene_global_uniform_buffer_object *ubo, f64 start_time)
{
    // ubo->model = mat4_euler_z((start_time * (90.0f * D_DEG2RAD_MULTIPLIER)));
    u32 s_width;
    u32 s_height;
    platform_get_window_dimensions(&s_width, &s_height);

    f32 fov_rad      = 45 * D_DEG2RAD_MULTIPLIER;
    f32 aspect_ratio = static_cast<f32>(s_width) / static_cast<f32>(s_height);

    math::vec3 velocity = math::vec3();
    f32        step     = 0.01f;

    ubo->projection = mat4_perspective(fov_rad, aspect_ratio, 0.01f, 1000.0f);

    static math::vec3 camera_pos   = math::vec3(0, 0, 6);
    static math::vec3 camera_euler = math::vec3(0, 0, 0);

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
        math::vec3 forward  = mat4_forward(ubo->view);
        velocity           += forward;
    }
    if (input_is_key_down(KEY_S))
    {

        math::vec3 backward  = mat4_backward(ubo->view);
        velocity            += backward;
    }

    math::vec3 z_axis          = math::vec3();
    f32        temp_move_speed = start_time;

    f32 scalar = 100.0f;

    if (!vec3_compare(z_axis, velocity, 0.0002f))
    {
        // Be sure to normalize the velocity before applying speed.
        velocity.normalize();
        velocity     *= scalar;
        camera_pos.x += velocity.x * temp_move_speed;
        camera_pos.y += velocity.y * temp_move_speed;
        camera_pos.z += velocity.z * temp_move_speed;
    }

    math::mat4 rotation    = mat4_euler_xyz(camera_euler.x, camera_euler.y, camera_euler.z);
    math::mat4 translation = mat4_translation(camera_pos);

    ubo->view = rotation * translation;
    ubo->view = mat4_inverse(ubo->view);
}
