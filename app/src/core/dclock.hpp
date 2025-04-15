#pragma once
#include "defines.hpp"

struct dclock
{
    f64 start_time;
    f64 time_elapsed;
};

void clock_update(dclock *out_clock);
void clock_stop(dclock *out_clock);
