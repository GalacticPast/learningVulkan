#include "event.hpp"
#include "core/logger.hpp"

static event_system_state *event_system_state_ptr;

bool event_system_startup(u64 *event_system_memory_requirements, void *state)
{
    *event_system_memory_requirements = sizeof(event_system_state);
    if (!state)
    {
        return true;
    }
    event_system_state_ptr = (event_system_state *)state;
    return true;
}
void event_system_shutdown()
{
    if (event_system_state_ptr)
    {
        event_system_state_ptr = 0;
    }
    else
    {
        DERROR("Event system is not initialzed yet. Cant shutdown something that has not been initialized");
    }
}

bool event_system_register(event_code code, event_listener_callback, void *data)
{
    if (!event_system_state_ptr)
    {
        DERROR("Event system is not initialzed yet. Cant register for events when event_system has not been initialized");
        return false;
    }
    if (code > MAX_EVENT_CODES)
    {
        DERROR("event_code provided does not exist. Make sure you are passing a valid event_code.");
    }
    if (data == nullptr)
    {
        DWARN("Passed on data is nullptr. Cannot pass nullptr as data, you can register for the specific event but the max number of listeners that can register for this event is 1");
    }
}

bool event_system_unregister(event_code code, void *data);
