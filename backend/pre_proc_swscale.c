/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "pre_proc.h"
#include <assert.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
// #define DEBUG

#if CONFIG_SWSCALE || HAVE_FFMPEG

#ifdef DEBUG
static void DumpBGRpToFile(const Image *out_image) {
    FILE *fp;
    char file_name[256] = {};
    static int dump_frame_num = 0;

    sprintf(file_name, "ff_pre_proc%03d.rgb", dump_frame_num++);
    fp = fopen(file_name, "w+b");
    assert(fp);

    const uint8_t *b_channel = out_image->planes[0];
    const uint8_t *g_channel = out_image->planes[1];
    const uint8_t *r_channel = out_image->planes[2];

    int size = out_image->height * out_image->width * 3;
    uint8_t *data = (uint8_t *)malloc(size);
    memset(data, 0, size);

    for (int i = 0; i < out_image->height; i++) {
        for (int j = 0; j < out_image->width; j++) {
            data[3 * j + i * 3 * out_image->width] = r_channel[j + i * out_image->width];
            data[3 * j + i * 3 * out_image->width + 1] = g_channel[j + i * out_image->width];
            data[3 * j + i * 3 * out_image->width + 2] = b_channel[j + i * out_image->width];
        }
    }
    fwrite(data, out_image->height * out_image->width * 3, 1, fp);
    free(data);
    fclose(fp);
}

static void DumpRGBpToFile(const Image *out_image) {
    FILE *fp;
    char file_name[256] = {};
    static int dump_frame_num = 0;

    sprintf(file_name, "ff_rgbp_source%03d.rgb", dump_frame_num++);
    fp = fopen(file_name, "w+b");
    assert(fp);

    const uint8_t *b_channel = out_image->planes[2];
    const uint8_t *g_channel = out_image->planes[1];
    const uint8_t *r_channel = out_image->planes[0];

    int size = out_image->height * out_image->width * 3;
    uint8_t *data = (uint8_t *)malloc(size);
    memset(data, 0, size);

    for (int i = 0; i < out_image->height; i++) {
        for (int j = 0; j < out_image->width; j++) {
            data[3 * j + i * 3 * out_image->width] = r_channel[j + i * out_image->width];
            data[3 * j + i * 3 * out_image->width + 1] = g_channel[j + i * out_image->width];
            data[3 * j + i * 3 * out_image->width + 2] = b_channel[j + i * out_image->width];
        }
    }
    fwrite(data, out_image->height * out_image->width * 3, 1, fp);
    free(data);
    fclose(fp);
}

static inline void DumpImageInfo(const Image *p) {
    av_log(NULL, AV_LOG_INFO, "Image w:%d h:%d f:%x, plane: %p %p %p  stride: %d %d %d \n", p->width, p->height,
           p->format, p->planes[0], p->planes[1], p->planes[2], p->stride[0], p->stride[1], p->stride[2]);
}

#endif

static inline enum AVPixelFormat FOURCC2FFmpegFormat(int format) {
    switch (format) {
    case FOURCC_NV12:
        return AV_PIX_FMT_NV12;
    case FOURCC_BGRA:
        return AV_PIX_FMT_BGRA;
    case FOURCC_BGRX:
        return AV_PIX_FMT_BGRA;
    case FOURCC_BGR:
        return AV_PIX_FMT_BGR24;
    case FOURCC_RGBP:
        return AV_PIX_FMT_RGBP;
    case FOURCC_I420:
        return AV_PIX_FMT_YUV420P;
    }
    return AV_PIX_FMT_NONE;
}

typedef struct FFPreProc {
    struct SwsContext *sws_context[3];
    Image image_yuv;
    Image image_bgr;
} FFPreProc;

