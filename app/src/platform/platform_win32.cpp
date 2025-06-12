#include "core/input.hpp"
#include "defines.hpp"

#ifdef DPLATFORM_WINDOWS
#include "core/application.hpp"
#include "platform/platform.hpp"

#include "core/event.hpp"
#include "core/logger.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h> // param input extraction

#include <stdlib.h>
#include <timeapi.h>

/* VULKAN */
#include "renderer/vulkan/vulkan_platform.hpp"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

typedef struct platform_state
{
    HINSTANCE h_instance;
    HWND      hwnd;

    u32 width;
    u32 height;

    platform_info info;

} platform_state;

// Clock
static f64           clock_frequency;
static LARGE_INTEGER start_time;

static platform_state *platform_state_ptr;

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param);

bool platform_system_startup(u64 *platform_mem_requirements, void *plat_state, application_config *app_config)
{
    *platform_mem_requirements = sizeof(platform_state);
    if (plat_state == 0)
    {
        return true;
    }
    platform_state_ptr = reinterpret_cast<platform_state *>(plat_state);

    platform_state_ptr->h_instance = GetModuleHandleA(0);

    // Setup and register window class.
    HICON     icon = LoadIcon(platform_state_ptr->h_instance, IDI_APPLICATION);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style         = CS_DBLCLKS; // Get double-clicks
    wc.lpfnWndProc   = win32_process_message;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = platform_state_ptr->h_instance;
    wc.hIcon         = icon;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW); // NULL; // Manage the cursor manually
    wc.hbrBackground = NULL;                        // Transparent
    wc.lpszClassName = "window_class";

    if (!RegisterClassA(&wc))
    {
        MessageBoxA(0, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    HINSTANCE dummy_h_instance = GetModuleHandleA(0);
    HWND      dummy_handle =
        CreateWindowExA(WS_EX_APPWINDOW, "window_class", app_config->application_name,
                        WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION, 100, 100, 100, 100, 0, 0, dummy_h_instance, 0);

    if (dummy_handle == 0)
    {
        MessageBoxA(NULL, "Dummy Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        DDEBUG("%lld", GetLastError());
        DFATAL("Dummy Window creation failed!");
        return false;
    }

    HDC dummy_device_context = GetDC(dummy_handle);
    // HDC dummy_device_context = 0 /*GetDC(dummy_handle) */;

    // u32 device_display_width  = 0;
    // u32 device_display_height = 0;

    u32 device_display_width  = GetDeviceCaps(dummy_device_context, HORZRES);
    u32 device_display_height = GetDeviceCaps(dummy_device_context, VERTRES);

    DDEBUG("Device display dimensions: Width_p: %d   Height_p: %d", device_display_width, device_display_height);
    // destroy dummy window
    DestroyWindow(dummy_handle);

    u32 default_window_width  = 0;
    u32 default_window_height = 0;

    // TODO: make this configurable.
    if (device_display_width == 2560 && device_display_height == 1440)
    {
        default_window_width  = 1280;
        default_window_height = 720;
    }
    else
    {
        default_window_width  = 800;
        default_window_height = 600;
    }

    // Create window
    u32 client_width  = app_config->width == INVALID_ID ? default_window_width : app_config->width;
    u32 client_height = app_config->height == INVALID_ID ? default_window_height : app_config->height;
    u32 client_x      = app_config->x == INVALID_ID ? (device_display_width / 2) - (client_width / 2) : app_config->x;
    u32 client_y      = app_config->y == INVALID_ID ? (device_display_height / 2) - (client_height / 2) : app_config->y;

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
    window_width  += border_rect.right - border_rect.left;
    window_height += border_rect.bottom - border_rect.top;

    HWND handle = CreateWindowExA(window_ex_style, "window_class", app_config->application_name, window_style, window_x,
                                  window_y, window_width, window_height, 0, 0, platform_state_ptr->h_instance, 0);

    if (handle == 0)
    {
        MessageBoxA(NULL, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        DFATAL("Window creation failed!");
        return false;
    }
    else
    {
        platform_state_ptr->hwnd = handle;
    }

    // Show the window
    bool should_activate           = 1; // TODO: if the window should not accept input, this should be false.
    s32  show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;
    // If initially minimized, use SW_MINIMIZE : SW_SHOWMINNOACTIVE;
    // If initially maximized, use SW_SHOWMAXIMIZED : SW_MAXIMIZE
    ShowWindow(platform_state_ptr->hwnd, show_window_command_flags);

    // Clock setup
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock_frequency = 1.0 / static_cast<f64>(frequency.QuadPart);
    QueryPerformanceCounter(&start_time);

    // set the default timer resolution to the target fps
    timeBeginPeriod(1.0);

    platform_state_ptr->info = platform_get_info();

    return true;
}

void platform_system_shutdown(void *state)
{
    // Simply cold-cast to the known type.

    if (platform_state_ptr->hwnd)
    {
        DestroyWindow(platform_state_ptr->hwnd);
        platform_state_ptr->hwnd = 0;
    }
    timeEndPeriod(1.0);
}

bool platform_pump_messages()
{
    MSG message;
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return true;
}

void *platform_virtual_reserve(u64 size, bool aligned)
{
    DASSERT(size != INVALID_ID_64);
    platform_info info = platform_get_info();
    void         *ptr  = VirtualAlloc(info.min_app_address, size, MEM_RESERVE, PAGE_NOACCESS);
    DASSERT_MSG(ptr == info.min_app_address,
                "The returned ptr was not the same as the min application adress. This is an issue because we depend "
                "on the returned ptr being at the same place as the min app adrees.");
    return ptr;
}

void platform_virtual_free(void *block,u64 size, bool aligned)
{
    bool result = VirtualFree( block, size, MEM_DECOMMIT);
    if(!result)
    {
        DFATAL("Virtual free failed. Windows error_code: %d", GetLastError());
    }
}
// this will commit page size
void *platform_virtual_commit(void *ptr, u32 num_pages)
{
    void         *return_ptr = nullptr;
    platform_info info;
    if (platform_state_ptr)
    {
        info = platform_state_ptr->info;
    }
    else
    {
        info = platform_get_info();
    }

    if (reinterpret_cast<uintptr_t>(ptr) % info.page_size == 0)
    {
        u64 size = info.page_size * num_pages;
        return_ptr = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
        // better error handling
        DASSERT_MSG(return_ptr, "Coulnd't virtual Alloc");
    }
    else
    {
        DFATAL("The ptr %lld is not a multiple of the page size %d", reinterpret_cast<uintptr_t>(ptr), info.page_size);
        return nullptr;
    }
    return return_ptr;
}

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
void *platform_set_memory(void *block, s64 value, u64 size)
{
    return memset(block, value, size);
}

void *platform_copy_memory(void *dest, const void *source, u64 size)
{
    return memcpy(dest, source, size);
}

void *platform_set_memory(void *dest, s32 value, u64 size)
{
    return memset(dest, value, size);
}

// NOTE: colors -> https://learn.microsoft.com/en-us/windows/console/console-screen-buffers

void platform_console_write(const char *message, u8 colour)
{
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    u8     white          = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    static u8 levels[6]   = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(console_handle, levels[colour]);
    OutputDebugStringA(message);
    u64     length         = strlen(message);
    LPDWORD number_written = 0;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, static_cast<DWORD>(length), number_written, 0);
    SetConsoleTextAttribute(console_handle, white);
}

void platform_console_write_error(const char *message, u8 colour)
{
    HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);
    u8     white          = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    static u8 levels[6]   = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(console_handle, levels[colour]);
    OutputDebugStringA(message);
    u64     length         = strlen(message);
    LPDWORD number_written = 0;
    WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, static_cast<DWORD>(length), number_written, 0);
    SetConsoleTextAttribute(console_handle, white);
}

