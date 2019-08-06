/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "pre_proc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if CONFIG_VAAPI
#include <va/va.h>
#include <va/va_vpp.h>
#endif

#if CONFIG_VAAPI

#define VA_CALL(_FUNC)                                                                                                 \
    do {                                                                                                               \
        VAStatus _status = _FUNC;                                                                                      \
        if (_status != VA_STATUS_SUCCESS) {                                                                            \
            printf(#_FUNC " failed, sts = %d (%s).\n", _status, vaErrorStr(_status));                                  \
            assert(0);                                                                                                 \
        }                                                                                                              \
    } while (0)

typedef struct _VAAPIPreProc {
    VADisplay display;
    VAConfigID va_config;
    VAContextID va_context;
    VAImageFormat *format_list; //!< Surface formats which can be used with this device.
    int nb_formats;
    VAImage va_image;
    VAImageFormat va_format_selected;
} VAAPIPreProc;

static uint32_t Fourcc2RTFormat(int format_fourcc) {
    switch (format_fourcc) {
#if VA_MAJOR_VERSION >= 1
    case VA_FOURCC_I420:
        return VA_FOURCC_I420;
#endif
    case VA_FOURCC_NV12:
        return VA_RT_FORMAT_YUV420;
    case VA_FOURCC_RGBP:
        return VA_RT_FORMAT_RGBP;
    default:
        return VA_RT_FORMAT_RGB32;
    }
}

static VASurfaceID CreateVASurface(VADisplay va_display, const Image *src) {
    unsigned int rtformat = Fourcc2RTFormat(src->format);
    VASurfaceID va_surface_id;
    VASurfaceAttrib surface_attrib;
    surface_attrib.type = VASurfaceAttribPixelFormat;
    surface_attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
    surface_attrib.value.type = VAGenericValueTypeInteger;
    surface_attrib.value.value.i = src->format;

    VA_CALL(vaCreateSurfaces(va_display, rtformat, src->width, src->height, &va_surface_id, 1, &surface_attrib, 1));
    return va_surface_id;
}

static int VAAPIPreProcInit(PreProcContext *context, PreProcInitParam *param) {
    VAAPIPreProc *vaapi_pre_proc = (VAAPIPreProc *)context->priv;
    VADisplay va_display = (VADisplay)param->va_display;
    VAConfigID va_config = VA_INVALID_ID;
    VAContextID va_context = VA_INVALID_ID;
    VAImageFormat *image_list = NULL;
    int image_count;

    image_count = vaMaxNumImageFormats(va_display);
    if (image_count <= 0) {
        return -1;
    }
    image_list = malloc(image_count * sizeof(*image_list));
    if (!image_list) {
        return -1;
    }

    VA_CALL(vaQueryImageFormats(va_display, image_list, &image_count));
    VA_CALL(vaCreateConfig(va_display, VAProfileNone, VAEntrypointVideoProc, NULL, 0, &va_config));
    VA_CALL(vaCreateContext(va_display, va_config, 0, 0, VA_PROGRESSIVE, 0, 0, &va_context));

    vaapi_pre_proc->display = va_display;
    vaapi_pre_proc->format_list = image_list;
    vaapi_pre_proc->nb_formats = image_count;
    vaapi_pre_proc->va_config = va_config;
    vaapi_pre_proc->va_context = va_context;

    for (int i = 0; i < vaapi_pre_proc->nb_formats; i++) {
        if (vaapi_pre_proc->format_list[i].fourcc == VA_RT_FORMAT_RGBP) {
            vaapi_pre_proc->va_format_selected = vaapi_pre_proc->format_list[i];
            break;
        }
    }
    return VA_STATUS_SUCCESS;
}

static void VAAPIPreProcConvert(PreProcContext *context, const Image *src, Image *dst, int bAllocateDestination) {
    VAAPIPreProc *vaapi_pre_proc = (VAAPIPreProc *)context->priv;
    VAProcPipelineParameterBuffer pipeline_param = {};
    VARectangle surface_region = {};
    VABufferID pipeline_param_buf_id = VA_INVALID_ID;

    VADisplay va_display = vaapi_pre_proc->display;
    VAContextID va_context = vaapi_pre_proc->va_context;
    VASurfaceID src_surface = (VASurfaceID)src->surface_id;

    if (dst->type == MEM_TYPE_ANY) {
        dst->surface_id = CreateVASurface(vaapi_pre_proc->display, dst);
        dst->va_display = va_display;
        dst->type = MEM_TYPE_VAAPI;
    }

    pipeline_param.surface = src_surface;
    surface_region = (VARectangle){.x = (int16_t)src->rect.x,
                                   .y = (int16_t)src->rect.y,
                                   .width = (uint16_t)src->rect.width,
                                   .height = (uint16_t)src->rect.height};
    if (surface_region.width > 0 && surface_region.height > 0)
        pipeline_param.surface_region = &surface_region;

    // pipeline_param.filter_flags = VA_FILTER_SCALING_HQ; // High-quality scaling method
    pipeline_param.filter_flags = VA_FILTER_SCALING_DEFAULT;

    VA_CALL(vaCreateBuffer(va_display, va_context, VAProcPipelineParameterBufferType, sizeof(pipeline_param), 1,
                           &pipeline_param, &pipeline_param_buf_id));

    VA_CALL(vaBeginPicture(va_display, va_context, (VASurfaceID)dst->surface_id));

    VA_CALL(vaRenderPicture(va_display, va_context, &pipeline_param_buf_id, 1));

    VA_CALL(vaEndPicture(va_display, va_context));

    VA_CALL(vaDestroyBuffer(va_display, pipeline_param_buf_id));
}

static void VAAPIPreProcReleaseImage(PreProcContext *context, Image *image) {
    VAAPIPreProc *vaapi_pre_proc = (VAAPIPreProc *)context->priv;

    if (!vaapi_pre_proc)
        return;

    if (image->type == MEM_TYPE_VAAPI && image->surface_id && image->surface_id != VA_INVALID_ID) {
        VA_CALL(vaDestroySurfaces(vaapi_pre_proc->display, (uint32_t *)&image->surface_id, 1));
        image->type = MEM_TYPE_ANY;
    }
}

static void VAAPIPreProcDestroy(PreProcContext *context) {
    VAAPIPreProc *vaapi_pre_proc = (VAAPIPreProc *)context->priv;

    if (!vaapi_pre_proc)
        return;

    if (vaapi_pre_proc->va_context != VA_INVALID_ID) {
        vaDestroyContext(vaapi_pre_proc->display, vaapi_pre_proc->va_context);
        vaapi_pre_proc->va_context = VA_INVALID_ID;
    }

    if (vaapi_pre_proc->va_config != VA_INVALID_ID) {
        vaDestroyConfig(vaapi_pre_proc->display, vaapi_pre_proc->va_config);
        vaapi_pre_proc->va_config = VA_INVALID_ID;
    }

    if (vaapi_pre_proc->format_list) {
        free(vaapi_pre_proc->format_list);
        vaapi_pre_proc->format_list = NULL;
    }
}

typedef struct VAAPIImageMap {
    VADisplay va_display;
    VAImage va_image;
} VAAPIImageMap;

static Image VAAPIMap(ImageMapContext *context, const Image *image) {
    VAAPIImageMap *m = (VAAPIImageMap *)context->priv;
    VADisplay va_display = image->va_display;
    VAImage va_image = {};
    VAImageFormat va_format = {};
    void *surface_p = NULL;
    Image image_sys = {};

    assert(image->type == MEM_TYPE_VAAPI);

    if (image->format == VA_FOURCC_RGBP) {
        va_format = (VAImageFormat){.fourcc = (uint32_t)image->format,
                                    .byte_order = VA_LSB_FIRST,
                                    .bits_per_pixel = 24,
                                    .depth = 24,
                                    .red_mask = 0xff0000,
                                    .green_mask = 0xff00,
                                    .blue_mask = 0xff,
                                    .alpha_mask = 0,
                                    .va_reserved = {}};
    }

    // VA_CALL(vaSyncSurface(va_display, image->surface_id));

    if (va_format.fourcc &&
        vaCreateImage(va_display, &va_format, image->width, image->height, &va_image) == VA_STATUS_SUCCESS) {
        VA_CALL(vaGetImage(va_display, image->surface_id, 0, 0, image->width, image->height, va_image.image_id));
    } else {
        VA_CALL(vaDeriveImage(va_display, image->surface_id, &va_image));
    }

    VA_CALL(vaMapBuffer(va_display, va_image.buf, &surface_p));

    image_sys.type = MEM_TYPE_SYSTEM;
    image_sys.width = image->width;
    image_sys.height = image->height;
    image_sys.format = image->format;
    for (uint32_t i = 0; i < va_image.num_planes; i++) {
        image_sys.planes[i] = (uint8_t *)surface_p + va_image.offsets[i];
        image_sys.stride[i] = va_image.pitches[i];
    }

    m->va_display = va_display;
    m->va_image = va_image;
    return image_sys;
}

static void VAAPIUnmap(ImageMapContext *context) {
    VAAPIImageMap *m = (VAAPIImageMap *)context->priv;
    if (m->va_display) {
        VA_CALL(vaUnmapBuffer(m->va_display, m->va_image.buf));
        VA_CALL(vaDestroyImage(m->va_display, m->va_image.image_id));
    }
}

ImageMap image_map_vaapi = {
    .name = "vaapi",
    .priv_size = sizeof(VAAPIImageMap),
    .Map = VAAPIMap,
    .Unmap = VAAPIUnmap,
};

PreProc pre_proc_vaapi = {
    .name = "vaapi",
    .priv_size = sizeof(VAAPIPreProc),
    .mem_type = MEM_TYPE_VAAPI,
    .Init = VAAPIPreProcInit,
    .Convert = VAAPIPreProcConvert,
    .ReleaseImage = VAAPIPreProcReleaseImage,
    .Destroy = VAAPIPreProcDestroy,
};

#endif // #if CONFIG_VAAPI