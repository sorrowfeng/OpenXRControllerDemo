/*
 * Copyright 2024 - 2024 PICO. All rights reserved.
 *
 * NOTICE: All information contained herein is, and remains the property of PICO.
 * The intellectual and technical concepts contained herein are proprietary to PICO.
 * and may be covered by patents, patents in process, and are protected by trade
 * secret or copyright law. Dissemination of this information or reproduction of
 * this material is strictly forbidden unless prior written permission is obtained
 * from PICO.
 */

#ifndef PICONATIVEOPENXRSAMPLES_LOGUTILS_H
#define PICONATIVEOPENXRSAMPLES_LOGUTILS_H

#include <android/log.h>
#include <stdarg.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 *  SYSTEM LOG LEVEL CONFIGURATION INSTRUCTIONS:
 *
 *  3: error
 *  4: error, warn
 *  5: error, warn, info
 *  6: error, warn, info, debug
 *  7: error, warn, info, debug, verbose
 */
enum PLOG_LEVEL {
    PLOG_SILENT = 1,  // log off
    PLOG_FATAL = 2,
    PLOG_ERROR = 3,    //!< Error messages: indicating a problem
    PLOG_WARN = 4,     //!< Warning messages: indicating a potential problem
    PLOG_INFO = 5,     //!< Info messages: not very verbose, not indicating a problem.
    PLOG_DEBUG = 6,    //!< Debug messages, verbose.
    PLOG_VERBOSE = 7,  //!< Trace messages, highly verbose.
    PLOG_RAW = 8
};

static inline bool validLogLevel(enum PLOG_LEVEL logLevel, enum PLOG_LEVEL minLogLevel) {
    if (minLogLevel == PLOG_SILENT) {
        // close down all log
        return false;
    } else if (logLevel == PLOG_RAW) {
        // ignore min_log_level
        return true;
    } else if (logLevel <= minLogLevel) {
        // the number is bigger, the level is lower
        return true;
    }
    return false;
}

static android_LogPriority pxrlog_convert_priority(enum PLOG_LEVEL level) {
    switch (level) {
    case PLOG_VERBOSE:
        return ANDROID_LOG_VERBOSE;
    case PLOG_INFO:
        return ANDROID_LOG_INFO;
    case PLOG_DEBUG:
        return ANDROID_LOG_DEBUG;
    case PLOG_WARN:
        return ANDROID_LOG_WARN;
    case PLOG_ERROR:
        return ANDROID_LOG_ERROR;
    case PLOG_FATAL:
        return ANDROID_LOG_FATAL;
    case PLOG_RAW:
        return ANDROID_LOG_INFO;
    case PLOG_SILENT:
    default:
        return ANDROID_LOG_SILENT;
    }
}

typedef struct PxrLogger {
    enum PLOG_LEVEL min_log_level;
    const char* tag;
} PxrLogger;

//#define DEBUG_SWAN_90HZ

#define TAG(tag) #tag
#define TAG_(tag) TAG(tag)
#define TAG_STR TAG_(LOGTAG)

#ifndef LOGTAG
#define LOGTAG PXRSampleFramework
#define USE_DEFAULT_TAG
#endif

#define MIN_LOG_LEVEL_VAR(tag) minLogLevel_##tag
#define GET_LOG_LEVEL_VAR(tag) MIN_LOG_LEVEL_VAR(tag)
#define MIN_MODULE_LOG_LEVEL GET_LOG_LEVEL_VAR(LOGTAG)

#ifdef USE_DEFAULT_TAG
static int MIN_MODULE_LOG_LEVEL = PLOG_INFO;
#else
extern int MIN_MODULE_LOG_LEVEL;
#endif

#define INIT_LOG_LEVEL(level) int MIN_MODULE_LOG_LEVEL = level
static void SET_MODULE_LOG_LEVEL(enum PLOG_LEVEL level) {
    MIN_MODULE_LOG_LEVEL = level;
}
static enum PLOG_LEVEL GET_LOG_LEVEL() {
    return (enum PLOG_LEVEL)MIN_MODULE_LOG_LEVEL;
}

static inline void log_print(const PxrLogger logArgs, enum PLOG_LEVEL level, const char* format, va_list args) {
    if (!validLogLevel(level, logArgs.min_log_level)) {
        return;
    }

    android_LogPriority prio = pxrlog_convert_priority(level);
    __android_log_vprint(prio, logArgs.tag, format, args);
}

static inline void log_default(enum PLOG_LEVEL logLevel, const char* format, ...) {
    PxrLogger logArgs = {(enum PLOG_LEVEL)MIN_MODULE_LOG_LEVEL, TAG_STR};

    va_list args;
    va_start(args, format);
    log_print(logArgs, logLevel, format, args);
    va_end(args);
}

#define PLOGV(format, ...) log_default(PLOG_VERBOSE, format, ##__VA_ARGS__)
#define PLOGD(format, ...) log_default(PLOG_DEBUG, format, ##__VA_ARGS__)
#define PLOGI(format, ...) log_default(PLOG_INFO, format, ##__VA_ARGS__)
#define PLOGW(format, ...) log_default(PLOG_WARN, format, ##__VA_ARGS__)
#define PLOGE(format, ...) log_default(PLOG_ERROR, format, ##__VA_ARGS__)
#define PLOGF(format, ...) log_default(PLOG_FATAL, format, ##__VA_ARGS__)
#define PLOGR(format, ...) log_default(PLOG_RAW, format, ##__VA_ARGS__)

static pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
static inline void log_with_tag(const char* tag, enum PLOG_LEVEL logLevel, const char* format, ...) {
    PxrLogger logArgs = {(enum PLOG_LEVEL)MIN_MODULE_LOG_LEVEL, tag};

    va_list args;
    va_start(args, format);
    log_print(logArgs, logLevel, format, args);
    va_end(args);
}

#define PLOGVT(tag, format, ...) log_with_tag(tag, PLOG_VERBOSE, format, ##__VA_ARGS__)
#define PLOGDT(tag, format, ...) log_with_tag(tag, PLOG_DEBUG, format, ##__VA_ARGS__)
#define PLOGIT(tag, format, ...) log_with_tag(tag, PLOG_INFO, format, ##__VA_ARGS__)
#define PLOGWT(tag, format, ...) log_with_tag(tag, PLOG_WARN, format, ##__VA_ARGS__)
#define PLOGET(tag, format, ...) log_with_tag(tag, PLOG_ERROR, format, ##__VA_ARGS__)
#define PLOGFT(tag, format, ...) log_with_tag(tag, PLOG_FATAL, format, ##__VA_ARGS__)
#define PLOGRT(tag, format, ...) log_with_tag(tag, PLOG_RAW, format, ##__VA_ARGS__)

static inline void log_with_level(enum PLOG_LEVEL minLevel, enum PLOG_LEVEL logLevel, const char* format, ...) {
    PxrLogger logArgs = {minLevel, TAG_STR};

    va_list args;
    va_start(args, format);
    log_print(logArgs, logLevel, format, args);
    va_end(args);
}

#define PLOGVL(minLevel, format, ...) log_with_level(minLevel, PLOG_VERBOSE, format, ##__VA_ARGS__)
#define PLOGDL(minLevel, format, ...) log_with_level(minLevel, PLOG_DEBUG, format, ##__VA_ARGS__)
#define PLOGIL(minLevel, format, ...) log_with_level(minLevel, PLOG_INFO, format, ##__VA_ARGS__)
#define PLOGWL(minLevel, format, ...) log_with_level(minLevel, PLOG_WARN, format, ##__VA_ARGS__)
#define PLOGEL(minLevel, format, ...) log_with_level(minLevel, PLOG_ERROR, format, ##__VA_ARGS__)
#define PLOGFL(minLevel, format, ...) log_with_level(minLevel, PLOG_FATAL, format, ##__VA_ARGS__)
#define PLOGRL(minLevel, format, ...) log_with_level(minLevel, PLOG_RAW, format, ##__VA_ARGS__)

static inline void log_with_arg(const PxrLogger* logger, enum PLOG_LEVEL logLevel, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_print(*logger, logLevel, format, args);
    va_end(args);
}

#define REGISTELOGGER(name, tag, minLevel)                    \
    static PxrLogger __tmp_##name = PxrLogger{minLevel, tag}; \
    static PxrLogger* name = &__tmp_##name;
static inline void SET_LOG_LEVEL(PxrLogger* logger, enum PLOG_LEVEL level) {
    logger->min_log_level = level;
}

#define PLOGVA(logger, format, ...) log_with_arg(logger, PLOG_VERBOSE, format, ##__VA_ARGS__)
#define PLOGDA(logger, format, ...) log_with_arg(logger, PLOG_DEBUG, format, ##__VA_ARGS__)
#define PLOGIA(logger, format, ...) log_with_arg(logger, PLOG_INFO, format, ##__VA_ARGS__)
#define PLOGWA(logger, format, ...) log_with_arg(logger, PLOG_WARN, format, ##__VA_ARGS__)
#define PLOGEA(logger, format, ...) log_with_arg(logger, PLOG_ERROR, format, ##__VA_ARGS__)
#define PLOGFA(logger, format, ...) log_with_arg(logger, PLOG_FATAL, format, ##__VA_ARGS__)
#define PLOGRA(logger, format, ...) log_with_arg(logger, PLOG_RAW, format, ##__VA_ARGS__)

static PxrLogger rawLogger = {PLOG_RAW, TAG_STR};
#define PLOGRV(format, ...) PLOGVA(&rawLogger, format, ##__VA_ARGS__)
#define PLOGRD(format, ...) PLOGDA(&rawLogger, format, ##__VA_ARGS__)
#define PLOGRI(format, ...) PLOGIA(&rawLogger, format, ##__VA_ARGS__)
#define PLOGRW(format, ...) PLOGWA(&rawLogger, format, ##__VA_ARGS__)
#define PLOGRE(format, ...) PLOGEA(&rawLogger, format, ##__VA_ARGS__)
#define PLOGRF(format, ...) PLOGFA(&rawLogger, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif  //PICONATIVEOPENXRSAMPLES_LOGUTILS_H
