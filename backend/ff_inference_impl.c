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

#include "ff_inference_impl.h"
#include "ff_base_inference.h"
#include "ff_list.h"
#include "image_inference.h"
#include "logger.h"
#include "model_proc.h"
#include <libavutil/avassert.h>
#include <libavutil/log.h>
#include <pthread.h>

typedef enum {
    INFERENCE_EXECUTED = 1,
    INFERENCE_SKIPPED_PER_PROPERTY = 2, // frame skipped due to every-nth-frame set to value greater than 1
    INFERENCE_SKIPPED_REALTIME_QOS = 3, // frame skipped due to realtime-qos policy
    INFERENCE_SKIPPED_ROI = 4           // roi skipped because is_roi_classification_needed() returned false
} InferenceStatus;

typedef struct __Model {
    const char *name;
    char *object_class;
    ImageInferenceContext *infer_ctx;
    FFInferenceImpl *infer_impl;
    // std::map<std::string, void *> proc;
    void *input_preproc;

    void *proc_config;
    ModelInputPreproc model_preproc;
    ModelOutputPostproc model_postproc;
} Model;

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
    int frame_num;
    pthread_mutex_t _mutex; // Maybe not necessary for ffmpeg
    const FFBaseInference *base_inference;

    Model *model;

    // output frames list
    pthread_mutex_t output_frames_mutex;
    ff_list_t *output_frames;
    ff_list_t *processed_frames; // TODO: consider remove it if all output frames can be consumed instantly
};

static void SplitString(char *str, const char *delim, char **array, int *num, int max) {
    char *p;
    int i = 0;

    if (!str || !delim || !array || !num)
        return;

    while (p = strtok(str, delim)) {
        int j = 0;
        char *s;
        size_t end;

        /* remove head blanks */
        while (p[j] == '\n' || p[j] == ' ')
            j++;

        if (!p[j])
            continue;

        /* remove tail blanks */
        s = p + j;
        end = strlen(s) - 1;
        while (s[end] == '\n' || s[end] == ' ')
            s[end--] = '\0';

        array[i++] = s;
        av_assert0(i < max);

        /* string is cached */
        str = NULL;
    }

    *num = i;
}

static inline int avFormatToFourCC(int format) {
    switch (format) {
    case AV_PIX_FMT_NV12:
        VAII_DEBUG("AV_PIX_FMT_NV12");
        return FOURCC_NV12;
    case AV_PIX_FMT_BGR0:
        VAII_DEBUG("AV_PIX_FMT_BGR0");
        return FOURCC_BGRX;
    case AV_PIX_FMT_BGRA:
        VAII_DEBUG("AV_PIX_FMT_BGRA");
        return FOURCC_BGRA;
    case AV_PIX_FMT_BGR24:
        VAII_DEBUG("AV_PIX_FMT_BGR24");
        return FOURCC_BGR;
    case AV_PIX_FMT_RGBP:
        VAII_DEBUG("AV_PIX_FMT_RGBP");
        return FOURCC_RGBP;
    case AV_PIX_FMT_YUV420P:
        VAII_DEBUG("AV_PIX_FMT_YUV420P");
        return FOURCC_I420;
    case AV_PIX_FMT_VAAPI:
        VAII_DEBUG("AV_PIX_FMT_VAAPI");
        return FOURCC_RGBP;
    }

    VAII_LOGE("Unsupported AV Format: %d.", format);
    return 0;
}

static void ff_buffer_map(AVFrame *frame, Image *image, MemoryType memoryType) {
    const int n_planes = 4;

    image->type = memoryType;
    image->format = avFormatToFourCC(frame->format);
    image->width = frame->width;
    image->height = frame->height;
    for (int i = 0; i < n_planes; i++) {
        image->stride[i] = frame->linesize[i];
    }

#ifdef CONFIG_VAAPI
    if (memoryType == MEM_TYPE_VAAPI) {
        image->surface_id = (uint32_t)frame->data[3];
        image->colorspace = frame->colorspace;
    }
#endif
    if (memoryType == MEM_TYPE_SYSTEM) {
        for (int i = 0; i < n_planes; i++) {
            image->planes[i] = frame->data[i];
        }
    }
}

static int CheckObjectClass(const char *requested, const InferDetection *detection) {
    LabelsArray *label_array = NULL;
    if (!requested)
        return 1;

    if (!detection->label_buf)
        return 1;

    label_array = (LabelsArray *)detection->label_buf->data;
    av_assert0(detection->label_id < label_array->num);

    return !strcmp(requested, label_array->label[detection->label_id]) ? 1 : 0;
}

