#pragma once

#include <uru/type/queue.h>
#include <stdint.h>

typedef struct uru_flag_s uru_flag_t;
typedef struct uru_flag_pool_s uru_flag_pool_t;

struct uru_flag_s {
    volatile int64_t counter;
    uru_queue_t* queue;

    /**
     * @brief The signal will be triggered only after `num_event` events.
     */
    uint32_t num_event;

    int32_t idx; /**< Used for signal pool */
};

struct uru_flag_pool_s {
    uint32_t size;
    uint32_t used;
    uru_flag_t* arr;
    uru_flag_t** rmt_base;
    pthread_spinlock_t lock;
};

extern uru_flag_pool_t *flag_pool;

/* It should be called in URI channel initialization. */
void init_flag_pool();

uru_flag_t* uru_flag_pool_alloc();