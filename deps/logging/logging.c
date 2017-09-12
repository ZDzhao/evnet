#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <sys/timeb.h>
#include <time.h>
#ifdef __linux__
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef _WIN32
#include <direct.h>
#define snprintf _snprintf
#define flockfile _lock_file
#define funlockfile _unlock_file
#define mkdir(p, m) mkdir(p)
#endif

struct _loghandler {
    int32_t type;
    log_handle_fn fn;
    void *userdata;
    log_userdate_destroy_fn destroy;
    loglevel_t level;
    loghandler_t *next;
};

typedef struct _log {
    loghandler_t *handle;
} log_t;

typedef struct _logfile {
    char prefix[32];
    char dir[512];
    int reserve;
    char date[11];
    FILE *fp;
} logfile_t;

static log_t __log__  = { 0 };
static char *lvl_str[] = { "T", "D", "I", "W", "E", "C" };

void log_init() {
    __log__.handle = NULL;
}

void log_cleanup() {
    loghandler_t *handle = __log__.handle;
    while (handle) {
        __log__.handle = handle->next;
        if (handle->destroy)
            handle->destroy(handle->userdata);
        free(handle);
        handle = __log__.handle;
    }
}

void log_set_handler(loglevel_t level, log_handle_fn fn,
                     void *userdata, log_userdate_destroy_fn destroy_fn)
{
    loghandler_t *handle;

    if ((handle = (loghandler_t *)malloc(sizeof(loghandler_t))) == 0) {
        return;
    }
    memset(handle, 0, sizeof(loghandler_t));
    handle->level = level;
    handle->fn = fn;
    handle->userdata = userdata;
    handle->destroy = destroy_fn;
    handle->next = __log__.handle;
    __log__.handle = handle;
}

void log_logger(loglevel_t level,
                int line, const char *fn,
                const char *file, const char *fmt, ...)
{
    va_list ap, cpy;
    int length;
    loghandler_t *handle;
    char timebuf[64];
    char *msg = 0;
    struct timeb t;

    ftime(&t);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&t.time));
    snprintf(timebuf + 19, sizeof(timebuf) - 19, ".%03d", t.millitm);
    va_start(ap, fmt);
    va_copy(cpy, ap);
    length = vsnprintf(NULL, 0, fmt, cpy);

    if ((msg = (char *)malloc(length + 1)) != 0) {
        if (vsnprintf (msg, length + 1, fmt, ap) < 0) {
            *msg = 0;
        }
    }

    if (msg && *msg) {
        handle = __log__.handle;
        while (handle) {
            if (level >= handle->level) {
                handle->fn(handle->userdata,
                           timebuf, level, lvl_str[level], file, fn, line, msg);
            }
            handle = handle->next;
        }
    }

    if (msg) {
        free(msg);
    }

    va_end(ap);
    return;
}

void log_stdout_simple(void *userdata,
                       const char *time, loglevel_t level,
                       const char *level_str, const char *file,
                       const char *fn, int line, const char *msg)
{
    flockfile(stdout);
    fprintf(stdout, "%s %s %s:%d %s\n", time, level_str, fn, line, msg);
    fflush(stdout);
    funlockfile(stdout);
}

