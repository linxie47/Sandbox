/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "logger.h"
#include <stdio.h>
#include <string.h>

static VAIILogFuncPtr inference_log_function = default_log_function;

void set_log_function(VAIILogFuncPtr log_func) {
    inference_log_function = log_func;
}

void debug_log(int level, const char *file, const char *function, int line, const char *message) {
    (*inference_log_function)(level, file, function, line, message);
}

void default_log_function(int level, const char *file, const char *function, int line, const char *message) {
    const char log_level[][16] = {"DEFAULT", "ERROR", "WARNING", "INFO", "VERBOSE", "DEBUG", "TRACE", "MEMDUMP"};
    fprintf(stderr, "%s \t %s:%i : %s \t %s \n", (char *)&log_level[level], file, line, function, message);
}

static VAIITraceFuncPtr inference_trace_function = default_trace_function;

void set_trace_function(VAIITraceFuncPtr trace_func) {
    inference_trace_function = trace_func;
}

void trace_log(int level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    (*inference_trace_function)(level, fmt, args);
    va_end(args);
}

void default_trace_function(int level, const char *fmt, va_list vl) {
    const char log_level[][16] = {"DEFAULT", "ERROR", "WARNING", "INFO", "VERBOSE", "DEBUG", "TRACE", "MEMDUMP"};
    fprintf(stderr, "%s \t", (char *)&log_level[level]);
    vprintf(fmt, vl);
}