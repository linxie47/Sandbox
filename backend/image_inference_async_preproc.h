/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#pragma once

#include "image_inference.h"
#include "pre_proc.h"
#include "safe_queue.h"
#include <pthread.h>

typedef struct PreprocImage {
    Image image;
    ImageMapContext *img_map_ctx;
    IFramePtr user_data;        // Pass through to wrapped inference backend
    PreProcessor pre_processor; // Pass through to wrapped inference backend
} PreprocImage;

typedef struct ImageInferenceAsyncPreproc {
    ImageInferenceContext *actual;
    PreProcContext *pre_proc;

    PreprocImage **preproc_images;
    size_t num_preproc_images;

    // Threading
    pthread_t async_thread;
    SafeQueueT *freeImages;    // PreprocImage queue
    SafeQueueT *workingImages; // PreprocImage queue
} ImageInferenceAsyncPreproc;