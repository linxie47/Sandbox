/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

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
