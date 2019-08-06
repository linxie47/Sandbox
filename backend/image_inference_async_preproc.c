/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include <assert.h>
#include <string.h>

#include "image_inference.h"
#include "image_inference_async_preproc.h"
#include "logger.h"

#define MOCKER_PRE_PROC_MAGIC 0x47474747

static void *AsyncPreprocWorkingFunction(void *arg);

static void PreprocImagesFree(PreprocImage **imgs, size_t num_imgs) {
    if (!imgs || !num_imgs)
        return;

    for (size_t i = 0; i < num_imgs; i++) {
        if (imgs[i]) {
            image_map_free(imgs[i]->img_map_ctx);
            free(imgs[i]);
        }
    }
    free(imgs);
}

static int ImageInferenceAsyncPreprocCreate(ImageInferenceContext *async_preproc_context,
                                            ImageInferenceContext *inference_context, PreProcContext *preproc_context,
                                            int image_queue_size, void *opaque) {
    int ret = 0;
    int width = 0, height = 0, format = 0;
    ImageInferenceAsyncPreproc *async_preproc = (ImageInferenceAsyncPreproc *)async_preproc_context->priv;
    PreProcInitParam pp_init_param = {};
    assert(inference_context && preproc_context);

    VAII_INFO("Using async preproc image inference.");

    async_preproc->actual = inference_context;
    async_preproc->pre_proc = preproc_context;

    // TODO: create image pool
    async_preproc->preproc_images = (PreprocImage **)malloc(image_queue_size * sizeof(*async_preproc->preproc_images));
    if (!async_preproc->preproc_images) {
        VAII_ERROR("Creat preproc images failed!");
        goto err;
    }
    async_preproc->num_preproc_images = image_queue_size;

    async_preproc->freeImages = SafeQueueCreate();
    async_preproc->workingImages = SafeQueueCreate();
    if (!async_preproc->freeImages || !async_preproc->workingImages) {
        VAII_ERROR("Creat images queues failed!");
        goto err;
    }

    inference_context->inference->GetModelInputInfo(inference_context, &width, &height, &format);
    pp_init_param.va_display = opaque;
    pp_init_param.num_surfaces = image_queue_size;
    pp_init_param.width = width;
    pp_init_param.height = height;
    pp_init_param.format = format;

    if (preproc_context->pre_proc->Init)
        preproc_context->pre_proc->Init(preproc_context, &pp_init_param);

    for (size_t n = 0; n < async_preproc->num_preproc_images; n++) {
        PreprocImage *preproc_image = (PreprocImage *)malloc(sizeof(*preproc_image));
        if (!preproc_image)
            goto err;
        memset(preproc_image, 0, sizeof(*preproc_image));

        preproc_image->image.type = MEM_TYPE_ANY;
        preproc_image->image.width = width;
        preproc_image->image.height = height;
        preproc_image->image.format = format;
        if (MOCKER_PRE_PROC_MAGIC == (uint32_t)opaque)
            preproc_image->img_map_ctx = image_map_alloc(image_map_get_by_name("mocker"));
        else
            preproc_image->img_map_ctx = image_map_alloc(image_map_get_by_name("vaapi"));

        async_preproc->preproc_images[n] = preproc_image;
        SafeQueuePush(async_preproc->freeImages, preproc_image);
    }

    ret = pthread_create(&async_preproc->async_thread, NULL, AsyncPreprocWorkingFunction, async_preproc_context);
    if (ret != 0) {
        VAII_ERROR("Create async preproc thread error!");
        goto err;
    }

    return 0;
err:
    PreprocImagesFree(async_preproc->preproc_images, async_preproc->num_preproc_images);

    if (async_preproc->freeImages)
        SafeQueueDestroy(async_preproc->freeImages);
    if (async_preproc->workingImages)
        SafeQueueDestroy(async_preproc->workingImages);

    return -1;
}

static int ImageInferenceAsyncPreprocCreateDummy(ImageInferenceContext *ctx, MemoryType type, const char *devices,
                                                 const char *model, int batch_size, int nireq, const char *config,
                                                 void *allocator, CallbackFunc callback) {
    /* Leave empty */
    return 0;
}

static void ImageInferenceAsyncPreprocSubmtImage(ImageInferenceContext *ctx, const Image *image, IFramePtr user_data,
                                                 PreProcessor pre_processor) {
    ImageInferenceAsyncPreproc *async_preproc = (ImageInferenceAsyncPreproc *)ctx->priv;
    PreProcContext *pp_ctx = async_preproc->pre_proc;
    PreprocImage *pp_image = NULL;

    pp_image = (PreprocImage *)SafeQueuePop(async_preproc->freeImages);
    pp_ctx->pre_proc->Convert(pp_ctx, image, &pp_image->image, 1);
    pp_image->user_data = user_data;
    pp_image->pre_processor = pre_processor;
    SafeQueuePush(async_preproc->workingImages, pp_image);
}