static inline void PushOutput(FFInferenceImpl *impl) {
    ff_list_t *out = impl->output_frames;
    ff_list_t *processed = impl->processed_frames;
    while (!out->empty(out)) {
        OutputFrame *front = (OutputFrame *)out->front(out);
        AVFrame *frame = front->frame;
        if (front->inference_count > 0) {
            break; // inference not completed yet
        }

        processed->push_back(processed, frame);
        out->pop_front(out);
        av_free(front);
    }
}

static void InferenceCompletionCallback(OutputBlobArray *blobs, UserDataBuffers *user_data) {
    Model *model = NULL;
    FFInferenceImpl *impl = NULL;
    InferenceResult *result = NULL;
    const FFBaseInference *base = NULL;
    InferenceROIArray inference_frames_array = {};

    if (0 == user_data->num_buffers)
        return;

    result = (InferenceResult *)user_data->frames[0];
    model = result->model;
    impl = model->infer_impl;
    base = impl->base_inference;

    for (int i = 0; i < user_data->num_buffers; i++) {
        result = (InferenceResult *)user_data->frames[i];
        av_dynarray_add(&inference_frames_array.infer_ROIs, &inference_frames_array.num_infer_ROIs,
                        &result->inference_frame);
    }

    if (base->post_proc) {
        ((PostProcFunction)base->post_proc)(blobs, &inference_frames_array, &model->model_postproc, model->name, base);
    }

    pthread_mutex_lock(&impl->output_frames_mutex);

    for (int i = 0; i < inference_frames_array.num_infer_ROIs; i++) {
        OutputFrame *output;
        ff_list_t *out = impl->output_frames;
        InferenceROI *frame = inference_frames_array.infer_ROIs[i];
        iterator it = out->iterator_get(out);
        while (it) {
            output = (OutputFrame *)out->iterate_value(it);
            if (frame->frame == output->frame || frame->frame == output->writable_frame) {
                output->inference_count--;
                break;
            }
            it = out->iterate_next(out, it);
        }
    }

    PushOutput(impl);

    for (int i = 0; i < user_data->num_buffers; i++)
        av_free(user_data->frames[i]);

    pthread_mutex_unlock(&impl->output_frames_mutex);

    av_free(inference_frames_array.infer_ROIs);
}

static Model *CreateModel(FFBaseInference *base, const char *model_file, const char *model_proc_path,
                          const char *object_class) {
    int ret = 0;
    Model *model = NULL;
    const ImageInference *inference = image_inference_get_by_name("openvino");
    const OutputBlobMethod *method = output_blob_method_get_by_name("openvino");
    ImageInferenceContext *context = NULL;

    VAII_LOGI("Loading model: device=%s, path=%s\n", base->param.device, model_file);
    VAII_LOGI("Setting batch_size=%d, nireq=%d\n", base->param.batch_size, base->param.nireq);

    context = image_inference_alloc(inference, method, "ffmpeg-image-infer");
    model = (Model *)av_mallocz(sizeof(*model));
    av_assert0(context && model);

    if (model_proc_path) {
        void *proc = model_proc_read_config_file(model_proc_path);
        if (!proc) {
            VAII_LOGE("Could not read proc config file:"
                   "%s\n",
                   model_proc_path);
            av_assert0(proc);
        }

        if (model_proc_parse_input_preproc(proc, &model->model_preproc) < 0) {
            VAII_ERROR("Parse input preproc error.\n");
        }

        if (model_proc_parse_output_postproc(proc, &model->model_postproc) < 0) {
            VAII_ERROR("Parse output postproc error.\n");
        }

        model->proc_config = proc;
    }

    ret = context->inference->Create(context, MEM_TYPE_ANY, base->param.device, model_file, base->param.batch_size,
                                     base->param.nireq, base->param.infer_config, NULL, InferenceCompletionCallback);
    av_assert0(ret == 0);

    // Create async pre_proc image inference backend
    if (base->param.opaque) {
        PreProcContext *preproc_ctx = NULL;
        ImageInferenceContext *async_preproc_ctx = NULL;

        const ImageInference *inference = image_inference_get_by_name("async_preproc");
        async_preproc_ctx = image_inference_alloc(inference, NULL, "async-preproc-infer");
        if (base->param.vpp_device == VPP_DEVICE_HW)
            preproc_ctx = pre_proc_alloc(pre_proc_get_by_name("vaapi"));
        else
            preproc_ctx = pre_proc_alloc(pre_proc_get_by_name("mocker"));

        av_assert0(async_preproc_ctx && preproc_ctx);

        async_preproc_ctx->inference->CreateAsyncPreproc(async_preproc_ctx, context, preproc_ctx, 6,
                                                         base->param.opaque);

        // substitute for opevino image inference
        context = async_preproc_ctx;
    }

    model->infer_ctx = context;
    model->name = context->inference->GetModelName(context);
    model->object_class = object_class ? av_strdup(object_class) : NULL;
    model->input_preproc = NULL;

    return model;
}

