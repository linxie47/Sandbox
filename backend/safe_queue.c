/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "safe_queue.h"
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#include "queue.c"

#define mutex_t pthread_mutex_t
#define cond_t pthread_cond_t

#define mutex_init(m) pthread_mutex_init((m), NULL)

#define mutex_lock pthread_mutex_lock
#define mutex_unlock pthread_mutex_unlock
#define mutex_destroy pthread_mutex_destroy

#define cond_init(c) pthread_cond_init((c), NULL)
#define cond_signal pthread_cond_signal
#define cond_broadcast pthread_cond_broadcast
#define cond_wait pthread_cond_wait
#define cond_destroy pthread_cond_destroy

struct _SafeQueue {
    queue_t *q;

    mutex_t mutex;
    cond_t cond;
};

SafeQueueT *SafeQueueCreate() {
    SafeQueueT *sq = (SafeQueueT *)malloc(sizeof(SafeQueueT));
    if (!sq)
        return NULL;

    sq->q = queue_create();
    assert(sq->q);

    mutex_init(&sq->mutex);
    cond_init(&sq->cond);
    return sq;
}

void SafeQueueDestroy(SafeQueueT *sq) {
    if (!sq)
        return;

    mutex_lock(&sq->mutex);
    queue_destroy(sq->q);
    mutex_unlock(&sq->mutex);

    mutex_destroy(&sq->mutex);
    cond_destroy(&sq->cond);
    free(sq);
}

void SafeQueuePush(SafeQueueT *sq, void *t) {
    mutex_lock(&sq->mutex);
    queue_push_back(sq->q, t);
    cond_signal(&sq->cond);
    mutex_unlock(&sq->mutex);
}

void SafeQueuePushFront(SafeQueueT *sq, void *t) {
    mutex_lock(&sq->mutex);
    queue_push_front(sq->q, t);
    cond_signal(&sq->cond);
    mutex_unlock(&sq->mutex);
}

void *SafeQueueFront(SafeQueueT *sq) {
    void *value;
    mutex_lock(&sq->mutex);
    while (queue_count(sq->q) == 0) {
        cond_wait(&sq->cond, &sq->mutex);
    }
    value = queue_peek_front(sq->q);
    mutex_unlock(&sq->mutex);
    return value;
}

void *SafeQueuePop(SafeQueueT *sq) {
    void *value;
    mutex_lock(&sq->mutex);
    while (queue_count(sq->q) == 0) {
        cond_wait(&sq->cond, &sq->mutex);
    }
    value = queue_pop_front(sq->q);
    cond_signal(&sq->cond);
    mutex_unlock(&sq->mutex);
    return value;
}

int SafeQueueSize(SafeQueueT *sq) {
    int size = 0;
    mutex_lock(&sq->mutex);
    size = queue_count(sq->q);
    mutex_unlock(&sq->mutex);
    return size;
}

int SafeQueueEmpty(SafeQueueT *sq) {
    int empty = 0;
    mutex_lock(&sq->mutex);
    empty = (queue_count(sq->q) == 0);
    mutex_unlock(&sq->mutex);
    return empty;
}

void SafeQueueWaitEmpty(SafeQueueT *sq) {
    mutex_lock(&sq->mutex);
    while (queue_count(sq->q) != 0) {
        cond_wait(&sq->cond, &sq->mutex);
    }
    mutex_unlock(&sq->mutex);
}

#if 0

#include <stdio.h>

int main(int argc, char *argv[])
{
    SafeQueueT *queue = SafeQueueCreate();

    char TEST[] = {'A', 'B', 'C', 'D'};

    SafeQueuePush(queue, (void *)&TEST[0]);
    SafeQueuePush(queue, (void *)&TEST[1]);
    SafeQueuePush(queue, (void *)&TEST[2]);
    SafeQueuePush(queue, (void *)&TEST[3]);

    SafeQueuePushFront(queue, (void *)&TEST[0]);
    SafeQueuePushFront(queue, (void *)&TEST[1]);
    SafeQueuePushFront(queue, (void *)&TEST[2]);
    SafeQueuePushFront(queue, (void *)&TEST[3]);

    while (!SafeQueueEmpty(queue))
    {
        char *c = (char *)SafeQueueFront(queue);
        printf("%c\n", *c);
        c = SafeQueuePop(queue);
    }

    SafeQueueDestroy(queue);
    return 0;
}
#endif
