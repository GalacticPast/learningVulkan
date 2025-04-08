#include "core/input.hpp"
#include "core/dmemory.hpp"
#include "core/event.hpp"
#include "core/logger.hpp"

typedef struct keyboard_state
{
    bool keys[256];
} keyboard_state;

typedef struct mouse_state
{
    s16 x;
    s16 y;
    u8 buttons[BUTTON_MAX_BUTTONS];
} mouse_state;

typedef struct input_state
{
    keyboard_state keyboard_current;
    keyboard_state keyboard_previous;
    mouse_state mouse_current;
    mouse_state mouse_previous;
} input_state;

// Internal input state pointer
static input_state *input_state_ptr;

void input_system_startup(u64 *memory_requirement, void *state)
{
    *memory_requirement = sizeof(input_state);
    if (state == 0)
    {
        return;
    }
    DINFO("Initializing input system.");
    dzero_memory(state, sizeof(input_state));
    input_state_ptr = (input_state *)state;
}

void input_system_shutdown(void *state)
{
    // TODO: Add shutdown routines when needed.
    input_state_ptr = 0;
}

void input_update(f64 delta_time)
{
    if (!input_state_ptr)
    {
        return;
    }

    // Copy current states to previous states.
    dcopy_memory(&input_state_ptr->keyboard_previous, &input_state_ptr->keyboard_current, sizeof(keyboard_state));
    dcopy_memory(&input_state_ptr->mouse_previous, &input_state_ptr->mouse_current, sizeof(mouse_state));
}

void input_process_key(keys key, bool pressed)
{
    // Only handle this if the state actually changed.
    if (input_state_ptr && input_state_ptr->keyboard_current.keys[key] != pressed)
    {
        // Update internal input_state_ptr->
        input_state_ptr->keyboard_current.keys[key] = pressed;

        if (key == KEY_LALT)
        {
            DINFO("Left alt %s.", pressed ? "pressed" : "released");
        }
        else if (key == KEY_RALT)
        {
            DINFO("Right alt %s.", pressed ? "pressed" : "released");
        }

        if (key == KEY_LCONTROL)
        {
            DINFO("Left ctrl %s.", pressed ? "pressed" : "released");
        }
        else if (key == KEY_RCONTROL)
        {
            DINFO("Right ctrl %s.", pressed ? "pressed" : "released");
        }

        if (key == KEY_LSHIFT)
        {
            DINFO("Left shift %s.", pressed ? "pressed" : "released");
        }
        else if (key == KEY_RSHIFT)
        {
            DINFO("Right shift %s.", pressed ? "pressed" : "released");
        }

        // Fire off an event for immediate processing.
        event_context context;
        context.data.u16[0] = key;
        event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, context);
    }
}

void input_process_button(buttons button, bool pressed)
{
    // If the state changed, fire an event.
    if (input_state_ptr->mouse_current.buttons[button] != pressed)
    {
        input_state_ptr->mouse_current.buttons[button] = pressed;

        // Fire the event.
        event_context context;
        context.data.u16[0] = button;
        event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, context);
    }
}

void input_process_mouse_move(s16 x, s16 y)
{
    // Only process if actually different
    if (input_state_ptr->mouse_current.x != x || input_state_ptr->mouse_current.y != y)
    {
        // NOTE: Enable this if debugging.
        // DDEBUG("Mouse pos: %i, %i!", x, y);

        // Update internal input_state_ptr->
        input_state_ptr->mouse_current.x = x;
        input_state_ptr->mouse_current.y = y;

        // Fire the event.
        event_context context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        event_fire(EVENT_CODE_MOUSE_MOVED, context);
    }
}

void input_process_mouse_wheel(s8 z_delta)
{
    // NOTE: no internal state to update.

    // Fire the event.
    event_context context;
    context.data.u8[0] = z_delta;
    event_fire(EVENT_CODE_MOUSE_WHEEL, context);
}

bool input_is_key_down(keys key)
{
    if (!input_state_ptr)
    {
        return false;
    }
    return input_state_ptr->keyboard_current.keys[key] == true;
}

bool input_is_key_up(keys key)
{
    if (!input_state_ptr)
    {
        return true;
    }
    return input_state_ptr->keyboard_current.keys[key] == false;
}

bool input_was_key_down(keys key)
{
    if (!input_state_ptr)
    {
        return false;
    }
    return input_state_ptr->keyboard_previous.keys[key] == true;
}

bool input_was_key_up(keys key)
{
    if (!input_state_ptr)
    {
        return true;
    }
    return input_state_ptr->keyboard_previous.keys[key] == false;
}

// mouse input
bool input_is_button_down(buttons button)
{
    if (!input_state_ptr)
    {
        return false;
    }
    return input_state_ptr->mouse_current.buttons[button] == true;
}

bool input_is_button_up(buttons button)
{
    if (!input_state_ptr)
    {
        return true;
    }
    return input_state_ptr->mouse_current.buttons[button] == false;
}

bool input_was_button_down(buttons button)
{
    if (!input_state_ptr)
    {
        return false;
    }
    return input_state_ptr->mouse_previous.buttons[button] == true;
}

bool input_was_button_up(buttons button)
{
    if (!input_state_ptr)
    {
        return true;
    }
    return input_state_ptr->mouse_previous.buttons[button] == false;
}

void input_get_mouse_position(s32 *x, s32 *y)
{
    if (!input_state_ptr)
    {
        *x = 0;
        *y = 0;
        return;
    }
    *x = input_state_ptr->mouse_current.x;
    *y = input_state_ptr->mouse_current.y;
}

void input_get_previous_mouse_position(s32 *x, s32 *y)
{
    if (!input_state_ptr)
    {
        *x = 0;
        *y = 0;
        return;
    }
    *x = input_state_ptr->mouse_previous.x;
    *y = input_state_ptr->mouse_previous.y;
}
