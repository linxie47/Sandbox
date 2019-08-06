/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "image_inference.h"
#include <limits.h>
#include <string.h>

extern OutputBlobMethod output_blob_method_openvino;
extern ImageInference image_inference_openvino;
extern ImageInference image_inference_async_preproc;

static const ImageInference *const image_inference_list[] = {&image_inference_openvino, &image_inference_async_preproc,
                                                             NULL};
static const OutputBlobMethod *const output_blob_method_list[] = {&output_blob_method_openvino, NULL};

static const ImageInference *image_inference_iterate(void **opaque) {
    uintptr_t i = (uintptr_t)*opaque;
    const ImageInference *ii = image_inference_list[i];

    if (ii != NULL)
        *opaque = (void *)(i + 1);

    return ii;
}

static const OutputBlobMethod *output_blob_iterate(void **opaque) {
    uintptr_t i = (uintptr_t)*opaque;
    const OutputBlobMethod *obm = output_blob_method_list[i];

    if (obm != NULL)
        *opaque = (void *)(i + 1);

    return obm;
}

const ImageInference *image_inference_get_by_name(const char *name) {
    const ImageInference *ii = NULL;
    void *opaque = 0;

    if (name == NULL)
        return NULL;

    while ((ii = image_inference_iterate(&opaque)))
        if (!strcmp(ii->name, name))
            return (ImageInference *)ii;

    return NULL;
}

ImageInferenceContext *image_inference_alloc(const ImageInference *infernce, const OutputBlobMethod *obm,
                                             const char *instance_name) {
    ImageInferenceContext *ret;

    if (infernce == NULL)
        return NULL;

    ret = (ImageInferenceContext *)malloc(sizeof(*ret));
    if (!ret)
        return NULL;
    memset(ret, 0, sizeof(*ret));

    ret->inference = infernce;
    ret->output_blob_method = obm;
    ret->name = instance_name ? strdup(instance_name) : NULL;
    if (infernce->priv_size > 0) {
        ret->priv = malloc(infernce->priv_size);
        if (!ret->priv)
            goto err;
        memset(ret->priv, 0, infernce->priv_size);
    }

    return ret;
err:
    free(ret->priv);
    free(ret);
    return NULL;
}

void image_inference_free(ImageInferenceContext *inference_context) {
    if (inference_context == NULL)
        return;

    if (inference_context->priv)
        free(inference_context->priv);
    if (inference_context->name)
        free(inference_context->name);
    free(inference_context);
}

const OutputBlobMethod *output_blob_method_get_by_name(const char *name) {
    const OutputBlobMethod *ob = NULL;
    void *opaque = 0;

    if (name == NULL)
        return NULL;

    while ((ob = output_blob_iterate(&opaque)))
        if (!strcmp(ob->name, name))
            return (OutputBlobMethod *)ob;

    return NULL;
}

OutputBlobContext *output_blob_alloc(const OutputBlobMethod *obm) {
    OutputBlobContext *ret;

    if (obm == NULL)
        return NULL;

    ret = (OutputBlobContext *)malloc(sizeof(*ret));
    if (!ret)
        return NULL;
    memset(ret, 0, sizeof(*ret));

    ret->output_blob_method = obm;
    if (obm->priv_size > 0) {
        ret->priv = malloc(obm->priv_size);
        if (!ret->priv)
            goto err;
        memset(ret->priv, 0, obm->priv_size);
    }

    return ret;
err:
    free(ret->priv);
    free(ret);
    return NULL;
}

void output_blob_free(OutputBlobContext *context) {
    if (context == NULL)
        return;

    if (context->priv)
        free(context->priv);
    free(context);
}

#define FF_DYNARRAY_ADD(av_size_max, av_elt_size, av_array, av_size, av_success, av_failure)                           \
    do {                                                                                                               \
        size_t av_size_new = (av_size);                                                                                \
        if (!((av_size) & ((av_size)-1))) {                                                                            \
            av_size_new = (av_size) ? (av_size) << 1 : 1;                                                              \
            if (av_size_new > (av_size_max) / (av_elt_size)) {                                                         \
                av_size_new = 0;                                                                                       \
            } else {                                                                                                   \
                void *av_array_new = realloc((av_array), av_size_new * (av_elt_size));                                 \
                if (!av_array_new)                                                                                     \
                    av_size_new = 0;                                                                                   \
                else                                                                                                   \
                    (av_array) = (void **)av_array_new;                                                                \
            }                                                                                                          \
        }                                                                                                              \
        if (av_size_new) {                                                                                             \
            {av_success}(av_size)++;                                                                                   \
        } else {                                                                                                       \
            av_failure                                                                                                 \
        }                                                                                                              \
    } while (0)

static void ii_freep(void *arg) {
    void *val;

    memcpy(&val, arg, sizeof(val));
    memcpy(arg, &(void *){NULL}, sizeof(val));
    free(val);
}

void image_inference_dynarray_add(void *tab_ptr, int *nb_ptr, void *elem) {
    void **tab;
    memcpy(&tab, tab_ptr, sizeof(tab));

    FF_DYNARRAY_ADD(INT_MAX, sizeof(*tab), tab, *nb_ptr,
                    {
                        tab[*nb_ptr] = elem;
                        memcpy(tab_ptr, &tab, sizeof(tab));
                    },
                    {
                        *nb_ptr = 0;
                        ii_freep(tab_ptr);
                    });
}