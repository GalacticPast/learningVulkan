#include "ui_system.hpp"
#include "containers/darray.hpp"
#include "core/dasserts.hpp"
#include "core/dmemory.hpp"
#include "core/dstring.hpp"
#include "core/input.hpp"
#include "defines.hpp"
#include "main.hpp"
#include "math/dmath.hpp"
#include "resources/font_system.hpp"
#include "resources/geometry_system.hpp"
#include "resources/resource_types.hpp"

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

    u64 active_id = INVALID_ID_64;
    u64 hot_id    = INVALID_ID_64;

    u64             geometry_id = INVALID_ID_64;
    geometry_config ui_geometry_config;
    u32             local_index_offset = INVALID_ID;

    font_data *system_font;

    ui_element root;
};

static ui_system_state *ui_sys_state_ptr;
static u64              _generate_id();
static bool             ui_system_generate_quad_config(f32 width, f32 height, f32 posx, f32 posy, vec4 color);
#define CURSOR_AABB {{(float)x, (float)y}, {5, 5}}

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
// box1 top left corner
// box2 top left corner
struct box
{
    vec2 position;
    vec2 dimensions;
};

bool _check_collision(box a, box b)
{
    return (a.position.x < b.position.x + b.dimensions.w && a.position.x + a.dimensions.w > b.position.x &&
            a.position.y < b.position.y + b.dimensions.h && a.position.y + a.dimensions.h > b.position.y);
}

// it will also transition it
bool _check_if_pressed(u64 id)
{
    bool result     = false;
    bool return_res = false;
    if (ui_sys_state_ptr->active_id == id)
    {
        result = input_is_button_up(BUTTON_LEFT);
        if (result)
        {
            result = ui_sys_state_ptr->hot_id == id;
            if (result)
            {
                return_res                  = true;
                ui_sys_state_ptr->active_id = INVALID_ID_64;
            }
        }
    }
    else if (ui_sys_state_ptr->hot_id == id)
    {
        result = input_is_button_down(BUTTON_LEFT);
        if (result)
        {
            ui_sys_state_ptr->active_id = id;
        }
    }
    return return_res;
}

bool _check_if_hot(u64 id, box a, box b)
{
    // check the bounding box of the curosr against the bounding box of the button
    bool is_hot = false;
    {
        if (_check_collision(a, b))
        {
            ui_sys_state_ptr->hot_id = id;
            is_hot                   = true;
        }
    }
    return is_hot;
}

bool ui_button(u64 id, vec2 position)
{
    DASSERT(ui_sys_state_ptr);

    bool return_res = _check_if_pressed(id);

    u32 padding = 3;

    ui_element button{};
    button.id = id;

    s32 x;
    s32 y;
    input_get_mouse_position(&x, &y);

    dstring text = "Button";
    {

        u32              strlen = text.str_len;
        font_glyph_data *glyphs = ui_sys_state_ptr->system_font->glyphs;

        u32 hash = INVALID_ID;

        u32 txt_width = 0;

        for (u32 i = 0; i < strlen; i++)
        {
            char ch = text[i];
            if (ch == '\0')
                break;
            hash       = ch - 32;
            txt_width += glyphs[hash].xadvance;
        }
        // INFO: 20 because its the system font size;
        s32 posx = position.x - padding;
        s32 posy = position.y - padding;

        if (posx < 0)
        {
            posx = 0;
        }
        if (posy < 0)
        {
            posy = 0;
        }

        box box1 = {{(float)posx, (float)posy}, {(float)txt_width + padding, 20.0f + padding}};
        box box2 = {{(float)x, (float)y}, {1, 1}};

        vec4 color  = DARKBLUE;
        bool is_hot = _check_if_hot(id, box1, box2);
        if (is_hot)
        {
            color = GRAY;
        }

        ui_system_generate_quad_config(txt_width + padding, 20 + padding, posx, posy, color);
    }

    ui_text(id, &text, {position.x, position.y}, WHITE);

    return return_res;
}