static const char *ImageInferenceAsyncPreprocGetModelName(ImageInferenceContext *ctx) {
    ImageInferenceAsyncPreproc *async_preproc = (ImageInferenceAsyncPreproc *)ctx->priv;

    ImageInferenceContext *infer_ctx = async_preproc->actual;
    const ImageInference *infer = infer_ctx->inference;
    return infer->GetModelName(infer_ctx);
}

static int ImageInferenceAsyncPreprocIsQueueFull(ImageInferenceContext *ctx) {
    ImageInferenceAsyncPreproc *async_preproc = (ImageInferenceAsyncPreproc *)ctx->priv;

    ImageInferenceContext *infer_ctx = async_preproc->actual;
    const ImageInference *infer = infer_ctx->inference;
    return infer->IsQueueFull(infer_ctx);
}

static void ImageInferenceAsyncPreprocFlush(ImageInferenceContext *ctx) {
    ImageInferenceAsyncPreproc *async_preproc = (ImageInferenceAsyncPreproc *)ctx->priv;

    ImageInferenceContext *infer_ctx = async_preproc->actual;
    const ImageInference *infer = infer_ctx->inference;
    return infer->Flush(infer_ctx);
}

static void ImageInferenceAsyncPreprocClose(ImageInferenceContext *ctx) {
    ImageInferenceAsyncPreproc *async_preproc = (ImageInferenceAsyncPreproc *)ctx->priv;
    ImageInferenceContext *infer_ctx = async_preproc->actual;
    const ImageInference *infer = infer_ctx->inference;

    if (async_preproc->async_thread) {
        // add one empty request
        PreprocImage pp_image = {};
        SafeQueuePush(async_preproc->workingImages, &pp_image);
        // wait for thread reaching empty request
        pthread_join(async_preproc->async_thread, NULL);
    }

    for (size_t n = 0; n < async_preproc->num_preproc_images; n++) {
        PreprocImage *pp_image = async_preproc->preproc_images[n];
        PreProcContext *pp_ctx = async_preproc->pre_proc;
        pp_ctx->pre_proc->ReleaseImage(pp_ctx, &pp_image->image);
    }

    infer->Close(infer_ctx);
    image_inference_free(infer_ctx);
    pre_proc_free(async_preproc->pre_proc);

    PreprocImagesFree(async_preproc->preproc_images, async_preproc->num_preproc_images);

    if (async_preproc->freeImages)
        SafeQueueDestroy(async_preproc->freeImages);
    if (async_preproc->workingImages)
        SafeQueueDestroy(async_preproc->workingImages);
}

static void *AsyncPreprocWorkingFunction(void *arg) {
    ImageInferenceContext *ctx = (ImageInferenceContext *)arg;
    ImageInferenceAsyncPreproc *async_preproc = (ImageInferenceAsyncPreproc *)ctx->priv;
    ImageInferenceContext *infer_ctx = async_preproc->actual;
    const ImageInference *infer = infer_ctx->inference;

    while (1) {
        PreprocImage *pp_image = (PreprocImage *)SafeQueueFront(async_preproc->workingImages);
        // empty ctx means ending
        if (!pp_image->img_map_ctx)
            break;

        {
            ImageMapContext *map_ctx = pp_image->img_map_ctx;
            Image image_sys = map_ctx->mapper->Map(map_ctx, &pp_image->image);
            infer->SubmitImage(infer_ctx, &image_sys, pp_image->user_data, pp_image->pre_processor);
            map_ctx->mapper->Unmap(map_ctx);
        }
        pp_image = (PreprocImage *)SafeQueuePop(async_preproc->workingImages);
        SafeQueuePush(async_preproc->freeImages, pp_image);
    }

    return NULL;
}

ImageInference image_inference_async_preproc = {
    .name = "async_preproc",
    .priv_size = sizeof(ImageInferenceAsyncPreproc),
    .CreateAsyncPreproc = ImageInferenceAsyncPreprocCreate,
    .Create = ImageInferenceAsyncPreprocCreateDummy,
    .SubmitImage = ImageInferenceAsyncPreprocSubmtImage,
    .GetModelName = ImageInferenceAsyncPreprocGetModelName,
    .IsQueueFull = ImageInferenceAsyncPreprocIsQueueFull,
    .Flush = ImageInferenceAsyncPreprocFlush,
    .Close = ImageInferenceAsyncPreprocClose,
};
