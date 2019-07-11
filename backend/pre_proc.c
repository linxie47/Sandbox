/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "pre_proc.h"
#include <stdlib.h>
#include <string.h>

extern PreProc pre_proc_ffmpeg;
extern PreProc pre_proc_opencv;
extern PreProc pre_proc_gapi;

static const PreProc *const pre_proc_list[] = {
#if HAVE_FFMPEG || CONFIG_SWSCALE
    &pre_proc_ffmpeg,
#endif
#if HAVE_OPENCV
    &pre_proc_opencv,
#endif
#if HAVE_GAPI
    &pre_proc_gapi,
#endif
    NULL};

int GetPlanesCount(int fourcc) {
    switch (fourcc) {
    case FOURCC_BGRA:
    case FOURCC_BGRX:
    case FOURCC_BGR:
    case FOURCC_RGBA:
    case FOURCC_RGBX:
        return 1;
    case FOURCC_NV12:
        return 2;
    case FOURCC_BGRP:
    case FOURCC_RGBP:
    case FOURCC_I420:
        return 3;
    }

    return 0;
}
static const PreProc *pre_proc_iterate(void **opaque) {
    uintptr_t i = (uintptr_t)*opaque;
    const PreProc *pp = pre_proc_list[i];

    if (pp != NULL)
        *opaque = (void *)(i + 1);

    return pp;
}

const PreProc *pre_proc_get_by_name(const char *name) {
    const PreProc *pp = NULL;
    void *opaque = 0;

    if (name == NULL)
        return NULL;

    while ((pp = pre_proc_iterate(&opaque)))
        if (!strcmp(pp->name, name))
            return pp;

    return NULL;
}

const PreProc *pre_proc_get_by_type(MemoryType type) {
    const PreProc *ret = NULL;

    if (type == MEM_TYPE_SYSTEM) {
        ret = pre_proc_get_by_name("ffmpeg");
        if (!ret)
            ret = pre_proc_get_by_name("gapi");
        if (!ret)
            ret = pre_proc_get_by_name("opencv");
    } else if (type == MEM_TYPE_VAAPI) {
        ret = pre_proc_get_by_name("vaapi");
    }

    return ret;
}

PreProcContext *pre_proc_alloc(const PreProc *pre_proc) {
    PreProcContext *ret;

    if (pre_proc == NULL)
        return NULL;

    ret = (PreProcContext *)malloc(sizeof(*ret));
    if (!ret)
        return NULL;
    memset(ret, 0, sizeof(*ret));

    ret->pre_proc = pre_proc;
    if (pre_proc->priv_size > 0) {
        ret->priv = malloc(pre_proc->priv_size);
        if (!ret->priv)
            goto err;
        memset(ret->priv, 0, pre_proc->priv_size);
    }

    return ret;
err:
    free(ret->priv);
    free(ret);
    return NULL;
}

void pre_proc_free(PreProcContext *context) {
    if (context == NULL)
        return;

    if (context->priv)
        free(context->priv);
    free(context);
}
