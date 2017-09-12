/* -*- encoding: utf-8; -*- */
/* -*- c-mode -*- */
/* File-name:    <logging.h> */
/* Author:       <Xsoda> */
/* Create:       <Tuesday September 03 11:07:48 2013> */
/* Time-stamp:   <Monday December 23, 12:32:56 2013> */
/* Mail:         <Xsoda@Live.com> */

#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOGLVL_TRACE,
    LOGLVL_DEBUG,
    LOGLVL_INFO,
    LOGLVL_WARN,
    LOGLVL_ERROR,
    LOGLVL_CRIT,
    LOGLVL_NONE,
} loglevel_t;

#define LOGLVL_MAX LOGLVL_CRIT

typedef struct _loghandler loghandler_t;

typedef void (*log_userdate_destroy_fn)(void *userdata);
typedef void (*log_handle_fn)(void *userdata,
                              const char *time, loglevel_t level, const char *level_str,
                              const char *file, const char *fn,
                              int line, const char *msg);

void log_init();
void log_cleanup();

const FILE *log_get_fp();

/*
 * log_output_file
 *   logfolder: log file directory
 *   logprefix: log file name prefix, file name is `prefix`.yyyy-mm-dd.log
 *   reserve:   log file max reserved days
 */
void log_output_file(loglevel_t level, const char *logfolder, const char *logprefix, int reserve);

void log_logger(loglevel_t level, int line,
                const char *fn, const char *file,
                const char *fmt, ...);

void log_set_handler(loglevel_t level, log_handle_fn fn,
                     void *userdata, log_userdate_destroy_fn destroy_fn);

void log_stdout_simple(void *userdata,
                       const char *time, loglevel_t level, const char *level_str,
                       const char *file, const char *fn,
                       int line, const char *msg);

void log_stdout_colorful(void *userdata,
                         const char *time, loglevel_t level, const char *level_str,
                         const char *file, const char *fn,
                         int line, const char *msg);

void log_hex_dump(FILE *out, const void *data, size_t size);
void log_hex_dumph(FILE *out, const void *data, size_t size);

#if __linux__
#define LOG_IMPLICIT(lvl_base, fmt, ...) \
   log_logger(LOG ## lvl_base, __LINE__, __func__, __FILE__, fmt, ## __VA_ARGS__)
#else
#define LOG_IMPLICIT(lvl_base, fmt, ...) \
   log_logger(LOG ## lvl_base, __LINE__, __FUNCTION__, __FILE__, fmt, ## __VA_ARGS__)
#endif

#define LOG_EXPLICIT(lvl_base, fmt, ...) \
        LOG_IMPLICIT(lvl_base, fmt, ## __VA_ARGS__ )


/**
 * the following functions send a message of the specified level to
 * the debug logging system. These are noop if lcb was not
 * compiled with debugging.
 */
#define log_trace(fmt, ...) \
    LOG_EXPLICIT(LVL_TRACE, fmt,  ## __VA_ARGS__)

#define log_info(fmt, ...) \
    LOG_EXPLICIT(LVL_INFO, fmt,  ## __VA_ARGS__)

#define log_debug(fmt, ...) \
    LOG_EXPLICIT(LVL_DEBUG, fmt,  ## __VA_ARGS__)

#define log_warn(fmt, ...) \
    LOG_EXPLICIT(LVL_WARN, fmt,  ## __VA_ARGS__)

#define log_err(fmt, ...) \
    LOG_EXPLICIT(LVL_ERROR, fmt, ## __VA_ARGS__)

#define log_crit(fmt, ...) \
    LOG_EXPLICIT(LVL_CRIT, fmt, ## __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif  /* _LOGGING_H_ */
