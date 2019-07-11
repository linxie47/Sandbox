/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "ff_list.h"
#include "queue.c"

static void *_ff_list_pop_back(void *thiz) {
    return queue_pop_back((queue_t *)((ff_list_t *)thiz)->opaque);
}

static void *_ff_list_pop_front(void *thiz) {
    return queue_pop_front((queue_t *)((ff_list_t *)thiz)->opaque);
}

static void _ff_list_push_back(void *thiz, void *item) {
    queue_push_back((queue_t *)((ff_list_t *)thiz)->opaque, item);
}

static void _ff_list_push_front(void *thiz, void *item) {
    queue_push_front((queue_t *)((ff_list_t *)thiz)->opaque, item);
}

static void *_ff_list_front(void *thiz) {
    return queue_peek_front((queue_t *)((ff_list_t *)thiz)->opaque);
}

static int _ff_list_empty(void *thiz) {
    return queue_count((queue_t *)((ff_list_t *)thiz)->opaque) == 0;
}

static iterator _ff_list_iterator_get(void *thiz) {
    return queue_iterate((queue_t *)((ff_list_t *)thiz)->opaque);
}

static iterator _ff_list_iterate_next(void *thiz, iterator it) {
    return queue_iterate_next((queue_t *)((ff_list_t *)thiz)->opaque, (queue_entry_t *)it);
}

static void *_ff_list_iterate_value(iterator it) {
    return queue_iterate_value((queue_entry_t *)it);
}

static size_t _ff_list_size(void *thiz) {
    return queue_count((queue_t *)((ff_list_t *)thiz)->opaque);
}

ff_list_t *ff_list_alloc(void) {
    ff_list_t *thiz = (ff_list_t *)malloc(sizeof(*thiz));
    if (!thiz)
        return NULL;

    queue_t *q = queue_create();
    assert(q);

    thiz->opaque = q;
    thiz->size = _ff_list_size;
    thiz->empty = _ff_list_empty;
    thiz->front = _ff_list_front;
    thiz->pop_back = _ff_list_pop_back;
    thiz->pop_front = _ff_list_pop_front;
    thiz->push_back = _ff_list_push_back;
    thiz->push_front = _ff_list_push_front;
    thiz->iterator_get = _ff_list_iterator_get;
    thiz->iterate_next = _ff_list_iterate_next;
    thiz->iterate_value = _ff_list_iterate_value;

    return thiz;
}

void ff_list_free(ff_list_t *thiz) {
    if (!thiz)
        return;

    queue_destroy((queue_t *)thiz->opaque);
    free(thiz);
}