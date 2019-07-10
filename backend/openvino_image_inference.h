/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

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

    CallbackFunc callback;

    // Inference Engine
    ie_plugin_t *plugin;
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
