#include "input.h"
#include "events.h"

void input_process_key(keys key, b8 state)
{

    event_context context = {};
    context.data.u32[0]   = key;
    context.data.u8[0]    = state;

    if (key == KEY_ESCAPE)
    {
        event_fire(ON_APPLICATION_QUIT, context);
    }

    event_fire(ON_KEY_PRESS, context);
}
