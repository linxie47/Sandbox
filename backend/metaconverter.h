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

#include <libavutil/frame.h>
#include "libavfilter/avfilter.h"
#include <json-c/json.h>

#define OFFSET(x) offsetof(MetaConvertContext, x)
#define FLAGS (AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_FILTERING_PARAM)

typedef struct MetaConvertContext {
    const AVClass *class;

    char *model;
    char *converter;
    char *method;
    char *location;
    char *layer;

    int frame_number;
    FILE *f;

    int (*convert_func)(AVFilterContext *ctx, AVFrame *frame, json_object *info_object);

} MetaConvertContext;

int detection_to_json(AVFilterContext *ctx, AVFrame *frame, json_object *info_object);

int classification_to_json(AVFilterContext *ctx, AVFrame *frame, json_object *info_object);

int all_to_json(AVFilterContext *ctx, AVFrame *frame, json_object *info_object);

int tensors_to_file(AVFilterContext *ctx, AVFrame *frame, json_object *info_object);

int convert_roi_detection(json_object *info_object, AVFrame *frame);

int convert_roi_tensor(json_object *info_object, AVFrame *frame);