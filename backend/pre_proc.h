/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#pragma once

#include "config.h"
#include "image.h"

typedef struct PreProcContext PreProcContext;

typedef struct PreProcInitParam {
    union {
        struct { // VAAPI
            void *va_display;
            int num_surfaces;
            int width;
            int height;
            int format; // FourCC
        };
        void *reserved; // Others
    };
} PreProcInitParam;

typedef struct PreProc {
    /* image pre processing module name. Must be non-NULL and unique among pre processing modules. */
    const char *name;

    int (*Init)(PreProcContext *context, PreProcInitParam *param);

    void (*Destroy)(PreProcContext *context);

    void (*Convert)(PreProcContext *context, const Image *src, Image *dst, int bAllocateDestination);

    // to be called if Convert called with bAllocateDestination = true
    void (*ReleaseImage)(PreProcContext *context, Image *dst);

    MemoryType mem_type;

    int priv_size; ///< size of private data to allocate for pre processing
} PreProc;

struct PreProcContext {
    const PreProc *pre_proc;
    void *priv;
};

int GetPlanesCount(int fourcc);

const PreProc *pre_proc_get_by_name(const char *name);

const PreProc *pre_proc_get_by_type(MemoryType type);

PreProcContext *pre_proc_alloc(const PreProc *pre_proc);

void pre_proc_free(PreProcContext *context);
