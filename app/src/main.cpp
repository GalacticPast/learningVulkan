#include "core/application.hpp"
#include "core/dclock.hpp"
#include "core/dmemory.hpp"
#include "core/input.hpp"
#include "core/logger.hpp"
#include "main.hpp"

#include "containers/darray.hpp"

#include "defines.hpp"

#include "math/dmath.hpp"

#include "math/dmath_types.hpp"
#include "platform/platform.hpp"

#include "renderer/renderer.hpp"
#include "resources/geometry_system.hpp"
#include "resources/material_system.hpp"
#include "resources/resource_types.hpp"
#include "resources/shader_system.hpp"

#include "../tests/containers/darray_test.hpp"
#include "../tests/containers/dhashtable_test.hpp"
#include "../tests/events/event_system_test.hpp"
#include "../tests/linear_allocator/linear_allocator_test.hpp"
#include "../tests/test_manager.hpp"

void update_camera(scene_global_uniform_buffer_object *ubo, ui_global_uniform_buffer_object *ui_ubo,
                   light_global_uniform_buffer_object *lbo, f64 start_time);

void run_tests()
{
    u64 test_manager_mem_requirements = 0;
    u64 memory_mem_requirements       = 0;

    memory_system_startup(&memory_mem_requirements, 0);
    void *block = platform_allocate(memory_mem_requirements, false);
    memory_system_startup(&memory_mem_requirements, block);

    u64 arena_pool_size = 32;
    u32 num_arenas      = 1;
    arena_allocate_arena_pool(arena_pool_size, num_arenas);

    arena *arena = arena_get_arena();
    DASSERT(arena);

    test_manager_initialize(arena, &test_manager_mem_requirements, nullptr);
    test_manager *test_instance = static_cast<test_manager *>(malloc(test_manager_mem_requirements));
    darray<test>  test_array;
    test_instance->tests = &test_array;
    test_manager_initialize(arena, &test_manager_mem_requirements, test_instance);

    linear_allocator_register_tests();
    event_system_register_tests();
    darray_register_tests();
    dhashtable_register_tests();

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

    ui_global_uniform_buffer_object ui_ubo{};
    ui_ubo.view = mat4_inverse(mat4());

    scene_global_uniform_buffer_object scene_ubo{};
    scene_ubo.view          = mat4_look_at({2, 2, 2}, {0, 0, 0}, {0, 0, 1.0f});
    scene_ubo.projection    = mat4_perspective(fov_rad, aspect_ratio, 0.01f, 1000.0f);
    scene_ubo.ambient_color = {0.3, 0.3, 0.3, 1.0f};

    light_global_uniform_buffer_object light_ubo{};
    light_ubo.color = {0.6, 0.6, 0.6, 1.0};

    render_data triangle{};

    geometry **geos_3D           = nullptr;
    u32        geometry_count_3D = INVALID_ID;

    geometry **geos_2D           = nullptr;
    u32        geometry_count_2D = INVALID_ID;

    light_ubo.direction = {-0.578f, -0.578f, -0.578f};
    light_ubo.color     = {0.8f, 0.8f, 0.8f, 1.0f};

#if false
    {
        const char *obj_file_name  = "sponza.obj";
        const char *mtl_file_name  = "sponza.mtl";
        geometry_system_get_geometries_from_file(obj_file_name, mtl_file_name, &geos_3D, &geometry_count_3D);

    }
#endif

#if true
    {
        dstring         sphere_obj  = "sphere.obj";
        geometry_config red_light   = *geometry_system_generate_config(sphere_obj);
        geometry_config green_light = *geometry_system_generate_config(sphere_obj);
        geometry_config blue_light  = *geometry_system_generate_config(sphere_obj);

        dstring mat_name   = DEFAULT_LIGHT_MATERIAL_HANDLE;
        red_light.material = material_system_get_from_name(&mat_name);
        scale_geometries(&red_light, {0.2f, 0.2f, 0.2f});
        green_light.material = material_system_get_from_name(&mat_name);
        scale_geometries(&green_light, {0.2f, 0.2f, 0.2f});
        blue_light.material = material_system_get_from_name(&mat_name);
        scale_geometries(&blue_light, {0.2f, 0.2f, 0.2f});

        u64 red   = geometry_system_create_geometry(&red_light, false);
        u64 green = geometry_system_create_geometry(&green_light, false);
        u64 blue  = geometry_system_create_geometry(&blue_light, false);

        geos_3D = static_cast<geometry **>(dallocate(app_state.system_arena, sizeof(geometry *) * 6, MEM_TAG_UNKNOWN));
        geos_3D[0]           = geometry_system_get_default_geometry();
        mat_name             = "orange_lines_512.conf";
        geos_3D[0]->material = material_system_get_from_config_file(&mat_name);
        mat_name.clear();

        geos_3D[1]            = geometry_system_get_geometry(red);
        geos_3D[1]->ubo.model = mat4_translation({2, 0, 0});

        geos_3D[2]            = geometry_system_get_geometry(green);
        geos_3D[2]->ubo.model = mat4_translation({0, 2, 0});

        geos_3D[3]            = geometry_system_get_geometry(blue);
        geos_3D[3]->ubo.model = mat4_translation({0, 0, 2});

        geometry_count_3D = 4;

        geos_2D = static_cast<geometry **>(dallocate(app_state.system_arena, sizeof(geometry *) * 2, MEM_TAG_UNKNOWN));
    }
#endif

    triangle.test_geometry_3D  = geos_3D;
    triangle.geometry_count_3D = geometry_count_3D;
    geometry_count_2D          = 1;

    triangle.test_geometry_2D  = geos_2D;
    triangle.geometry_count_2D = geometry_count_2D;

    triangle.scene_ubo = scene_ubo;
    triangle.ui_ubo    = ui_ubo;
    triangle.light_ubo = light_ubo;

    f64 frame_start_time   = 0;
    f64 frame_end_time     = 0;
    f64 frame_elapsed_time = 0;

    f64 req_frame_time = static_cast<f64>((1.0f / 240));

    u32 index = 0;
    f32 z     = 0.1f;

    u64 def_material_shader = shader_system_get_default_material_shader_id();
    u64 def_skybox_shader   = shader_system_get_default_skybox_shader_id();
    u64 def_grid_shader     = shader_system_get_default_grid_shader_id();
    u64 def_ui_shader       = shader_system_get_default_ui_shader_id();

    dstring font_atlas = DEFAULT_FONT_ATLAS_TEXTURE_HANDLE;

    dstring test;
    test             = "FPS: ";
    dstring abc_cap  = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    dstring abc_sall = "abcdefghijklmnopqrstuvwxyz";
    dstring num      = "0123456789";
    dstring symbols  = "!@#$%^&*()_+{}[]:><|''";
    u32 frames = 0;
    u32     fps      = 0;

    while (app_state.is_running)
    {
        ZoneScoped;
        frame_start_time = platform_get_absolute_time();

        if(frames % 120  == 0)
        {
            test.str_len += u32_to_string(&test.string[4], fps);
        }

        geometry_system_generate_text_geometry(&test, {0, 10});

        geometry_system_generate_text_geometry(&abc_cap, {0, 60});
        geometry_system_generate_text_geometry(&abc_sall, {0, 120});
        geometry_system_generate_text_geometry(&num, {0, 180});
        geometry_system_generate_text_geometry(&symbols, {0, 220});

        u64 quad_id          = geometry_system_flush_text_geometries();
        geos_2D[0]           = geometry_system_get_geometry(quad_id);
        geos_2D[0]->material = material_system_get_from_name(&font_atlas);

        update_camera(&triangle.scene_ubo, &triangle.ui_ubo, &triangle.light_ubo, frame_elapsed_time);

        shader_system_bind_shader(def_material_shader);
        shader_system_update_per_frame(&triangle.scene_ubo, &triangle.light_ubo);

        shader_system_bind_shader(def_skybox_shader);
        shader_system_update_per_frame(&triangle.scene_ubo, nullptr);

        shader_system_bind_shader(def_grid_shader);
        shader_system_update_per_frame(&triangle.scene_ubo, nullptr);

        shader_system_bind_shader(def_ui_shader);
        shader_system_update_per_frame(&triangle.ui_ubo, nullptr);

        renderer_draw_frame(&triangle);

        // Figure out how long the frame took and, if below
        f64 frame_end_time    = platform_get_absolute_time();
        frame_elapsed_time    = frame_end_time - frame_start_time;
        f64 remaining_seconds = req_frame_time - frame_elapsed_time;

        if (remaining_seconds > 0)
        {
            u64  remaining_ms = (remaining_seconds * 1000);
            bool limit_frames = true;
            if (remaining_ms > 0 && limit_frames)
            {
                platform_sleep(remaining_ms - 1);
            }
        }
        frame_end_time     = platform_get_absolute_time();
        frame_elapsed_time = frame_end_time - frame_start_time;
        fps                = 1 / (frame_elapsed_time);
        frames++;
        input_update(0);
    }
    application_shutdown();
}

