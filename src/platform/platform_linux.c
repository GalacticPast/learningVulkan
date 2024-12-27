#include "platform.h"

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

#if PLATFORM_LINUX_X11

#include <X11/XKBlib.h>   // sudo apt-get install libx11-dev
#include <X11/Xlib-xcb.h> // sudo apt-get install libxkbcommon-x11-dev
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <xcb/xcb.h>

typedef struct internal_state
{
    Display *display;
    xcb_connection_t *connection;
    xcb_window_t window;
    xcb_screen_t *screen;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_win;
} internal_state;

b8 platform_startup(platform_state *plat_state, const char *application_name, i32 x, i32 y, i32 width, i32 height)
{
    // Create the internal state.
    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state *state = (internal_state *)plat_state->internal_state;

    // Connect to X
    state->display = XOpenDisplay(NULL);

    // Turn off key repeats.
    XAutoRepeatOff(state->display);

    // Retrieve the connection from the display.
    state->connection = XGetXCBConnection(state->display);

    if (xcb_connection_has_error(state->connection))
    {
        DFATAL("Failed to connect to X server via XCB.");
        return false;
    }

    // Get data from the X server
    const struct xcb_setup_t *setup = xcb_get_setup(state->connection);

    // Loop through screens using iterator
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    i32 screen_p = 0;
    for (i32 s = screen_p; s > 0; s--)
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
    u32 event_values = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS |
                       XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION |
                       XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    // Values to be sent over XCB (bg colour, events)
    u32 value_list[] = {state->screen->black_pixel, event_values};

    // Create the window
    xcb_void_cookie_t cookie = xcb_create_window(state->connection,
                                                 XCB_COPY_FROM_PARENT, // depth
                                                 state->window,
                                                 state->screen->root,           // parent
                                                 x,                             // x
                                                 y,                             // y
                                                 width,                         // width
                                                 height,                        // height
                                                 0,                             // No border
                                                 XCB_WINDOW_CLASS_INPUT_OUTPUT, // class
                                                 state->screen->root_visual, event_mask, value_list);

    // Change the title
    xcb_change_property(state->connection, XCB_PROP_MODE_REPLACE, state->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
                        8, // data should be viewed 8 bits at a time
                        strlen(application_name), application_name);

    // Tell the server to notify when the window manager
    // attempts to destroy the window.
    xcb_intern_atom_cookie_t wm_delete_cookie =
        xcb_intern_atom(state->connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocols_cookie =
        xcb_intern_atom(state->connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *wm_delete_reply = xcb_intern_atom_reply(state->connection, wm_delete_cookie, NULL);
    xcb_intern_atom_reply_t *wm_protocols_reply = xcb_intern_atom_reply(state->connection, wm_protocols_cookie, NULL);
    state->wm_delete_win = wm_delete_reply->atom;
    state->wm_protocols = wm_protocols_reply->atom;

    xcb_change_property(state->connection, XCB_PROP_MODE_REPLACE, state->window, wm_protocols_reply->atom, 4, 32, 1,
                        &wm_delete_reply->atom);

    // Map the window to the screen
    xcb_map_window(state->connection, state->window);

    // Flush the stream
    i32 stream_result = xcb_flush(state->connection);
    if (stream_result <= 0)
    {
        DFATAL("An error occurred when flusing the stream: %d", stream_result);
        return false;
    }

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
// VULKAN

/* Wayland code */
typedef struct internal_state
{
    /* Globals */
    struct wl_display *wl_display;
    struct wl_registry *wl_registry;
    struct wl_shm *wl_shm;
    struct wl_seat *wl_seat;
    struct wl_compositor *wl_compositor;
    struct xdg_wm_base *xdg_wm_base;

    /* Objects */
    struct wl_surface *wl_surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;

    /* input */
    struct wl_keyboard *wl_keyboard;
    struct wl_mouse *wl_mouse;

} internal_state;

u32 translate_keycode(u32 key);

static void wl_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard, u32 format, i32 fd, u32 size)
{
    struct internal_state *state = data;
}

static void wl_keyboard_enter(void *data, struct wl_keyboard *wl_keyboard, u32 serial, struct wl_surface *surface,
                              struct wl_array *keys)
{
}

static void wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard, u32 serial, u32 time, u32 key, u32 state)
{
    struct internal_state *internal_state = data;
}

static void wl_keyboard_leave(void *data, struct wl_keyboard *wl_keyboard, u32 serial, struct wl_surface *surface)
{
}

static void wl_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard, u32 serial, u32 mods_depressed,
                                  u32 mods_latched, u32 mods_locked, u32 group)
{
}

static void wl_keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard, i32 rate, i32 delay)
{
}

static const struct wl_keyboard_listener wl_keyboard_listener = {
    .keymap = wl_keyboard_keymap,
    .enter = wl_keyboard_enter,
    .leave = wl_keyboard_leave,
    .key = wl_keyboard_key,
    .modifiers = wl_keyboard_modifiers,
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
static void xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, i32 width, i32 height,
                                   struct wl_array *states)
{
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
}
struct xdg_toplevel_listener xdg_toplevel_listener = {.configure = xdg_toplevel_configure, .close = xdg_toplevel_close};
//

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, u32 serial)
{
    xdg_surface_ack_configure(xdg_surface, serial);
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

    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        state->wl_compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, 4);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        state->xdg_wm_base = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(state->xdg_wm_base, &xdg_wm_base_listener, state);
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0)
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
    .global = registry_global,
    .global_remove = registry_global_remove,
};

b8 platform_startup(platform_state *plat_state, const char *application_name, i32 x, i32 y, i32 width, i32 height)
{
    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state *state = (internal_state *)plat_state->internal_state;

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

    return true;
}

void platform_shutdown(platform_state *plat_state)
{
    internal_state *state = (internal_state *)plat_state->internal_state;

    xdg_toplevel_destroy(state->xdg_toplevel);
    xdg_surface_destroy(state->xdg_surface);
    wl_surface_destroy(state->wl_surface);
    wl_display_disconnect(state->wl_display);
}

#endif

#endif
