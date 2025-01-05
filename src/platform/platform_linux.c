#include "containers/array.h"

#include "core/events.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/strings.h"

#include "platform/platform.h"

#ifdef PLATFORM_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h> // nanosleep
#else
#include <unistd.h> // usleep
#endif

#ifdef PLATFORM_LINUX_X11

#include <X11/XKBlib.h>   // sudo apt-get install libx11-dev
#include <X11/Xlib-xcb.h> // sudo apt-get install libxkbcommon-x11-dev
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <xcb/xcb.h>

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

#include "vulkan/vulkan_types.h"

typedef struct internal_state
{
    Display          *display;
    xcb_connection_t *connection;
    xcb_window_t      window;
    xcb_screen_t     *screen;
    xcb_atom_t        wm_protocols;
    xcb_atom_t        wm_delete_win;
} internal_state;

// INFO: x11/xcb reference https://www.x.org/releases/X12R7.6/doc/libxcb/tutorial/index.html

keys xcb_translate_keycode(xcb_keycode_t code);

b8 platform_startup(platform_state *plat_state, const char *application_name, s32 x, s32 y, s32 width, s32 height)
{
    INFO("Initializing linux-x11 platform...");

    // Create the internal state.
    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state *state      = (internal_state *)plat_state->internal_state;

    // Connect to X
    state->display = XOpenDisplay(NULL);

    // Turn off key repeats.
    XAutoRepeatOff(state->display);

    // Retrieve the connection from the display.
    state->connection = XGetXCBConnection(state->display);

    if (xcb_connection_has_error(state->connection))
    {
        FATAL("Failed to connect to X server via XCB.");
        return false;
    }

    // Get data from the X server
    const struct xcb_setup_t *setup = xcb_get_setup(state->connection);

    // Loop through screens using iterator
    xcb_screen_iterator_t it       = xcb_setup_roots_iterator(setup);
    s32                   screen_p = 0;
    for (s32 s = screen_p; s > 0; s--)
    {
        xcb_screen_next(&it);
    }

    // After screens have been looped through, assign it.
    state->screen = it.data;

    // Allocate a XID for the window to be created.
    state->window = xcb_generate_id(state->connection);

    // Register event types.
    // XCB_CW_BACK_PIXEL = filling then window bg with a single colour
    // XCB_CW_EVENT_MASK is required.
    u32 event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

    // Listen for keyboard and mouse buttons
    u32 event_values = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION |
                       XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    // Values to be sent over XCB (bg colour, events)
    u32 value_list[] = {state->screen->black_pixel, event_values};

    // Create the window
    UNUSED xcb_void_cookie_t cookie = xcb_create_window(state->connection,
                                                        XCB_COPY_FROM_PARENT, // depth
                                                        state->window,
                                                        state->screen->root,           // parent
                                                        (u16)x,                        // x
                                                        (u16)y,                        // y
                                                        (u16)width,                    // width
                                                        (u16)height,                   // height
                                                        0,                             // No border
                                                        XCB_WINDOW_CLASS_INPUT_OUTPUT, // class
                                                        state->screen->root_visual, event_mask, value_list);

    // Change the title
    xcb_change_property(state->connection, XCB_PROP_MODE_REPLACE, state->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
                        8, // data should be viewed 8 bits at a time
                        (u32)strlen(application_name), application_name);

    // Tell the server to notify when the window manager
    // attempts to destroy the window.
    xcb_intern_atom_cookie_t wm_delete_cookie    = xcb_intern_atom(state->connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(state->connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *wm_delete_reply     = xcb_intern_atom_reply(state->connection, wm_delete_cookie, NULL);
    xcb_intern_atom_reply_t *wm_protocols_reply  = xcb_intern_atom_reply(state->connection, wm_protocols_cookie, NULL);
    state->wm_delete_win                         = wm_delete_reply->atom;
    state->wm_protocols                          = wm_protocols_reply->atom;

    xcb_change_property(state->connection, XCB_PROP_MODE_REPLACE, state->window, wm_protocols_reply->atom, 4, 32, 1, &wm_delete_reply->atom);

    // Map the window to the screen
    xcb_map_window(state->connection, state->window);

    // Flush the stream
    s32 stream_result = xcb_flush(state->connection);
    if (stream_result <= 0)
    {
        FATAL("An error occurred when flusing the stream: %d", stream_result);
        return false;
    }

    INFO("Initialized linux-x11 platform.");

    return true;
}

b8 platform_pump_messages(platform_state *plat_state)
{
    internal_state *state = (internal_state *)plat_state->internal_state;

    xcb_generic_event_t *event = xcb_wait_for_event(state->connection);

    switch (event->response_type & ~0x80)
    {
        case XCB_EXPOSE: {
        }
        break;
        case XCB_KEY_PRESS:
        case XCB_KEY_RELEASE: {

            b8 state = (event->response_type & ~0x80) == XCB_KEY_PRESS ? 1 : 0;

            xcb_key_press_event_t *key_event = (xcb_key_press_event_t *)event;

            keys key = xcb_translate_keycode(key_event->detail);

            if (key == KEY_ESCAPE)
            {
                event_context context = {};
                event_fire(ON_APPLICATION_QUIT, context);
            }
            input_process_key(key, state);
        }
        break;
    }
    free(event);
    return true;
}

void platform_shutdown(platform_state *plat_state)
{
    // Simply cold-cast to the known type.
    internal_state *state = (internal_state *)plat_state->internal_state;

    // Turn key repeats back on since this is global for the OS... just... wow.
    XAutoRepeatOn(state->display);

    xcb_destroy_window(state->connection, state->window);
}

b8 platform_create_vulkan_surface(platform_state *plat_state, struct vulkan_context *context)
{

    internal_state *state = (internal_state *)plat_state->internal_state;

    VkXcbSurfaceCreateInfoKHR xcb_surface_create_info = {};

    xcb_surface_create_info.sType      = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    xcb_surface_create_info.pNext      = 0;
    xcb_surface_create_info.flags      = 0;
    xcb_surface_create_info.connection = state->connection;
    xcb_surface_create_info.window     = state->window;

    VK_CHECK(vkCreateXcbSurfaceKHR(context->instance, &xcb_surface_create_info, 0, &context->surface));

    return true;
}

void *platform_get_required_surface_extensions(char **required_extension_names)
{
    char *surface_name       = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
    required_extension_names = array_push_value(required_extension_names, &surface_name);
    return (void *)required_extension_names;
}

keys xcb_translate_keycode(xcb_keycode_t code)
{
    switch (code)
    {
        case 9: {
            return KEY_ESCAPE;
        }
        case 24: {
            return KEY_Q;
        }
        case 25: {
            return KEY_W;
        }
        case 26: {
            return KEY_E;
        }
        case 27: {
            return KEY_R;
        }
        case 28: {
            return KEY_T;
        }
        case 29: {
            return KEY_Y;
        }
        case 30: {
            return KEY_U;
        }
        case 31: {
            return KEY_I;
        }
        case 32: {
            return KEY_O;
        }
        case 33: {
            return KEY_P;
        }
        case 38: {
            return KEY_A;
        }
        case 39: {
            return KEY_S;
        }
        case 40: {
            return KEY_D;
        }
        case 41: {
            return KEY_F;
        }
        case 42: {
            return KEY_G;
        }
        case 43: {
            return KEY_H;
        }
        case 44: {
            return KEY_J;
        }
        case 45: {
            return KEY_K;
        }
        case 46: {
            return KEY_L;
        }
        case 52: {
            return KEY_Z;
        }
        case 53: {
            return KEY_X;
        }
        case 54: {
            return KEY_C;
        }
        case 55: {
            return KEY_V;
        }
        case 56: {
            return KEY_B;
        }
        case 57: {
            return KEY_N;
        }
        case 58: {
            return KEY_M;
        }
    }
    return UINT32_MAX;
}

#endif

#if PLATFORM_LINUX_WAYLAND

//
#include "wayland/xdg-shell-client-protocol.h"
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

// TODO: this is temporary migrate the shared memory to use vulkan instead of mannually allocating buffers
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
//

#include "platform.h"

#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

#include "vulkan/vulkan_types.h"

/* Wayland code */
typedef struct internal_state
{
    /* Globals */
    struct wl_display    *wl_display;
    struct wl_registry   *wl_registry;
    struct wl_shm        *wl_shm;
    struct wl_seat       *wl_seat;
    struct wl_compositor *wl_compositor;
    struct xdg_wm_base   *xdg_wm_base;

    /* Objects */
    struct wl_surface   *wl_surface;
    struct xdg_surface  *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;

    /* input */
    struct wl_keyboard *wl_keyboard;
    struct wl_mouse    *wl_mouse;

    s32 width;
    s32 height;

} internal_state;

u32 translate_keycode(u32 key);

static void wl_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard, u32 format, s32 fd, u32 size)
{
    // struct internal_state *state = data;
}

static void wl_keyboard_enter(void *data, struct wl_keyboard *wl_keyboard, u32 serial, struct wl_surface *surface, struct wl_array *keys)
{
    DEBUG("Keyboard in scope");
}

static void wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard, u32 serial, u32 time, u32 key, u32 state)
{
    // struct internal_state *internal_state = data;

    keys code = translate_keycode(key);

    if (code == KEY_ESCAPE)
    {
        event_context context = {};
        context.data.u32[0]   = code;
        event_fire(ON_APPLICATION_QUIT, context);
    }
}