static void ReleaseModel(Model *model) {
    ImageInferenceContext *ii_ctx;
    if (!model)
        return;

    ii_ctx = model->infer_ctx;
    ii_ctx->inference->Close(ii_ctx);
    image_inference_free(ii_ctx);

    model_proc_release_model_proc(model->proc_config, &model->model_preproc, &model->model_postproc);

    if (model->object_class)
        av_free(model->object_class);
    av_free(model);
}

static void SubmitImage(Model *model, FFVideoRegionOfInterestMeta *meta, Image *image, AVFrame *frame) {
    ImageInferenceContext *s = model->infer_ctx;
    PreProcessor preProcessFunction = NULL;

    InferenceResult *result = (InferenceResult *)malloc(sizeof(*result));
    av_assert0(result);
    result->inference_frame.frame = frame;
    result->inference_frame.roi = *meta;
    result->model = model;

    image->rect = (Rectangle){.x = (int)meta->x, .y = (int)meta->y, .width = (int)meta->w, .height = (int)meta->h};
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
    s->inference->SubmitImage(s, image, (IFramePtr)result, preProcessFunction);
}

static int SubmitImages(FFInferenceImpl *impl, const ROIMetaArray *metas, AVFrame *frame) {
    int ret = 0;
    Image image = {};

    // TODO: map frame w/ different memory type to image
    // BufferMapContext mapContext;

    // map to the image according to the mem type
    ff_buffer_map(frame, &image, frame->hw_frames_ctx ? MEM_TYPE_VAAPI : MEM_TYPE_SYSTEM);

    for (int i = 0; i < metas->num_metas; i++) {
        SubmitImage(impl->model, metas->roi_metas[i], &image, frame);
    }

    // ff_buffer_unmap(buffer, image, mapContext);

    return ret;
}

FFInferenceImpl *FFInferenceImplCreate(FFBaseInference *ff_base_inference) {
    Model *dnn_model = NULL;
    FFInferenceImpl *impl = (FFInferenceImpl *)av_mallocz(sizeof(*impl));

    av_assert0(impl && ff_base_inference && ff_base_inference->param.model);

    dnn_model = CreateModel(ff_base_inference, ff_base_inference->param.model, ff_base_inference->param.model_proc,
                            ff_base_inference->param.object_class);
    dnn_model->infer_impl = impl;

    impl->model = dnn_model;
    impl->base_inference = ff_base_inference;
    impl->output_frames = ff_list_alloc();
    impl->processed_frames = ff_list_alloc();

    av_assert0(impl->output_frames && impl->processed_frames);

    pthread_mutex_init(&impl->_mutex, NULL);
    pthread_mutex_init(&impl->output_frames_mutex, NULL);

    return impl;
}

void FFInferenceImplRelease(FFInferenceImpl *impl) {
    if (!impl)
        return;

    ReleaseModel(impl->model);

    ff_list_free(impl->output_frames);
    ff_list_free(impl->processed_frames);

    pthread_mutex_destroy(&impl->_mutex);
    pthread_mutex_destroy(&impl->output_frames_mutex);

    av_free(impl);
}

