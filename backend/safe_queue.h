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
