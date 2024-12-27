#include "logger.h"
#include "platform/platform.h"
#include <stdarg.h>
#include <stdio.h>

void log_message(log_levels level, const char *msg, ...)
{
    const u32 max_chars = 1024;
    char buffer[max_chars];

    va_list arg_list;
    va_start(arg_list, msg);
    vsnprintf(buffer, max_chars, msg, arg_list);
    va_end(arg_list);

    platform_log_message(buffer, level, max_chars);
}
