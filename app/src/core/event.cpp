#include "event.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"

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

static event_system_state *event_system_state_ptr;

bool event_system_startup(arena* arena)
{
    DASSERT(arena);
    DINFO("Starting up event system...");
    event_system_state_ptr = nullptr;
    event_system_state_ptr = static_cast<event_system_state *>(dallocate(arena, sizeof(event_system_state), MEM_TAG_APPLICATION));
    DASSERT(event_system_state_ptr);

    return true;
}
void event_system_shutdown()
{
    if (event_system_state_ptr)
    {
        DINFO("Shutting down event system...");
        event_system_state_ptr = 0;
    }
    else
    {
        DERROR("Event system is not initialzed yet. Cant shutdown something that has not been initialized");
    }
}

bool event_system_register(event_code code, void *data,
                           bool (*event_listener_callback)(event_context context, void *data))
{
    if (!event_system_state_ptr)
    {
        DERROR(
            "Event system is not initialzed yet. Cant register for events when event_system has not been initialized");
        return false;
    }
    if (code > MAX_EVENT_CODES)
    {
        DERROR("event_code provided does not exist. Make sure you are passing a valid event_code.");
        return false;
    }

    s32 num_registered = 0;
    for (s32 i = 0; i < MAX_REGISTERED_LISTENERS_FOR_SINGLE_EVENT; i++)
    {
        if (event_system_state_ptr->events[code].listener_infos[i].func_ptr != nullptr)
        {
            num_registered++;
            continue;
        }
        event_system_state_ptr->events[code].listener_infos[i].func_ptr      = event_listener_callback;
        event_system_state_ptr->events[code].listener_infos[i].listener_data = data;
        break;
    }
    if (num_registered)
    {
        DWARN("Already registered for this event %d times", num_registered);
    }
    return true;
}

bool event_system_unregister(event_code code, void *data,
                             bool (*event_listener_callback)(event_context context, void *data))
{
    if (!event_system_state_ptr)
    {
        DERROR(
            "Event system is not initialzed yet. Cant register for events when event_system has not been initialized");
        return false;
    }
    if (code > MAX_EVENT_CODES)
    {
        DERROR("event_code provided does not exist. Make sure you are passing a valid event_code.");
        return false;
    }

    for (s32 i = 0; i < MAX_REGISTERED_LISTENERS_FOR_SINGLE_EVENT; i++)
    {
        if (event_system_state_ptr->events[code].listener_infos[i].func_ptr == event_listener_callback &&
            event_system_state_ptr->events[code].listener_infos[i].listener_data == data)
        {
            event_system_state_ptr->events[code].listener_infos[i].func_ptr      = nullptr;
            event_system_state_ptr->events[code].listener_infos[i].listener_data = nullptr;
            return true;
        }
    }
    return false;
}

void event_fire(event_code code, event_context context)
{
    if (!event_system_state_ptr)
    {
        DERROR("Event system is not initialzed yet. Cant fire events when event_system has not been initialized");
        return;
    }
    if (code > MAX_EVENT_CODES)
    {
        DERROR("event_code provided does not exist. Make sure you are passing a valid event_code.");
        return;
    }

    bool result = false;

    for (s32 i = 0; i < MAX_REGISTERED_LISTENERS_FOR_SINGLE_EVENT; i++)
    {
        if (event_system_state_ptr->events[code].listener_infos[i].func_ptr == nullptr)
        {
            break;
        }
        result = event_system_state_ptr->events[code].listener_infos[i].func_ptr(
            context, event_system_state_ptr->events[code].listener_infos[i].listener_data);
        if (!result)
        {
            DERROR("The %dth event_listener at %dth events failed unexpecdetly", i, code);
        }
    }
}
