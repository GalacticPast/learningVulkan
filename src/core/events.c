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

    state.events = array_create(event_system);
    is_initialized = true;

    INFO("Event system initialized");
}
void event_system_destroy()
{
}

void event_register(event_type type, fp_on_event fp_on_event)
{
    if (!is_initialized)
    {
        ERROR("Event system is unitialized, initialize it");
        return;
    }

    u64    events_size = array_get_length(state.events);
    event *found_event = 0;

    // INFO: check if we have already registered for this event; I think this is ugly and I should make it better but atm idk how
    for (u32 i = 0; i < events_size; i++)
    {
        if (state.events[i].type == type)
        {
            found_event = &state.events[i];
            if (state.events[i].listeners != 0)
            {
                u64 listeners_size = array_get_length(state.events[i].listeners);
                for (u32 j = 0; j < listeners_size; j++)
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
        event event = {};
        event.type = type;
        event.listeners = array_create(event_listener);
        found_event = &event;
    }

    event_listener listener = {fp_on_event};
    array_push_value(found_event->listeners, &listener);
    state.events = array_push_value(state.events, found_event);
    INFO("Successfully registered for event");
}

void event_unregister(event_type type, fp_on_event fp_on_event)
{
}
