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

#include <assert.h>
#include <string.h>

#include "image_inference.h"
#include "logger.h"
#include "openvino_image_inference.h"

#define II_MAX(a, b) ((a) > (b) ? (a) : (b))
#define II_MIN(a, b) ((a) > (b) ? (b) : (a))

typedef enum { VPP_DEVICE_HW, VPP_DEVICE_SW } DEVICE_TYPE;

static void *WorkingFunction(void *arg);

static inline int getNumberChannels(int format) {
    switch (format) {
    case FOURCC_BGRA:
    case FOURCC_BGRX:
    case FOURCC_RGBA:
    case FOURCC_RGBX:
        return 4;
    case FOURCC_BGR:
        return 3;
    }
    return 0;
}

static IEColorFormat FormatNameToIEColorFormat(const char *format) {
    static const char *formats[] = {"NV12", "RGB", "BGR", "RGBX", "BGRX", "RGBA", "BGRA"};
    const IEColorFormat ie_color_formats[] = {NV12, RGB, BGR, RGBX, BGRX, RGBX, BGRX};

    int num_formats = sizeof(formats) / sizeof(formats[0]);
    for (int i = 0; i < num_formats; i++) {
        if (!strcmp(format, formats[i]))
            return ie_color_formats[i];
    }

    VAII_ERROR("Unsupported color format by Inference Engine preprocessing");
    return RAW;
}

static inline void RectToIERoi(roi_t *roi, const Rectangle *rect) {
    roi->id = 0;
    roi->posX = rect->x;
    roi->posY = rect->y;
    roi->sizeX = rect->width;
    roi->sizeY = rect->height;
}

static void GetNextImageBuffer(ImageInferenceContext *ctx, const BatchRequest *request, Image *image) {
    OpenVINOImageInference *vino = (OpenVINOImageInference *)ctx->priv;
    const char *input_name;
    dimensions_t blob_dims = {};
    int batchIndex, plane_size;

    VAII_DEBUG(__FUNCTION__);
    assert(vino->num_inputs != 0);

    input_name = vino->inputs[0]->name; // assuming one input layer
    infer_request_get_blob_dims(request->infer_request, input_name, &blob_dims);

    memset(image, 0, sizeof(*image));
    image->width = blob_dims.dims[3];  // W
    image->height = blob_dims.dims[2]; // H
    image->format = FOURCC_RGBP;
    batchIndex = request->buffers.num_buffers;
    plane_size = image->width * image->height;
    image->planes[0] = (uint8_t *)infer_request_get_blob_data(request->infer_request, input_name) +
                       batchIndex * plane_size * blob_dims.dims[1];
    image->planes[1] = image->planes[0] + plane_size;
    image->planes[2] = image->planes[1] + plane_size;
    image->stride[0] = image->width;
    image->stride[1] = image->width;
    image->stride[2] = image->width;
}

static inline Image ApplyCrop(const Image *src) {
    int planes_count;
    int rect_x, rect_y, rect_width, rect_height;
    Image dst = *src;
    dst.rect = (Rectangle){0};

    VAII_DEBUG(__FUNCTION__);

    planes_count = GetPlanesCount(src->format);
    if (!src->rect.width && !src->rect.height) {
        dst = *src;
        for (int i = 0; i < planes_count; i++)
            dst.planes[i] = src->planes[i];
        return dst;
    }

    if (src->rect.x >= src->width || src->rect.y >= src->height || src->rect.x + src->rect.width <= 0 ||
        src->rect.y + src->rect.height <= 0) {
        fprintf(stderr, "ERROR: ApplyCrop: Requested rectangle is out of image boundaries\n");
        assert(0);
    }

    rect_x = II_MAX(src->rect.x, 0);
    rect_y = II_MAX(src->rect.y, 0);
    rect_width = II_MIN(src->rect.width - (rect_x - src->rect.x), src->width - rect_x);
    rect_height = II_MIN(src->rect.height - (rect_y - src->rect.y), src->height - rect_y);

    switch (src->format) {
    case FOURCC_NV12: {
        dst.planes[0] = src->planes[0] + rect_y * src->stride[0] + rect_x;
        dst.planes[1] = src->planes[1] + (rect_y / 2) * src->stride[1] + rect_x;
        break;
    }
    case FOURCC_I420: {
        dst.planes[0] = src->planes[0] + rect_y * src->stride[0] + rect_x;
        dst.planes[1] = src->planes[1] + (rect_y / 2) * src->stride[1] + (rect_x / 2);
        dst.planes[2] = src->planes[2] + (rect_y / 2) * src->stride[2] + (rect_x / 2);
        break;
    }
    case FOURCC_RGBP: {
        dst.planes[0] = src->planes[0] + rect_y * src->stride[0] + rect_x;
        dst.planes[1] = src->planes[1] + rect_y * src->stride[1] + rect_x;
        dst.planes[2] = src->planes[2] + rect_y * src->stride[2] + rect_x;
        break;
    }
    case FOURCC_BGR: {
        int channels = 3;
        dst.planes[0] = src->planes[0] + rect_y * src->stride[0] + rect_x * channels;
        break;
    }
    default: {
        int channels = 4;
        dst.planes[0] = src->planes[0] + rect_y * src->stride[0] + rect_x * channels;
        break;
    }
    }

    if (rect_width)
        dst.width = rect_width;
    if (rect_height)
        dst.height = rect_height;

    return dst;
}