static void wl_keyboard_leave(void *data, struct wl_keyboard *wl_keyboard, u32 serial, struct wl_surface *surface)
{
    DEBUG("Mouse not in scope");
}

static void wl_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard, u32 serial, u32 mods_depressed, u32 mods_latched, u32 mods_locked, u32 group)
{
}

static void wl_keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard, s32 rate, s32 delay)
{
}

static const struct wl_keyboard_listener wl_keyboard_listener = {
    .keymap      = wl_keyboard_keymap,
    .enter       = wl_keyboard_enter,
    .leave       = wl_keyboard_leave,
    .key         = wl_keyboard_key,
    .modifiers   = wl_keyboard_modifiers,
    .repeat_info = wl_keyboard_repeat_info,
};
//

static void wl_seat_capabilites(void *data, struct wl_seat *wl_seat, u32 capabilities)
{
    internal_state *state = data;

    // TODO: mouse events
    //

    b8 have_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

    if (have_keyboard && state->wl_keyboard == NULL)
    {
        state->wl_keyboard = wl_seat_get_keyboard(state->wl_seat);
        wl_keyboard_add_listener(state->wl_keyboard, &wl_keyboard_listener, state);
    }
    else if (!have_keyboard && state->wl_keyboard != NULL)
    {
        wl_keyboard_release(state->wl_keyboard);
        state->wl_keyboard = NULL;
    }
}
static void wl_seat_name(void *data, struct wl_seat *wl_seat, const char *name)
{
}

