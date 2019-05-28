/*******************************************************************************
 * Copyright (C) <2018-2019> Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#ifndef __INFERENCE_IMPL_H__
#define __INFERENCE_IMPL_H__

#include "inference_backend/image_inference.h"
#include "ff_base_inference.h"

#include <list>
#include <memory>
#include <mutex>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

typedef struct {
    AVFrame *frame;
    FFVideoRegionOfInterestMeta roi;
} InferenceROI;

class InferenceImpl {
  public:
    InferenceImpl(FFBaseInference *base_inference);

    // void FlushInference();
    
    int FilterFrame(FFBaseInference *ovino, AVFrame *frame);

    void FetchFrame(FFBaseInference *ovino, ProcessedFrame *out);

    virtual ~InferenceImpl();

  private:
    struct ClassificationModel {
        std::string model_name;
        std::string object_class;
        std::shared_ptr<InferenceBackend::ImageInference> inference;
        std::map<std::string, void *> model_proc;
        void *input_preproc;
    };

    void PushOutput();
    void InferenceCompletionCallback(std::map<std::string, InferenceBackend::OutputBlob::Ptr> blobs,
                                     std::vector<std::shared_ptr<InferenceBackend::ImageInference::IFrameBase>> frames);
    ClassificationModel CreateClassificationModel(FFBaseInference *ff_base_inference,
                                                  // std::shared_ptr<InferenceBackend::Allocator> &allocator,
                                                  const std::string &model_file, const std::string &model_proc_path,
                                                  const std::string &object_class);


    int SubmitImages(const std::vector<FFVideoRegionOfInterestMeta *> &metas, AVFrame *frame);

    void SubmitImage(ClassificationModel &model, FFVideoRegionOfInterestMeta *meta, InferenceBackend::Image &image,
                     AVFrame *frame);

    std::string CreateNestedErrorMsg(const std::exception &e, int level = 0);

    mutable std::mutex _mutex;
    int frame_num;
    std::vector<ClassificationModel> models;
    FFBaseInference *ff_base_inference;

    struct OutputFrame {
        AVFrame *frame;
        AVFrame *writable_frame;
        int inference_count;
    };

    struct InferenceResult : public InferenceBackend::ImageInference::IFrameBase {
        InferenceROI inference_frame;
        ClassificationModel *model;
    };

    std::list<OutputFrame> output_frames;
    std::list<ProcessedFrame> processed_frames;
    std::mutex output_frames_mutex;
};

#endif /* __INFERENCE_IMPL_H__ */