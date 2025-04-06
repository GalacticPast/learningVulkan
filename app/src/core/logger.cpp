#include "logger.hpp"
#include "dasserts.hpp"
#include "dmemory.hpp"
#include "platform/platform.hpp"

// TODO: temporary
#include <cstdio>
#include <stdarg.h>

void log_output(log_level level, const char *message, ...)
{
    // TODO: These string operations are all pretty slow. This needs to be
    // moved to another thread eventually, along with the file writes, to
    // avoid slowing things down while the engine is trying to run.
    const char *level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]:  ", "[INFO]:  ", "[DEBUG]: ", "[TRACE]: "};
    bool        is_error         = level < LOG_LEVEL_WARN;

    // Technically imposes a 32k character limit on a single log entry, but...
    // DON'T DO THAT!
    char out_message[32000];
    dzero_memory(out_message, sizeof(out_message));

    // Format original message.
    // NOTE: Oddly enough, MS's headers override the GCC/Clang va_list type with a "typedef char* va_list" in some
    // cases, and as a result throws a strange error here. The workaround for now is to just use __builtin_va_list,
    // which is the type GCC/Clang's va_start expects.
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);

    char buffer[32000];
    s32  written    = vsnprintf(buffer, 32000, message, arg_ptr);
    buffer[written] = 0;
    dcopy_memory(out_message, buffer, written + 1);

    va_end(arg_ptr);

    // Prepend log level to message.

    char out_message2[32000];
    dzero_memory(out_message2, sizeof(out_message));

    sprintf(out_message2, "%s%s\n", level_strings[level], out_message);
    // Print accordingly
    if (is_error)
    {
        platform_console_write_error(out_message2, level);
    }
    else
    {
        platform_console_write(out_message2, level);
    }

    // Queue a copy to be written to the log file.
}

void report_assertion_failure(const char *expression, const char *message, const char *file, s32 line)
{
    log_output(LOG_LEVEL_FATAL, "Assertion Failure: %s, message: '%s', in file: %s, line: %d\n", expression, message, file, line);
}