static void SubmitImagePreProcess(ImageInferenceContext *ctx, const BatchRequest *request, const Image *pSrc,
                                  PreProcessor preProcessor) {
    OpenVINOImageInference *vino = (OpenVINOImageInference *)ctx->priv;

    if (vino->resize_by_inference) {
        const char *input_name = vino->inputs[0]->name;
        // ie preprocess can only support system memory right now
        assert(pSrc->type == MEM_TYPE_SYSTEM);
        if (pSrc->format != FOURCC_NV12) {
            roi_t roi, *_roi = NULL;
            if (pSrc->rect.width != 0 && pSrc->rect.height != 0) {
                RectToIERoi(&roi, &pSrc->rect);
                _roi = &roi;
            }
            infer_request_set_blob(request->infer_request, input_name, pSrc->width, pSrc->height, vino->ie_color_format,
                                   (uint8_t **)pSrc->planes, _roi);
        } else {
            Image src = {};
            src = ApplyCrop(pSrc);
            infer_request_set_blob(request->infer_request, input_name, src.width, src.height, vino->ie_color_format,
                                   src.planes, NULL);
        }
    } else {
        Image src = {};
        Image dst = {};

        dst.type = pSrc->type;
        GetNextImageBuffer(ctx, request, &dst);

        if (pSrc->planes[0] != dst.planes[0]) { // only convert if different buffers
            if (!vino->vpp_ctx) {
                vino->vpp_ctx = pre_proc_alloc(pre_proc_get_by_type(MEM_TYPE_SYSTEM));
                assert(vino->vpp_ctx);
            }
#ifdef HAVE_GAPI
            vino->vpp_ctx->pre_proc->Convert(vino->vpp_ctx, &src, &dst, 0);
#else
            if (pSrc->type == MEM_TYPE_SYSTEM)
                src = ApplyCrop(pSrc);
            else
                src = *pSrc;
            vino->vpp_ctx->pre_proc->Convert(vino->vpp_ctx, &src, &dst, 0);
#endif
            // model specific pre-processing
            if (preProcessor)
                preProcessor(&dst);
        }
    }
}