int FFInferenceImplAddFrame(void *ctx, FFInferenceImpl *impl, AVFrame *frame) {
    FFBaseInference *base_inference = (FFBaseInference *)impl->base_inference;
    ROIMetaArray metas = {};
    FFVideoRegionOfInterestMeta full_frame_meta = {};
    int inference_count = 0;

    InferenceStatus status = INFERENCE_EXECUTED;
    if (++base_inference->num_skipped_frames < base_inference->param.every_nth_frame) {
        status = INFERENCE_SKIPPED_PER_PROPERTY;
    }

    if (base_inference->param.realtime_qos) {
        ImageInferenceContext *ii_ctx = impl->model->infer_ctx;
        if (ii_ctx->inference->IsQueueFull(ii_ctx)) {
            status = INFERENCE_SKIPPED_REALTIME_QOS;
        }
    }

    if (status == INFERENCE_EXECUTED) {
        base_inference->num_skipped_frames = 0;
    }

    // Collect all ROI metas into ROIMetaArray
    if (base_inference->param.is_full_frame) {
        if (base_inference->crop_full_frame) {
            full_frame_meta.x = base_inference->param.crop_rect.x;
            full_frame_meta.y = base_inference->param.crop_rect.y;
            full_frame_meta.w = base_inference->param.crop_rect.width;
            full_frame_meta.h = base_inference->param.crop_rect.height;
            full_frame_meta.index = 0;
        } else {
            full_frame_meta.x = 0;
            full_frame_meta.y = 0;
            full_frame_meta.w = frame->width;
            full_frame_meta.h = frame->height;
            full_frame_meta.index = 0;
        }
        av_dynarray_add(&metas.roi_metas, &metas.num_metas, &full_frame_meta);
    } else {
        BBoxesArray *bboxes = NULL;
        InferDetectionMeta *detect_meta = NULL;
        AVFrameSideData *side_data = av_frame_get_side_data(frame, AV_FRAME_DATA_INFERENCE_DETECTION);
        if (side_data) {
            detect_meta = (InferDetectionMeta *)(side_data->data);
            av_assert0(detect_meta);
            bboxes = detect_meta->bboxes;
            if (bboxes) {
                ModelInputPreproc *model_preproc = &impl->model->model_preproc;
                for (int i = 0; i < bboxes->num; i++) {
                    FFVideoRegionOfInterestMeta *roi_meta = NULL;
                    if (!CheckObjectClass(model_preproc->object_class, bboxes->bbox[i]))
                        continue;
                    roi_meta = (FFVideoRegionOfInterestMeta *)av_malloc(sizeof(*roi_meta));
                    if (roi_meta == NULL)
                        goto exit;
                    roi_meta->x = bboxes->bbox[i]->x_min;
                    roi_meta->y = bboxes->bbox[i]->y_min;
                    roi_meta->w = bboxes->bbox[i]->x_max - bboxes->bbox[i]->x_min;
                    roi_meta->h = bboxes->bbox[i]->y_max - bboxes->bbox[i]->y_min;
                    roi_meta->index = i;
                    av_dynarray_add(&metas.roi_metas, &metas.num_metas, roi_meta);
                }
            }
        }
    }

    // count number ROIs to run inference on
    inference_count = (status == INFERENCE_EXECUTED) ? metas.num_metas : 0;
    impl->frame_num++;

    // push into output_frames queue
    {
        OutputFrame *output_frame;
        ff_list_t *output = impl->output_frames;
        ff_list_t *processed = impl->processed_frames;
        pthread_mutex_lock(&impl->output_frames_mutex);

        if (!inference_count && output->empty(output)) {
            processed->push_back(processed, frame);
            pthread_mutex_unlock(&impl->output_frames_mutex);
            goto exit;
        }

        output_frame = (OutputFrame *)av_malloc(sizeof(*output_frame));
        if (output_frame == NULL) {
            pthread_mutex_unlock(&impl->output_frames_mutex);
            goto exit;
        }
        output_frame->frame = frame;
        output_frame->writable_frame = NULL; // TODO: alloc new frame if not writable
        output_frame->inference_count = inference_count;
        impl->output_frames->push_back(impl->output_frames, output_frame);

        if (!inference_count) {
            // If we don't need to run inference and there are no frames queued for inference then finish transform
            pthread_mutex_unlock(&impl->output_frames_mutex);
            goto exit;
        }

        pthread_mutex_unlock(&impl->output_frames_mutex);
    }

    SubmitImages(impl, &metas, frame);

exit:
    if (!base_inference->param.is_full_frame) {
        for (int n = 0; n < metas.num_metas; n++)
            av_free(metas.roi_metas[n]);
    }
    av_free(metas.roi_metas);
    return 0;
}

int FFInferenceImplGetFrame(void *ctx, FFInferenceImpl *impl, AVFrame **frame) {
    ff_list_t *l = impl->processed_frames;

    if (l->empty(l) || !frame)
        return AVERROR(EAGAIN);

    pthread_mutex_lock(&impl->output_frames_mutex);
    *frame = (AVFrame *)l->front(l);
    l->pop_front(l);
    pthread_mutex_unlock(&impl->output_frames_mutex);

    return 0;
}

size_t FFInferenceImplGetQueueSize(void *ctx, FFInferenceImpl *impl) {
    ff_list_t *out = impl->output_frames;
    ff_list_t *pro = impl->processed_frames;
    VAII_LOGI("output:%zu processed:%zu\n", out->size(out), pro->size(pro));
    return out->size(out) + pro->size(pro);
}

size_t FFInferenceImplResourceStatus(void *ctx, FFInferenceImpl *impl) {
    return impl->model->infer_ctx->inference->ResourceStatus(impl->model->infer_ctx);
}

void FFInferenceImplSinkEvent(void *ctx, FFInferenceImpl *impl, FF_INFERENCE_EVENT event) {
    if (event == INFERENCE_EVENT_EOS) {
        impl->model->infer_ctx->inference->Flush(impl->model->infer_ctx);
    }
}
