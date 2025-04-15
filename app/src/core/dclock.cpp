#include "dclock.hpp"
#include "platform/platform.hpp"

void clock_start(dclock *out_clock)
{
    out_clock->start_time   = platform_get_absolute_time();
    out_clock->time_elapsed = 0;
}

void clock_update(dclock *out_clock)
{
    if (out_clock->start_time != 0)
    {
        out_clock->time_elapsed = platform_get_absolute_time() - out_clock->start_time;
    }
}
void clock_stop(dclock *out_clock)
{
    out_clock->start_time   = 0;
    out_clock->time_elapsed = 0;
}