struct wl_seat_listener wl_seat_listener = {.capabilities = wl_seat_capabilites, .name = wl_seat_name};

// actual surface
static void xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, s32 width, s32 height, struct wl_array *states)
{
    internal_state *state = (internal_state *)data;

    if (state->width != width || state->height != height)
    {
        state->width  = width;
        state->height = height;
    }
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
}
struct xdg_toplevel_listener xdg_toplevel_listener = {.configure = xdg_toplevel_configure, .close = xdg_toplevel_close};
//

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, u32 serial)
{
    internal_state *state = (internal_state *)data;

    xdg_surface_ack_configure(xdg_surface, serial);
    wl_surface_commit(state->wl_surface);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

// shell
static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, u32 serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};
//

static void registry_global(void *data, struct wl_registry *wl_registry, u32 name, const char *interface, u32 version)
{
    internal_state *state = data;

    if (string_compare(interface, wl_compositor_interface.name))
    {
        state->wl_compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, 4);
    }
    else if (string_compare(interface, xdg_wm_base_interface.name))
    {
        state->xdg_wm_base = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(state->xdg_wm_base, &xdg_wm_base_listener, state);
    }
    else if (string_compare(interface, wl_seat_interface.name))
    {
        state->wl_seat = wl_registry_bind(wl_registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(state->wl_seat, &wl_seat_listener, state);
    }
}

