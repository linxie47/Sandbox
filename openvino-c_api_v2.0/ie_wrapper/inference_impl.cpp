/*******************************************************************************
 * Copyright (C) <2018-2019> Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "inference_impl.h"
#include "inference_backend/logger.h"

#include <cstring>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace std::placeholders;
using namespace InferenceBackend;

namespace
{
inline int avFormatToFourCC(int format) {
    switch (format) {
    case AV_PIX_FMT_NV12:
        GVA_DEBUG("AV_PIX_FMT_NV12");
        return InferenceBackend::FourCC::FOURCC_NV12;
    case AV_PIX_FMT_BGR0:
        GVA_DEBUG("AV_PIX_FMT_BGR0");
        return InferenceBackend::FourCC::FOURCC_BGRX;
    case AV_PIX_FMT_BGRA:
        GVA_DEBUG("AV_PIX_FMT_BGRA");
        return InferenceBackend::FourCC::FOURCC_BGRA;
#if VA_MAJOR_VERSION >= 1
    case AV_PIX_FMT_YUV420P:
        GVA_DEBUG("AV_PIX_FMT_YUV420P");
        return InferenceBackend::FourCC::FOURCC_I420;
#endif
    }

    printf("Unsupported AV Format: %d.", format);
    return 0;
}

bool CheckObjectClass(std::string requested, int quark) {
    if (requested.empty())
        return true;
    if (!quark)
        return false;
// return requested == g_quark_to_string(quark);
    return true;
}

inline std::vector<std::string> SplitString(const std::string input, char delimiter = ',') {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(input);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

inline std::string GetStringArrayElem(const std::string &in_str, int index) {
    auto tokens = SplitString(in_str);
    if (index < 0 || (size_t)index >= tokens.size())
        return "";
    return tokens[index];
}

inline std::map<std::string, std::string> String2Map(std::string const &s) {
    std::string key, val;
    std::istringstream iss(s);
    std::map<std::string, std::string> m;

    while (std::getline(std::getline(iss, key, '=') >> std::ws, val)) {
        m[key] = val;
    }

    return m;
}
#if 0
inline std::shared_ptr<Allocator> CreateAllocator(const char *const allocator_name) {
    std::shared_ptr<Allocator> allocator;
    if (allocator_name != nullptr) {
        try {
            allocator = std::make_shared<GstAllocatorWrapper>(allocator_name);
            GVA_TRACE("GstAllocatorWrapper is created");
        } catch (const std::exception &e) {
            GVA_ERROR(e.what());
            throw;
        }
    }
    return allocator;
}
#endif
inline std::map<std::string, std::string> CreateInferConfig(const char *const infer_config_str,
                                                            const char *const cpu_streams_c_str, const int nireq) {
    std::map<std::string, std::string> infer_config = String2Map(infer_config_str);
    if (cpu_streams_c_str != nullptr && strlen(cpu_streams_c_str) > 0) {
        std::string cpu_streams = cpu_streams_c_str;
        if (cpu_streams == "true")
            cpu_streams = std::to_string(nireq);
        if (cpu_streams != "false")
            infer_config[KEY_CPU_THROUGHPUT_STREAMS] = cpu_streams;
    }
    return infer_config;
}

bool ff_buffer_map(AVFrame *frame, InferenceBackend::Image &image, InferenceBackend::MemoryType memoryType) {
#ifdef DISABLE_VAAPI
    (void)(memoryType);
#endif
    image = {};

    const int n_planes = 4;
    if (n_planes > InferenceBackend::Image::MAX_PLANES_NUMBER)
        throw std::logic_error("Planes number " + std::to_string(n_planes) + " isn't supported");

    image.format = avFormatToFourCC(frame->format);
    image.width = static_cast<int>(frame->width);
    image.height = static_cast<int>(frame->height);
    for (int i = 0; i < n_planes; i++) {
        image.stride[i] = frame->linesize[i];
    }

#ifndef DISABLE_VAAPI
    // TODO VAAPI
#endif
    {
        for (int i = 0; i < n_planes; i++) {
            image.planes[i] = frame->data[i];
        }
    }
    return true;
}

} // namespace

InferenceImpl::ClassificationModel InferenceImpl::CreateClassificationModel(FFBaseInference *ff_base_inference,
                                                                            const std::string &model_file,
                                                                            const std::string &model_proc_path,
                                                                            const std::string &object_class) {

    printf("Loading model: device=%s, path=%s\n", ff_base_inference->device, model_file.c_str());
    printf("Setting batch_size=%zd, nireq=%zd\n", ff_base_inference->batch_size, ff_base_inference->nireq);
    // set_log_function(GST_logger);
    std::map<std::string, std::string> infer_config =
        CreateInferConfig(ff_base_inference->infer_config, ff_base_inference->cpu_streams, ff_base_inference->nireq);

    auto infer = ImageInference::make_shared(MemoryType::ANY, ff_base_inference->device, model_file,
                                             ff_base_inference->batch_size, ff_base_inference->nireq, infer_config,
                                             nullptr,
                                             std::bind(&InferenceImpl::InferenceCompletionCallback, this, _1, _2));

    ClassificationModel model;
    model.inference = infer;
    model.model_name = infer->GetModelName();
    model.object_class = object_class;

    return model;
}

InferenceImpl::InferenceImpl(FFBaseInference *ff_base_inference)
    : frame_num(-1), ff_base_inference(ff_base_inference) {
    if (!ff_base_inference->model) {
        throw std::runtime_error("Model not specified");
    }
    std::vector<std::string> model_files = SplitString(ff_base_inference->model);
    std::vector<std::string> model_procs;
    if (ff_base_inference->model_proc) {
        model_procs = SplitString(ff_base_inference->model_proc);
    }

    std::map<std::string, std::string> infer_config =
        CreateInferConfig(ff_base_inference->infer_config, ff_base_inference->cpu_streams, ff_base_inference->nireq);

    // allocator = CreateAllocator(ff_base_inference->allocator_name);
    
    std::vector<std::string> object_classes = SplitString(ff_base_inference->object_class);
    for (size_t i = 0; i < model_files.size(); i++) {
        std::string model_proc = i < model_procs.size() ? model_procs[i] : std::string();
        std::string object_class = i < object_classes.size() ? object_classes[i] : "";
        auto model = CreateClassificationModel(ff_base_inference, model_files[i], model_proc, object_class);
        this->models.push_back(std::move(model));
    }
}

InferenceImpl::~InferenceImpl() {
    models.clear();
}

void InferenceImpl::FetchFrame(FFBaseInference *ovino, ProcessedFrame *out) {
    if (processed_frames.empty())
        return;
    
    auto frame = processed_frames.front();
    out->frame = frame.frame;
    out->out_blobs = frame.out_blobs;
    processed_frames.pop_front();
}

void InferenceImpl::PushOutput() {
    while (!output_frames.empty()) {
        OutputFrame &front = output_frames.front();
        if (front.inference_count > 0) {
            break; // inference not completed yet
        }
        ProcessedFrame processed_frame = {};
        processed_frame.frame = front.frame;
        //TODO: fill blob data
        processed_frames.push_back(processed_frame);
        output_frames.pop_front();
    }
}

void InferenceImpl::InferenceCompletionCallback(
    std::map<std::string, InferenceBackend::OutputBlob::Ptr> blobs,
    std::vector<std::shared_ptr<InferenceBackend::ImageInference::IFrameBase>> frames) {
    if (frames.empty())
        return;

    std::lock_guard<std::mutex> guard(output_frames_mutex);

    printf("Inference complete callback!\n");

    std::vector<InferenceROI> inference_frames;
    ClassificationModel *model = nullptr;
    for (auto &frame : frames) {
        auto f = std::dynamic_pointer_cast<InferenceResult>(frame);
        model = f->model;
        inference_frames.push_back(f->inference_frame);
        // AVFrame *av_frame = inference_frames.back().frame;
        // check if we have writable version of this buffer (this function called multiple times on same buffer)
        /*
        for (OutputFrame &output : output_frames) {
            if (output.frame == av_frame) {
                if (output.writable_frame)
                    buffer = inference_frames.back().buffer = output.writable_buffer;
                break;
            }
        }
        */
    }

    // TODO: post proc
    /*
    if (gva_base_inference->post_proc) {
        ((PostProcFunction)gva_base_inference->post_proc)(blobs, inference_frames, model->model_proc,
                                                          model ? model->model_name.c_str() : nullptr,
                                                          gva_base_inference);
    }
    */

    for (const auto &blob_desc : blobs)
    {
        InferenceBackend::OutputBlob::Ptr blob = blob_desc.second;
        const std::string &layer_name = blob_desc.first;

        const float *detections = (const float *)blob->GetData();
        auto dims = blob->GetDims();
        auto layout = blob->GetLayout();

        printf("DIMS:\n");
        for (auto dim = dims.begin(); dim < dims.end(); dim++)
        {
            printf("\t%lu\n", *dim);
        }

        int object_size = 0;
        int max_proposal_count = 0;
        switch (layout) {
        case InferenceBackend::OutputBlob::Layout::NCHW:
            object_size = dims[3];
            max_proposal_count = dims[2];
            break;
        default:
            printf("Unsupported output layout, boxes won't be extracted\n");
            continue;
        }
        if (object_size != 7) { // SSD DetectionOutput format
            printf("Unsupported output dimensions, boxes won't be extracted\n");
            continue;
        }
    }

    for (InferenceROI &frame : inference_frames) {
        for (OutputFrame &output : output_frames) {
            if (frame.frame == output.frame || frame.frame == output.writable_frame) {
                output.inference_count--;
                printf("output frame ref count %d!\n", output.inference_count);
                break;
            }
        }
    }

    PushOutput();
}

