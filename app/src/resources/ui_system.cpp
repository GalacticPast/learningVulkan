#include "containers/darray.hpp"
#include "core/dasserts.hpp"
#include "core/dmemory.hpp"
#include "core/dstring.hpp"
#include "core/input.hpp"
#include "defines.hpp"
#include "main.hpp"
#include "math/dmath.hpp"
#include "memory/arenas.hpp"
#include "resources/font_system.hpp"
#include "resources/geometry_system.hpp"
#include "resources/resource_types.hpp"
#include "ui_system.hpp"

#define BORDER_COLOR RED
#define SLIDER_DEFAULT_DIMENSIONS (vec2){30, 60}
struct ui_element
{
    u64 id = INVALID_ID_64;

    ui_element_type type;

    vec2    text_position;
    dstring text;

    s32 value = INVALID_ID_S32; // slider value if it is a slider
    s32 min   = INVALID_ID_S32;
    s32 max   = INVALID_ID_S32;

    // num of rows and columns if it's a container
    u32 num_rows    = INVALID_ID;
    u32 num_columns = INVALID_ID;

    // where it is positioned in respect to the container if its a ui_primitive.
    u32 row_pos = INVALID_ID;
    u32 col_pos = INVALID_ID;

    vec2 position; // absolute position

    vec2 dimensions;
    vec2 interactable_dimensions;

    vec4 color;

    darray<ui_element> nodes; // aka children

