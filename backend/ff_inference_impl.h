/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "ff_base_inference.h"

typedef struct __FFInferenceImpl FFInferenceImpl;

FFInferenceImpl *FFInferenceImplCreate(FFBaseInference *ff_base_inference);

void FFInferenceImplRelease(FFInferenceImpl *impl);

int FFInferenceImplAddFrame(void *ctx, FFInferenceImpl *impl, AVFrame *frame);

int FFInferenceImplGetFrame(void *ctx, FFInferenceImpl *impl, AVFrame **frame);

size_t FFInferenceImplGetQueueSize(void *ctx, FFInferenceImpl *impl);

void FFInferenceImplSinkEvent(void *ctx, FFInferenceImpl *impl, FF_INFERENCE_EVENT event);