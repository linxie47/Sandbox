/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "inference_impl.h"
#include "image_inference.h"
#include <pthread.h>

typedef struct __Model {
    char *name;
    char *object_class;
    ImageInferenceContext *inference;
    // std::map<std::string, void *> proc;
    void *input_preproc;
} Model;

typedef struct __InferenceROI {
    AVFrame *frame;
    FFVideoRegionOfInterestMeta roi;
} InferenceROI;

typedef struct __ROIMetaArray {
    FFVideoRegionOfInterestMeta **roi_metas;
    int num_metas;
} ROIMetaArray;

/* \brief output frames stored in queue */
typedef struct __OutputFrame {
    AVFrame *frame;
    AVFrame *writable_frame;
    int inference_count;
} OutputFrame;

/* \brief structure taken as IFramPtr carried by \func SubmitImage */
typedef struct __InferenceResult {
    InferenceROI inference_frame;
    Model *model;
} InferenceResult;

struct __FFInferenceImpl {
    const FFBaseInference *base_inference;
    pthread_mutex_t _mutex;
    int frame_num;

    // Model array
    Model **models;
    int num_models;

    // std::list<OutputFrame> output_frames;
    // std::list<ProcessedFrame> processed_frames;
    pthread_mutex_t output_frames_mutex;
};

static void PushOutput(FFInferenceImpl *impl);

static void InferenceCompletionCallback(OutputBlobArray *Blobs, UserDataBuffers *frames);

static Model *CreateModel(FFInferenceImpl *impl, const char *model_file, const char *model_proc_path,
                          const char *object_class) {
}

static int SubmitImages(FFInferenceImpl *impl, const ROIMetaArray *metas, AVFrame *frame) {
}

static void SubmitImage(Model *model, FFVideoRegionOfInterestMeta *meta, Image *image, AVFrame *frame) {
}

FFInferenceImpl *FFInferenceImplCreate(FFBaseInference *ff_base_inference) {
}

int FFInferenceImplAddFrame(FFInferenceImpl *impl, AVFrame *frame) {
}

int FFInferenceImplGetFrame(FFInferenceImpl *impl, AVFrame **frame) {
}

size_t FFInferenceImplGetQueueSize(FFInferenceImpl *impl) {
}

void FFInferenceImplRelease(FFInferenceImpl *impl) {
}

void FFInferenceImplSinkEvent(FFInferenceImpl *impl, FF_INFERENCE_EVENT event) {
}