static int OpenVINOImageInferenceCreate(ImageInferenceContext *ctx, MemoryType type, const char *devices,
                                        const char *model, int batch_size, int nireq, const char *configs,
                                        void *allocator, CallbackFunc callback) {
    int ret = 0, cpu_extension_needed = 0;
    OpenVINOImageInference *vino = (OpenVINOImageInference *)ctx->priv;
    VAII_DEBUG("Create");

    if (!model || !devices) {
        VAII_ERROR("No model or device!");
        return -1;
    }

    if (!callback) {
        VAII_ERROR("Callback function is not assigned!");
        return -1;
    }

    vino->core = ie_core_create();
    if (!vino->core) {
        VAII_ERROR("Create ie core failed!");
        return -1;
    }

    if (configs) {
        const char *pre_processor_name = NULL, *multi_device_list = NULL, *hetero_device_list = NULL;

        ie_core_set_config(vino->core, configs, devices);
        pre_processor_name = ie_core_get_config(vino->core, KEY_PRE_PROCESSOR_TYPE);
        vino->resize_by_inference = (pre_processor_name && !strcmp(pre_processor_name, "ie")) ? 1 : 0;

        multi_device_list = ie_core_get_config(vino->core, "MULTI_DEVICE_PRIORITIES");
        hetero_device_list = ie_core_get_config(vino->core, "TARGET_FALLBACK");
        if (multi_device_list && strstr(multi_device_list, "CPU") ||
            hetero_device_list && strstr(hetero_device_list, "CPU")) {
            cpu_extension_needed = 1;
        }
    }

    // Extension for custom layers
    if (cpu_extension_needed || strstr(devices, "CPU")) {
        const char *cpu_ext = ie_core_get_config(vino->core, KEY_CPU_EXTENSION);
        ie_core_add_extension(vino->core, cpu_ext, "CPU");
        VAII_DEBUG("Cpu extension loaded!");
    }

    // Load network
    vino->network = ie_network_create(vino->core, model, NULL);
    if (!vino->network) {
        VAII_ERROR("Create network failed!");
        goto err;
    }

    vino->batch_size = batch_size;

    // Check model input
    vino->num_inputs = ie_network_get_input_number(vino->network);
    vino->num_outputs = ie_network_get_output_number(vino->network);
    if (vino->num_inputs == 0) {
        VAII_ERROR("Input layer not found!");
        goto err;
    }

    vino->inputs = (ie_input_info_t **)malloc(vino->num_inputs * sizeof(*vino->inputs));
    vino->outputs = (ie_output_info_t **)malloc(vino->num_outputs * sizeof(*vino->outputs));
    if (!vino->inputs || !vino->outputs) {
        VAII_ERROR("Alloc in/outputs ptr failed!");
        goto err;
    }
    memset(vino->inputs,  0, vino->num_inputs *  sizeof(*vino->inputs));
    memset(vino->outputs, 0, vino->num_outputs * sizeof(*vino->outputs));

    for (size_t i = 0; i < vino->num_inputs; i++) {
        vino->inputs[i] = (ie_input_info_t *)malloc(sizeof(*vino->inputs[i]));
        if (!vino->inputs[i])
            goto err;
        memset(vino->inputs[i], 0, sizeof(*vino->inputs[i]));
    }
    for (size_t i = 0; i < vino->num_outputs; i++) {
        vino->outputs[i] = (ie_output_info_t *)malloc(sizeof(*vino->outputs[i]));
        if (!vino->outputs[i])
            goto err;
        memset(vino->outputs[i], 0, sizeof(*vino->outputs[i]));
    }

    ie_network_get_all_inputs(vino->network, vino->inputs);

    if (batch_size > 1) {
        for (int i = 0; i < vino->num_inputs; i++)
            ie_network_input_reshape(vino->network, vino->inputs[i], batch_size);
    }

    ie_network_get_all_outputs(vino->network, vino->outputs);

    ie_input_info_set_precision(vino->inputs[0], "U8");
    ie_input_info_set_layout(vino->inputs[0], "NCHW");
    if (vino->resize_by_inference) {
        const char *image_format_name = ie_core_get_config(vino->core, KEY_IMAGE_FORMAT);
        if (image_format_name == NULL) {
            VAII_ERROR("Input image format name must be specified!");
            goto err;
        }
        vino->ie_color_format = FormatNameToIEColorFormat(image_format_name);
        ie_input_info_set_preprocess(vino->inputs[0], vino->network, RESIZE_BILINEAR, vino->ie_color_format);
    }

    // Create infer requests
    if (nireq == 0) {
        VAII_ERROR("Input layer not found!");
        goto err;
    }

    vino->infer_requests = ie_network_create_infer_requests(vino->network, nireq, devices);
    if (!vino->infer_requests) {
        VAII_ERROR("Creat infer requests failed!");
        goto err;
    }

    vino->batch_requests = (BatchRequest **)malloc(nireq * sizeof(*vino->batch_requests));
    if (!vino->batch_requests) {
        VAII_ERROR("Creat batch requests failed!");
        goto err;
    }
    vino->num_batch_requests = nireq;

    vino->freeRequests = SafeQueueCreate();
    vino->workingRequests = SafeQueueCreate();
    if (!vino->freeRequests || !vino->workingRequests) {
        VAII_ERROR("Creat request queues failed!");
        goto err;
    }

    for (size_t n = 0; n < vino->infer_requests->num_reqs; n++) {
        BatchRequest *batch_request = (BatchRequest *)malloc(sizeof(*batch_request));
        if (!batch_request)
            goto err;
        memset(batch_request, 0, sizeof(*batch_request));
        batch_request->infer_request = vino->infer_requests->requests[n];
        vino->batch_requests[n] = batch_request;
        SafeQueuePush(vino->freeRequests, batch_request);
    }

    // TODO: handle allocator

    vino->model_name = strdup(model);
    if (!vino->model_name) {
        VAII_ERROR("Copy model name failed!");
        goto err;
    }

    vino->callback = callback;
    ret = pthread_create(&vino->working_thread, NULL, WorkingFunction, ctx);
    if (ret != 0) {
        VAII_ERROR("Create thread error!");
        goto err;
    }

    pthread_mutex_init(&vino->flush_mutex, NULL);

    return 0;
err:
    if (vino->inputs) {
        for (size_t i = 0; i < vino->num_inputs; i++)
            if (vino->inputs[i])
                free(vino->inputs[i]);
        free(vino->inputs);
    }
    if (vino->outputs) {
        for (size_t i = 0; i < vino->num_outputs; i++)
            if (vino->outputs[i])
                free(vino->outputs[i]);
        free(vino->outputs);
    }
    if (vino->batch_requests) {
        for (size_t i = 0; i < vino->num_batch_requests; i++)
            if (vino->batch_requests[i])
                free(vino->batch_requests[i]);
        free(vino->batch_requests);
    }
    if (vino->freeRequests)
        SafeQueueDestroy(vino->freeRequests);
    if (vino->workingRequests)
        SafeQueueDestroy(vino->workingRequests);
    ie_network_destroy(vino->network);
    ie_core_destroy(vino->core);
    return -1;
}

