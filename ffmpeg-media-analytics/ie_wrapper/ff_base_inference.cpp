/*******************************************************************************
 * Copyright (C) <2018-2019> Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "ff_base_inference.h"
#include "inference_backend/logger.h"
#include "inference_impl.h"
#include "proc_factory.h"

#include <libavutil/mem.h>
#include <libavutil/log.h>

#define DEFAULT_MODEL NULL
#define DEFAULT_INFERENCE_ID NULL
#define DEFAULT_MODEL_PROC NULL
#define DEFAULT_OBJECT_CLASS ""
#define DEFAULT_DEVICE "CPU"
#define DEFAULT_META_FORMAT ""
#define DEFAULT_CPU_EXTENSION ""
#define DEFAULT_GPU_EXTENSION ""
#define DEFAULT_RESIZE_BY_INFERENCE FALSE

#define DEFAULT_MIN_BATCH_SIZE 1
#define DEFAULT_MAX_BATCH_SIZE 1024
#define DEFAULT_BATCH_SIZE 1

#define DEFALUT_MIN_THRESHOLD 0.
#define DEFALUT_MAX_THRESHOLD 1.
#define DEFALUT_THRESHOLD 0.5

#define DEFAULT_MIN_EVERY_NTH_FRAME 1
#define DEFAULT_MAX_EVERY_NTH_FRAME UINT_MAX
#define DEFAULT_EVERY_NTH_FRAME 1

#define DEFAULT_MIN_NIREQ 1
#define DEFAULT_MAX_NIREQ 64
#define DEFAULT_NIREQ 2

#define DEFAULT_CPU_STREAMS ""

#define UNUSED(x) (void)(x)

extern "C" {

static void release_inference_instance(FFBaseInference *base_inference) {
    delete (InferenceImpl *)base_inference->inference;
}

static void ff_base_inference_cleanup(FFBaseInference *base_inference) {
    if (base_inference == NULL)
        return;

    if (base_inference->inference) {
        release_inference_instance(base_inference);
        base_inference->inference = NULL;
    }
#define RELEASE_PTR(x) do { if(x) { av_free(x); (x) = NULL; } } while(0)
    RELEASE_PTR(base_inference->model);
    RELEASE_PTR(base_inference->object_class);
    RELEASE_PTR(base_inference->device);
    RELEASE_PTR(base_inference->model_proc);
    RELEASE_PTR(base_inference->inference_id);
    RELEASE_PTR(base_inference->infer_config);
#undef RELEASE_PTR
    base_inference->initialized = FALSE;
}

static void ff_log_function(int level, const char *file, const char *function, int line, const char *message) {
    int log_levels[] = { AV_LOG_QUIET, AV_LOG_ERROR, AV_LOG_WARNING,
                         AV_LOG_VERBOSE, AV_LOG_INFO, AV_LOG_DEBUG,
                         AV_LOG_INFO, AV_LOG_TRACE, AV_LOG_PANIC};

    av_log(NULL, log_levels[level], "%s:%i : %s \t %s \n", file, line, function, message);
}

void ff_base_inference_reset(FFBaseInference *base_inference)
{
    if (base_inference == NULL)
        return;

    ff_base_inference_cleanup(base_inference);

    base_inference->model = av_strdup(DEFAULT_MODEL);
    base_inference->object_class = av_strdup(DEFAULT_OBJECT_CLASS);
    base_inference->device = av_strdup(DEFAULT_DEVICE);
    base_inference->model_proc = av_strdup(DEFAULT_MODEL_PROC);
    base_inference->batch_size = DEFAULT_BATCH_SIZE;
    base_inference->every_nth_frame = DEFAULT_EVERY_NTH_FRAME;
    base_inference->nireq = DEFAULT_NIREQ;
    base_inference->inference_id = av_strdup(DEFAULT_INFERENCE_ID);
    base_inference->cpu_streams = av_strdup(DEFAULT_CPU_STREAMS);
    base_inference->infer_config = av_strdup("");

    base_inference->initialized = FALSE;
    base_inference->is_full_frame = TRUE;
    base_inference->inference = NULL;
    base_inference->pre_proc = NULL;
    base_inference->post_proc = NULL;
    base_inference->get_roi_pre_proc = NULL;
}

int ff_base_inference_init(FFBaseInference *base_inference)
{
    set_log_function(ff_log_function);

    InferenceImpl *infer = new InferenceImpl(base_inference);

    if (!infer)
        return -1;

    base_inference->inference = infer;
    base_inference->initialized = TRUE;
    base_inference->post_proc = (void *)getPostProcFunctionByName(base_inference->inference_id);

    return 0;
}

int ff_base_inference_release(FFBaseInference *base_inference)
{
    ff_base_inference_cleanup(base_inference);
    return 0;
}

int ff_base_inference_send(FFBaseInference *base_inference, AVFrame *frame_in)
{
    ((InferenceImpl *)base_inference->inference)->FilterFrame(base_inference, frame_in);

    return 0;
}

int ff_base_inference_fetch(FFBaseInference *base_inference, ProcessedFrame *frame_out)
{
    if (!frame_out)
        return -1;
    
    ((InferenceImpl *)base_inference->inference)->FetchFrame(base_inference, frame_out);

    return 0;
}

size_t ff_base_inference_ouput_frame_queue_size(FFBaseInference *base_inference)
{
    return ((InferenceImpl *)base_inference->inference)->OutputFrameQueueSize();
}

void ff_base_inference_sink_event(FFBaseInference *base_inference, int event)
{
    return ((InferenceImpl *)base_inference->inference)->SinkEvent((InferenceImpl::EVENT)event);
}

}