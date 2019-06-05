/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "ie_wrapper.h"

#ifndef TRUE
/** The TRUE value of a UBool @stable ICU 2.0 */
#   define TRUE  1
#endif
#ifndef FALSE
/** The FALSE value of a UBool @stable ICU 2.0 */
#   define FALSE 0
#endif

struct _FFBaseInference {
    // properties
    char  *model;
    char  *object_class;
    char  *model_proc;
    char  *device;
    size_t batch_size;
    size_t every_nth_frame;
    size_t nireq;
    char  *inference_id;
    char  *cpu_streams;
    char  *infer_config;
    float  threshold;
    ModelInputPreproc   *model_preproc;
    ModelOutputPostproc *model_postproc;
    
    // other fields
    int   is_full_frame;
    void *inference;        // C++ type: InferenceImpl*
    void *pre_proc;         // C++ type: PreProcFunction
    void *post_proc;        // C++ type: PostProcFunction
    void *get_roi_pre_proc; // C++ type: GetROIPreProcFunction
    int   initialized;
};

typedef struct _FFVideoRegionOfInterestMeta {
    int roi_type;
    
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
} FFVideoRegionOfInterestMeta;

int ff_base_inference_init(FFBaseInference *base_inference);

int ff_base_inference_release(FFBaseInference *base_inference);

int ff_base_inference_send(FFBaseInference *base_inference, AVFrame *frame_in);

int ff_base_inference_fetch(FFBaseInference *base_inference, ProcessedFrame *output);

size_t ff_base_inference_ouput_frame_queue_size(FFBaseInference *base_inference);

void ff_base_inference_sink_event(FFBaseInference *base_inference, int event);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include "inference_backend/image_inference.h"
#include <functional>

typedef struct {
    AVFrame *frame;
    FFVideoRegionOfInterestMeta roi;
} InferenceROI;

// Pre-processing and post-processing function signatures
typedef void (*PreProcFunction)(void *preproc, InferenceBackend::Image &image);
typedef std::function<void(InferenceBackend::Image &)> (*GetROIPreProcFunction)(void *preproc,
                                                                                FFVideoRegionOfInterestMeta *roi_meta);
typedef void (*PostProcFunction)(const std::map<std::string, InferenceBackend::OutputBlob::Ptr> &output_blobs,
                                 std::vector<InferenceROI> frames,
                                 const std::map<std::string, void *> &model_proc, const char *model_name,
                                 FFBaseInference *ff_base_inference);
#endif