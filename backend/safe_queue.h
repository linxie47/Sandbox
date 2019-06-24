#ifndef __SAFE_QUEUE_H
#define __SAFE_QUEUE_H

typedef struct _SafeQueue SafeQueueT;

SafeQueueT *SafeQueueCreate();

void SafeQueueDestroy(SafeQueueT *sq);

void SafeQueuePush(SafeQueueT *sq, void *t);

void SafeQueuePushFront(SafeQueueT *sq, void *t);

void *SafeQueueFront(SafeQueueT *sq);

void *SafeQueuePop(SafeQueueT *sq);

int SafeQueueEmpty(SafeQueueT *sq);

void SafeQueueWaitEmpty(SafeQueueT *sq);

// Debug only
void *SafeQueueActual(SafeQueueT *sq);

#endif // __SAFE_QUEUE_H