static void registry_global_remove(void *data, struct wl_registry *wl_registry, u32 name)
{
    /* This space deliberately left blank */
}

static const struct wl_registry_listener wl_registry_listener = {
    .global        = registry_global,
    .global_remove = registry_global_remove,
};

b8 platform_startup(platform_state *plat_state, const char *application_name, s32 x, s32 y, s32 width, s32 height)
{
    INFO("Initializing linux-Wayland platform...");
    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state *state      = (internal_state *)plat_state->internal_state;

    state->width  = width;
    state->height = height;

    state->wl_display = wl_display_connect(NULL);
    if (!state->wl_display)
    {
        return false;
    }

    state->wl_registry = wl_display_get_registry(state->wl_display);
    if (!state->wl_registry)
    {
        return false;
    }
    wl_registry_add_listener(state->wl_registry, &wl_registry_listener, state);

    wl_display_roundtrip(state->wl_display);

    state->wl_surface = wl_compositor_create_surface(state->wl_compositor);
    if (!state->wl_surface)
    {
        return false;
    }

    state->xdg_surface = xdg_wm_base_get_xdg_surface(state->xdg_wm_base, state->wl_surface);
    if (!state->xdg_surface)
    {
        return false;
    }

    xdg_surface_add_listener(state->xdg_surface, &xdg_surface_listener, state);

    state->xdg_toplevel = xdg_surface_get_toplevel(state->xdg_surface);

    if (!state->xdg_toplevel)
    {
        return false;
    }
    xdg_toplevel_add_listener(state->xdg_toplevel, &xdg_toplevel_listener, state);

    xdg_toplevel_set_title(state->xdg_toplevel, application_name);
    xdg_toplevel_set_app_id(state->xdg_toplevel, application_name);

    wl_surface_commit(state->wl_surface);

    INFO("Linux-Wayland platform initialized");

    return true;
}

b8 platform_pump_messages(platform_state *plat_state)
{
    internal_state *state = (internal_state *)plat_state->internal_state;

    s32 result = wl_display_dispatch(state->wl_display);

    return result == -1 ? false : true;
}

void platform_shutdown(platform_state *plat_state)
{
    internal_state *state = (internal_state *)plat_state->internal_state;

    xdg_toplevel_destroy(state->xdg_toplevel);
    xdg_surface_destroy(state->xdg_surface);
    wl_surface_destroy(state->wl_surface);
    wl_display_disconnect(state->wl_display);
}

void *platform_get_required_surface_extensions(char **required_extension_names)
{
    char *surface_name       = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
    required_extension_names = array_push_value(required_extension_names, &surface_name);
    return (void *)required_extension_names;
}

b8 platform_create_vulkan_surface(platform_state *plat_state, vulkan_context *context)
{
    internal_state *state = (internal_state *)plat_state->internal_state;

    VkWaylandSurfaceCreateInfoKHR create_surface_info = {};
    create_surface_info.sType                         = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    create_surface_info.pNext                         = 0;
    create_surface_info.flags                         = 0;
    create_surface_info.display                       = state->wl_display;
    create_surface_info.surface                       = state->wl_surface;

    VK_CHECK(vkCreateWaylandSurfaceKHR(context->instance, &create_surface_info, 0, &context->surface));

    return true;
}