#ifndef _WIN32
#define _FG "3"
#define _BG "4"
#define _BRIGHT_FG "1"
#define _INTENSE_FG "9"
#define _DIM_FG "2"
#define _YELLOW "3"
#define _WHITE "7"
#define _MAGENTA "5"
#define _CYAN "6"
#define _BLUE "4"
#define _GREEN "2"
#define _RED "1"
#define _BLACK "0"
static const char *Color_domain_fmt = "\033["  _INTENSE_FG _MAGENTA "m";
static const char *Color_reset_fmt = "\033[0m";
void log_stdout_colorful(void *userdata,
                         const char *time, loglevel_t level, const char *level_str,
                         const char *file, const char *fn, int line, const char *msg)
{
    const char *reset_fmt = "", *line_fmt = "";
    reset_fmt = Color_reset_fmt;

    switch (level) {
    case LOGLVL_CRIT:
        line_fmt = "\033[" _FG _RED "m";
        break;

    case LOGLVL_ERROR:
        line_fmt = "\033[" _BRIGHT_FG ";" _FG _RED "m";
        break;

    case LOGLVL_WARN:
        line_fmt = "\033[" _FG _YELLOW "m";
        break;

    case LOGLVL_DEBUG:
        line_fmt = "\033[" _DIM_FG ";" _FG _CYAN "m";
        break;

    case LOGLVL_TRACE:
        line_fmt = "\033[" _DIM_FG ";" _FG _WHITE "m";
        break;

    case LOGLVL_INFO:
        line_fmt = "\033[" _BRIGHT_FG ";" _FG _BLUE "m";
        break;

    default:
        /* No color */
        line_fmt = "";
        break;
    }

    flockfile(stdout);
    fprintf(stdout, "%s%s %s [%s:%d] %s%s\n", line_fmt, time, level_str, fn, line, msg, reset_fmt);
    fflush(stdout);
    funlockfile(stdout);
}
#else
#include <windows.h>
void log_stdout_colorful(log_t *log, void *userdata, const char *time, loglevel_t level,
                         const char *level_str, const char *file,
                         const char *fn, int line, const char *msg)
{
    WORD reset_fmt = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
    WORD line_fmt;
    HANDLE std;
    std = GetStdHandle(STD_OUTPUT_HANDLE);

    switch (level) {
    case LOGLVL_CRIT:
        line_fmt = FOREGROUND_RED | FOREGROUND_INTENSITY;
        break;

    case LOGLVL_ERROR:
        line_fmt = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;

    case LOGLVL_WARN:
        line_fmt = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_GREEN;
        break;

    case LOGLVL_DEBUG:
        line_fmt = FOREGROUND_GREEN;
        break;

    case LOGLVL_TRACE:
        line_fmt = FOREGROUND_INTENSITY;
        break;

    case LOGLVL_INFO:
        line_fmt =  FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;

    default:
        /* No color */
        line_fmt = 0;
        break;
    }

    flockfile(stdout);

    SetConsoleTextAttribute(std, line_fmt);

    fprintf(stdout, "%s %s [%s:%d] %s\n", time, level_str, fn, line, msg);

    SetConsoleTextAttribute(std, reset_fmt);

    fflush(stdout);
    funlockfile(stdout);
}
#endif

/* dumps size bytes of *data to stdout. Looks like:
 * [0000] 00 9A 06 00 78 F7 EB 00  4F 9E 03 00 08 00 00 00 ....x... O......
 * [0010] 00 00 00 00 0F 00 00 00                          ........
 */
void log_hex_dump(FILE *out, const void *data, size_t size)
{
    unsigned char *p = (unsigned char *)data;
    unsigned char c;
    size_t n;

    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16 * 3 + 5] = {0};
    char charstr[16 * 1 + 5] = {0};
    flockfile(out);

    for (n = 1; n <= size; n++) {
        if (n % 16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4lx",
                     (unsigned long)
                     ((size_t)p - (size_t)data));
        }

        c = *p;

        // if (isalnum(c) == 0)
        if (isprint(c) == 0) {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr) - strlen(hexstr) - 1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr) - strlen(charstr) - 1);

        if (n % 16 == 0) {
            /* line completed */
            fprintf(out, "[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;

        } else if (n % 8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr) - strlen(hexstr) - 1);
            strncat(charstr, " ", sizeof(charstr) - strlen(charstr) - 1);
        }

        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        fprintf(out, "[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
    }

    fflush(out);
    funlockfile(out);
}

/* dumps size bytes of *data to stdout. Looks like:
 *
 * +-------+--------------------------------------------------+-------------------+
 * | a \ o | 0  1  2  3  4  5  6  7   8  9  a  b  c  d  e  f  | ascii characters  |
 * +-------+--------------------------------------------------+-------------------+
 * | 00000 | 00 9A 06 00 78 F7 EB 00  4F 9E 03 00 08 00 00 00 | ....x... O....... |
 * | 00010 | 00 00 00 00 0F 00 00 00                          | ........          |
 * +-------+--------------------------------------------------+-------------------+
 */
void log_hex_dumph(FILE *out, const void *data, size_t size)
{
    unsigned char *p = (unsigned char *)data;
    unsigned char c;
    size_t n;

    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16 * 3 + 5] = {0};
    char charstr[16 * 1 + 5] = {0};
    flockfile(out);
    fprintf(out, "%s\n", "+-------+-------------------------"
            "-------------------------+-------------------+");
    fprintf(out, "%s\n", "| a \\ o | 0  1  2  3  4  5  6  7 "
            "  8  9  a  b  c  d  e  f  | ascii characters  |");
    fprintf(out, "%s\n", "+-------+-------------------------"
            "-------------------------+-------------------+");

    for (n = 1; n <= size; n++) {
        if (n % 16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.5lx",
                     (unsigned long)
                     ((size_t)p - (size_t)data));
        }

        c = *p;

        // if (isalnum(c) == 0)
        if (isprint(c) == 0) {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr) - strlen(hexstr) - 1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr) - strlen(charstr) - 1);

        if (n % 16 == 0) {
            /* line completed */
            fprintf(out, "| %5.5s | %-49.49s| %-17.17s |\n", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;

        } else if (n % 8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, " ", sizeof(hexstr) - strlen(hexstr) - 1);
            strncat(charstr, " ", sizeof(charstr) - strlen(charstr) - 1);
        }

        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        fprintf(out, "| %5.5s | %-49.49s| %-17.17s |\n", addrstr, hexstr, charstr);
    }

    fprintf(out, "%s\n", "+-------+-------------------------"
            "-------------------------+-------------------+");
    fflush(out);
    funlockfile(out);
}

