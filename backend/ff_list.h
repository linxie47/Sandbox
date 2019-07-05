/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#ifndef __FF_LIST_H
#define __FF_LIST_H

typedef struct __ff_list {
    const void *opaque; // private data

    void *(*pop_back)(void *thiz);

    void *(*pop_front)(void *thiz);

    void (*push_back)(void *thiz, void *item);

    void (*push_front)(void *thiz, void *item);

    void *(*front)(void *thiz);

    int (*empty)(void *thiz);
} ff_list_t;

ff_list_t *ff_list_alloc();
void ff_list_free(ff_list_t *thiz);

#endif // __FF_LIST_H
