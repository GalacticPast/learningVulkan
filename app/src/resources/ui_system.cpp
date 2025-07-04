#include "containers/darray.hpp"
#include "containers/dhashtable.hpp"

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
#define DEFAULT_WINDOW_DIMENSIONS (vec2){200, 30}
#define DEFAULT_SLIDER_DIMENSIONS (vec2){200, 20}
#define DEFAULT_SLIDER_KNOB_DIMENSIONS (vec2){10, 20}

struct ui_element
{
    u64 id = INVALID_ID_64;

    ui_element_type type;

    vec2    text_position;
    vec2    text_dimensions;
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

    bool is_stale = true;
    u64  hot_id   = INVALID_ID_64;

    u64             geometry_id = INVALID_ID_64;
    geometry_config ui_geometry_config;
    u32             local_index_offset = INVALID_ID;

    font_data *system_font;

    u64             hover_win_id = INVALID_ID_64;
    dhashtable<u32> window_indicies;
    ui_element      root;

    vec2 elem_padding;

    dhashtable<vec2> container_dimensions;
    dhashtable<vec2> container_position;
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
static void             _adjust_sizes(ui_element *element);
static void             _transition_state(ui_element *element);
static bool             _generate_quad_config(vec2 position, vec2 dimensions, vec4 color);
static bool             _check_mouse_collision(box box);
static bool             _check_box_if_hot(u64 id, box box1);
static bool             _check_if_pressed(u64 id);

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

    ui_sys_state_ptr->container_dimensions.c_init(resource_arena, 4096);
    ui_sys_state_ptr->container_dimensions.is_non_resizable = true;

    ui_sys_state_ptr->container_position.c_init(resource_arena, 4096);
    ui_sys_state_ptr->container_position.is_non_resizable = true;

    ui_sys_state_ptr->hover_win_id = INVALID_ID_64;
    ui_sys_state_ptr->window_indicies.c_init(resource_arena, 1024);

    ui_sys_state_ptr->elem_padding = {8, 8};

    return true;
}

bool ui_system_set_arena(arena *arena)
{
    DASSERT(arena);
    ui_sys_state_ptr->arena = arena;

    return true;
}

void ui_system_start_frame()
{
    static arena *arena = ui_sys_state_ptr->arena;
    ui_sys_state_ptr->root.nodes.c_init(arena);
    ui_sys_state_ptr->hover_win_id = INVALID_ID_64;
}

