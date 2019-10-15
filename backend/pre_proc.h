/*
 * Copyright (c) 2018-2019 Intel Corporation
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

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
