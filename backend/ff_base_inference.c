#include "ff_base_inference.h"

FFBaseInference *av_base_inference_create(const char *inference_id) {}

int av_base_inference_set_params(FFBaseInference *base, FFInferenceParam *param) {}

void av_base_inference_release(FFBaseInference *base) {}

int av_base_inference_send_frame(void *ctx, FFBaseInference *base, AVFrame *frame) {}

AVFrame *av_base_inference_get_frame(void *ctx, FFBaseInference *base) {}

int av_base_inference_frame_queue_empty(void *ctx, FFBaseInference *base) {}

void av_base_inference_send_event(void *ctx, FFBaseInference *base, FF_INFERENCE_EVENT event) {}