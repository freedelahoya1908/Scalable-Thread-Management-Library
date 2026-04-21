#ifndef LOGGER_H
#define LOGGER_H

/*
 * Thread-safe logger with log levels, timestamps, and thread-id tagging.
 * Production-grade logging for concurrent systems.
 */

#include <stdio.h>

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO  = 1,
    LOG_WARN  = 2,
    LOG_ERROR = 3,
    LOG_FATAL = 4
} LogLevel;

/* ── API ── */
void        logger_init(LogLevel min_level, const char* file_path);
void        logger_set_level(LogLevel min_level);
void        logger_set_colors(int enabled);
void        logger_log(LogLevel level, const char* module, const char* fmt, ...);
void        logger_shutdown(void);
const char* log_level_str(LogLevel level);

/* ── Convenience macros ── */
#define LOG_D(mod, ...)  logger_log(LOG_DEBUG, mod, __VA_ARGS__)
#define LOG_I(mod, ...)  logger_log(LOG_INFO,  mod, __VA_ARGS__)
#define LOG_W(mod, ...)  logger_log(LOG_WARN,  mod, __VA_ARGS__)
#define LOG_E(mod, ...)  logger_log(LOG_ERROR, mod, __VA_ARGS__)
#define LOG_F(mod, ...)  logger_log(LOG_FATAL, mod, __VA_ARGS__)

#endif
