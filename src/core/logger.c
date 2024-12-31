#include "logger.h"
#include "platform/platform.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

u32 write_log_level_to_buf(char *buffer, log_levels level)
{
    u32 chars_written = 0;

    switch (level)
    {
        case LOG_LEVEL_FATAL: {
            char *string = "[FATAL]:";
            chars_written = 8;
            memcpy(buffer, string, chars_written);
        }
        break;
        case LOG_LEVEL_ERROR: {
            char *string = "[ERROR]:";
            chars_written = 8;
            memcpy(buffer, string, chars_written);
        }
        break;
        case LOG_LEVEL_DEBUG: {
            char *string = "[DEBUG]:";
            chars_written = 8;
            memcpy(buffer, string, chars_written);
        }
        break;
        case LOG_LEVEL_WARN: {
            char *string = "[WARN]:";
            chars_written = 7;
            memcpy(buffer, string, chars_written);
        }
        break;
        case LOG_LEVEL_INFO: {
            char *string = "[INFO]:";
            chars_written = 7;
            memcpy(buffer, string, chars_written);
        }
        break;
        case LOG_LEVEL_TRACE: {
            char *string = "[TRACE]:";
            chars_written = 8;
            memcpy(buffer, string, chars_written);
        }
        break;
        case LOG_LEVEL_MAX_LEVEL: {
            return 0;
        }
        break;
    }

    return chars_written;
}

void log_message(log_levels level, const char *msg, ...)
{
    const u32 max_chars = 4096;
    char      buffer[max_chars];

    u32 chars_written = write_log_level_to_buf(buffer, level);

    va_list arg_list;
    va_start(arg_list, msg);
    vsnprintf(buffer + chars_written, max_chars - chars_written, msg, arg_list);
    va_end(arg_list);
    platform_log_message(buffer, level, max_chars);
}
