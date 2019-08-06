/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

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
