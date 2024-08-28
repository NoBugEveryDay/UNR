#include <uru/dev/dev.h>
#include <uru/mpi/mpi.h>
#include <uru/sys/preprocessor.h>
#include <uru/sys/realloc.h>
#include <uru/type/flag.h>
#include <mpi.h>
#include <pthread.h>
#include <stdlib.h>

uru_flag_pool_t *flag_pool = NULL;

void init_flag_pool() {
    static int initialized = 0;
    if (initialized) {
        return;
    }
    char* env = getenv("URU_NUM_FLAGS");
    size_t num_flags;
    if (env != NULL) {
        num_flags = atoi(env);
        URU_ASSERT(0 < num_flags && num_flags <= ((uint64_t)1<<32));
    } else {
        num_flags = 65536;
    }

    flag_pool = (uru_flag_pool_t*)uru_malloc(sizeof(uru_flag_pool_t));
    flag_pool->arr = (uru_flag_t*)uru_calloc(num_flags, sizeof(uru_flag_t));
    flag_pool->rmt_base = (uru_flag_t**)uru_malloc(sizeof(uru_flag_t*) * uru_mpi->size);
    flag_pool->size = num_flags;
    flag_pool->used = 1; /* 0 is reserved */
    pthread_spin_init(&(flag_pool->lock), PTHREAD_PROCESS_PRIVATE);
    MPI_Allgather(&(flag_pool->arr), sizeof(uru_flag_t*), MPI_BYTE, flag_pool->rmt_base, sizeof(uru_flag_t*), MPI_BYTE, MPI_COMM_WORLD);
}

uru_flag_t* uru_flag_pool_alloc() {
    uru_flag_t* ret;
    if (flag_pool) {
        pthread_spin_lock(&(flag_pool->lock));
        if (flag_pool->used == flag_pool->size) {
            printf("Please enlarge environment variable URU_NUM_FLAGS\n");
            exit(URU_ERROR_EXIT);
        }
        ret = &(flag_pool->arr[flag_pool->used]);
        flag_pool->used++;
        pthread_spin_unlock(&(flag_pool->lock));
    }
    else {
        ret = (uru_flag_t*)uru_calloc(1, sizeof(uru_flag_t));
    }
    return ret;
}