static void OpenVINOImageInferenceSubmtImage(ImageInferenceContext *ctx, const Image *image, IFramePtr user_data,
                                             PreProcessor pre_processor) {
    OpenVINOImageInference *vino = (OpenVINOImageInference *)ctx->priv;
    const Image *pSrc = image;
    BatchRequest *request = NULL;

    VAII_DEBUG(__FUNCTION__);

    // pop() call blocks if freeRequests is empty, i.e all requests still in workingRequests list and not completed
    request = (BatchRequest *)SafeQueuePop(vino->freeRequests);

    SubmitImagePreProcess(ctx, request, pSrc, pre_processor);

    image_inference_dynarray_add(&request->buffers.frames, &request->buffers.num_buffers, user_data);

    // start inference asynchronously if enough buffers for batching
    if (request->buffers.num_buffers >= vino->batch_size) {
#if 1 // TODO: remove when license-plate-recognition-barrier model will take one input
        if (vino->num_inputs > 1 && !strcmp(vino->inputs[1]->name, "seq_ind")) {
            // 'seq_ind' input layer is some relic from the training
            // it should have the leading 0.0f and rest 1.0f
            dimensions_t dims = {};
            float *blob_data;
            int maxSequenceSizePerPlate;

            infer_request_get_blob_dims(request->infer_request, vino->inputs[1]->name, &dims);
            maxSequenceSizePerPlate = dims.dims[0];
            blob_data = (float *)infer_request_get_blob_data(request->infer_request, vino->inputs[1]->name);
            blob_data[0] = 0.0f;
            for (int n = 1; n < maxSequenceSizePerPlate; n++)
                blob_data[n] = 1.0f;
        }
#endif
        infer_request_infer_async(request->infer_request);
        SafeQueuePush(vino->workingRequests, request);
    } else {
        SafeQueuePushFront(vino->freeRequests, request);
    }
}

static const char *OpenVINOImageInferenceGetModelName(ImageInferenceContext *ctx) {
    OpenVINOImageInference *vino = (OpenVINOImageInference *)ctx->priv;
    return vino->model_name;
}

