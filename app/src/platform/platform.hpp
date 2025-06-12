#pragma once
#include "defines.hpp"

struct platform_info
{
    u32   page_size;
    u32   allocation_granualarity;
    void *min_app_address;
    void *max_app_address;
};

bool platform_system_startup(u64 *platform_mem_requirements, void *plat_state, struct application_config *config);

void platform_system_shutdown(void *state);

bool platform_pump_messages();

platform_info platform_get_info();

// malloc
void *platform_allocate(u64 size, bool aligned);
void  platform_free(void *block, bool aligned);

// virtual alloc
void *platform_virtual_reserve(u64 size, bool aligned);
void  platform_virtual_free(void *block, bool aligned);
void  platform_virtual_commit(u64 mem_size, bool aligned);

void *platform_zero_memory(void *block, u64 size);
void *platform_copy_memory(void *dest, const void *source, u64 size);
void *platform_set_memory(void *dest, s64 value, u64 size);

void platform_console_write(const char *message, u8 color);
void platform_console_write_error(const char *message, u8 color);

f64 platform_get_absolute_time();

void platform_get_window_dimensions(u32 *width, u32 *height);
// Sleep on the thread for the provided ms. This blocks the main thread.
// Should only be used for giving time back to the OS for unused update power.
// Therefore it is not exported.
void platform_sleep(u64 ms);