void update_camera(scene_global_uniform_buffer_object *ubo, ui_global_uniform_buffer_object *ui_ubo,
                   light_global_uniform_buffer_object *lbo, f64 start_time)
{
    // ubo->model = mat4_euler_z((start_time * (90.0f * D_DEG2RAD_MULTIPLIER)));
    u32 s_width;
    u32 s_height;
    platform_get_window_dimensions(&s_width, &s_height);

    f32 fov_rad      = 45 * D_DEG2RAD_MULTIPLIER;
    f32 aspect_ratio = static_cast<f32>(s_width) / static_cast<f32>(s_height);

    vec3 velocity = vec3();
    f32  step     = 0.01f;

    ubo->projection    = mat4_perspective(fov_rad, aspect_ratio, 0.01f, 1000.0f);
    ui_ubo->projection = mat4_orthographic(0, s_width, s_height, 0, -1, 1);

    static vec3 camera_pos   = vec3(0, 6, 6);
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

    vec3 z_axis          = vec3();
    f32  temp_move_speed = start_time;

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

    ubo->camera_pos = camera_pos;

    mat4 rotation    = mat4_euler_xyz(camera_euler.x, camera_euler.y, camera_euler.z);
    mat4 translation = mat4_translation(camera_pos);

    ubo->view = rotation * translation;
    ubo->view = mat4_inverse(ubo->view);
}
