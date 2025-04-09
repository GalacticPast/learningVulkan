#include "core/dasserts.hpp"
#include "core/logger.hpp"
#include "platform.hpp"

// Linux platform layer.
#ifdef DPLATFORM_LINUX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h> // nanosleep
#else
#include <unistd.h> // usleep
#endif

#ifdef DPLATFORM_LINUX_X11

#include <X11/XKBlib.h>   // sudo apt-get install libx11-dev
#include <X11/Xlib-xcb.h> // sudo apt-get install libxkbcommon-x11-dev
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <xcb/xcb.h>

#define VK_USE_PLATFORM_XCB_KHR
#include "renderer/vulkan/vulkan_platform.h"
#include "renderer/vulkan/vulkan_types.h"
#include <vulkan/vulkan.h>

typedef struct platform_state
{
    Display *display;
    xcb_connection_t *connection;
    xcb_window_t window;
    xcb_screen_t *screen;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_win;

    u32 width;
    u32 height;
} platform_state;

static platform_state *platform_state_ptr;

// Key translation
keys translate_keycode(u32 wl_keycode);

bool platform_startup(u64 *platform_mem_requirements, void *plat_state, const char *application_name, s32 x, s32 y, s32 width, s32 height)
{
    *platform_mem_requirements = sizeof(platform_state);
    if (plat_state == 0)
    {
        return true;
    }
    platform_state_ptr = plat_state;

    // Connect to X
    platform_state_ptr->display = XOpenDisplay(NULL);

    // Turn off key repeats.
    XAutoRepeatOff(platform_state_ptr->display);

    // Retrieve the connection from the display.
    platform_state_ptr->connection = XGetXCBConnection(platform_state_ptr->display);

    if (xcb_connection_has_error(platform_state_ptr->connection))
    {
        DFATAL("Failed to connect to X server via XCB.");
        return false;
    }

    // Get data from the X server
    const struct xcb_setup_t *setup = xcb_get_setup(platform_state_ptr->connection);

    // Loop through screens using iterator
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    s32 screen_p             = 0;
    for (s32 s = screen_p; s > 0; s--)
    {
        xcb_screen_next(&it);
    }

    // After screens have been looped through, assign it.
    platform_state_ptr->screen = it.data;

    // Allocate a XID for the window to be created.
    platform_state_ptr->window = xcb_generate_id(platform_state_ptr->connection);

    // Register event types.
    // XCB_CW_BACK_PIXEL = filling then window bg with a single color
    // XCB_CW_EVENT_MASK is required.
    u32 event_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

    // Listen for keyboard and mouse buttons
    u32 event_values = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION |
                       XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    // Values to be sent over XCB (bg color, events)
    u32 value_list[] = {platform_state_ptr->screen->black_pixel, event_values};

    // Create the window
    xcb_void_cookie_t cookie = xcb_create_window(platform_state_ptr->connection,
                                                 XCB_COPY_FROM_PARENT, // depth
                                                 platform_state_ptr->window,
                                                 platform_state_ptr->screen->root, // parent
                                                 x,                                // x
                                                 y,                                // y
                                                 width,                            // width
                                                 height,                           // height
                                                 0,                                // No border
                                                 XCB_WINDOW_CLASS_INPUT_OUTPUT,    // class
                                                 platform_state_ptr->screen->root_visual, event_mask, value_list);

    // Change the title
    xcb_change_property(platform_state_ptr->connection, XCB_PROP_MODE_REPLACE, platform_state_ptr->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
                        8, // data should be viewed 8 bits at a time
                        strlen(application_name), application_name);

    // Tell the server to notify when the window manager
    // attempts to destroy the window.
    xcb_intern_atom_cookie_t wm_delete_cookie    = xcb_intern_atom(platform_state_ptr->connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(platform_state_ptr->connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *wm_delete_reply     = xcb_intern_atom_reply(platform_state_ptr->connection, wm_delete_cookie, NULL);
    xcb_intern_atom_reply_t *wm_protocols_reply  = xcb_intern_atom_reply(platform_state_ptr->connection, wm_protocols_cookie, NULL);
    platform_state_ptr->wm_delete_win            = wm_delete_reply->atom;
    platform_state_ptr->wm_protocols             = wm_protocols_reply->atom;

    xcb_change_property(platform_state_ptr->connection, XCB_PROP_MODE_REPLACE, platform_state_ptr->window, wm_protocols_reply->atom, 4, 32, 1, &wm_delete_reply->atom);

    // Map the window to the screen
    xcb_map_window(platform_state_ptr->connection, platform_state_ptr->window);

    // Flush the stream
    s32 stream_result = xcb_flush(platform_state_ptr->connection);
    if (stream_result <= 0)
    {
        DFATAL("An error occurred when flusing the stream: %d", stream_result);
        return false;
    }

    return true;
}

void platform_shutdown()
{
    // Simply cold-cast to the known type.
    // Turn key repeats back on since this is global for the OS... just... wow.
    XAutoRepeatOn(platform_state_ptr->display);

    xcb_destroy_window(platform_state_ptr->connection, platform_state_ptr->window);
}

bool platform_pump_messages()
{

    xcb_generic_event_t *event;
    xcb_client_message_event_t *cm;

    bool quit_flagged = false;

    // Poll for events until null is returned.
    while (event != 0)
    {
        event = xcb_poll_for_event(platform_state_ptr->connection);
        if (event == 0)
        {
            break;
        }

        // Input events
        switch (event->response_type & ~0x80)
        {
        case XCB_KEY_PRESS:
        case XCB_KEY_RELEASE: {
            // Key press event - xcb_key_press_event_t and xcb_key_release_event_t are the same
            xcb_key_press_event_t *kb_event = (xcb_key_press_event_t *)event;
            bool pressed                    = event->response_type == XCB_KEY_PRESS;
            xcb_keycode_t code              = kb_event->detail;
            KeySym key_sym                  = XkbKeycodeToKeysym(platform_state_ptr->display,
                                                                 (KeyCode)code, // event.xkey.keycode,
                                                                 0, code & ShiftMask ? 1 : 0);

            keys key = translate_keycode(key_sym);

            // Pass to the input subsystem for processing.
            input_process_key(key, pressed);
        }
        break;
        case XCB_BUTTON_PRESS:
        case XCB_BUTTON_RELEASE: {
            xcb_button_press_event_t *mouse_event = (xcb_button_press_event_t *)event;
            bool pressed                          = event->response_type == XCB_BUTTON_PRESS;
            buttons mouse_button                  = BUTTON_MAX_BUTTONS;
            switch (mouse_event->detail)
            {
            case XCB_BUTTON_INDEX_1:
                mouse_button = BUTTON_LEFT;
                break;
            case XCB_BUTTON_INDEX_2:
                mouse_button = BUTTON_MIDDLE;
                break;
            case XCB_BUTTON_INDEX_3:
                mouse_button = BUTTON_RIGHT;
                break;
            }

            // Pass over to the input subsystem.
            if (mouse_button != BUTTON_MAX_BUTTONS)
            {
                input_process_button(mouse_button, pressed);
            }
        }
        break;
        case XCB_MOTION_NOTIFY: {
            // Mouse move
            xcb_motion_notify_event_t *move_event = (xcb_motion_notify_event_t *)event;

            // Pass over to the input subsystem.
            input_process_mouse_move(move_event->event_x, move_event->event_y);
        }
        break;
        case XCB_CONFIGURE_NOTIFY: {
            // Resizing - note that this is also triggered by moving the window, but should be
            // passed anyway since a change in the x/y could mean an upper-left resize.
            // The application layer can decide what to do with this.
            xcb_configure_notify_event_t *configure_event = (xcb_configure_notify_event_t *)event;

            // Fire the event. The application layer should pick this up, but not handle it
            // as it shouldn be visible to other parts of the application.
        }
        break;

        case XCB_CLIENT_MESSAGE: {
            cm = (xcb_client_message_event_t *)event;

            // Window close
            if (cm->data.data32[0] == platform_state_ptr->wm_delete_win)
            {
                quit_flagged = true;
            }
        }
        break;
        default:
            // Something else
            break;
        }

        free(event);
    }
    return !quit_flagged;
}

void platform_get_window_dimensions(u32 *width, u32 *height)
{
    if (platform_state_ptr->width != 0 || platform_state_ptr->height != 0)
    {
        *width  = platform_state_ptr->width;
        *height = platform_state_ptr->height;
    }
}

// Key translation
keys translate_keycode(u32 xk_keycode)
{
    switch (xk_keycode)
    {
    case XK_BackSpace:
        return KEY_BACKSPACE;
    case XK_Return:
        return KEY_ENTER;
    case XK_Tab:
        return KEY_TAB;
        // case XK_Shift: return KEY_SHIFT;
        // case XK_Control: return KEY_CONTROL;

    case XK_Pause:
        return KEY_PAUSE;
    case XK_Caps_Lock:
        return KEY_CAPITAL;

    case XK_Escape:
        return KEY_ESCAPE;
        // Not supported
        // case : return KEY_CONVERT;
        // case : return KEY_NONCONVERT;
        // case : return KEY_ACCEPT;

    case XK_Mode_switch:
        return KEY_MODECHANGE;

    case XK_space:
        return KEY_SPACE;
    case XK_Prior:
        return KEY_PRIOR;
    case XK_Next:
        return KEY_NEXT;
    case XK_End:
        return KEY_END;
    case XK_Home:
        return KEY_HOME;
    case XK_Left:
        return KEY_LEFT;
    case XK_Up:
        return KEY_UP;
    case XK_Right:
        return KEY_RIGHT;
    case XK_Down:
        return KEY_DOWN;
    case XK_Select:
        return KEY_SELECT;
    case XK_Print:
        return KEY_PRINT;
    case XK_Execute:
        return KEY_EXECUTE;
    // case XK_snapshot: return KEY_SNAPSHOT; // not supported
    case XK_Insert:
        return KEY_INSERT;
    case XK_Delete:
        return KEY_DELETE;
    case XK_Help:
        return KEY_HELP;

    case XK_Meta_L:
        return KEY_LWIN; // TODO: not sure this is right
    case XK_Meta_R:
        return KEY_RWIN;
        // case XK_apps: return KEY_APPS; // not supported

        // case XK_sleep: return KEY_SLEEP; //not supported

    case XK_KP_0:
        return KEY_NUMPAD0;
    case XK_KP_1:
        return KEY_NUMPAD1;
    case XK_KP_2:
        return KEY_NUMPAD2;
    case XK_KP_3:
        return KEY_NUMPAD3;
    case XK_KP_4:
        return KEY_NUMPAD4;
    case XK_KP_5:
        return KEY_NUMPAD5;
    case XK_KP_6:
        return KEY_NUMPAD6;
    case XK_KP_7:
        return KEY_NUMPAD7;
    case XK_KP_8:
        return KEY_NUMPAD8;
    case XK_KP_9:
        return KEY_NUMPAD9;
    case XK_multiply:
        return KEY_MULTIPLY;
    case XK_KP_Add:
        return KEY_ADD;
    case XK_KP_Separator:
        return KEY_SEPARATOR;
    case XK_KP_Subtract:
        return KEY_SUBTRACT;
    case XK_KP_Decimal:
        return KEY_DECIMAL;
    case XK_KP_Divide:
        return KEY_DIVIDE;
    case XK_F1:
        return KEY_F1;
    case XK_F2:
        return KEY_F2;
    case XK_F3:
        return KEY_F3;
    case XK_F4:
        return KEY_F4;
    case XK_F5:
        return KEY_F5;
    case XK_F6:
        return KEY_F6;
    case XK_F7:
        return KEY_F7;
    case XK_F8:
        return KEY_F8;
    case XK_F9:
        return KEY_F9;
    case XK_F10:
        return KEY_F10;
    case XK_F11:
        return KEY_F11;
    case XK_F12:
        return KEY_F12;
    case XK_F13:
        return KEY_F13;
    case XK_F14:
        return KEY_F14;
    case XK_F15:
        return KEY_F15;
    case XK_F16:
        return KEY_F16;
    case XK_F17:
        return KEY_F17;
    case XK_F18:
        return KEY_F18;
    case XK_F19:
        return KEY_F19;
    case XK_F20:
        return KEY_F20;
    case XK_F21:
        return KEY_F21;
    case XK_F22:
        return KEY_F22;
    case XK_F23:
        return KEY_F23;
    case XK_F24:
        return KEY_F24;

    case XK_Num_Lock:
        return KEY_NUMLOCK;
    case XK_Scroll_Lock:
        return KEY_SCROLL;

    case XK_KP_Equal:
        return KEY_NUMPAD_EQUAL;

    case XK_Shift_L:
        return KEY_LSHIFT;
    case XK_Shift_R:
        return KEY_RSHIFT;
    case XK_Control_L:
        return KEY_LCONTROL;
    case XK_Control_R:
        return KEY_RCONTROL;
    // case XK_Menu: return KEY_LMENU;
    case XK_Menu:
        return KEY_RMENU;

    case XK_semicolon:
        return KEY_SEMICOLON;
    case XK_plus:
        return KEY_PLUS;
    case XK_comma:
        return KEY_COMMA;
    case XK_minus:
        return KEY_MINUS;
    case XK_period:
        return KEY_PERIOD;
    case XK_slash:
        return KEY_SLASH;
    case XK_grave:
        return KEY_GRAVE;

    case XK_a:
    case XK_A:
        return KEY_A;
    case XK_b:
    case XK_B:
        return KEY_B;
    case XK_c:
    case XK_C:
        return KEY_C;
    case XK_d:
    case XK_D:
        return KEY_D;
    case XK_e:
    case XK_E:
        return KEY_E;
    case XK_f:
    case XK_F:
        return KEY_F;
    case XK_g:
    case XK_G:
        return KEY_G;
    case XK_h:
    case XK_H:
        return KEY_H;
    case XK_i:
    case XK_I:
        return KEY_I;
    case XK_j:
    case XK_J:
        return KEY_J;
    case XK_k:
    case XK_K:
        return KEY_K;
    case XK_l:
    case XK_L:
        return KEY_L;
    case XK_m:
    case XK_M:
        return KEY_M;
    case XK_n:
    case XK_N:
        return KEY_N;
    case XK_o:
    case XK_O:
        return KEY_O;
    case XK_p:
    case XK_P:
        return KEY_P;
    case XK_q:
    case XK_Q:
        return KEY_Q;
    case XK_r:
    case XK_R:
        return KEY_R;
    case XK_s:
    case XK_S:
        return KEY_S;
    case XK_t:
    case XK_T:
        return KEY_T;
    case XK_u:
    case XK_U:
        return KEY_U;
    case XK_v:
    case XK_V:
        return KEY_V;
    case XK_w:
    case XK_W:
        return KEY_W;
    case XK_x:
    case XK_X:
        return KEY_X;
    case XK_y:
    case XK_Y:
        return KEY_Y;
    case XK_z:
    case XK_Z:
        return KEY_Z;

    default:
        return 0;
    }
}

#endif

#ifdef DPLATFORM_LINUX_WAYLAND

#define CHECK_WL_RESULT(expr)                                                                                                                                                                          \
    {                                                                                                                                                                                                  \
        DASSERT(expr != 0);                                                                                                                                                                            \
    }
//
#include "core/application.hpp"

#include "wayland/xdg-shell-client-protocol.h"
#include <wayland-client.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
//
// VULKAN
#include "renderer/vulkan/vulkan_platform.hpp"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>

/* Wayland code */
typedef struct platform_state
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
    struct xkb_state *xkb_state;
    struct xkb_context *xkb_context;
    struct xkb_keymap *xkb_keymap;

    /* dimensions */
    u32 width;
    u32 height;
} platform_state;

static struct platform_state *platform_state_ptr;

u32 translate_keycode(xkb_keysym_t xkb_sym);

// keyboard

static void wl_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard, u32 format, s32 fd, u32 size)
{
    DASSERT(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

    char *map_shm = (char *)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    DASSERT(map_shm != MAP_FAILED);

    struct xkb_keymap *xkb_keymap = xkb_keymap_new_from_string(platform_state_ptr->xkb_context, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map_shm, size);
    close(fd);

    struct xkb_state *xkb_state = xkb_state_new(xkb_keymap);
    xkb_keymap_unref(platform_state_ptr->xkb_keymap);
    xkb_state_unref(platform_state_ptr->xkb_state);
    platform_state_ptr->xkb_keymap = xkb_keymap;
    platform_state_ptr->xkb_state  = xkb_state;
}

static void wl_keyboard_enter(void *data, struct wl_keyboard *wl_keyboard, u32 serial, struct wl_surface *surface, struct wl_array *keys)
{
    DDEBUG("Keyboard in scope");
}

static void wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard, u32 serial, u32 time, u32 key, u32 state)
{

    // uint32_t keycode = key + 8;

    // xkb_keysym_t sym = xkb_state_key_get_one_sym(platform_state_ptr->xkb_state, keycode);

    // keys code = translate_keycode(sym);

    // input_process_key(code, (bool)state);
}