static void FFPreProcConvert(PreProcContext *context, const Image *src, Image *dst, int bAllocateDestination) {
    FFPreProc *ff_pre_proc = (FFPreProc *)context->priv;
    struct SwsContext **sws_context = ff_pre_proc->sws_context;
    Image *image_yuv = &ff_pre_proc->image_yuv;
    Image *image_bgr = &ff_pre_proc->image_bgr;
    uint8_t *gbr_planes[3] = {};

    // if identical format and resolution
    if (src->format == dst->format && src->format == FOURCC_RGBP && src->width == dst->width &&
        src->height == dst->height) {
        int planes_count = GetPlanesCount(src->format);
        // RGB->BGR
        Image src_bgr = *src;
        src_bgr.planes[0] = src->planes[2];
        src_bgr.planes[2] = src->planes[0];
        for (int i = 0; i < planes_count; i++) {
            if (src_bgr.width == src_bgr.stride[i]) {
                memcpy(dst->planes[i], src_bgr.planes[i], src_bgr.width * src_bgr.height * sizeof(uint8_t));
            } else {
                int dst_stride = dst->stride[i] * sizeof(uint8_t);
                int src_stride = src_bgr.stride[i] * sizeof(uint8_t);
                for (int r = 0; r < src_bgr.height; r++) {
                    memcpy(dst->planes[i] + r * dst_stride, src_bgr.planes[i] + r * src_stride, dst->width);
                }
            }
        }

        return;
    }
#define PLANE_NUM 3
    // init image YUV
    if (image_yuv->width != dst->width || image_yuv->height != dst->height) {
        int ret = 0;
        image_yuv->width = dst->width;
        image_yuv->height = dst->height;
        image_yuv->format = src->format; // no CSC for the 1st stage

        if (image_yuv->planes[0])
            av_freep(&image_yuv->planes[0]);
        ret = av_image_alloc(image_yuv->planes, image_yuv->stride, image_yuv->width, image_yuv->height,
                             FOURCC2FFmpegFormat(image_yuv->format), 16);
        if (ret < 0) {
            fprintf(stderr, "Alloc yuv image buffer error!\n");
            assert(0);
        }
    }

    // init image BGR24
    if (image_bgr->width != image_yuv->width || image_bgr->height != image_yuv->height) {
        int ret = 0;
        image_bgr->width = image_yuv->width;
        image_bgr->height = image_yuv->height;
        image_bgr->format = FOURCC_BGR; // YUV -> BGR packed for the 2nd stage

        if (image_bgr->planes[0])
            av_freep(&image_bgr->planes[0]);
        ret = av_image_alloc(image_bgr->planes, image_bgr->stride, image_bgr->width, image_bgr->height,
                             FOURCC2FFmpegFormat(image_bgr->format), 16);
        if (ret < 0) {
            fprintf(stderr, "Alloc bgr image buffer error!\n");
            assert(0);
        }
    }

    sws_context[0] = sws_getCachedContext(sws_context[0], src->width, src->height, FOURCC2FFmpegFormat(src->format),
                                          image_yuv->width, image_yuv->height, FOURCC2FFmpegFormat(image_yuv->format),
                                          SWS_FAST_BILINEAR, NULL, NULL, NULL);
    sws_context[1] = sws_getCachedContext(sws_context[1], image_yuv->width, image_yuv->height,
                                          FOURCC2FFmpegFormat(image_yuv->format), image_bgr->width, image_bgr->height,
                                          FOURCC2FFmpegFormat(image_bgr->format), SWS_FAST_BILINEAR, NULL, NULL, NULL);
    sws_context[2] = sws_getCachedContext(sws_context[2], image_bgr->width, image_bgr->height,
                                          FOURCC2FFmpegFormat(image_bgr->format), dst->width, dst->height,
                                          AV_PIX_FMT_GBRP, SWS_FAST_BILINEAR, NULL, NULL, NULL);

    for (int i = 0; i < 3; i++)
        assert(sws_context[i]);

    // BGR->GBR
    gbr_planes[0] = dst->planes[1];
    gbr_planes[1] = dst->planes[0];
    gbr_planes[2] = dst->planes[2];

    // stage 1: yuv -> yuv, resize to dst size
    if (!sws_scale(sws_context[0], (const uint8_t *const *)src->planes, src->stride, 0, src->height,
                   image_yuv->planes, image_yuv->stride)) {
        fprintf(stderr, "Error on FFMPEG sws_scale stage 1\n");
        assert(0);
    }
    // stage 2: yuv -> bgr packed, no resize
    if (!sws_scale(sws_context[1], (const uint8_t *const *)image_yuv->planes, image_yuv->stride, 0, image_yuv->height,
                   image_bgr->planes, image_bgr->stride)) {
        fprintf(stderr, "Error on FFMPEG sws_scale stage 2\n");
        assert(0);
    }
    // stage 3: bgr -> gbr planer, no resize
    if (!sws_scale(sws_context[2], (const uint8_t *const *)image_bgr->planes, image_bgr->stride, 0, image_bgr->height,
                   gbr_planes, dst->stride)) {
        fprintf(stderr, "Error on FFMPEG sws_scale stage 3\n");
        assert(0);
    }

    /* dump pre-processed image to file */
    // DumpBGRpToFile(dst);
}

static void FFPreProcDestroy(PreProcContext *context) {
    FFPreProc *ff_pre_proc = (FFPreProc *)context->priv;

    for (int i = 0; i < 3; i++) {
        if (ff_pre_proc->sws_context[i]) {
            sws_freeContext(ff_pre_proc->sws_context[i]);
            ff_pre_proc->sws_context[i] = NULL;
        }
    }
    if (ff_pre_proc->image_yuv.planes[0]) {
        av_freep(&ff_pre_proc->image_yuv.planes[0]);
        ff_pre_proc->image_yuv.planes[0] = NULL;
    }
    if (ff_pre_proc->image_bgr.planes[0]) {
        av_freep(&ff_pre_proc->image_bgr.planes[0]);
        ff_pre_proc->image_bgr.planes[0] = NULL;
    }
}

PreProc pre_proc_swscale = {
    .name = "swscale",
    .priv_size = sizeof(FFPreProc),
    .mem_type = MEM_TYPE_SYSTEM,
    .Convert = FFPreProcConvert,
    .Destroy = FFPreProcDestroy,
};

#endif