bool ui_window(u64 parent_id, u64 id, dstring label, u32 num_rows, u32 num_columns)
{
    DASSERT(ui_sys_state_ptr);

    ui_element window{};
    window.type = UI_WINDOW;
    window.id   = id;

    bool state = _check_if_pressed(id);

    window.color = DARKGRAY;

    window.num_rows    = 3;
    window.num_columns = 3;

    window.text = label;

    window.init(ui_sys_state_ptr->arena);

    _insert_element(parent_id, id, &ui_sys_state_ptr->root, &window);

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
    slider.color    = DARKPURPLE;
    slider.text     = "slider";
    slider.value    = *value;

    slider.row_pos = row;
    slider.col_pos = column;

    bool state = _check_if_pressed(id);

    bool return_val = id == ui_sys_state_ptr->hot_id && input_is_button_down(BUTTON_LEFT);
    if (return_val)
    {
        s32 prev_x, prev_y = 0;
        input_get_previous_mouse_position(&prev_x, &prev_y);

        s32 x, y = 0;
        input_get_mouse_position(&x, &y);

        s32 delta_x = x - prev_x;
        s32 delta_y = y - prev_y;

        slider.value += delta_x;
        slider.value  = DMIN(max, slider.value);
        slider.value  = DMAX(min, slider.value);

        *value = slider.value;
    }

    _insert_element(parent_id, id, &ui_sys_state_ptr->root, &slider);

    return return_val;
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

// box1 top left corner
// box2 top left corner
bool _check_collision(box a, box b)
{
    return (a.position.x < b.position.x + b.dimensions.w && a.position.x + a.dimensions.w > b.position.x &&
            a.position.y < b.position.y + b.dimensions.h && a.position.y + a.dimensions.h > b.position.y);
}

static bool _check_mouse_collision(box box1)
{
    s32 mouse_x, mouse_y = 0;
    input_get_mouse_position(&mouse_x, &mouse_y);
    box box2 = {{(float)mouse_x, (float)mouse_y}, {1, 1}};

    bool res = _check_collision(box1, box2);
    return res;
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
        if (_check_collision(a, b) && ui_sys_state_ptr->is_stale)
        {
            ui_sys_state_ptr->hot_id   = id;
            ui_sys_state_ptr->is_stale = false;
            is_hot                     = true;
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
        u32 *index = ui_sys_state_ptr->window_indicies.find(id);
        if (index)
        {
            u32         ind  = *index;
            ui_element *elem = &root->nodes[ind];
            {
                elem->id                      = element->id;
                elem->type                    = element->type;
                elem->text_position           = element->text_position;
                elem->text_dimensions         = element->text_dimensions;
                elem->text                    = element->text;
                elem->value                   = element->value; // slider value if it is a slider
                elem->min                     = element->min;
                elem->max                     = element->max;
                elem->num_rows                = element->num_rows;
                elem->num_columns             = element->num_columns;
                elem->row_pos                 = element->row_pos;
                elem->col_pos                 = element->col_pos;
                elem->position                = element->position; // absolute position
                elem->dimensions              = element->dimensions;
                elem->interactable_dimensions = element->interactable_dimensions;
                elem->color                   = element->color;
                elem->nodes                   = element->nodes; // aka children
            }
            // HACK:
            root->nodes.length = DMAX(ind + 1, root->nodes.length);
        }
        else
        {
            u32 ind = root->nodes.size();
            root->nodes.push_back(*element);
            ui_sys_state_ptr->window_indicies.insert(id, ind);
        }
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
    vec2 position = element->position;

    static vec2 padding = ui_sys_state_ptr->elem_padding;

    if (element->type == UI_WINDOW)
    {

        vec2 *prev_pos = ui_sys_state_ptr->container_position.find(element->id);
        if (!prev_pos)
        {
            s32 x    = drandom_in_range(0, 800);
            s32 y    = drandom_in_range(0, 600);
            position = {(float)x, (float)y};
            ui_sys_state_ptr->container_position.insert(element->id, position);
        }
        else
        {
            position = *prev_pos;
        }

        bool return_val = element->id == ui_sys_state_ptr->hot_id && input_is_button_down(BUTTON_LEFT);
        if (return_val)
        {
            s32 prev_x, prev_y = 0;
            input_get_previous_mouse_position(&prev_x, &prev_y);

            s32 x, y = 0;
            input_get_mouse_position(&x, &y);

            s32 delta_x = x - prev_x;
            s32 delta_y = y - prev_y;

            if (prev_pos)
            {
                position.x += delta_x;
                position.y += delta_y;
            }
        }

        ui_sys_state_ptr->container_position.update(element->id, position);

        element->position      = position;
        element->text_position = position;

        position.y += element->interactable_dimensions.y;
    }

    position.x += padding.x;

    u32 length = element->nodes.size();
    if (length)
    {
        static arena *arena = ui_sys_state_ptr->arena;

        u32 num_rows    = element->num_rows;
        u32 num_columns = element->num_columns;
        // INFO: I might not need this tbh. This is implemented to guard againt the case where the elements were not
        // sorted by the grid row layouts
        //   for example we have 3 rows and 3 columsn
        //   the element array might have been initialied with the 0,0 as the first element 1,0 as the second and third
        //   might be the 0,1 element and so on.. so this is esentially sorting it by the rows. Well just storing the
        //   indicies for faster computation. :)
        darray<darray<u32>> grid_indicies;
        grid_indicies.c_init(arena, element->num_rows);
        grid_indicies.resize(element->num_rows);

        for (u32 i = 0; i < num_rows; i++)
        {
            grid_indicies[i].c_init(arena, num_columns);
            grid_indicies[i].resize(num_columns);
            grid_indicies[i].fill(INVALID_ID);
        }

        for (u32 i = 0; i < length; i++)
        {
            ui_element *elem = &element->nodes[i];

            u32 row_pos = elem->row_pos;
            u32 col_pos = elem->col_pos;

            grid_indicies[row_pos][col_pos] = i;
        }
        for (u32 i = 0; i < num_rows; i++)
        {
            u32 row_ind = grid_indicies[i][0];
            if (row_ind == INVALID_ID)
            {
                break;
            }

            ui_element *row = &element->nodes[row_ind];
            row->position   = position;

            position.y += row->dimensions.y + padding.y;
        }
        // calculate positions according to the set grid
        for (u32 i = 0; i < num_rows; i++)
        {
            u32 row_ind = grid_indicies[i][0];
            if (row_ind == INVALID_ID)
            {
                break;
            }
            ui_element *row = &element->nodes[row_ind];

            position    = row->position;
            position.x += row->dimensions.x + padding.x;

            for (u32 j = 1; j < num_columns; j++)
            {
                u32 ind = grid_indicies[i][j];
                if (ind == INVALID_ID)
                {
                    continue;
                }
                ui_element *elem = &element->nodes[ind];
                elem->position   = position;

                position.x += elem->dimensions.x + padding.x;
            }
        }

        for (u32 i = 0; i < num_rows; i++)
        {
            for (u32 j = 0; j < num_columns; j++)
            {
                u32 ind = grid_indicies[i][j];
                if (ind == INVALID_ID)
                {
                    continue;
                }
                ui_element *elem = &element->nodes[ind];

                switch (elem->type)
                {
                case UI_BUTTON:
                case UI_WINDOW: {
                    // center the text
                    elem->text_position = elem->position;
                }
                break;
                case UI_SLIDER: {
                    elem->text_position.x = elem->position.x + elem->dimensions.x;
                    elem->text_position.y = elem->position.y;
                }
                break;
                case UI_UNKNOWN: {
                }
                break;
                }
            }
        }

        for (u32 i = 0; i < num_rows; i++)
        {
            grid_indicies[i].~darray();
        }
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
            dimensions.y += element->nodes[i].dimensions.y;
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
    // FIXME: what if we resize then the text dimensions?? or what if we resize the container shouldn't that cutoff the
    // text??
    text_dimensions.x         = DMAX(txt_width, dimensions.x);
    text_dimensions.y        += 20;
    element->text_dimensions  = text_dimensions;

    switch (element->type)
    {
    case UI_WINDOW: {

        vec2 *prev_dimensions = ui_sys_state_ptr->container_dimensions.find(element->id);
        vec2  def             = DEFAULT_WINDOW_DIMENSIONS;
        if (prev_dimensions)
        {
            dimensions.x = DMAX(prev_dimensions->x, dimensions.x);
            dimensions.y = DMAX(prev_dimensions->y, dimensions.y);

            box  resize_box;
            vec2 position         = *ui_sys_state_ptr->container_position.find(element->id);
            resize_box.dimensions = {10, 10};
            resize_box.position.x = position.x + dimensions.x - resize_box.dimensions.x;
            resize_box.position.y = position.y + dimensions.y - resize_box.dimensions.y;

            bool is_colliding = _check_mouse_collision(resize_box);
            if (is_colliding && input_is_button_down(BUTTON_LEFT))
            {
                s32 prev_x, prev_y = 0;
                input_get_previous_mouse_position(&prev_x, &prev_y);

                s32 x, y = 0;
                input_get_mouse_position(&x, &y);

                s32 delta_x = x - prev_x;
                s32 delta_y = y - prev_y;

                dimensions.x += delta_x;
                dimensions.y += delta_y;
            }
        }
        else
        {
            dimensions.y += text_dimensions.y;

            dimensions.x = DMAX(def.x, dimensions.x);
            dimensions.y = DMAX(def.y, dimensions.y);

            ui_sys_state_ptr->container_dimensions.insert(element->id, dimensions);
        }

        ui_sys_state_ptr->container_dimensions.update(element->id, dimensions);

        element->dimensions              = dimensions;
        vec2 header_dimensions           = {dimensions.x, 25};
        element->interactable_dimensions = header_dimensions;
    }
    break;
    case UI_BUTTON: {
        dimensions                       = text_dimensions;
        element->interactable_dimensions = dimensions;
        element->dimensions              = dimensions;
    }
    break;
    case UI_SLIDER: {
        element->dimensions               = DEFAULT_SLIDER_DIMENSIONS;
        element->dimensions.x            += text_dimensions.x;
        element->interactable_dimensions  = DEFAULT_SLIDER_KNOB_DIMENSIONS;
    }
    break;
    case UI_UNKNOWN: {
        DERROR("UI element %llu is unknown", element->id);
    }
    break;
    }
}

static void _transition_state(ui_element *element)
{
    if (element == nullptr)
    {
        return;
    }
    u32 size = element->nodes.size();
    for (u32 i = 0; i < size; i++)
    {
        ui_element *elem = &element->nodes[i];
        _transition_state(elem);
    }

    box a = {element->position, element->interactable_dimensions};

    if (element->type == UI_SLIDER)
    {
        vec2 knob_position = {element->position.x + element->value, element->position.y};
        a.position         = knob_position;
    }

    bool is_hot     = _check_box_if_hot(element->id, a);
    bool is_pressed = _check_if_pressed(element->id);

    if (is_hot && ui_sys_state_ptr->hover_win_id == INVALID_ID_64)
    {
        ui_sys_state_ptr->hover_win_id = element->id;
    }

    if (element->type == UI_WINDOW && is_hot)
    {
        box  b     = {element->position, element->dimensions};
        bool check = _check_mouse_collision(b);

        if (check && element->id == ui_sys_state_ptr->hover_win_id && input_was_button_down(BUTTON_LEFT) &&
            input_is_button_up(BUTTON_LEFT))
        {
            u32       *index = ui_sys_state_ptr->window_indicies.find(element->id);
            ui_element elem  = *element;
            // well so that we dont pop and push back again.
            if (ui_sys_state_ptr->root.nodes[*index].id == elem.id)
            {
                ui_sys_state_ptr->root.nodes.pop_at(*index);
                ui_sys_state_ptr->root.nodes.push_back(elem);
                ui_sys_state_ptr->hover_win_id = elem.id;
            }
        }
    }
    return;
}

static void _generate_element_geometry(ui_element *element)
{
    if (element == nullptr)
    {
        return;
    }

    vec2 border_dimensions = {element->dimensions.x + 4, element->dimensions.y + 4};
    vec2 position          = {element->position.x - 2, element->position.y - 2};
    _generate_quad_config(position, border_dimensions, BORDER_COLOR);

    _generate_quad_config(element->position, element->dimensions, element->color);
    _generate_quad_config(element->position, element->interactable_dimensions, element->color);

    if (element->type == UI_WINDOW)
    {
        box resize_box;
        resize_box.dimensions = {10, 10};
        resize_box.position.x = element->position.x + element->dimensions.x - resize_box.dimensions.x;
        resize_box.position.y = element->position.y + element->dimensions.y - resize_box.dimensions.y;
        _generate_quad_config(resize_box.position, resize_box.dimensions, YELLOW);
    }

    if (element->type == UI_SLIDER)
    {
        vec2 knob_dimensions = DEFAULT_SLIDER_KNOB_DIMENSIONS;
        vec2 knob_position   = {element->position.x + element->value, element->position.y};

        _generate_quad_config(knob_position, knob_dimensions, GREEN);
    }

    vec4 text_color = WHITE;
    if (element->id == ui_sys_state_ptr->hot_id)
    {
        text_color = YELLOW;
    }

    ui_text(element->id, &element->text, element->text_position, text_color);

    u32 length = element->nodes.size();
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

// this will also adjust the position of the final text.
static void _adjust_sizes(ui_element *element) // if it needs to
{
    if (element == nullptr)
    {
        return;
    }
    static vec2 elem_padding = ui_sys_state_ptr->elem_padding;

    if (element->type == UI_WINDOW)
    {
        u32  length     = element->nodes.size();
        vec2 dimensions = element->dimensions;

        if (length)
        {
            static arena *arena = ui_sys_state_ptr->arena;

            u32 num_rows    = element->num_rows;
            u32 num_columns = element->num_columns;
            // INFO: I might not need this tbh. This is implemented to guard againt the case where the elements were not
            // sorted by the grid row layouts
            //   for example we have 3 rows and 3 columsn
            //   the element array might have been initialied with the 0,0 as the first element 1,0 as the second and
            //   third might be the 0,1 element and so on.. so this is esentially sorting it by the rows. Well just
            //   storing the indicies for faster computation. :)
            darray<darray<u32>> grid_indicies;
            grid_indicies.c_init(arena, num_rows);
            grid_indicies.resize(num_rows);

            for (u32 i = 0; i < num_rows; i++)
            {
                grid_indicies[i].c_init(arena, num_columns);
                grid_indicies[i].resize(num_columns);
                grid_indicies[i].fill(INVALID_ID);
            }

            for (u32 i = 0; i < length; i++)
            {
                ui_element *elem = &element->nodes[i];

                u32 row_pos = elem->row_pos;
                u32 col_pos = elem->col_pos;

                grid_indicies[row_pos][col_pos] = i;
            }

            u32 width = 0;

            u32 height = element->nodes[grid_indicies[0][0]].position.y - element->position.y;

            for (u32 i = 0; i < num_rows; i++)
            {

                u32 row_width  = 0;
                u32 row_height = 0;
                for (u32 j = 0; j < num_columns; j++)
                {
                    u32 ind = grid_indicies[i][j];
                    if (ind == INVALID_ID)
                    {
                        continue;
                    }

                    ui_element *elem = &element->nodes[ind];

                    row_height = DMAX(elem->dimensions.y, row_height);
                    switch (elem->type)
                    {
                    case UI_BUTTON:
                    case UI_WINDOW: {
                        row_width += elem->dimensions.x + elem_padding.x;
                    }
                    break;
                    case UI_SLIDER: {

                        row_width += elem->dimensions.x + elem->text_dimensions.x + elem_padding.x + elem_padding.x;
                    }
                    break;
                    case UI_UNKNOWN: {
                    }
                    break;
                    }
                }
                width   = DMAX(width, row_width);
                height += row_height;
            }

            dimensions.x        = DMAX(width, dimensions.x);
            dimensions.y        = DMAX(height, dimensions.y);
            element->dimensions = dimensions;

            ui_sys_state_ptr->container_dimensions.update(element->id, dimensions);

            for (u32 i = 0; i < num_rows; i++)
            {
                grid_indicies[i].~darray();
            }
        }
    }
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

u64 ui_system_end_frame()
{

    ui_sys_state_ptr->is_stale = true;

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
        _adjust_sizes(elem);
    }

    for (s32 i = length - 1; i >= 0; i--)
    {
        ui_element *elem = &ui_sys_state_ptr->root.nodes[i];
        _transition_state(elem);
    }

    for (u32 i = 0; i < length; i++)
    {
        ui_element *elem = &ui_sys_state_ptr->root.nodes[i];
        ui_sys_state_ptr->window_indicies.update(elem->id, i);
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

    ui_sys_state_ptr->local_index_offset              = 0;
    ui_sys_state_ptr->ui_geometry_config.vertex_count = 0;
    ui_sys_state_ptr->ui_geometry_config.index_count  = 0;
    ui_sys_state_ptr->hover_win_id                    = INVALID_ID_64;

    if (ui_sys_state_ptr->is_stale)
    {
        ui_sys_state_ptr->hot_id = INVALID_ID_64;
    }

    return id;
}