/*
 * logging output to file
 */

static void logfile_destroy(void *log_file) {
    logfile_t *logfile = (logfile_t *)log_file;
    if (logfile) {
        if (logfile->fp) {
            fclose(logfile->fp);
        }

        free(logfile);
    }
}

static logfile_t *logfile_create(const char *folder, const char *prefix, int reserve) {
    logfile_t *logfile;
    size_t size;

    if ((logfile = (logfile_t *)malloc(sizeof(logfile_t))) == NULL) {
        return NULL;
    }

    memset(logfile, 0, sizeof(logfile_t));
    size = sizeof(logfile->dir);

    if (folder) {
        strcpy(logfile->dir, folder);
        mkdir(logfile->dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    } else {
        free(logfile);
        return NULL;
    }

    if (prefix) {
        strncpy(logfile->prefix, prefix, sizeof(logfile->prefix) - 1);
    }

    logfile->reserve = reserve > 1 ? reserve : 1;
    return logfile;
}

static void logfile_delete(logfile_t *logfile) {
    struct tm t;
    time_t time;
    int year, mouth, day, i;
    char filename[260];
    memset(&t, 0, sizeof(struct tm));
    sscanf(logfile->date, "%d-%02d-%02d", &year, &mouth, &day);
    t.tm_year = year - 1900;
    t.tm_mon = mouth - 1;
    t.tm_mday = day;
    t.tm_hour = 12;
    time = mktime(&t);
    time -= logfile->reserve * 24 * 60 * 60;

    for (i = 0; i < logfile->reserve; i++) {
        time_t tmp = time - i * 24 * 60 * 60;
        struct tm *_tm = localtime(&tmp);

        if (logfile->prefix[0])
            sprintf(filename, "%s/%s.%d-%02d-%02d.log", logfile->dir, logfile->prefix,
                    _tm->tm_year + 1900, _tm->tm_mon + 1, _tm->tm_mday);

        else
            sprintf(filename, "%s/%d-%02d-%02d.log", logfile->dir,
                    _tm->tm_year + 1900, _tm->tm_mon + 1, _tm->tm_mday);

        remove(filename);
    }
}

void logfile_output(void *log_file, const char *time,
                    loglevel_t level, const char *level_str,
                    const char *file, const char *fn,
                    int line, const char *msg) {
    char filepath[260];
    logfile_t *logfile = (logfile_t *)log_file;

    flockfile(stderr);        /* just for thread safe */
    if (strncmp(logfile->date, time, 10) != 0) {

        if (logfile->fp != NULL) {
            fclose(logfile->fp);
            logfile->fp = NULL;
        }

        strncpy(logfile->date, time, 10);

        if (logfile->prefix[0]) {
            snprintf(filepath, sizeof(filepath), "%s/%s.%s.log", logfile->dir, logfile->prefix, logfile->date);

        } else {
            snprintf(filepath, sizeof(filepath), "%s/%s.log", logfile->dir, logfile->date);
        }

        logfile->fp = fopen(filepath, "a+");
        logfile_delete(logfile);
    }
    funlockfile(stderr);

    if (logfile->fp == NULL) {
        return;
    }

    flockfile(logfile->fp);
    fprintf(logfile->fp, "%s %s (%s) %s:%d %s\n",
            time, level_str, file, fn, line, msg);
    fflush(logfile->fp);
    funlockfile(logfile->fp);
}

void log_output_file(loglevel_t level, const char *logfolder, const char *logprefix, int reserve) {
    logfile_t *logfile = logfile_create(logfolder, logprefix, reserve);
    if (logfile) {
        loghandler_t *handle;

        if ((handle = (loghandler_t *)malloc(sizeof(loghandler_t))) == 0) {
            free(logfile);
            return;
        }
        handle->type = 0xF12E;
        handle->level = level;
        handle->fn = logfile_output;
        handle->userdata = logfile;
        handle->destroy = logfile_destroy;
        handle->next = __log__.handle;
        __log__.handle = handle;
    }
}

const FILE *log_get_fp() {
    loghandler_t *handle = __log__.handle;
    while (handle) {
        if (handle->type == 0xF12E) {
            logfile_t *logfile = (logfile_t *)handle->userdata;
            return logfile->fp;
        }
        handle = handle->next;
    }
    return NULL;
}