static void wl_keyboard_leave(void *data, struct wl_keyboard *wl_keyboard, u32 serial, struct wl_surface *surface)
{
    DDEBUG("Mouse not in scope");
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

    // TODO: mouse events
    //

    bool have_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

    if (have_keyboard && platform_state_ptr->wl_keyboard == NULL)
    {
        platform_state_ptr->wl_keyboard = wl_seat_get_keyboard(platform_state_ptr->wl_seat);
        wl_keyboard_add_listener(platform_state_ptr->wl_keyboard, &wl_keyboard_listener, platform_state_ptr);
    }
    else if (!have_keyboard && platform_state_ptr->wl_keyboard != NULL)
    {
        wl_keyboard_release(platform_state_ptr->wl_keyboard);
        platform_state_ptr->wl_keyboard = NULL;
    }
}
static void wl_seat_name(void *data, struct wl_seat *wl_seat, const char *name)
{
}

struct wl_seat_listener wl_seat_listener = {.capabilities = wl_seat_capabilites, .name = wl_seat_name};

// actual surface
static void xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, s32 width, s32 height, struct wl_array *states)
{
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
}

static void xdg_toplevel_configure_bounds(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height)
{
}

static void xdg_toplevel_wm_capabilities(void *data, struct xdg_toplevel *xdg_toplevel, struct wl_array *capabilities)
{
}
struct xdg_toplevel_listener xdg_toplevel_listener = {.configure        = xdg_toplevel_configure,
                                                      .close            = xdg_toplevel_close,
                                                      .configure_bounds = xdg_toplevel_configure_bounds,
                                                      .wm_capabilities  = xdg_toplevel_wm_capabilities};
