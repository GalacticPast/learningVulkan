#include "core/events.h"
#include "core/input.h"
#include "core/strings.h"
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
    Display          *display;
    xcb_connection_t *connection;
    xcb_window_t      window;
    xcb_screen_t     *screen;
    xcb_atom_t        wm_protocols;
    xcb_atom_t        wm_delete_win;
} internal_state;

b8 platform_startup(platform_state *plat_state, const char *application_name, s32 x, s32 y, s32 width, s32 height)
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
    u32 event_values = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
                       XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

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
    xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(state->connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(state->connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *wm_delete_reply = xcb_intern_atom_reply(state->connection, wm_delete_cookie, NULL);
    xcb_intern_atom_reply_t *wm_protocols_reply = xcb_intern_atom_reply(state->connection, wm_protocols_cookie, NULL);
    state->wm_delete_win = wm_delete_reply->atom;
    state->wm_protocols = wm_protocols_reply->atom;

    xcb_change_property(state->connection, XCB_PROP_MODE_REPLACE, state->window, wm_protocols_reply->atom, 4, 32, 1, &wm_delete_reply->atom);

    // Map the window to the screen
    xcb_map_window(state->connection, state->window);

    // Flush the stream
    s32 stream_result = xcb_flush(state->connection);
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

/* Shared memory support code */
static void randname(char *buf)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i)
    {
        buf[i] = 'A' + (r & 15) + (r & 16) * 2;
        r >>= 5;
    }
}

static int create_shm_file(void)
{
    int retries = 100;
    do
    {
        char name[] = "/wl_shm-XXXXXX";
        randname(name + sizeof(name) - 7);
        --retries;
        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0)
        {
            shm_unlink(name);
            return fd;
        }
    } while (retries > 0 && errno == EEXIST);
    return -1;
}

static int allocate_shm_file(size_t size)
{
    int fd = create_shm_file();
    if (fd < 0)
        return -1;
    int ret;
    do
    {
        ret = ftruncate(fd, size);
    } while (ret < 0 && errno == EINTR);
    if (ret < 0)
    {
        close(fd);
        return -1;
    }
    return fd;
}
static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer)
{
    /* Sent by the compositor when it's no longer using this buffer */
    wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

static struct wl_buffer *draw_frame(struct internal_state *state)
{
    const int width = state->width;
    const int height = state->height;

    int stride = width * 4;
    int size = stride * height;

    int fd = allocate_shm_file(size);
    if (fd == -1)
    {
        return NULL;
    }

    u32 *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED)
    {
        close(fd);
        return NULL;
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, size);
    struct wl_buffer   *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            u8 alpha = 0xFF;
            u8 red = 0xFF;
            u8 green = 0xFF;
            u8 blue = 0x00;

            u32 color = ((u32)alpha << 24 | (u32)red << 16 | (u32)green << 8 | (u32)blue << 0);

            data[y * width + x] = color;
        }
    }

    wl_shm_pool_destroy(pool);
    close(fd);

    munmap(data, size);
    wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
    return buffer;
}

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
        context.data.u32[0] = code;
        event_fire(ON_APPLICATION_QUIT, context);
    }
}

static void wl_keyboard_leave(void *data, struct wl_keyboard *wl_keyboard, u32 serial, struct wl_surface *surface)
{
    DEBUG("Mouse not in scope");
}

static void wl_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard, u32 serial, u32 mods_depressed, u32 mods_latched, u32 mods_locked,
                                  u32 group)
{
}

static void wl_keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard, s32 rate, s32 delay)
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
static void xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, s32 width, s32 height, struct wl_array *states)
{
    internal_state *state = (internal_state *)data;

    if (state->width != width || state->height != height)
    {
        state->width = width;
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
    struct wl_buffer *buffer = draw_frame(state);
    wl_surface_attach(state->wl_surface, buffer, 0, 0);
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

    if (string_compare(interface, wl_shm_interface.name))
    {
        state->wl_shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, 1);
    }
    else if (string_compare(interface, wl_compositor_interface.name))
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
    .global = registry_global,
    .global_remove = registry_global_remove,
};

b8 platform_startup(platform_state *plat_state, const char *application_name, s32 x, s32 y, s32 width, s32 height)
{
    INFO("Initializing Linux-Walyand platform...");
    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state *state = (internal_state *)plat_state->internal_state;

    state->width = width;
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

    DEBUG("Linux-Walyand platform initialized");

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
