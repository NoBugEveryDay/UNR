#pragma once
#include <uru/type/uru_status.h>
#include <pthread.h>
#include <stdint.h>

typedef struct uru_queue_s uru_queue_t;

struct uru_queue_s {
    pthread_spinlock_t lock;
    int32_t size;
    int32_t head;
    int32_t tail;
    int32_t* buff;
};

uru_status_t uru_queue_init(uru_queue_t* queue, int32_t queue_size);

uru_status_t uru_queue_push(uru_queue_t* queue, int32_t data, int32_t need_lock);

uru_status_t uru_queue_pop(uru_queue_t* queue, int32_t* data, int32_t need_lock);