void InferenceImpl::SubmitImage(ClassificationModel &model, FFVideoRegionOfInterestMeta *meta,
                                InferenceBackend::Image &image, AVFrame *frame) {
    image.rect = {.x = (int)meta->x, .y = (int)meta->y, .width = (int)meta->w, .height = (int)meta->h};
    auto result = std::make_shared<InferenceResult>();
    result->inference_frame.frame = frame;
    result->inference_frame.roi = *meta;
    result->model = &model;

    std::function<void(InferenceBackend::Image &)> preProcessFunction = [](InferenceBackend::Image &) {};
    #if 0
    if (ff_base_inference->pre_proc && model.input_preproc) {
        preProcessFunction = [&](InferenceBackend::Image &image) {
            ((PreProcFunction)ff_base_inference->pre_proc)(model.input_preproc, image);
        };
    }
    if (ff_base_inference->get_roi_pre_proc && model.input_preproc) {
        preProcessFunction = ((GetROIPreProcFunction)ff_base_inference->get_roi_pre_proc)(model.input_preproc, meta);
    }
    #endif
    model.inference->SubmitImage(image, result, preProcessFunction);
}

int InferenceImpl::SubmitImages(const std::vector<FFVideoRegionOfInterestMeta *> &metas, AVFrame *frame)
{
    int ret = 0;

    InferenceBackend::Image image;
    
    // TODO: map frame w/ different memory type to image
    // BufferMapContext mapContext;

    ff_buffer_map(frame, image, InferenceBackend::MemoryType::ANY);
    try {
        for (ClassificationModel &model : models) {
            for (const auto meta : metas) {
                // if (CheckObjectClass(model.object_class, meta->roi_type)) {
                    SubmitImage(model, meta, image, frame);
                //}
            }
        }
    } catch (const std::exception &e) {
        fprintf(stderr, "%s", CreateNestedErrorMsg(e).c_str());
        ret = -1;
    }
    //gva_buffer_unmap(buffer, image, mapContext);

    return ret;
}

