#include "event_system_test.hpp"
#include "../expect.hpp"
#include "core/dmemory.hpp"
#include "core/event.hpp"
#include "core/logger.hpp"
#include <string.h>

bool event_code_test_A_callback(event_context context, void *data)
{
    DDEBUG("Hey it fired");

    expect_should_be(context.data.u32[0], 0);
    expect_should_be(context.data.u32[1], 1);
    expect_should_be(context.data.u32[2], 2);
    expect_should_be(context.data.u32[3], 3);

    return true;
}

bool event_system_register_and_unregister_test()
{
    event_system_state *event_state;
    u64                 event_system_memory_requirements = 0;

    event_system_startup(&event_system_memory_requirements, 0);
    event_state = (event_system_state *)malloc(event_system_memory_requirements);
    memset((void *)event_state, 0, event_system_memory_requirements);
    event_system_startup(&event_system_memory_requirements, event_state);

    int data = 5;

    event_system_register(EVENT_CODE_TEST_A, &data, event_code_test_A_callback);

    event_context context = {0};
    context.data.u32[0]   = 0;
    context.data.u32[1]   = 1;
    context.data.u32[2]   = 2;
    context.data.u32[3]   = 3;

    event_code code = EVENT_CODE_TEST_A;
    event_fire(code, context);

    event_system_unregister(EVENT_CODE_TEST_A, &data);

    for (s32 i = 0; i < MAX_REGISTERED_LISTENERS_FOR_SINGLE_EVENT; i++)
    {
        expect_should_be(event_state->events[code].listener_infos[i].listener_data, nullptr);
        expect_should_be(event_state->events[code].listener_infos[i].func_ptr, nullptr);
    }
    return true;
}

void event_system_register_tests()
{
    test_manager_register_tests(event_system_register_and_unregister_test, "Event system register and unregister test.");
}
