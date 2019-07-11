/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "ff_base_inference.h"
#include "ff_inference_impl.h"
#include "ff_proc_factory.h"
#include "logger.h"
#include <libavutil/avassert.h>
#include <libavutil/mem.h>

static void ff_log_function(int level, const char *file, const char *function, int line, const char *message) {
    int log_levels[] = {AV_LOG_QUIET, AV_LOG_ERROR, AV_LOG_WARNING, AV_LOG_VERBOSE, AV_LOG_INFO,
                        AV_LOG_DEBUG, AV_LOG_INFO,  AV_LOG_TRACE,   AV_LOG_PANIC};

    av_log(NULL, log_levels[level], "%s:%i : %s \t %s \n", file, line, function, message);
}

FFBaseInference *av_base_inference_create(const char *inference_id) {
    FFBaseInference *base_inference = (FFBaseInference *)av_mallocz(sizeof(*base_inference));

    base_inference->inference_id = inference_id ? av_strdup(inference_id) : NULL;

    base_inference->param.is_full_frame = TRUE; // default to true
    base_inference->param.every_nth_frame = 1;  // default to 1
    base_inference->param.nireq = 1;
    base_inference->param.batch_size = 1;
    base_inference->param.threshold = 0.5;

    return base_inference;
}

int av_base_inference_set_params(FFBaseInference *base, FFInferenceParam *param) {
    if (!base || !param)
        return AVERROR(EINVAL);

    if (base->initialized)
        return 0;

    base->param = *param;
    base->inference = (void *)FFInferenceImplCreate(base);
    base->initialized = TRUE;
    base->post_proc = (void *)getPostProcFunctionByName(base->inference_id, base->param.model);

    set_log_function(ff_log_function);

    return 0;
}

void av_base_inference_release(FFBaseInference *base) {
    if (!base)
        return;

    if (base->inference) {
        FFInferenceImplRelease((FFInferenceImpl *)base->inference);
        base->inference = NULL;
    }

    if (base->inference_id) {
        av_free((void *)base->inference_id);
        base->inference_id = NULL;
    }

    av_free(base);
}

int av_base_inference_send_frame(void *ctx, FFBaseInference *base, AVFrame *frame_in) {
    if (!base || !frame_in)
        return AVERROR(EINVAL);

    return FFInferenceImplAddFrame(ctx, (FFInferenceImpl *)base->inference, frame_in);
}

int av_base_inference_get_frame(void *ctx, FFBaseInference *base, AVFrame **frame_out) {
    if (!base || !frame_out)
        return AVERROR(EINVAL);

    return FFInferenceImplGetFrame(ctx, (FFInferenceImpl *)base->inference, frame_out);
}

int av_base_inference_frame_queue_empty(void *ctx, FFBaseInference *base) {
    if (!base)
        return AVERROR(EINVAL);

    return FFInferenceImplGetQueueSize(ctx, (FFInferenceImpl *)base->inference) == 0 ? TRUE : FALSE;
}

void av_base_inference_send_event(void *ctx, FFBaseInference *base, FF_INFERENCE_EVENT event) {
    if (!base)
        return;

    FFInferenceImplSinkEvent(ctx, (FFInferenceImpl *)base->inference, event);
}