static void OpenVINOImageInferenceGetModelInputInfo(ImageInferenceContext *ctx, int *width, int *height, int *format) {
    OpenVINOImageInference *vino = (OpenVINOImageInference *)ctx->priv;
    dimensions_t *input_blob_dims = &vino->inputs[0]->dim; // assuming one input layer
    assert(input_blob_dims->ranks > 2);
    *width = input_blob_dims->dims[3];  // W
    *height = input_blob_dims->dims[2]; // H
    *format = FOURCC_RGBP;
}

static int OpenVINOImageInferenceIsQueueFull(ImageInferenceContext *ctx) {
    OpenVINOImageInference *vino = (OpenVINOImageInference *)ctx->priv;
    return SafeQueueEmpty(vino->freeRequests);
}

static int OpenVINOImageInferenceResourceStatus(ImageInferenceContext *ctx) {
    OpenVINOImageInference *vino = (OpenVINOImageInference *)ctx->priv;
    return SafeQueueSize(vino->freeRequests) * vino->batch_size;
}

static void OpenVINOImageInferenceFlush(ImageInferenceContext *ctx) {
    OpenVINOImageInference *vino = (OpenVINOImageInference *)ctx->priv;
    BatchRequest *request = NULL;

    pthread_mutex_lock(&vino->flush_mutex);

    if (vino->already_flushed) {
        pthread_mutex_unlock(&vino->flush_mutex);
        return;
    }

    vino->already_flushed = 1;

    request = (BatchRequest *)SafeQueuePop(vino->freeRequests);
    if (request->buffers.num_buffers > 0) {
        // push the last request to infer
        infer_request_infer_async(request->infer_request);
        SafeQueuePush(vino->workingRequests, request);
        SafeQueueWaitEmpty(vino->workingRequests);
    } else {
        SafeQueuePush(vino->freeRequests, request);
    }

    pthread_mutex_unlock(&vino->flush_mutex);
}

static void OpenVINOImageInferenceClose(ImageInferenceContext *ctx) {
    OpenVINOImageInference *vino = (OpenVINOImageInference *)ctx->priv;

    if (vino->working_thread) {
        // add one empty request
        BatchRequest batch_request = {};
        SafeQueuePush(vino->workingRequests, &batch_request);
        // wait for thread reaching empty request
        pthread_join(vino->working_thread, NULL);
    }

    if (vino->batch_requests) {
        for (size_t i = 0; i < vino->num_batch_requests; i++)
            if (vino->batch_requests[i])
                free(vino->batch_requests[i]);
        free(vino->batch_requests);
    }
    if (vino->freeRequests)
        SafeQueueDestroy(vino->freeRequests);
    if (vino->workingRequests)
        SafeQueueDestroy(vino->workingRequests);

    if (vino->model_name)
        free(vino->model_name);

    pthread_mutex_destroy(&vino->flush_mutex);

    if (vino->vpp_ctx) {
        vino->vpp_ctx->pre_proc->Destroy(vino->vpp_ctx);
        pre_proc_free(vino->vpp_ctx);
    }

    if (vino->inputs) {
        for (size_t i = 0; i < vino->num_inputs; i++)
            if (vino->inputs[i])
                free(vino->inputs[i]);
        free(vino->inputs);
    }
    if (vino->outputs) {
        for (size_t i = 0; i < vino->num_outputs; i++)
            if (vino->outputs[i])
                free(vino->outputs[i]);
        free(vino->outputs);
    }

    ie_network_destroy(vino->network);
    ie_core_destroy(vino->core);
}

