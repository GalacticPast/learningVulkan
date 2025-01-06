#include "events.h"
#include "containers/array.h"
#include "core/logger.h"

static event_system state;
static b8           is_initialized;

void event_system_initialize()
{
    if (is_initialized)
    {
        ERROR("event_system already initialized");
    }

    state.events   = array_create(event);
    is_initialized = true;

    INFO("Event system initialized");
}
void event_system_destroy()
{
    INFO("Destroying event system...");
    s32 events_size = (s32)array_get_length(state.events);

    for (s32 i = 0; i < events_size; i++)
    {
        if (state.events[i].listeners)
        {
            array_destroy(state.events[i].listeners);
        }
    }
    array_destroy(state.events);
    state.events = 0;
    INFO("Event system destoryed");
}

void event_register(event_type type, fp_on_event fp_on_event, void *data)
{
    if (!is_initialized)
    {
        ERROR("Event system is unitialized, initialize it");
        return;
    }

    s32    events_size = (s32)array_get_length(state.events);
    event *found_event = 0;

    // INFO: check if we have already registered for this event; I think this is ugly and I should make it better but atm idk how
    for (s32 i = 0; i < events_size; i++)
    {
        if (state.events[i].type == type)
        {
            found_event = &state.events[i];
            if (state.events[i].listeners != 0)
            {
                s64 listeners_size = (s32)array_get_length(state.events[i].listeners);
                for (s32 j = 0; j < listeners_size; j++)
                {
                    if (state.events[i].listeners[j].callback == fp_on_event)
                    {
                        WARN("Already registered for this event");
                        return;
                    }
                }
            }
        }
    }

    if (!found_event)
    {
        event event     = {};
        event.type      = type;
        event.listeners = array_create(event_listener);
        found_event     = &event;
    }

    event_listener listener = {fp_on_event, data};
    array_push_value(found_event->listeners, &listener);
    state.events = array_push_value(state.events, found_event);
    DEBUG("Successfully registered for event");
}

void event_unregister(event_type type, fp_on_event fp_on_event)
{
}

void event_fire(event_type type, event_context context)
{
    if (!is_initialized)
    {
        ERROR("Event_system is not initialized!!");
        return;
    }

    s32 events_size = (s32)array_get_length(state.events);

    switch (type)
    {
        case ON_KEY_PRESS: {
        }
        break;
        case ON_APPLICATION_QUIT: {
            for (s32 i = 0; i < events_size; i++)
            {
                if (state.events[i].type == ON_APPLICATION_QUIT)
                {
                    u64 listeners_size = array_get_length(state.events[i].listeners);
                    for (u32 j = 0; j < listeners_size; j++)
                    {
                        void *user_data = state.events[i].listeners[j].user_data;
                        state.events[i].listeners[j].callback(ON_APPLICATION_QUIT, context, user_data);
                    }
                }
            }
        }
        break;
        case ON_APPLICATION_RESIZE: {
            for (s32 i = 0; i < events_size; i++)
            {
                if (state.events[i].type == ON_APPLICATION_RESIZE)
                {
                    u64 listeners_size = array_get_length(state.events[i].listeners);
                    for (u32 j = 0; j < listeners_size; j++)
                    {
                        void *user_data = state.events[i].listeners[j].user_data;
                        state.events[i].listeners[j].callback(ON_APPLICATION_QUIT, context, user_data);
                    }
                }
            }
        }
        break;
        default: {
        }
    }
}