int InferenceImpl::FilterFrame(FFBaseInference *ff_base_inference, AVFrame *frame)
{
    std::unique_lock<std::mutex> lock(_mutex);

    // Collect all ROI metas into std::vector
    std::vector<FFVideoRegionOfInterestMeta *> metas;
    FFVideoRegionOfInterestMeta full_frame_meta;
    if (ff_base_inference->is_full_frame) {
        full_frame_meta = {};
        full_frame_meta.x = 0;
        full_frame_meta.y = 0;
        full_frame_meta.w = frame->width;
        full_frame_meta.h = frame->height;
        metas.push_back(&full_frame_meta);
    } else {
        // FFVideoRegionOfInterestMeta *meta = NULL;
        // TODO: get ROI from side-data
        // 
    }
    
    frame_num++;
    // count number ROIs to run inference on
    int inference_count = 0;

    for (ClassificationModel &model : models) {
        for (auto meta : metas) {
            if (CheckObjectClass(model.object_class, meta->roi_type)) {
                inference_count++;
            }
        }
    }
    
    bool run_inference =
        !(inference_count == 0 ||
          // ff_base_inference->every_nth_frame == -1 || // TODO separate property instead of -1
          (ff_base_inference->every_nth_frame > 0 && frame_num % ff_base_inference->every_nth_frame > 0));
    
    // push into output_frames queue
    AVFrame *frame_copy;
    {
        std::lock_guard<std::mutex> guard(output_frames_mutex);
        if (!run_inference && output_frames.empty()) {
            return 0;
        }

        frame_copy = av_frame_alloc();
        av_frame_ref(frame_copy, frame);
        InferenceImpl::OutputFrame output_frame = {.frame = frame_copy,
                                                   .writable_frame = NULL,
                                                   .inference_count = run_inference ? inference_count : 0 };
        output_frames.push_back(output_frame);

        if (!run_inference) {
            // If we don't need to run inference and there are no frames queued for inference then finish transform
            return 0;
        }
    }

    return SubmitImages(metas, frame_copy);
}

std::string InferenceImpl::CreateNestedErrorMsg(const std::exception &e, int level) {
    static std::string msg = "\n";
    msg += std::string(level, ' ') + e.what() + "\n";
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception &e) {
        CreateNestedErrorMsg(e, level + 1);
    }
    return msg;
}
