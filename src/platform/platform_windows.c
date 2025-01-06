#include "platform.h"

#ifdef PLATFORM_WINDOWS

#include "containers/array.h"
#include "core/events.h"
#include "core/input.h"
#include "core/logger.h"

#include <stdlib.h>
#include <windows.h>
#include <windowsx.h> // param input extraction

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "vulkan/vulkan_types.h"

typedef struct internal_state
{
    HINSTANCE h_instance;
    HWND      hwnd;
} internal_state;

// Clock

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param);
keys             translate_keycode(u32 key);

b8 platform_startup(platform_state *plat_state, const char *application_name, s32 x, s32 y, s32 width, s32 height)
{
    INFO("initializing windows platform startup...");
    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state *state      = (internal_state *)plat_state->internal_state;

    state->h_instance = GetModuleHandleA(0);

    // Setup and register window class.
    HICON     icon = LoadIcon(state->h_instance, IDI_APPLICATION);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style         = CS_DBLCLKS; // Get double-clicks
    wc.lpfnWndProc   = win32_process_message;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = state->h_instance;
    wc.hIcon         = icon;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW); // NULL; // Manage the cursor manually
    wc.hbrBackground = NULL;                        // Transparent
    wc.lpszClassName = "learning_vulkan_class";

    if (!RegisterClassA(&wc))
    {
        MessageBoxA(0, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    u32 client_screen_width  = GetSystemMetrics(SM_CXFULLSCREEN);
    u32 client_screen_height = GetSystemMetrics(SM_CYFULLSCREEN);

    // Create window
    u32 client_x      = x == 0 ? (client_screen_width / 2 - width / 2) : x;
    u32 client_y      = y == 0 ? (client_screen_height / 2 - height / 2) : y;
    u32 client_width  = width;
    u32 client_height = height;

    u32 window_x      = client_x;
    u32 window_y      = client_y;
    u32 window_width  = client_width;
    u32 window_height = client_height;

    u32 window_style    = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
    u32 window_ex_style = WS_EX_APPWINDOW;

    window_style |= WS_MAXIMIZEBOX;
    window_style |= WS_MINIMIZEBOX;
    window_style |= WS_THICKFRAME;

    // Obtain the size of the border.
    RECT border_rect = {0, 0, 0, 0};
    AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

    // In this case, the border rectangle is negative.
    window_x += border_rect.left;
    window_y += border_rect.top;

    // Grow by the size of the OS border.
    window_width += border_rect.right - border_rect.left;
    window_height += border_rect.bottom - border_rect.top;

    HWND handle = CreateWindowExA(window_ex_style, "learning_vulkan_class", application_name, window_style, window_x, window_y, window_width, window_height, 0, 0, state->h_instance, 0);

    if (handle == 0)
    {
        MessageBoxA(NULL, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        FATAL("Window creation failed!");
        return false;
    }
    else
    {
        state->hwnd = handle;
    }

    // Show the window
    b8  should_activate           = 1; // TODO: if the window should not accept input, this should be false.
    s32 show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;
    // If initially minimized, use SW_MINIMIZE : SW_SHOWMINNOACTIVE;
    // If initially maximized, use SW_SHOWMAXIMIZED : SW_MAXIMIZE
    ShowWindow(state->hwnd, show_window_command_flags);

    return true;
}

void platform_shutdown(platform_state *plat_state)
{
    // Simply cold-cast to the known type.
    internal_state *state = (internal_state *)plat_state->internal_state;

    if (state->hwnd)
    {
        DestroyWindow(state->hwnd);
        state->hwnd = 0;
    }
}

b8 platform_pump_messages(platform_state *plat_state)
{
    MSG message;
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return true;
}

void platform_get_specific_surface_extensions(const char ***array)
{
    array_push_value(*array, &"VK_KHR_win32_surface");
}

void platform_log_message(const char *buffer, log_levels level, u32 max_chars)
{

    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    static u8 levels[6] = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(console_handle, levels[level]);

    u64     length         = strlen(buffer);
    LPDWORD number_written = 0;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buffer, (DWORD)length, number_written, 0);
}

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg)
    {
        case WM_ERASEBKGND:
            // Notify the OS that erasing will be handled by the application to prevent flicker.
            return 1;
        case WM_CLOSE:
            event_context context = {0};
            event_fire(ON_APPLICATION_QUIT, context);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE: {
            // Get the updated size.
            RECT r;
            GetClientRect(hwnd, &r);
            u32 width  = r.right - r.left;
            u32 height = r.bottom - r.top;

            event_context context = {0};
            context.data.u32[0]   = width;
            context.data.u32[1]   = height;
            event_fire(ON_APPLICATION_RESIZE, context);
        }
        break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            // Key pressed/released
            b8   pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            keys key     = translate_keycode((u16)w_param);
            DEBUG("Key: %d", key);

            input_process_key(key, pressed);
        }
        break;
        case WM_MOUSEMOVE: {
            // Mouse move

            // Pass over to the input subsystem.
        }
        break;
        case WM_MOUSEWHEEL: {
        }
        break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP: {
        }
        break;
    }

    return DefWindowProcA(hwnd, msg, w_param, l_param);
}

keys translate_keycode(u32 key)
{
    switch (key)
    {
        case 27: {
            return KEY_ESCAPE;
        }
        break;
    }
    return MAX_KEYS;
}

b8 platform_create_vulkan_surface(platform_state *plat_state, vulkan_context *context)
{
    internal_state *state = (internal_state *)plat_state->internal_state;

    VkWin32SurfaceCreateInfoKHR win32_surface_create_info = {};

    win32_surface_create_info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32_surface_create_info.pNext     = 0;
    win32_surface_create_info.flags     = 0;
    win32_surface_create_info.hinstance = state->h_instance;
    win32_surface_create_info.hwnd      = state->hwnd;

    VK_CHECK(vkCreateWin32SurfaceKHR(context->instance, &win32_surface_create_info, 0, &context->surface));
    return true;
}
void *platform_get_required_surface_extensions(char **required_extension_names)
{
    char *win32_surface_extension = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
    required_extension_names      = array_push_value(required_extension_names, &win32_surface_extension);
    return required_extension_names;
}

#endif
