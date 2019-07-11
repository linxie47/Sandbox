/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "logger.h"
#include <stdio.h>
#include <string.h>

static VAIILogFuncPtr inference_log_function = NULL;
static int inference_log_level = VAII_INFO_LOG_LEVEL;

void set_log_function(VAIILogFuncPtr log_func) {
    inference_log_function = log_func;
};

void set_log_level(int level) {
    inference_log_level = level;
};

void debug_log(int level, const char *file, const char *function, int line, const char *message) {
    if (!inference_log_function) {
        set_log_function(default_log_function);
    }
    (*inference_log_function)(level, file, function, line, message);
};

void default_log_function(int level, const char *file, const char *function, int line, const char *message) {
    const char log_level[][16] = {"DEFAULT", "ERROR", "WARNING", "FIXME", "INFO", "DEBUG", "LOG", "TRACE", "MEMDUMP"};
    fprintf(stderr, "%s \t %s:%i : %s \t %s \n", (char *)&log_level[level], file, line, function, message);
}
