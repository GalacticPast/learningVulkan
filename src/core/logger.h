#pragma once

typedef enum log_levels
{
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_TRACE,
    LOG_LEVEL_MAX_LEVEL
} log_levels;

void log_message(log_levels level, const char *msg, ...);

#define FATAL(msg, ...) log_message(LOG_LEVEL_FATAL, msg, ##__VA_ARGS__)
#define ERROR(msg, ...) log_message(LOG_LEVEL_ERROR, msg, ##__VA_ARGS__)
#define DEBUG(msg, ...) log_message(LOG_LEVEL_DEBUG, msg, ##__VA_ARGS__)
#define WARN(msg, ...) log_message(LOG_LEVEL_WARN, msg, ##__VA_ARGS__)
#define INFO(msg, ...) log_message(LOG_LEVEL_INFO, msg, ##__VA_ARGS__)
#define TRACE(msg, ...) log_message(LOG_LEVEL_TRACE, msg, ##__VA_ARGS__)
