/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#pragma once

enum {
    VAII_ERROR_LOG_LEVEL = 1,
    VAII_WARNING_LOG_LEVEL,
    VAII_FIXME_LOG_LEVEL,
    VAII_INFO_LOG_LEVEL,
    VAII_DEBUG_LEVEL,
    VAII_LOG_LOG_LEVEL,
    VAII_TRACE_LOG_LEVEL,
    VAII_MEMDUMP_LOG_LEVEL,
};

#define VAII_DEBUG_LOG(level, message) debug_log(level, __FILE__, __FUNCTION__, __LINE__, message);

#define VAII_MEMDUMP(message) VAII_DEBUG_LOG(VAII_MEMDUMP_LOG_LEVEL, message);
#define VAII_TRACE(message) VAII_DEBUG_LOG(VAII_TRACE_LOG_LEVEL, message);
#define VAII_LOG(message) VAII_DEBUG_LOG(VAII_LOG_LOG_LEVEL, message);
#define VAII_DEBUG(message) VAII_DEBUG_LOG(VAII_DEBUG_LEVEL, message);
#define VAII_INFO(message) VAII_DEBUG_LOG(VAII_INFO_LOG_LEVEL, message);
#define VAII_FIXME(message) VAII_DEBUG_LOG(VAII_FIXME_LOG_LEVEL, message);
#define VAII_WARNING(message) VAII_DEBUG_LOG(VAII_WARNING_LOG_LEVEL, message);
#define VAII_ERROR(message) VAII_DEBUG_LOG(VAII_ERROR_LOG_LEVEL, message);

typedef void (*VAIILogFuncPtr)(int level, const char *file, const char *function, int line, const char *message);

void set_log_function(VAIILogFuncPtr log_func);

void set_log_level(int level);

void debug_log(int level, const char *file, const char *function, int line, const char *message);

void default_log_function(int level, const char *file, const char *function, int line, const char *message);

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
