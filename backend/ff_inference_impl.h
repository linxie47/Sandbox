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

#include "ff_base_inference.h"

typedef struct __FFInferenceImpl FFInferenceImpl;

FFInferenceImpl *FFInferenceImplCreate(FFBaseInference *ff_base_inference);

void FFInferenceImplRelease(FFInferenceImpl *impl);

int FFInferenceImplAddFrame(void *ctx, FFInferenceImpl *impl, AVFrame *frame);

int FFInferenceImplGetFrame(void *ctx, FFInferenceImpl *impl, AVFrame **frame);

size_t FFInferenceImplGetQueueSize(void *ctx, FFInferenceImpl *impl);

size_t FFInferenceImplResourceStatus(void *ctx, FFInferenceImpl *impl);

void FFInferenceImplSinkEvent(void *ctx, FFInferenceImpl *impl, FF_INFERENCE_EVENT event);