//

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, u32 serial)
{
    xdg_surface_ack_configure(xdg_surface, serial);
    wl_surface_commit(platform_state_ptr->wl_surface);
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
    // DDEBUG("Print to see the version code for specific interfaces: Interface:%s Version:%d", interface, version);
    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        platform_state_ptr->wl_compositor = (wl_compositor *)wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        platform_state_ptr->xdg_wm_base = (xdg_wm_base *)wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, version);
        xdg_wm_base_add_listener(platform_state_ptr->xdg_wm_base, &xdg_wm_base_listener, platform_state_ptr);
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        platform_state_ptr->wl_seat = (wl_seat *)wl_registry_bind(wl_registry, name, &wl_seat_interface, version);
        wl_seat_add_listener(platform_state_ptr->wl_seat, &wl_seat_listener, platform_state_ptr);
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

bool platform_system_startup(u64 *platform_mem_requirements, void *plat_state, application_config *app_config)
{
    *platform_mem_requirements = sizeof(platform_state);
    if (plat_state == 0)
    {
        return true;
    }
    DINFO("Initializing linux-Wayland platform...");

    platform_state_ptr = (platform_state *)plat_state;

    platform_state_ptr->wl_display = wl_display_connect(NULL);
    if (!platform_state_ptr->wl_display)
    {
        return false;
    }

    platform_state_ptr->wl_registry = wl_display_get_registry(platform_state_ptr->wl_display);
    if (!platform_state_ptr->wl_registry)
    {
        return false;
    }

    platform_state_ptr->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!platform_state_ptr->xkb_context)
    {
        return false;
    }

    wl_registry_add_listener(platform_state_ptr->wl_registry, &wl_registry_listener, platform_state_ptr);

    wl_display_roundtrip(platform_state_ptr->wl_display);

    platform_state_ptr->wl_surface = wl_compositor_create_surface(platform_state_ptr->wl_compositor);
    if (!platform_state_ptr->wl_surface)
    {
        return false;
    }

    platform_state_ptr->xdg_surface = xdg_wm_base_get_xdg_surface(platform_state_ptr->xdg_wm_base, platform_state_ptr->wl_surface);
    if (!platform_state_ptr->xdg_surface)
    {
        return false;
    }

    xdg_surface_add_listener(platform_state_ptr->xdg_surface, &xdg_surface_listener, platform_state_ptr);

    platform_state_ptr->xdg_toplevel = xdg_surface_get_toplevel(platform_state_ptr->xdg_surface);

    if (!platform_state_ptr->xdg_toplevel)
    {
        return false;
    }
    xdg_toplevel_add_listener(platform_state_ptr->xdg_toplevel, &xdg_toplevel_listener, platform_state_ptr);

    xdg_toplevel_set_title(platform_state_ptr->xdg_toplevel, app_config->application_name);
    xdg_toplevel_set_app_id(platform_state_ptr->xdg_toplevel, app_config->application_name);

    wl_surface_commit(platform_state_ptr->wl_surface);

    return true;
}