f64 platform_get_absolute_time()
{
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return static_cast<f64>(now_time.QuadPart) * clock_frequency;
}

void platform_sleep(u64 ms)
{
    ZoneScoped;
    Sleep(ms);
}

void platform_get_window_dimensions(u32 *width, u32 *height)
{
    if (platform_state_ptr)
    {
        *width  = platform_state_ptr->width;
        *height = platform_state_ptr->height;
    }
}

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
        // Notify the OS that erasing will be handled by the application to prevent flicker.
        return 1;
    case WM_CLOSE: {
        event_context context = {};
        event_fire(EVENT_CODE_APPLICATION_QUIT, context);
        return 0;
    }
    break;
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }
    break;
    case WM_SIZE: {
        // Get the updated size.

        RECT r;

        event_context context = {};

        GetClientRect(hwnd, &r);
        u32 width  = r.right - r.left;
        u32 height = r.bottom - r.top;

        context.data.u32[0] = width;
        context.data.u32[1] = height;

        platform_state_ptr->width  = width;
        platform_state_ptr->height = height;

        event_fire(EVENT_CODE_APPLICATION_RESIZED, context);
    }
    break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        // Key pressed/released
        keys key = static_cast<keys>(w_param);

        // wtf windows why not just send lalt ralt messages seperatly??
        WORD key_flags = HIWORD(l_param);
        WORD scan_code = LOBYTE(key_flags);
        BOOL isExtendedKey =
            (key_flags & KF_EXTENDED) == KF_EXTENDED; // extended-key flag, 1 if scancode has 0xE0 prefix

        if (isExtendedKey)
        {
            scan_code = MAKEWORD(scan_code, 0xE0);
        }

        bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);

        switch (w_param)
        {
        case VK_SHIFT:   // converts to VK_LSHIFT or VK_RSHIFT
        case VK_CONTROL: // converts to VK_LCONTROL or VK_RCONTROL
        case VK_MENU: {
            UINT win_32_key = MapVirtualKeyW(scan_code, MAPVK_VSC_TO_VK_EX);
            key             = static_cast<keys>(win_32_key);
        }
        break;
        };

        // Pass to the input subsystem for processing.

        input_process_key(key, pressed);
    }
    break;
    case WM_MOUSEMOVE: {
        // Mouse move
        s32 x_position = GET_X_LPARAM(l_param);
        s32 y_position = GET_Y_LPARAM(l_param);

        //// Pass over to the input subsystem.
        input_process_mouse_move(x_position, y_position);
    }
    break;
    case WM_MOUSEWHEEL: {
        s32 z_delta = GET_WHEEL_DELTA_WPARAM(w_param);
        if (z_delta != 0)
        {
            // Flatten the input to an OS-independent (-1, 1)
            z_delta = (z_delta < 0) ? -1 : 1;
            input_process_mouse_wheel(z_delta);
        }
    }
    break;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP: {
        bool    pressed      = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
        buttons mouse_button = BUTTON_MAX_BUTTONS;
        switch (msg)
        {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            mouse_button = BUTTON_LEFT;
            break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            mouse_button = BUTTON_MIDDLE;
            break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
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
    }

    return DefWindowProcA(hwnd, msg, w_param, l_param);
}

