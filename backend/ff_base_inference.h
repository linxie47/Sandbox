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

#include "image_inference.h"
#include <libavutil/frame.h>
#include <stdint.h>

#if CONFIG_VAAPI
#include <libavutil/hwcontext_vaapi.h>
#endif

typedef enum {
    INFERENCE_EVENT_NONE,
    INFERENCE_EVENT_EOS,
} FF_INFERENCE_EVENT;

typedef enum { VPP_DEVICE_SW, VPP_DEVICE_HW } VPPDevice;

#ifndef TRUE
/** The TRUE value of a UBool @stable ICU 2.0 */
#define TRUE 1
#endif

#ifndef FALSE
/** The FALSE value of a UBool @stable ICU 2.0 */
#define FALSE 0
#endif

#define MOCKER_PRE_PROC_MAGIC 0x47474747

typedef struct __FFBaseInference FFBaseInference;
typedef struct __FFInferenceParam FFInferenceParam;
typedef struct __ProcessedFrame ProcessedFrame;
typedef struct __InputPreproc InputPreproc;
typedef struct __OutputPostproc OutputPostproc;
typedef struct __InputPreproc ModelInputPreproc;
typedef struct __ModelOutputPostproc ModelOutputPostproc;

#define FF_INFERENCE_OPTIONS                                                                                           \
    char *model;                                                                                                       \
    char *object_class;                                                                                                \
    char *model_proc;                                                                                                  \
    char *device;                                                                                                      \
    int batch_size;                                                                                                    \
    int every_nth_frame;                                                                                               \
    int nireq;                                                                                                         \
    char *cpu_streams;                                                                                                 \
    char *infer_config;                                                                                                \
    float threshold;                                                                                                   \
    int realtime_qos;

struct __FFInferenceParam {
    // exposed options
    FF_INFERENCE_OPTIONS

    VPPDevice vpp_device; // VPPDevice default:SW
    void *opaque;         // VADisplay for vaapi

    Rectangle crop_rect;
    int is_full_frame;
};

struct __FFBaseInference {
    // unique infer string id
    const char *inference_id;

    FFInferenceParam param;

    // other fields
    int initialized;
    int crop_full_frame;    // crop needed for full frame
    void *inference;        // type: FFInferenceImpl*
    void *pre_proc;         // type: PreProcFunction
    void *post_proc;        // type: PostProcFunction
    void *get_roi_pre_proc; // type: GetROIPreProcFunction

    unsigned int num_skipped_frames;
};

/* ROI for analytics */
typedef struct __FFVideoRegionOfInterestMeta {
    char type_name[16]; ///<! type name, e.g. face, vechicle etc.
    unsigned int index; ///<! mark as the serial no. in side data

    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
} FFVideoRegionOfInterestMeta;

/* model preproc */
struct __InputPreproc {
    int color_format;   ///<! input data format
    char *layer_name;   ///<! layer name of input
    char *object_class; ///<! interested object class
};

/* model postproc */
struct __OutputPostproc {
    char *layer_name;
    char *converter;
    char *attribute_name;
    char *method;
    double threshold;
    double tensor_to_text_scale;
    int tensor_to_text_precision;
    AVBufferRef *labels;
};

typedef enum {
    ANY = 0,
    NCHW = 1,
    NHWC = 2,
} IELayout;

typedef enum {
    FP32 = 10,
    U8 = 40,
} IEPrecision;

#define FF_TENSOR_MAX_RANK 8
typedef struct _IETensorMeta {
    IEPrecision precision;           /**< tensor precision */
    int ranks;                       /**< tensor rank */
    size_t dims[FF_TENSOR_MAX_RANK]; /**< array describing tensor's dimensions */
    IELayout layout;                 /**< tensor layout */
    char *layer_name;                /**< tensor output layer name */
    char *model_name;                /**< model name */
    void *data;                      /**< tensor data */
    size_t total_bytes;              /**< tensor size in bytes */
    const char *element_id;          /**< id of pipeline element that produced current tensor */
} IETensorMeta;

/* dynamic labels array */
typedef struct _LabelsArray {
    char **label;
    int num;
} LabelsArray;

typedef struct _InferDetection {
    float x_min;
    float y_min;
    float x_max;
    float y_max;
    float confidence;
    int label_id;
    int object_id;
    AVBufferRef *label_buf;
} InferDetection;

/* dynamic bounding boxes array */
typedef struct _BBoxesArray {
    InferDetection **bbox;
    int num;
} BBoxesArray;

typedef struct _InferDetectionMeta {
    BBoxesArray *bboxes;
} InferDetectionMeta;

typedef struct __InferenceROI {
    AVFrame *frame;
    FFVideoRegionOfInterestMeta roi;
} InferenceROI;

typedef struct __InferenceROIArray {
    InferenceROI **infer_ROIs;
    int num_infer_ROIs;
} InferenceROIArray;

typedef struct InferClassification {
    int detect_id;    ///< detected bbox index
    char *name;       ///< class name, e.g. emotion, age
    char *layer_name; ///< output layer name
    char *model;      ///< model name
    char *attributes;
    int label_id;     ///< label index in labels
    float confidence;
    float value;
    AVBufferRef *label_buf;  ///< label buffer
    AVBufferRef *tensor_buf; ///< output tensor buffer
} InferClassification;

/* dynamic classifications array */
typedef struct ClassifyArray {
    InferClassification **classifications;
    int num;
} ClassifyArray;

typedef struct InferClassificationMeta {
    ClassifyArray *c_array;
} InferClassificationMeta;

#define MAX_MODEL_OUTPUT 4
struct __ModelOutputPostproc {
    OutputPostproc procs[MAX_MODEL_OUTPUT];
};

typedef void (*PostProcFunction)(const OutputBlobArray *output_blobs, InferenceROIArray *infer_roi_array,
                                 ModelOutputPostproc *model_postproc, const char *model_name,
                                 const FFBaseInference *ff_base_inference);

FFBaseInference *av_base_inference_create(const char *inference_id);

int av_base_inference_set_params(FFBaseInference *base, FFInferenceParam *param);

// TODO: add interface to set options separately

void av_base_inference_release(FFBaseInference *base);

int av_base_inference_send_frame(void *ctx, FFBaseInference *base, AVFrame *frame);

int av_base_inference_get_frame(void *ctx, FFBaseInference *base, AVFrame **frame_out);

int av_base_inference_frame_queue_empty(void *ctx, FFBaseInference *base);

int av_base_inference_resource_status(void *ctx, FFBaseInference *base);

void av_base_inference_send_event(void *ctx, FFBaseInference *base, FF_INFERENCE_EVENT event);
