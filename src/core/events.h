#pragma once

#include "defines.h"

#define MAX_EVENTS 1024

typedef enum event_type
{
    ON_KEY_PRESS,
    EVENT_TYPE_MAX_EVENT
} event_type;

typedef struct event_context
{
    // 128 bits
    union data {
        s16 s16[8];
        u16 u16[8];

        s32 s32[4];
        u32 u32[4];
        f32 f32[4];

        s64 s64[2];
        u64 u64[2];
        f64 f64[2];
    } data;
} event_context;

typedef b8 (*fp_on_event)(event_type type, event_context data);

typedef struct event_listener
{
    fp_on_event callback;
} event_listener;

typedef struct event
{
    event_type      type;
    event_listener *listeners;
} event;

typedef struct event_system
{
    event *events;
} event_system;

void event_system_initialize();
void event_system_destroy();

// callback must point to the fucntion which you would like to be called when the event is trigerred/fired
void event_register(event_type type, fp_on_event fp_on_event);
void event_unregister(event_type type, fp_on_event fp_on_event);
