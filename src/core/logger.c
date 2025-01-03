#include "logger.h"
#include "platform/platform.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void log_message(log_levels level, const char *msg, ...)
{

    const char *log_level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[DEBUG]: ", "[WARN]:  ", "[INFO]:  ", "[TRACE]: "};

    const u32 max_chars = 4096;
    char      buffer[max_chars];
    memset(buffer, 0, max_chars);

    va_list arg_ptr;
    va_start(arg_ptr, msg);
    vsnprintf(buffer, max_chars, msg, arg_ptr);
    va_end(arg_ptr);

    char buffer2[max_chars];
    sprintf(buffer2, "%s%s\n", log_level_strings[level], buffer);

    platform_log_message(buffer2, level, max_chars);
}
