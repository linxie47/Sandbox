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

#ifndef __FF_LIST_H
#define __FF_LIST_H

typedef void *iterator;

typedef struct __ff_list {
    const void *opaque; // private data

    void *(*pop_back)(void *thiz);

    void *(*pop_front)(void *thiz);

    void (*push_back)(void *thiz, void *item);

    void (*push_front)(void *thiz, void *item);

    void *(*front)(void *thiz);

    void *(*next)(void *thiz, void *current);

    int (*empty)(void *thiz);

    unsigned long (*size)(void *thiz);

    iterator (*iterator_get)(void *thiz);

    iterator (*iterate_next)(void *thiz, iterator it);

    void *(*iterate_value)(iterator it);
} ff_list_t;

ff_list_t *ff_list_alloc(void);
void ff_list_free(ff_list_t *thiz);

#endif // __FF_LIST_H
