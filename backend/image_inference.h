/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#pragma once

#include "image.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    II_LAYOUT_ANY = 0,
    II_LAYOUT_NCHW = 1,
    II_LAYOUT_NHWC = 2,
} IILayout;

typedef enum {
    II_FP32 = 10,
    II_U8 = 40,
} IIPrecision;

/* Don't change this structure */
#define II_MAX_DIMENSIONS 8
typedef struct Dimensions {
    size_t num_dims;
    size_t dims[II_MAX_DIMENSIONS];
} Dimensions;

typedef void *IFramePtr;

typedef struct UserDataBuffers {
    IFramePtr *frames;
    int num_buffers;
} UserDataBuffers;

typedef struct OutputBlobMethod OutputBlobMethod;
typedef struct OutputBlobArray OutputBlobArray;
typedef struct OutputBlobContext OutputBlobContext;
typedef struct ImageInference ImageInference;
typedef struct ImageInferenceContext ImageInferenceContext;

/**
 * \brief Callback function when a inference request completed.
 * Image inference backend takes charge of memory management for @param Blobs and @param frames
 * Caller is responsible to every IFramePtr after reinterpreted as customed data structure
 */
typedef void (*CallbackFunc)(OutputBlobArray *Blobs, UserDataBuffers *frames);
typedef void (*PreProcessor)(Image *image);

struct ImageInference {
    /* image inference backend name. Must be non-NULL and unique among backends. */
    const char *name;

    /* create image inference engine */
    int (*Create)(ImageInferenceContext *ctx, MemoryType type, const char *devices, const char *model, int batch_size,
                  int nireq, const char *config, void *allocator, CallbackFunc callback);

    /* submit image */
    void (*SubmitImage)(ImageInferenceContext *ctx, const Image *image, IFramePtr user_data,
                        PreProcessor pre_processor);

    const char *(*GetModelName)(ImageInferenceContext *ctx);

    int (*IsQueueFull)(ImageInferenceContext *ctx);

    void (*Flush)(ImageInferenceContext *ctx);

    void (*Close)(ImageInferenceContext *ctx);

    int priv_size; ///< size of private data to allocate for the backend
};

struct ImageInferenceContext {
    const ImageInference *inference;
    const OutputBlobMethod *output_blob_method;
    char *name;
    void *priv;
};

struct OutputBlobMethod {
    /* output blob method name. Must be non-NULL and unique among output blob methods. */
    const char *name;

    const char *(*GetOutputLayerName)(OutputBlobContext *ctx);

    IILayout (*GetLayout)(OutputBlobContext *ctx);

    IIPrecision (*GetPrecision)(OutputBlobContext *ctx);

    Dimensions *(*GetDims)(OutputBlobContext *ctx);

    const void *(*GetData)(OutputBlobContext *ctx);

    int priv_size; ///< size of private data to allocate for the output blob
};

struct OutputBlobContext {
    const OutputBlobMethod *output_blob_method;
    void *priv;
};

struct OutputBlobArray {
    OutputBlobContext **output_blobs;
    int num_blobs;
};

#define __STRING(x) #x

#ifdef __cplusplus
#define __CONFIG_KEY(name) KEY_##name
#define __DECLARE_CONFIG_KEY(name) static const char *__CONFIG_KEY(name) = __STRING(name)
__DECLARE_CONFIG_KEY(CPU_EXTENSION);          // library with implementation of custom layers
__DECLARE_CONFIG_KEY(CPU_THREADS_NUM);        // threads number CPU plugin use for inference
__DECLARE_CONFIG_KEY(CPU_THROUGHPUT_STREAMS); // number inference requests running in parallel
__DECLARE_CONFIG_KEY(RESIZE_BY_INFERENCE);    // experimental, don't use
#else
#define KEY_CPU_EXTENSION __STRING(CPU_EXTENSION)                   // library with implementation of custom layers
#define KEY_CPU_THREADS_NUM __STRING(CPU_THREADS_NUM)               // threads number CPU plugin use for inference
#define KEY_CPU_THROUGHPUT_STREAMS __STRING(CPU_THROUGHPUT_STREAMS) // number inference requests running in parallel
#define KEY_RESIZE_BY_INFERENCE __STRING(RESIZE_BY_INFERENCE)       // experimental, don't use
#endif

const ImageInference *image_inference_get_by_name(const char *name);

ImageInferenceContext *image_inference_alloc(const ImageInference *infernce, const OutputBlobMethod *blob,
                                             const char *instance_name);

void image_inference_free(ImageInferenceContext *inference_context);

const OutputBlobMethod *output_blob_method_get_by_name(const char *name);

OutputBlobContext *output_blob_alloc(const OutputBlobMethod *method);

void output_blob_free(OutputBlobContext *context);

void image_inference_dynarray_add(void *tab_ptr, int *nb_ptr, void *elem);