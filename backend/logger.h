/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#pragma once

#include <stdarg.h>

enum {
    VAII_ERROR_LOG_LEVEL = 1,
    VAII_WARNING_LOG_LEVEL,
    VAII_INFO_LOG_LEVEL,
    VAII_VERBOSE_LOG_LEVEL,
    VAII_DEBUG_LOG_LEVEL,
    VAII_TRACE_LOG_LEVEL,
    VAII_MEMDUMP_LOG_LEVEL,
};

#define VAII_DEBUG_LOG(level, message) debug_log(level, __FILE__, __FUNCTION__, __LINE__, message);

#define VAII_MEMDUMP(message) VAII_DEBUG_LOG(VAII_MEMDUMP_LOG_LEVEL, message);
#define VAII_TRACE(message) VAII_DEBUG_LOG(VAII_TRACE_LOG_LEVEL, message);
#define VAII_DEBUG(message) VAII_DEBUG_LOG(VAII_DEBUG_LOG_LEVEL, message);
#define VAII_INFO(message) VAII_DEBUG_LOG(VAII_INFO_LOG_LEVEL, message);
#define VAII_FIXME(message) VAII_DEBUG_LOG(VAII_FIXME_LOG_LEVEL, message);
#define VAII_WARNING(message) VAII_DEBUG_LOG(VAII_WARNING_LOG_LEVEL, message);
#define VAII_ERROR(message) VAII_DEBUG_LOG(VAII_ERROR_LOG_LEVEL, message);

#define VAII_LOGE(f_, ...) trace_log(VAII_ERROR_LOG_LEVEL, (f_), ##__VA_ARGS__);
#define VAII_LOGW(f_, ...) trace_log(VAII_WARNING_LOG_LEVEL, (f_), ##__VA_ARGS__);
#define VAII_LOGI(f_, ...) trace_log(VAII_INFO_LOG_LEVEL, (f_), ##__VA_ARGS__);
#define VAII_LOGV(f_, ...) trace_log(VAII_VERBOSE_LOG_LEVEL, (f_), ##__VA_ARGS__);
#define VAII_LOGD(f_, ...) trace_log(VAII_DEBUG_LOG_LEVEL, (f_), ##__VA_ARGS__);

typedef void (*VAIILogFuncPtr)(int level, const char *file, const char *function, int line, const char *message);

void set_log_function(VAIILogFuncPtr log_func);

void debug_log(int level, const char *file, const char *function, int line, const char *message);

void default_log_function(int level, const char *file, const char *function, int line, const char *message);

typedef void (*VAIITraceFuncPtr)(int, const char *, va_list);

void set_trace_function(VAIITraceFuncPtr trace_func);

void trace_log(int level, const char *fmt, ...);

void default_trace_function(int level, const char *fmt, va_list vl);

#if defined(HAVE_ITT)
#include "ittnotify.h"
#include <string>

static __itt_domain *itt_domain = NULL;
inline void taskBegin(const char *name) {
    if (itt_domain == NULL) {
        itt_domain = __itt_domain_create("video-analytics");
    }
    __itt_task_begin(itt_domain, __itt_null, __itt_null, __itt_string_handle_create(name));
}

inline void taskEnd(void) {
    __itt_task_end(itt_domain);
}

#else

#define taskBegin(x)
#define taskEnd(x)

#endif