static void *WorkingFunction(void *arg) {
    ImageInferenceContext *ctx = (ImageInferenceContext *)arg;
    OpenVINOImageInference *vino = (OpenVINOImageInference *)ctx->priv;
    VAII_DEBUG(__FUNCTION__);

#define IE_REQUEST_WAIT_RESULT_READY -1
    for (;;) {
        IEStatusCode sts;
        BatchRequest *request = (BatchRequest *)SafeQueueFront(vino->workingRequests);
        assert(request);
        VAII_DEBUG("loop");
        if (!request->buffers.num_buffers) {
            break;
        }
        sts = (IEStatusCode)infer_request_wait(request->infer_request, IE_REQUEST_WAIT_RESULT_READY);
        if (IE_STATUS_OK == sts) {
            OutputBlobArray blob_array = {};
            ie_output_info_t **outputs = vino->outputs;

            for (size_t i = 0; i < vino->num_outputs; i++) {
                OpenVINOOutputBlob *vino_blob;
                OutputBlobContext *blob_ctx = output_blob_alloc(ctx->output_blob_method);
                assert(blob_ctx);
                vino_blob = (OpenVINOOutputBlob *)blob_ctx->priv;
                vino_blob->name = ie_info_get_name(outputs[i]);
                vino_blob->blob = infer_request_get_blob(request->infer_request, vino_blob->name);
                image_inference_dynarray_add(&blob_array.output_blobs, &blob_array.num_blobs, blob_ctx);
            }

            vino->callback(&blob_array, &request->buffers);

            for (int n = 0; n < blob_array.num_blobs; n++) {
                OutputBlobContext *blob_ctx = blob_array.output_blobs[n];
                OpenVINOOutputBlob *vino_blob = (OpenVINOOutputBlob *)blob_ctx->priv;
                infer_request_put_blob(vino_blob->blob);
                output_blob_free(blob_ctx);
            }
            blob_array.num_blobs = 0;
            free(blob_array.output_blobs);
        } else {
            fprintf(stderr, "Inference Error: %d model: %s\n", sts, vino->model_name);
            assert(0);
        }

        // move request from workingRequests to freeRequests list
        request = (BatchRequest *)SafeQueuePop(vino->workingRequests);
        // clear buffers
        if (request->buffers.frames) {
            free(request->buffers.frames);
            request->buffers.frames = NULL;
        }
        request->buffers.num_buffers = 0;
        SafeQueuePush(vino->freeRequests, request);
    }

    VAII_DEBUG("EXIT");

    return NULL;
}

static const char *OpenVINOOutputBlobGetOutputLayerName(OutputBlobContext *ctx) {
    OpenVINOOutputBlob *vino_blob = (OpenVINOOutputBlob *)ctx->priv;
    return vino_blob->name;
}

static Dimensions *OpenVINOOutputBlobGetDims(OutputBlobContext *ctx) {
    OpenVINOOutputBlob *vino_blob = (OpenVINOOutputBlob *)ctx->priv;
    return (Dimensions *)ie_blob_get_dims(vino_blob->blob);
}

static IILayout OpenVINOOutputBlobGetLayout(OutputBlobContext *ctx) {
    OpenVINOOutputBlob *vino_blob = (OpenVINOOutputBlob *)ctx->priv;
    return (IILayout)ie_blob_get_layout(vino_blob->blob);
}

static IIPrecision OpenVINOOutputBlobGetPrecision(OutputBlobContext *ctx) {
    OpenVINOOutputBlob *vino_blob = (OpenVINOOutputBlob *)ctx->priv;
    return (IIPrecision)ie_blob_get_precision(vino_blob->blob);
}

static const void *OpenVINOOutputBlobGetData(OutputBlobContext *ctx) {
    OpenVINOOutputBlob *vino_blob = (OpenVINOOutputBlob *)ctx->priv;
    return ie_blob_get_data(vino_blob->blob);
}

OutputBlobMethod output_blob_method_openvino = {
    .name = "openvino",
    .priv_size = sizeof(OpenVINOOutputBlob),
    .GetOutputLayerName = OpenVINOOutputBlobGetOutputLayerName,
    .GetDims = OpenVINOOutputBlobGetDims,
    .GetLayout = OpenVINOOutputBlobGetLayout,
    .GetPrecision = OpenVINOOutputBlobGetPrecision,
    .GetData = OpenVINOOutputBlobGetData,
};

ImageInference image_inference_openvino = {
    .name = "openvino",
    .priv_size = sizeof(OpenVINOImageInference),
    .Create = OpenVINOImageInferenceCreate,
    .SubmitImage = OpenVINOImageInferenceSubmtImage,
    .GetModelName = OpenVINOImageInferenceGetModelName,
    .GetModelInputInfo = OpenVINOImageInferenceGetModelInputInfo,
    .IsQueueFull = OpenVINOImageInferenceIsQueueFull,
    .ResourceStatus = OpenVINOImageInferenceResourceStatus,
    .Flush = OpenVINOImageInferenceFlush,
    .Close = OpenVINOImageInferenceClose,
};
