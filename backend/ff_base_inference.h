/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#pragma once

#include <libavutil/frame.h>
#include <stdint.h>

typedef enum {
    INFERENCE_EVENT_NONE,
    INFERENCE_EVENT_EOS,
} FF_INFERENCE_EVENT;

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
    float threshold;

struct __FFInferenceParam {
    // exposed options
    FF_INFERENCE_OPTIONS

    int is_full_frame;
    ModelInputPreproc *model_preproc;
    ModelOutputPostproc *model_postproc;
};

struct __FFBaseInference {
    // unique infer string id
    const char *inference_id;

    // exposed options
    FF_INFERENCE_OPTIONS

    // internal options
    int is_full_frame;

    // other fields
    int initialized;
    void *inference;        // type: FFInferenceImpl*
    void *pre_proc;         // type: PreProcFunction
    void *post_proc;        // type: PostProcFunction
    void *get_roi_pre_proc; // type: GetROIPreProcFunction
    ModelInputPreproc *model_preproc;
    ModelOutputPostproc *model_postproc;
};

/* ROI for analytics */
typedef struct __FFVideoRegionOfInterestMeta {
    char type_name[16];

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
    double tensor2text_scale;
    int tensor2text_precision;
    AVBufferRef *labels;
};

#define MAX_MODEL_OUTPUT 4
struct __ModelOutputPostproc {
    OutputPostproc procs[MAX_MODEL_OUTPUT];
};

FFBaseInference *av_base_inference_create(const char *inference_id);

int av_base_inference_set_params(FFBaseInference *base, FFInferenceParam *param);

// TODO: add interface to set options separately

void av_base_inference_release(FFBaseInference *base);

int av_base_inference_send_frame(void *ctx, FFBaseInference *base, AVFrame *frame);

AVFrame *av_base_inference_get_frame(void *ctx, FFBaseInference *base);

int av_base_inference_frame_queue_empty(void *ctx, FFBaseInference *base);

void av_base_inference_send_event(void *ctx, FFBaseInference *base, FF_INFERENCE_EVENT event);
