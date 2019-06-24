/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "openvino_image_inference.h"
#include "image_inference.h"

typedef struct OpenVINOImageInference {

} OpenVINOImageInference;

int OpenVINOImageInferenceCreate(ImageInferenceContext *ctx, MemoryType type, const char *devices, const char *model,
                                 int batch_size, int nireq, void *config, void *allocator, CallbackFunc callback) {

    return 0;
}

void OpenVINOImageInferenceSubmtImage(ImageInferenceContext *ctx, const Image *image, void *user_data,
                                      PreProcessor *pre_processor) {
}

const char OpenVINOImageInferenceGetModelName(ImageInferenceContext *ctx) {
    return NULL;
}

int OpenVINOImageInferenceIsQueueFull(ImageInferenceContext *ctx) {
    return 0;
}

void OpenVINOImageInferenceFlush(ImageInferenceContext *ctx) {
}

void OpenVINOImageInferenceClose(ImageInferenceContext *ctx) {
}

ImageInference image_inference_openvino = {
    .name = "openvino",
    .priv_size = sizeof(OpenVINOImageInference),
    .Create = OpenVINOImageInferenceCreate,
    .SubmitImage = OpenVINOImageInferenceSubmtImage,
    .GetModelName = OpenVINOImageInferenceGetModelName,
    .IsQueueFull = OpenVINOImageInferenceIsQueueFull,
    .Flush = OpenVINOImageInferenceFlush,
    .Close = OpenVINOImageInferenceClose,
};