    void init(arena *arena)
    {
        nodes.c_init(arena);
    }
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

struct box
{
    vec2 position;
    vec2 dimensions;
};

static ui_system_state *ui_sys_state_ptr;
static bool             _insert_element(u64 parent_id, u64 id, ui_element *root, ui_element *element);
static void             _generate_element_geometry(ui_element *element);
static void             _calculate_element_dimensions(ui_element *element);
static void             _calculate_element_positions(ui_element *element);
static bool             _generate_quad_config(vec2 position, vec2 dimensions, vec4 color);
static bool             _check_mouse_collision(box box);
static bool             _check_box_if_hot(u64 id, box box1);

bool ui_system_initialize(arena *system_arena, arena *resource_arena)
{
    DASSERT(system_arena);
    DASSERT(resource_arena);

    ui_sys_state_ptr =
        static_cast<ui_system_state *>(dallocate(system_arena, sizeof(ui_system_state), MEM_TAG_APPLICATION));
    ui_sys_state_ptr->arena       = resource_arena;
    ui_sys_state_ptr->geometry_id = INVALID_ID_64;
    ui_sys_state_ptr->ui_geometry_config.vertices =
        dallocate(resource_arena, sizeof(vertex_2D) * MB(1), MEM_TAG_GEOMETRY);
    ui_sys_state_ptr->ui_geometry_config.indices =
        static_cast<u32 *>(dallocate(resource_arena, sizeof(u32) * MB(1), MEM_TAG_GEOMETRY));
    ui_sys_state_ptr->system_font             = font_system_get_system_font();
    ui_sys_state_ptr->ui_geometry_config.type = GEO_TYPE_2D;

    return true;
}

bool ui_system_set_arena(arena *arena)
{
    DASSERT(arena);
    ui_sys_state_ptr->arena = arena;
    // HACK:
    ui_sys_state_ptr->root.init(arena);
    return true;
}

// box1 top left corner
// box2 top left corner
bool _check_collision(box a, box b)
{
    return (a.position.x < b.position.x + b.dimensions.w && a.position.x + a.dimensions.w > b.position.x &&
            a.position.y < b.position.y + b.dimensions.h && a.position.y + a.dimensions.h > b.position.y);
}

// it will also transition it
// INFO: There will always be a delay of one frame. Because _ui_flush() will set up the hot_id in the first iteration.
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

bool _insert_element(u64 parent_id, u64 id, ui_element *root, ui_element *element)
{
    DASSERT(id != INVALID_ID);
    // INFO: this should never happen anyways.
    if (root == nullptr)
    {
        return false;
    }

    if (parent_id == INVALID_ID_64)
    {
        root->nodes.push_back(*element);
        return true;
    }

    u32 size = root->nodes.size();
    for (u32 i = 0; i < size; i++)
    {
        if (root->nodes[i].id == parent_id)
        {
            root->nodes[i].nodes.push_back(*element);
            return true;
        }
    }

    for (u32 i = 0; i < size; i++)
    {
        bool res = _insert_element(parent_id, id, &root->nodes[i], element);
        if (res)
        {
            return true;
        }
    }
    return false;
}

static void _calculate_element_positions(ui_element *element)
{
    if (element == nullptr)
    {
        return;
    }
    vec2 padding   = {4, 4};
    vec2 position  = element->position;
    position.x    += padding.x;

    u32 length = element->nodes.size();
    for (u32 i = 0; i < length; i++)
    {
        ui_element *elem     = &element->nodes[i];
        position.y          += padding.y + elem->dimensions.y;
        elem->position       = position;
        elem->text_position  = position;

        switch (elem->type)
        {
        case UI_DROPDOWN: {
        }
        break;
        case UI_BUTTON: {
        }
        break;
        case UI_SLIDER: {
            elem->text_position = {elem->position.x + elem->interactable_dimensions.x, elem->position.y};
        }
        break;
        case UI_UNKNOWN: {
            DERROR("UI element %llu is unknown", element->id);
        }
        break;
        }
        position = elem->position;
    }
}

static void _calculate_element_dimensions(ui_element *element)
{
    if (element == nullptr)
    {
        return;
    }

    static font_glyph_data *glyphs = ui_sys_state_ptr->system_font->glyphs;

    u32  length          = element->nodes.size();
    vec2 dimensions      = vec2();
    vec2 text_dimensions = vec2();
    vec2 position        = vec2();
    u32  padding         = 4;

    if (length)
    {
        for (u32 i = 0; i < length; i++)
        {
            ui_element *elem = &element->nodes[i];
            _calculate_element_dimensions(elem);
        }
        for (u32 i = 0; i < length; i++)
        {
            dimensions.x  = DMAX(element->nodes[i].dimensions.x, dimensions.x);
            dimensions.x += padding;
            dimensions.y += element->nodes[i].dimensions.y + padding;
        }
    }

    dstring text      = element->text;
    u32     strlen    = text.str_len;
    u32     hash      = 0;
    u32     txt_width = 0;

    for (u32 i = 0; i < strlen; i++)
    {
        char ch = text[i];
        if (ch == '\0')
            break;
        hash       = ch - 32;
        txt_width += glyphs[hash].xadvance;
    }

    text_dimensions.x  = DMAX(txt_width + padding, dimensions.x);
    text_dimensions.y += 20 + padding;

    switch (element->type)
    {
    case UI_DROPDOWN: {
        dimensions.y        += text_dimensions.y;
        element->dimensions  = dimensions;
    }
    break;
    case UI_BUTTON: {
        dimensions                       = text_dimensions;
        element->interactable_dimensions = dimensions;
        element->dimensions              = dimensions;
    }
    break;
    case UI_SLIDER: {
        vec2 def_dimension = SLIDER_DEFAULT_DIMENSIONS;

        dimensions    = text_dimensions;
        dimensions.x += def_dimension.x;
        dimensions.y  = DMAX(def_dimension.y + padding, dimensions.y);

        element->dimensions              = dimensions;
        element->interactable_dimensions = dimensions - text_dimensions;
    }
    break;
    case UI_UNKNOWN: {
        DERROR("UI element %llu is unknown", element->id);
    }
    break;
    }
}

static void _generate_element_geometry(ui_element *element)
{
    if (element == nullptr)
    {
        return;
    }

    // if this is the first element it will have a position set.
    // from second..n  will be calculated.
    u32 length = element->nodes.size();

    static u32 padding = 4;

    if (element->type == UI_DROPDOWN)
    {
        vec2 border_dimensions = {element->dimensions.x + 4, element->dimensions.y + 4};
        vec2 position          = {element->position.x - 2, element->position.y - 2};
        _generate_quad_config(position, border_dimensions, BORDER_COLOR);
    }

    _generate_quad_config(element->position, element->dimensions, element->color);

    if (element->type == UI_DROPDOWN)
    {
        vec2 header_dimensions = {element->dimensions.x, 25};
        _generate_quad_config(element->position, header_dimensions, element->color);
        element->interactable_dimensions = header_dimensions;
    }
    box  box1   = {element->position, element->interactable_dimensions};
    bool is_hot = _check_box_if_hot(element->id, box1);

    if (is_hot)
    {
        ui_text(element->id, &element->text, element->text_position, YELLOW);
    }
    else
    {
        ui_text(element->id, &element->text, element->text_position, WHITE);
    }

    for (u32 i = 0; i < length; i++)
    {
        ui_element *elem = &element->nodes[i];
        _generate_element_geometry(elem);
    }

    element->nodes.~darray();

    return;
}

static bool _check_box_if_hot(u64 id, box box1)
{
    s32 mouse_x, mouse_y = 0;
    input_get_mouse_position(&mouse_x, &mouse_y);
    box box2 = {{(float)mouse_x, (float)mouse_y}, {1, 1}};

    bool is_hot = _check_if_hot(id, box1, box2);

    return is_hot;
}

bool ui_window(u64 parent_id, u64 id, vec2 position, u32 num_rows, u32 num_columns)
{
    DASSERT(ui_sys_state_ptr);

    u32 padding = 3;

    ui_element dropdown{};
    dropdown.type          = UI_DROPDOWN;
    dropdown.id            = id;
    dropdown.text_position = position;
    dropdown.position      = position;
    dropdown.color         = DARKGRAY;

    dropdown.num_rows    = num_rows;
    dropdown.num_columns = num_columns;

    dropdown.text       = "DropDown";
    dropdown.dimensions = vec2();
    dropdown.init(ui_sys_state_ptr->arena);

    _insert_element(parent_id, id, &ui_sys_state_ptr->root, &dropdown);
    return true;
}

bool ui_slider(u64 parent_id, u64 id, s32 min, s32 max, s32 *value, u32 row, u32 column)
{
    DASSERT(value);

    ui_element slider{};

    slider.id       = id;
    slider.type     = UI_SLIDER;
    slider.min      = min;
    slider.max      = max;
    slider.position = vec2();
    slider.color    = DARKBLUE;
    slider.text     = "slider";

    slider.row_pos = row;
    slider.col_pos = column;

    bool return_res = _check_if_pressed(id);

    // do the logic

    //
    if (return_res)
    {
        slider.value = *value;
    }
    _insert_element(parent_id, id, &ui_sys_state_ptr->root, &slider);
    return return_res;
}

bool ui_button(u64 parent_id, u64 id, dstring *text, u32 row, u32 column)
{
    DASSERT(ui_sys_state_ptr);
    DASSERT(id != INVALID_ID_64);

    bool return_res = _check_if_pressed(id);

    ui_element button{};
    button.id       = id;
    button.type     = UI_BUTTON;
    button.position = vec2();
    button.color    = DARKBLUE;

    button.row_pos = row;
    button.col_pos = column;

    if (text)
    {
        button.text = *text;
    }
    else
    {
        button.text = "BUTTON";
    }
    button.dimensions = vec2();
    button.init(ui_sys_state_ptr->arena);

    _insert_element(parent_id, id, &ui_sys_state_ptr->root, &button);

    return return_res;
}

bool _generate_quad_config(vec2 position, vec2 dimensions, vec4 color)
{
    f32 width  = dimensions.x;
    f32 height = dimensions.y;

    if (dimensions.x == 0)
    {
        DWARN("Width must be nonzero. Defaulting to one.");
        width = 1.0f;
    }
    if (dimensions.y == 0)
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

    // the full quad
    {
        vertices[vert_ind].position    = {position.x, position.y};
        vertices[vert_ind].color       = color;
        vertices[vert_ind++].tex_coord = {0.0f, 0.0f};

        vertices[vert_ind].position    = {position.x + width, position.y};
        vertices[vert_ind].color       = color;
        vertices[vert_ind++].tex_coord = {0.0f, 0.0f};

        vertices[vert_ind].position    = {position.x, position.y + height};
        vertices[vert_ind].color       = color;
        vertices[vert_ind++].tex_coord = {0.0f, 0.0f};

        vertices[vert_ind].position    = {width + position.x, position.y + height};
        vertices[vert_ind].color       = color;
        vertices[vert_ind++].tex_coord = {0.0f, 0.0f};

        config->indices[index_ind++] = local_offset + 0;
        config->indices[index_ind++] = local_offset + 2;
        config->indices[index_ind++] = local_offset + 1;

        config->indices[index_ind++] = local_offset + 1;
        config->indices[index_ind++] = local_offset + 2;
        config->indices[index_ind++] = local_offset + 3;
    }

    ui_sys_state_ptr->local_index_offset              = local_offset + 4;
    ui_sys_state_ptr->ui_geometry_config.vertex_count = vert_ind;
    ui_sys_state_ptr->ui_geometry_config.index_count  = index_ind;

    return true;
}

bool ui_text(u64 id, dstring *text, vec2 position, vec4 color)
{
    DASSERT(text);
    u32                     length       = INVALID_ID;
    static font_glyph_data *glyphs       = ui_sys_state_ptr->system_font->glyphs;
    u32                     atlas_height = ui_sys_state_ptr->system_font->atlas_height;

    DASSERT(glyphs);

    u32  strlen  = text->str_len;
    u32  hash    = 0;
    vec2 pos     = position;
    u32  padding = 4;

    vertex_2D q0;
    vertex_2D q1;
    vertex_2D q2;
    vertex_2D q3;

    q0.color = color;
    q1.color = color;
    q2.color = color;
    q3.color = color;

    static vertex_2D *vertices = static_cast<vertex_2D *>(ui_sys_state_ptr->ui_geometry_config.vertices);
    static u32       *indices  = ui_sys_state_ptr->ui_geometry_config.indices;

    // this is the ptr to the next starting block
    u32 vert_ind = ui_sys_state_ptr->ui_geometry_config.vertex_count;

    // this is the ptr to the next starting block
    u32 index_ind = ui_sys_state_ptr->ui_geometry_config.index_count;

    u32 local_offset = ui_sys_state_ptr->local_index_offset;

    float x_off           = F32_MIN;
    float y_off           = F32_MAX;
    u32   txt_width       = 0;
    vec2  text_dimensions = vec2();

    for (u32 i = 0; i < strlen; i++)
    {
        char ch = (*text)[i];
        if (ch == '\0')
            break;
        hash    = ch - 32;
        float x = glyphs[hash].xoff;
        float y = glyphs[hash].yoff;

        x_off      = DMAX(x_off, x);
        y_off      = DMIN(y_off, y);
        txt_width += glyphs[hash].xadvance;
    }

    float baseline = position.y - y_off;

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

    ui_sys_state_ptr->local_index_offset              = local_offset;
    ui_sys_state_ptr->ui_geometry_config.vertex_count = vert_ind;
    ui_sys_state_ptr->ui_geometry_config.index_count  = index_ind;

    return true;
}

u64 ui_system_flush_geometries()
{

    u64 id     = ui_sys_state_ptr->geometry_id;
    u32 length = ui_sys_state_ptr->root.nodes.size();

    for (u32 i = 0; i < length; i++)
    {
        ui_element *elem = &ui_sys_state_ptr->root.nodes[i];
        _calculate_element_dimensions(elem);
    }
    for (u32 i = 0; i < length; i++)
    {
        ui_element *elem = &ui_sys_state_ptr->root.nodes[i];
        _calculate_element_positions(elem);
    }

    for (u32 i = 0; i < length; i++)
    {
        ui_element *elem = &ui_sys_state_ptr->root.nodes[i];
        _generate_element_geometry(elem);
    }

    if (id == INVALID_ID_64)
    {
        id                            = geometry_system_create_geometry(&ui_sys_state_ptr->ui_geometry_config, false);
        ui_sys_state_ptr->geometry_id = id;
    }
    else
    {
        geometry_system_update_geometry(&ui_sys_state_ptr->ui_geometry_config, id);
    }

    ui_sys_state_ptr->root.nodes.~darray();
    ui_sys_state_ptr->root.init(ui_sys_state_ptr->arena);
    ui_sys_state_ptr->local_index_offset              = 0;
    ui_sys_state_ptr->ui_geometry_config.vertex_count = 0;
    ui_sys_state_ptr->ui_geometry_config.index_count  = 0;

    return id;
}