bool vulkan_platform_get_required_vulkan_extensions(darray<const char *> &extensions_array)
{
    const char *win32_surface = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
    extensions_array.push_back(win32_surface);
    return true;
}
bool vulkan_platform_create_surface(vulkan_context *vk_context)
{
    DDEBUG("Creating vulkan win32 surface...");
    VkWin32SurfaceCreateInfoKHR surface_create_info{};

    surface_create_info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.pNext     = nullptr;
    surface_create_info.flags     = 0;
    surface_create_info.hwnd      = platform_state_ptr->hwnd;
    surface_create_info.hinstance = GetModuleHandle(nullptr);

    VkResult result = vkCreateWin32SurfaceKHR(vk_context->vk_instance, &surface_create_info, vk_context->vk_allocator,
                                              &vk_context->vk_surface);
    VK_CHECK(result);

    return true;
}

platform_info platform_get_info()
{
    platform_info info{};

    SYSTEM_INFO win32_sys_info{};
    GetSystemInfo(&win32_sys_info);

    info.page_size               = win32_sys_info.dwPageSize;
    info.allocation_granualarity = win32_sys_info.dwAllocationGranularity;
    info.min_app_address         = win32_sys_info.lpMinimumApplicationAddress;
    info.max_app_address         = win32_sys_info.lpMaximumApplicationAddress;

    return info;
}

#endif // DPLATFORM_WINDOWS