void platform_get_framebuffer_size(platform_state *plat_state, u32 *width, u32 *height)
{
    internal_state *state = (internal_state *)plat_state->internal_state;

    *width  = state->width;
    *height = state->height;
}

u32 translate_keycode(u32 wl_keycode)
{
    switch (wl_keycode)
    {
        case 1: {
            return KEY_ESCAPE;
        }
        break;
        case 2: {
            return MAX_KEYS;
            // return KEY_NUMPAD1;
        }
        break;
        case 3: {
            return MAX_KEYS;
            // return KEY_NUMPAD2;
        }
        break;
        case 4: {
            return MAX_KEYS;
            // return KEY_NUMPAD3;
        }
        break;
        case 5: {
            return MAX_KEYS;
            // return KEY_NUMPAD4;
        }
        break;
        case 6: {
            return MAX_KEYS;
            // return KEY_NUMPAD5;
        }
        break;
        case 7: {
            return MAX_KEYS;
            // return KEY_NUMPAD6;
        }
        break;
        case 8: {
            return MAX_KEYS;
            // return KEY_NUMPAD7;
        }
        break;
        case 9: {
            return MAX_KEYS;
            // return KEY_NUMPAD8;
        }
        break;
        case 10: {
            return MAX_KEYS;
            // return KEY_NUMPAD9;
        }
        break;
        case 11: {
            return MAX_KEYS;
            // return KEY_NUMPADMAX_KEYS;
        }
        break;
        case 12: {
            return MAX_KEYS;
        }
        break;
        case 13: {
            return MAX_KEYS;
        }
        break;
        case 14: {
            return MAX_KEYS;
        }
        break;
        case 15: {
            return MAX_KEYS;
        }
        break;
        case 16: {
            return KEY_Q;
        }
        break;
        case 17: {
            return KEY_W;
        }
        break;
        case 18: {
            return KEY_E;
        }
        break;
        case 19: {
            return KEY_R;
        }
        break;
        case 20: {
            return KEY_T;
        }
        break;
        case 21: {
            return KEY_Y;
        }
        break;
        case 22: {
            return KEY_U;
        }
        break;
        case 23: {
            return KEY_I;
        }
        break;
        case 24: {
            return KEY_O;
        }
        break;
        case 25: {
            return KEY_P;
        }
        break;
        case 26: {
            return MAX_KEYS;
        }
        break;
        case 27: {
            return MAX_KEYS;
        }
        break;
        case 28: {
            return MAX_KEYS;
        }
        break;
        case 29: {
            return MAX_KEYS;
        }
        break;
        case 30: {
            return KEY_A;
        }
        break;
        case 31: {
            return KEY_S;
        }
        break;
        case 32: {
            return KEY_D;
        }
        break;
        case 33: {
            return KEY_F;
        }
        break;
        case 34: {
            return KEY_G;
        }
        break;
        case 35: {
            return KEY_H;
        }
        break;
        case 36: {
            return KEY_J;
        }
        break;
        case 37: {
            return KEY_K;
        }
        break;
        case 38: {
            return KEY_L;
        }
        break;
        case 39: {
            return MAX_KEYS;
        }
        break;
        case 40: {
            return MAX_KEYS;
        }
        break;
        case 41: {
            return MAX_KEYS;
        }
        break;
        case 42: {
            return MAX_KEYS;
        }
        break;
        case 43: {
            return MAX_KEYS;
        }
        break;
        case 44: {
            return KEY_Z;
        }
        break;
        case 45: {
            return KEY_X;
        }
        break;
        case 46: {
            return KEY_C;
        }
        break;
        case 47: {
            return KEY_V;
        }
        break;
        case 48: {
            return KEY_B;
        }
        break;
        case 49: {
            return KEY_N;
        }
        break;
        case 50: {
            return KEY_M;
        }
        break;
        case 51: {
            return MAX_KEYS;
        }
        break;
        case 52: {
            return MAX_KEYS;
        }
        break;
        case 53: {
            return MAX_KEYS;
        }
        break;
        case 54: {
            return MAX_KEYS;
        }
        break;
        case 55: {
            return MAX_KEYS;
        }
        break;
        case 56: {
            return MAX_KEYS;
        }
        break;
        case 57: {
            return MAX_KEYS;
        }
        break;
        case 58: {
            return MAX_KEYS;
        }
        break;
        case 59: {
            return MAX_KEYS;
        }
        break;
        case 60: {
            return MAX_KEYS;
        }
        break;
        case 61: {
            return MAX_KEYS;
        }
        break;
        case 62: {
            return MAX_KEYS;
        }
        break;
        case 63: {
            return MAX_KEYS;
        }
        break;
        case 64: {
            return MAX_KEYS;
        }
        break;
        case 65: {
            return MAX_KEYS;
        }
        break;
        case 66: {
            return MAX_KEYS;
        }
        break;
        case 67: {
            return MAX_KEYS;
        }
        break;
        case 68: {
            return MAX_KEYS;
        }
        break;
        case 69: {
            return MAX_KEYS;
        }
        break;
        case 70: {
            return MAX_KEYS;
        }
        break;
        case 71: {
            return MAX_KEYS;
        }
        break;
        case 72: {
            return MAX_KEYS;
        }
        break;
        case 73: {
            return MAX_KEYS;
        }
        break;
        case 74: {
            return MAX_KEYS;
        }
        break;
        case 75: {
            return MAX_KEYS;
        }
        break;
        case 76: {
            return MAX_KEYS;
        }
        break;
        case 77: {
            return MAX_KEYS;
        }
        break;
        case 78: {
            return MAX_KEYS;
        }
        break;
        case 79: {
            return MAX_KEYS;
        }
        break;
        case 80: {
            return MAX_KEYS;
        }
        break;
        case 81: {
            return MAX_KEYS;
        }
        break;
        case 82: {
            return MAX_KEYS;
        }
        break;
        case 83: {
            return MAX_KEYS;
        }
        break;
        case 84: {
            return MAX_KEYS;
        }
        break;
        case 85: {
            return MAX_KEYS;
        }
        break;
        case 86: {
            return MAX_KEYS;
        }
        break;
        case 87: {
            return MAX_KEYS;
        }
        break;
        case 88: {
            return MAX_KEYS;
        }
        break;
        case 89: {
            return MAX_KEYS;
        }
        break;
        case 90: {
            return MAX_KEYS;
        }
        break;
        case 91: {
            return MAX_KEYS;
        }
        break;
        case 92: {
            return MAX_KEYS;
        }
        break;
        case 93: {
            return MAX_KEYS;
        }
        break;
        case 94: {
            return MAX_KEYS;
        }
        break;
        case 95: {
            return MAX_KEYS;
        }
        break;
        case 96: {
            return MAX_KEYS;
        }
        break;
        case 97: {
            return MAX_KEYS;
        }
        break;
        case 98: {
            return MAX_KEYS;
        }
        break;
        case 99: {
            return MAX_KEYS;
        }
        break;
        case 100: {
            return MAX_KEYS;
        }
        break;
        case 101: {
            return MAX_KEYS;
        }
        break;
        case 102: {
            return MAX_KEYS;
        }
        break;
    }
    return MAX_KEYS;
}

#endif

void platform_log_message(const char *buffer, log_levels level, u32 max_chars)
{
    // clang-format off
    //  https://stackoverflow.com/questions/5412761/using-colors-with-printf
    //                  FATAL  ERROR   DEBUG  WARN    INFO  TRACE 
    u32 level_color[] = { 41,   31  ,   32  ,   33   ,  34  ,  37 };
    
    // clang-format on 
    
    printf("\033[0;%dm %s",level_color[level],buffer);
    printf("\033[0;37m");

}

#endif
