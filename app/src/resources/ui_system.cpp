#include "containers/darray.hpp"
#include "core/dasserts.hpp"
#include "core/dmemory.hpp"
#include "core/dstring.hpp"
#include "defines.hpp"
#include "main.hpp"
#include "math/dmath.hpp"
#include "resources/font_system.hpp"
#include "resources/resource_types.hpp"
#include "ui_system.hpp"

struct ui_element
{
    u64 id = INVALID_ID_64;

    dstring string;
    vec2    position; // absolute position
    vec4    color;
    vec2    dimensions;

    darray<ui_element> nodes; // aka children
};

struct ui_system_state
{
    arena *arena;

    u64 active_id;
    u64 hot_id;

    u64             geometry_id = INVALID_ID_64;
    geometry_config ui_geometry_config;
    u32             local_index_offset = INVALID_ID;

    font_data *system_font;

    ui_element root;
};

static ui_system_state *ui_sys_state_ptr;
static u64              _generate_id();

bool ui_system_initialize(arena *system_arena, arena *resource_arena)
{
    DASSERT(system_arena);
    DASSERT(resource_arena);

    ui_sys_state_ptr =
        static_cast<ui_system_state *>(dallocate(system_arena, sizeof(ui_system_state), MEM_TAG_APPLICATION));
    ui_sys_state_ptr->arena = resource_arena;
    ui_sys_state_ptr->root.nodes.c_init(resource_arena);
    ui_sys_state_ptr->root.id     = 0;
    ui_sys_state_ptr->geometry_id = INVALID_ID_64;
    ui_sys_state_ptr->ui_geometry_config.vertices =
        dallocate(resource_arena, sizeof(vertex_2D) * MB(1), MEM_TAG_GEOMETRY);
    ui_sys_state_ptr->ui_geometry_config.indices =
        static_cast<u32 *>(dallocate(resource_arena, sizeof(u32) * MB(1), MEM_TAG_GEOMETRY));
    ui_sys_state_ptr->system_font = font_system_get_system_font();

    ui_create_button({100, 100});
    return true;
}

u64 _generate_id()
{
    // SplitMix64 (used in PCG and others)
    // -> https://github.com/indiesoftby/defold-splitmix64?tab=readme-ov-file
    u64 x  = drandom_s64();
    x     += 0x9e3779b97f4a7c15;
    x      = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
    x      = (x ^ (x >> 27)) * 0x94d049bb133111eb;
    return x ^ (x >> 31);
}

bool ui_create_button(vec2 position)
{
    DASSERT(ui_sys_state_ptr);

    ui_element button{};
    button.id = _generate_id();

    return false;
}
