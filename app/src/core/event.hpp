#pragma once

#include "defines.hpp"

enum event_code
{

    EVENT_CODE_APPLICATION_QUIT    = 0x00,
    EVENT_CODE_APPLICATION_RESIZED = 0x01,
    EVENT_CODE_KEY_PRESSED         = 0x02,
    EVENT_CODE_KEY_RELEASED        = 0x03,
    EVENT_CODE_BUTTON_PRESSED      = 0x04,
    EVENT_CODE_BUTTON_RELEASED     = 0x05,
    EVENT_CODE_MOUSE_MOVED         = 0x06,
    EVENT_CODE_MOUSE_WHEEL         = 0x07,

    EVENT_CODE_TEST_A = 0xFD,
    EVENT_CODE_TEST_B = 0xFE,
    EVENT_CODE_TEST_C = 0xFF,
    MAX_EVENT_CODES   = 256
};
/*
 * @param: event_listener_callback the function that you cant the event system to call when the event gets triggerd.
 * @param: void* listener_data: the data that you wnat the event system to pass to the call back function.
 * I also use the data to identify events for unregistering, so its an identifier too.
 */
struct event_context
{
    // 128 bytes
    u64 elements[2];
    union data {
        u64 u64[2];
        s64 s64[2];

        u32 u32[4];
        s32 s32[4];

        u16 u16[8];
        s16 s16[8];

        u8 u8[16];
        s8 s8[16];
    } data;
};

typedef bool (*event_listener_callback)(event_context context, void *data);

struct listener_info
{
    event_listener_callback func_ptr;
    void *listener_data;
};

#define MAX_REGISTERED_LISTENERS_FOR_SINGLE_EVENT 16
struct event
{
    listener_info listener_infos[MAX_REGISTERED_LISTENERS_FOR_SINGLE_EVENT];
};

struct event_system_state
{
    event events[MAX_EVENT_CODES];
};

bool event_system_startup(u64 *event_system_memory_requirements, void *state);
void event_system_shutdown(void *state);

void event_fire(event_code code, event_context context);

bool event_system_register(event_code code, void *data,
                           bool (*event_listener_callback)(event_context context, void *data));
bool event_system_unregister(event_code code, void *data,
                             bool (*event_listener_callback)(event_context context, void *data));
