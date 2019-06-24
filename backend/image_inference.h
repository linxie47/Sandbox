/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#pragma once

#include "image.h"
#include <stdio.h>
#include <stdlib.h>

typedef void (*CallbackFunc)(void *Blobs, void *IFrames);

typedef void (*PreProcessor)(Image *image);

typedef struct ImageInferenceContext ImageInferenceContext;

typedef struct ImageInference {
    /* image inference backend name. Must be non-NULL and unique among backends.
     */
    const char *name;

    /* create image inference engine */
    int (*Create)(ImageInferenceContext *ctx, MemoryType type, const char *devices, const char *model, int batch_size,
                  int nireq, void *config, void *allocator, CallbackFunc callback);

    /* submit image */
    void (*SubmitImage)(ImageInferenceContext *ctx, const Image *image, void *user_data, PreProcessor *pre_processor);

    const char (*GetModelName)(ImageInferenceContext *ctx);

    int (*IsQueueFull)(ImageInferenceContext *ctx);

    void (*Flush)(ImageInferenceContext *ctx);

    void (*Close)(ImageInferenceContext *ctx);

    int priv_size; ///< size of private data to allocate for the backend
} ImageInference;

struct ImageInferenceContext {
    const ImageInference *inference;
    char *name;
    void *priv;
};

ImageInferenceContext *image_inference_alloc(ImageInference *infernce, const char *instance_name);

typedef enum {
    II_LAYOUT_ANY = 0,
    II_LAYOUT_NCHW = 1,
    II_LAYOUT_NHWC = 2,
} IILayout;

typedef enum {
    II_FP32 = 10,
    II_U8 = 40,
} IIPrecision;

typedef struct OutputBlob {
    IILayout layout;
    IIPrecision precision;
    void *data;
} OutputBlob;

#if 0
#define __CONFIG_KEY(name) KEY_##name
#define __DECLARE_CONFIG_KEY(name) static const char *__CONFIG_KEY(name) = #name
__DECLARE_CONFIG_KEY(CPU_EXTENSION);          // library with implementation of custom layers
__DECLARE_CONFIG_KEY(CPU_THROUGHPUT_STREAMS); // number inference requests running in parallel
__DECLARE_CONFIG_KEY(RESIZE_BY_INFERENCE);    // experimental, don't use
#endif
