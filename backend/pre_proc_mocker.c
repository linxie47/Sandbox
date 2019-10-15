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

#include "pre_proc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void MockerPreProcConvert(PreProcContext *context, const Image *src, Image *dst, int bAllocateDestination) {
    assert(src->type == MEM_TYPE_SYSTEM);
    memcpy(dst, src, sizeof(*src));
}

static void MockerPreProcDestroy(PreProcContext *context) {
    // empty
}

static void MockerPreProcReleaseImage(PreProcContext *context, Image *image) {
    // empty
}

static Image MockerMap(ImageMapContext *context, const Image *image) {
    assert(image);
    return *image;
}

static void MockerUnmap(ImageMapContext *context) {
    // empty
}

ImageMap image_map_mocker = {
    .name = "mocker",
    .Map = MockerMap,
    .Unmap = MockerUnmap,
};

PreProc pre_proc_mocker = {
    .name = "mocker",
    .mem_type = MEM_TYPE_SYSTEM,
    .Convert = MockerPreProcConvert,
    .ReleaseImage = MockerPreProcReleaseImage,
    .Destroy = MockerPreProcDestroy,
};
