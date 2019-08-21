/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#ifndef __SAFE_QUEUE_H
#define __SAFE_QUEUE_H

typedef struct _SafeQueue SafeQueueT;

SafeQueueT *SafeQueueCreate(void);

void SafeQueueDestroy(SafeQueueT *sq);

void SafeQueuePush(SafeQueueT *sq, void *t);

void SafeQueuePushFront(SafeQueueT *sq, void *t);

void *SafeQueueFront(SafeQueueT *sq);

void *SafeQueuePop(SafeQueueT *sq);

int SafeQueueEmpty(SafeQueueT *sq);

void SafeQueueWaitEmpty(SafeQueueT *sq);

// Debug only
int SafeQueueSize(SafeQueueT *sq);

#endif // __SAFE_QUEUE_H
