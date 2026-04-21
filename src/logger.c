#include "../include/logger.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* ── Logger state ── */
static pthread_mutex_t g_log_lock   = PTHREAD_MUTEX_INITIALIZER;
static LogLevel        g_min_level  = LOG_INFO;
static FILE*           g_file       = NULL;
static int             g_colors     = 1;
static int             g_initialized = 0;

static const char* COLORS[] = {
    "\033[90m",   /* DEBUG: grey */
    "\033[36m",   /* INFO:  cyan */
    "\033[33m",   /* WARN:  yellow */
    "\033[31m",   /* ERROR: red */
    "\033[1;31m"  /* FATAL: bold red */
};
static const char* RESET = "\033[0m";

const char* log_level_str(LogLevel level) {
    switch(level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO ";
        case LOG_WARN:  return "WARN ";
        case LOG_ERROR: return "ERROR";
        case LOG_FATAL: return "FATAL";
        default:        return "?????";
    }
}

void logger_init(LogLevel min_level, const char* file_path) {
    pthread_mutex_lock(&g_log_lock);
    g_min_level   = min_level;
    g_initialized = 1;
    if (file_path) {
        g_file = fopen(file_path, "w");
        if (g_file) {
            fprintf(g_file, "── Logger started ──\n");
            fflush(g_file);
        }
    }
    pthread_mutex_unlock(&g_log_lock);
}

void logger_set_level(LogLevel min_level) {
    pthread_mutex_lock(&g_log_lock);
    g_min_level = min_level;
    pthread_mutex_unlock(&g_log_lock);
}

void logger_set_colors(int enabled) {
    pthread_mutex_lock(&g_log_lock);
    g_colors = enabled ? 1 : 0;
    pthread_mutex_unlock(&g_log_lock);
}

void logger_log(LogLevel level, const char* module, const char* fmt, ...) {
    if (!g_initialized) { g_min_level = LOG_INFO; g_initialized = 1; }
    if (level < g_min_level) return;

    /* Format timestamp */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm lt;
    localtime_r(&ts.tv_sec, &lt);
    char tbuf[32];
    snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d.%03ld",
             lt.tm_hour, lt.tm_min, lt.tm_sec, ts.tv_nsec / 1000000);

    /* Format body */
    char body[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(body, sizeof(body), fmt, ap);
    va_end(ap);

    pthread_t self = pthread_self();
    unsigned long tid_short = (unsigned long)self & 0xFFFF;

    pthread_mutex_lock(&g_log_lock);

    /* Console output */
    const char* color = (g_colors && level >= 0 && level <= 4) ? COLORS[level] : "";
    const char* rst   = (g_colors) ? RESET : "";
    fprintf(stdout, "%s[%s] %s %-9s t#%04lx │ %s%s\n",
            color, tbuf, log_level_str(level), module ? module : "app",
            tid_short, body, rst);
    fflush(stdout);

    /* File output (no color codes) */
    if (g_file) {
        fprintf(g_file, "[%s] %s %-9s t#%04lx │ %s\n",
                tbuf, log_level_str(level), module ? module : "app",
                tid_short, body);
        fflush(g_file);
    }

    pthread_mutex_unlock(&g_log_lock);
}

void logger_shutdown(void) {
    pthread_mutex_lock(&g_log_lock);
    if (g_file) {
        fprintf(g_file, "── Logger shutdown ──\n");
        fclose(g_file);
        g_file = NULL;
    }
    pthread_mutex_unlock(&g_log_lock);
}
