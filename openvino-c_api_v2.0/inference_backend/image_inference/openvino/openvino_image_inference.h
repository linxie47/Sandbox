/*******************************************************************************
 * Copyright (C) <2018-2019> Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#pragma once

#include "inference_backend/image_inference.h"
#include "inference_backend/pre_proc.h"

#include <atomic>
#include <inference_engine.hpp>
#include <map>
#include <string>
#include <thread>

#include "config.h"

#include "inference_backend/logger.h"
#include "safe_queue.h"

using namespace InferenceBackend;

class OpenVINOImageInference : public ImageInference {
  public:
    OpenVINOImageInference(std::string devices, std::string model, int batch_size, int nireq,
                           const std::map<std::string, std::string> &config, Allocator *allocator,
                           CallbackFunc callback);

    virtual ~OpenVINOImageInference();

    virtual void SubmitImage(const Image &image, IFramePtr user_data, std::function<void(Image &)> preProcessor);

    virtual const std::string &GetModelName() const;

    virtual bool IsQueueFull();

    virtual void Flush();

    virtual void Close();

  protected:
    typedef struct {
        InferenceEngine::InferRequest::Ptr infer_request;
        std::vector<IFramePtr> buffers;
        std::vector<InferenceBackend::Allocator::AllocContext *> alloc_context;
    } BatchRequest;

    void GetNextImageBuffer(BatchRequest &request, Image *image);

    void WorkingFunction(BatchRequest request);

    bool resize_by_inference;
    Allocator *allocator;
    CallbackFunc callback;

    // Inference Engine
    std::vector<InferenceEngine::InferencePlugin::Ptr> plugins;
    InferenceEngine::ConstInputsDataMap inputs;
    InferenceEngine::ConstOutputsDataMap outputs;
    std::string modelName;

    // Threading
    int batch_size;
    std::thread working_thread;
    SafeQueue<BatchRequest> freeRequests;
    SafeQueue<BatchRequest> workingRequests;

    // VPP
    std::unique_ptr<PreProc> sw_vpp;
    std::unique_ptr<PreProc> vaapi_vpp;

    std::mutex mutex_;
    std::mutex inference_completion_mutex_;
    std::atomic<unsigned int> requests_processing_;
    std::condition_variable request_processed_;
    bool already_flushed;
    std::mutex flush_mutex;
    std::shared_ptr<ImageInference> get_ptr();

  private:
    void SubmitImageSoftwarePreProcess(BatchRequest &request, const Image *pSrc,
                                       std::function<void(Image &)> preProcessor, std::function<void()> runner);
};
