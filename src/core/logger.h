#pragma once

typedef enum log_levels
{
    FATAL,
    ERROR,
    DEBUG,
    INFO,
    TRACE,
    MAX_LEVEL
} log_levels;

void log_message(log_levels level, const char *msg, ...);

#define FATAL(msg, ...) log_message(FATAL, msg, ##__VA_ARGS__)
#define ERROR(msg, ...) log_message(ERROR, msg, ##__VA_ARGS__)
#define DEBUG(msg, ...) log_message(DEBUG, msg, ##__VA_ARGS__)
#define INFO(msg, ...) log_message(INFO, msg, ##__VA_ARGS__)
#define TRACE(msg, ...) log_message(TRACE, msg, ##__VA_ARGS__)