bool ui_system_generate_quad_config(f32 width, f32 height, f32 posx, f32 posy, vec4 color)
{
    if (width == 0)
    {
        DWARN("Width must be nonzero. Defaulting to one.");
        width = 1.0f;
    }
    if (height == 0)
    {
        DWARN("Height must be nonzero. Defaulting to one.");
        height = 1.0f;
    }

    arena           *arena  = ui_sys_state_ptr->arena;
    geometry_config *config = &ui_sys_state_ptr->ui_geometry_config;

    u32        vert_ind = ui_sys_state_ptr->ui_geometry_config.vertex_count;
    vertex_2D *vertices = static_cast<vertex_2D *>(ui_sys_state_ptr->ui_geometry_config.vertices);

    u32  index_ind = ui_sys_state_ptr->ui_geometry_config.index_count;
    u32 *indices   = ui_sys_state_ptr->ui_geometry_config.indices;

    u32 local_offset = ui_sys_state_ptr->local_index_offset;

    vertices[vert_ind].position    = {posx, posy};
    vertices[vert_ind].color       = color;
    vertices[vert_ind++].tex_coord = {0.0f, 0.0f};

    vertices[vert_ind].position    = {posx + width, posy};
    vertices[vert_ind].color       = color;
    vertices[vert_ind++].tex_coord = {0.0f, 0.0f};

    vertices[vert_ind].position    = {posx, posy + height};
    vertices[vert_ind].color       = color;
    vertices[vert_ind++].tex_coord = {0.0f, 0.0f};

    vertices[vert_ind].position    = {width + posx, posy + height};
    vertices[vert_ind].color       = color;
    vertices[vert_ind++].tex_coord = {0.0f, 0.0f};

    config->indices[index_ind++] = local_offset + 0;
    config->indices[index_ind++] = local_offset + 2;
    config->indices[index_ind++] = local_offset + 1;

    config->indices[index_ind++] = local_offset + 1;
    config->indices[index_ind++] = local_offset + 2;
    config->indices[index_ind++] = local_offset + 3;

    ui_sys_state_ptr->local_index_offset              = local_offset + 4;
    ui_sys_state_ptr->ui_geometry_config.vertex_count = vert_ind;
    ui_sys_state_ptr->ui_geometry_config.index_count  = index_ind;

    return true;
}

