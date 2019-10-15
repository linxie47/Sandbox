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

#include <ie_c_api.h>
#include "image_inference.h"
#include "pre_proc.h"
#include "safe_queue.h"
#include <pthread.h>

typedef struct BatchRequest {
    infer_request_t *infer_request;
    UserDataBuffers buffers;
    // TODO: alloc_context
} BatchRequest;

typedef struct OpenVINOImageInference {
    int resize_by_inference;
    IEColorFormat ie_color_format;

    CallbackFunc callback;

    // Inference Engine
    ie_core_t *core;
    ie_network_t *network;
    ie_input_info_t **inputs;
    ie_output_info_t **outputs;
    size_t num_inputs;
    size_t num_outputs;
    char *model_name;
    infer_requests_t *infer_requests;

    BatchRequest **batch_requests;
    size_t num_batch_requests;

    // Threading
    int batch_size;
    pthread_t working_thread;
    SafeQueueT *freeRequests;    // BatchRequest queue
    SafeQueueT *workingRequests; // BatchRequest queue

    // VPP
    PreProcContext *vpp_ctx;

    int already_flushed;
    pthread_mutex_t flush_mutex;

} OpenVINOImageInference;

typedef struct OpenVINOOutputBlob {
    const char *name;
    ie_blob_t *blob;
} OpenVINOOutputBlob;