bool platform_pump_messages()
{

    s32 result = wl_display_dispatch(platform_state_ptr->wl_display);

    return result == -1 ? false : true;
}

void platform_system_shutdown(void *state)
{
    DINFO("Shutting down linux-wayland platform...");
    xdg_toplevel_destroy(platform_state_ptr->xdg_toplevel);
    xdg_surface_destroy(platform_state_ptr->xdg_surface);
    wl_surface_destroy(platform_state_ptr->wl_surface);
    wl_display_disconnect(platform_state_ptr->wl_display);
    platform_state_ptr = 0;
}

void platform_get_window_dimensions(u32 *width, u32 *height)
{
    *width  = platform_state_ptr->width == 0 ? 1270 : platform_state_ptr->width;
    *height = platform_state_ptr->height == 0 ? 800 : platform_state_ptr->height;
}

bool platform_get_required_vulkan_extensions(u32 *platform_required_extensions_count, const char **extensions_array)
{
    *platform_required_extensions_count = 2;
    if (!extensions_array)
    {
        return true;
    }
    extensions_array[0] = VK_KHR_SURFACE_EXTENSION_NAME;
    extensions_array[1] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
    return true;
}

#endif

void *platform_allocate(u64 size, bool aligned)
{
    return malloc(size);
}
void platform_free(void *block, bool aligned)
{
    free(block);
}
void *platform_zero_memory(void *block, u64 size)
{
    return memset(block, 0, size);
}
void *platform_copy_memory(void *dest, const void *source, u64 size)
{
    return memcpy(dest, source, size);
}
void *platform_set_memory(void *dest, s64 value, u64 size)
{
    return memset(dest, (s32)value, size);
}

void platform_console_write(const char *message, u8 color)
{
    // FATAL,ERROR,WARN,INFO,DDEBUG,TRACE
    const char *color_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "35;1"};
    printf("\033[%sm%s\033[0m", color_strings[color], message);
}
void platform_console_write_error(const char *message, u8 color)
{
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    const char *color_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "35;1"};
    printf("\033[%sm%s\033[0m", color_strings[color], message);
}

f64 platform_get_absolute_time()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (f64)now.tv_sec + (f64)now.tv_nsec * 0.000000001;
}

void platform_sleep(u64 ms)
{
#if _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000 * 1000;
    nanosleep(&ts, 0);
#else
    if (ms >= 1000)
    {
        sleep(ms / 1000);
    }
    usleep((ms % 1000) * 1000);
#endif
}

#endif