bool ui_text(u64 id, dstring *text, vec2 position, vec4 color)
{
    DASSERT(text);
    u32              length       = INVALID_ID;
    font_glyph_data *glyphs       = ui_sys_state_ptr->system_font->glyphs;
    u32              atlas_height = ui_sys_state_ptr->system_font->atlas_height;

    DASSERT(glyphs);

    u32  strlen = text->str_len;
    u32  hash   = 0;
    vec2 pos    = position;

    vertex_2D q0;
    vertex_2D q1;
    vertex_2D q2;
    vertex_2D q3;

    q0.color = color;
    q1.color = color;
    q2.color = color;
    q3.color = color;

    vertex_2D *vertices = static_cast<vertex_2D *>(ui_sys_state_ptr->ui_geometry_config.vertices);
    u32       *indices  = ui_sys_state_ptr->ui_geometry_config.indices;

    // this is the ptr to the next starting block
    u32 vert_ind = ui_sys_state_ptr->ui_geometry_config.vertex_count;

    // this is the ptr to the next starting block
    u32 index_ind = ui_sys_state_ptr->ui_geometry_config.index_count;

    u32 local_offset = ui_sys_state_ptr->local_index_offset;
    u32 padding      = 3;

    float x = F32_MIN;
    float y = F32_MAX;

    for (u32 i = 0; i < strlen; i++)
    {
        char ch = (*text)[i];
        if (ch == '\0')
            break;
        hash        = ch - 32;
        float x_off = glyphs[hash].xoff;
        float y_off = glyphs[hash].yoff;

        x = DMAX(x, x_off);
        y = DMIN(y, y_off);
    }

    float baseline = position.y - y;

    for (u32 i = 0; i < strlen; i++)
    {
        char ch = (*text)[i];
        if (ch == '\0')
            break;
        hash = ch - 32;

        float u0 = (glyphs[hash].x0 / (float)atlas_height);
        float v0 = -(glyphs[hash].y0 / (float)atlas_height);
        float u1 = (glyphs[hash].x1 / (float)atlas_height);
        float v1 = -(glyphs[hash].y1 / (float)atlas_height);

        float width  = (glyphs[hash].x1 - glyphs[hash].x0);
        float height = (glyphs[hash].y1 - glyphs[hash].y0);

        float xoff = glyphs[hash].xoff;
        float yoff = glyphs[hash].yoff;

        float x_pos = pos.x + xoff;
        float y_pos = baseline + yoff;

        q0.tex_coord = {u0, v0}; // Top-left
        q1.tex_coord = {u1, v0}; // Top-right
        q2.tex_coord = {u0, v1}; // Bottom-left
        q3.tex_coord = {u1, v1}; // Bottom-right

        q0.position = {x_pos, y_pos};
        q1.position = {x_pos + width, y_pos};
        q2.position = {x_pos, y_pos + height};
        q3.position = {x_pos + width, y_pos + height};

        vertices[vert_ind++] = q2;
        vertices[vert_ind++] = q0;
        vertices[vert_ind++] = q1;
        vertices[vert_ind++] = q3;

        indices[index_ind++] = local_offset + 0;
        indices[index_ind++] = local_offset + 1;
        indices[index_ind++] = local_offset + 2;

        indices[index_ind++] = local_offset + 0;
        indices[index_ind++] = local_offset + 2;
        indices[index_ind++] = local_offset + 3;

        // advacnce the index offset by 4 so that the next quad has the proper offset;
        local_offset += 4;

        pos.x += glyphs[hash].xadvance;
    }

    // INFO: creating a semi-transparent bounding box for the entire text
    {

        float x_pos = position.x;
        float y_pos = position.y;

        q0.tex_coord = {0, 0}; // Top-left
        q1.tex_coord = {0, 0}; // Top-right
        q2.tex_coord = {0, 0}; // Bottom-left
        q3.tex_coord = {0, 0}; // Bottom-right


        q0.position = {x_pos, y_pos};
        q1.position = {pos.x, y_pos};
        q2.position = {x_pos, y_pos + 20};
        q3.position = {pos.x, y_pos + 20};

        s32 x, y = 0;
        input_get_mouse_position(&x, &y);

        box box1 = {position, {pos.x, 20.0f}};
        box box2 = {{(float)x, (float)y}, {1, 1}};

        bool is_hot = _check_if_hot(id, box1, box2);


        q0.color = color; // Top-left
        q1.color = color; // Top-right
        q2.color = color; // Bottom-left
        q3.color = color; // Bottom-right

        if(is_hot)
        {
            q0.color.a = 0.5; // Top-left
            q1.color.a = 0.5; // Top-right
            q2.color.a = 0.5; // Bottom-left
            q3.color.a = 0.5; // Bottom-right
        }
        else
        {
            q0.color.a = 0;
            q1.color.a = 0;
            q2.color.a = 0;
            q3.color.a = 0;
        }

        vertices[vert_ind++] = q2;
        vertices[vert_ind++] = q0;
        vertices[vert_ind++] = q1;
        vertices[vert_ind++] = q3;

        indices[index_ind++] = local_offset + 0;
        indices[index_ind++] = local_offset + 1;
        indices[index_ind++] = local_offset + 2;

        indices[index_ind++] = local_offset + 0;
        indices[index_ind++] = local_offset + 2;
        indices[index_ind++] = local_offset + 3;

        // advacnce the index offset by 4 so that the next quad has the proper offset;
        local_offset += 4;

    }

    ui_sys_state_ptr->local_index_offset              = local_offset;
    ui_sys_state_ptr->ui_geometry_config.vertex_count = vert_ind;
    ui_sys_state_ptr->ui_geometry_config.index_count  = index_ind;

    return true;
}

u64 ui_system_flush_geometries()
{
    ui_sys_state_ptr->ui_geometry_config.type = GEO_TYPE_2D;

    u64 id = ui_sys_state_ptr->geometry_id;

    if (id == INVALID_ID_64)
    {
        id                            = geometry_system_create_geometry(&ui_sys_state_ptr->ui_geometry_config, false);
        ui_sys_state_ptr->geometry_id = id;
    }
    else
    {
        geometry_system_update_geometry(&ui_sys_state_ptr->ui_geometry_config, id);
    }
    ui_sys_state_ptr->local_index_offset              = 0;
    ui_sys_state_ptr->ui_geometry_config.vertex_count = 0;
    ui_sys_state_ptr->ui_geometry_config.index_count  = 0;

    return id;
}
