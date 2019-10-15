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

#include "image.h"
#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern ImageMap image_map_vaapi;
extern ImageMap image_map_mocker;

static const ImageMap *const image_map_list[] = {
#if CONFIG_VAAPI
    &image_map_vaapi,
#endif
    &image_map_mocker, NULL};

static const ImageMap *image_map_iterate(void **opaque) {
    uintptr_t i = (uintptr_t)*opaque;
    const ImageMap *im = image_map_list[i];

    if (im != NULL)
        *opaque = (void *)(i + 1);

    return im;
}

const ImageMap *image_map_get_by_name(const char *name) {
    const ImageMap *im = NULL;
    void *opaque = 0;

    if (name == NULL)
        return NULL;

    while ((im = image_map_iterate(&opaque)))
        if (!strcmp(im->name, name))
            return im;

    return NULL;
}

ImageMapContext *image_map_alloc(const ImageMap *image_map) {
    ImageMapContext *ret = NULL;

    if (image_map == NULL)
        return NULL;

    ret = (ImageMapContext *)malloc(sizeof(*ret));
    assert(ret);
    memset(ret, 0, sizeof(*ret));

    ret->mapper = image_map;
    if (image_map->priv_size > 0) {
        ret->priv = malloc(image_map->priv_size);
        if (!ret->priv)
            goto err;
        memset(ret->priv, 0, image_map->priv_size);
    }

    return ret;
err:
    free(ret);
    return NULL;
}

void image_map_free(ImageMapContext *context) {
    if (context == NULL)
        return;

    if (context->priv)
        free(context->priv);
    free(context);
}
