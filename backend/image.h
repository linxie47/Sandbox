/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#pragma once

#include <stdint.h>

typedef enum MemoryType { MEM_TYPE_ANY = 0, MEM_TYPE_SYSTEM = 1, MEM_TYPE_OPENCL = 2, MEM_TYPE_VAAPI = 3 } MemoryType;

typedef enum FourCC {
    FOURCC_NV12 = 0x3231564E,
    FOURCC_BGRA = 0x41524742,
    FOURCC_BGRX = 0x58524742,
    FOURCC_BGRP = 0x50524742,
    FOURCC_BGR = 0x00524742,
    FOURCC_RGBA = 0x41424752,
    FOURCC_RGBX = 0x58424752,
    FOURCC_RGBP = 0x50424752,
    FOURCC_RGBP_F32 = 0x07282024,
    FOURCC_I420 = 0x30323449,
} FourCC;

typedef struct Rectangle {
    int x;
    int y;
    int width;
    int height;
} Rectangle;

#define MAX_PLANES_NUMBER 4

typedef struct Image {
    MemoryType type;
    union {
        uint8_t *planes[MAX_PLANES_NUMBER]; // if type==SYSTEM
        void *cl_mem;                       // if type==OPENCL
    };
    int format; // FourCC
    int width;
    int height;
    int stride[MAX_PLANES_NUMBER];
    Rectangle rect;
} Image;
