#include <uru/dev/dev.h>
#include <uru/sys/realloc.h>
#include <uru/type/queue.h>
#include <stdlib.h>

uru_status_t uru_queue_init(uru_queue_t* queue, int32_t queue_size) {
    queue->head = 0;
    queue->tail = 0;
    queue->size = queue_size + 1;
    pthread_spin_init(&(queue->lock), PTHREAD_PROCESS_PRIVATE);
    queue->buff = (int32_t*)uru_malloc(sizeof(int32_t) * queue->size);
    return URU_STATUS_OK;
}

uru_status_t uru_queue_push(uru_queue_t* queue, int32_t data, int32_t need_lock) {
    if (need_lock) {
        pthread_spin_lock(&(queue->lock));
    }
    URU_ASSERT_REASON(queue->head != (queue->tail + 1) % queue->size, "queue is full");
    queue->buff[queue->tail] = data;
    queue->tail = (queue->tail + 1) % queue->size;
    if (need_lock) {
        pthread_spin_unlock(&(queue->lock));
    }
    return URU_STATUS_OK;
}

uru_status_t uru_queue_pop(uru_queue_t* queue, int32_t* data, int32_t need_lock) {
    volatile int32_t* head = &(queue->head);
    volatile int32_t* tail = &(queue->tail);
    if (*head == *tail) {
        *data = -1;
        return URU_STATUS_OK;
    }
    if (need_lock) {
        pthread_spin_lock(&(queue->lock));
    }
    if (*head == *tail) {
        *data = -1;
    } else {
        *data = queue->buff[*head];
        *head = ((*head) + 1) % queue->size;
    }
    if (need_lock) {
        pthread_spin_unlock(&(queue->lock));
    }
    return URU_STATUS_OK;
}