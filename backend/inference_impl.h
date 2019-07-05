/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "ff_base_inference.h"

typedef struct __FFInferenceImpl FFInferenceImpl;

FFInferenceImpl *FFInferenceImplCreate(FFBaseInference *ff_base_inference);

int FFInferenceImplAddFrame(FFInferenceImpl *impl, AVFrame *frame);

int FFInferenceImplGetFrame(FFInferenceImpl *impl, AVFrame **frame);

size_t FFInferenceImplGetQueueSize(FFInferenceImpl *impl);

void FFInferenceImplRelease(FFInferenceImpl *impl);

void FFInferenceImplSinkEvent(FFInferenceImpl *impl, FF_INFERENCE_EVENT event);