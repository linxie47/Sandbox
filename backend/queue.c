#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _queue_entry queue_entry_t;
typedef struct _queue queue_t;

struct _queue_entry {
    void *value;
    queue_entry_t *prev;
    queue_entry_t *next;
    queue_t *queue;
};

struct _queue {
    queue_entry_t *head;
    queue_entry_t *tail;
    size_t length;
};

static inline queue_entry_t *create_entry(queue_t *q) {
    queue_entry_t *new_entry = (queue_entry_t *)calloc(1, sizeof(queue_entry_t));

    assert(new_entry != NULL);

    new_entry->queue = q;
    return new_entry;
}

static queue_entry_t *queue_iterate(queue_t *q) {
    queue_entry_t *it = q->head->next;
    return it == q->tail ? NULL : it;
}

static queue_entry_t *queue_iterate_next(queue_t *q, queue_entry_t *it) {
    queue_entry_t *next = it->next;
    return next == q->tail ? NULL : next;
}

static void *queue_iterate_value(queue_entry_t *it) {
    return it->value;
}

static queue_t *queue_create(void) {
    queue_t *q = (queue_t *)malloc(sizeof(queue_t));
    if (!q)
        return NULL;

    memset(q, 0, sizeof(queue_t));
    q->head = create_entry(q);
    q->tail = create_entry(q);
    q->head->next = q->tail;
    q->tail->prev = q->head;
    q->head->prev = NULL;
    q->tail->next = NULL;

    return q;
}

static void queue_destroy(queue_t *q) {
    queue_entry_t *entry;
    if (!q)
        return;

    entry = q->head;
    while (entry != NULL) {
        queue_entry_t *temp = entry;
        entry = entry->next;
        free(temp);
    }

    q->head = NULL;
    q->tail = NULL;
    q->length = 0;
    free(q);
}

static size_t queue_count(queue_t *q) {
    return q ? q->length : 0;
}

static void queue_push_front(queue_t *q, void *val) {
    queue_entry_t *new_node = create_entry(q);
    queue_entry_t *original_next = q->head->next;

    new_node->value = val;

    q->head->next = new_node;
    original_next->prev = new_node;
    new_node->prev = q->head;
    new_node->next = original_next;
    q->length++;
}

static void queue_push_back(queue_t *q, void *val) {
    queue_entry_t *new_node = create_entry(q);
    queue_entry_t *original_prev = q->tail->prev;

    new_node->value = val;

    q->tail->prev = new_node;
    original_prev->next = new_node;
    new_node->next = q->tail;
    new_node->prev = original_prev;
    q->length++;
}

static void *queue_pop_front(queue_t *q) {
    queue_entry_t *front = q->head->next;
    queue_entry_t *new_head_next = front->next;
    void *ret = front->value;

    if (q->length == 0)
        return NULL;

    q->head->next = new_head_next;
    new_head_next->prev = q->head;
    free(front);
    q->length--;
    return ret;
}

static void *queue_pop_back(queue_t *q) {
    queue_entry_t *back = q->tail->prev;
    queue_entry_t *new_tail_prev = back->prev;
    void *ret = back->value;

    if (q->length == 0)
        return NULL;

    q->tail->prev = new_tail_prev;
    new_tail_prev->next = q->tail;
    free(back);
    q->length--;
    return ret;
}

static void *queue_peek_front(queue_t *q) {
    if (!q || q->length == 0)
        return NULL;

    return q->head->next->value;
}

static void *queue_peek_back(queue_t *q) {
    if (!q || q->length == 0)
        return NULL;

    return q->tail->prev->value;
}
