#include <uri/mpi/uri_mpi.h>
#include <stdint.h>

uru_status_t uri_mpi_int_pool_get(uri_mpi_int_pool_t* pool, int* ret) {
    pthread_spin_lock(&(pool->lock));
    if (pool->recycle_top) {
        pool->recycle_top--;
        *ret = pool->recycle_stack[pool->recycle_top];
    } else {
        *ret = pool->max;
        pool->max++;
        URU_ASSERT(pool->max < 2100000000/uru_mpi->size);
    }
    URU_ASSERT((0 <= (*ret)) && ((*ret) < 2100000000/uru_mpi->size));
    pthread_spin_unlock(&(pool->lock));
    *ret = (*ret)*uru_mpi->size + uru_mpi->rank;
    return URU_STATUS_OK;
}

uru_status_t uri_mpi_int_pool_return(uri_mpi_int_pool_t* pool, int num) {
    URU_ASSERT(num % uru_mpi->size == uru_mpi->rank);
    num = (num - uru_mpi->rank) / uru_mpi->size;
    pthread_spin_lock(&(pool->lock));
    if (pool->recycle_top == pool->recycle_len) {
        pool->recycle_stack = uru_realloc(pool->recycle_stack, sizeof(int) * pool->recycle_len, sizeof(int) * (2 * pool->recycle_len + 8));
        pool->recycle_len = 2 * pool->recycle_len + 8;
    }
    pool->recycle_stack[pool->recycle_top] = num;
    pool->recycle_top++;
    pthread_spin_unlock(&(pool->lock));
    return URU_STATUS_OK;
}

int uri_mpi_op_cmp(const void* a, const void* b) {
    const uri_mpi_op_t* op1 = (const uri_mpi_op_t*)a;
    const uri_mpi_op_t* op2 = (const uri_mpi_op_t*)b;
    return op1->rank - op2->rank;
}

uru_status_t uri_mpi_req_que_app(uri_channel_t* channel_p, size_t num_op, uri_mpi_op_t** mpi_op) {
    uri_mpi_channel_t* info = (uri_mpi_channel_t*)channel_p->info;
    pthread_spin_lock(&(info->mpi_req.lock));
    if (info->mpi_req.arr_tail + num_op > info->mpi_req.arr_len) {
        size_t new_size = (info->mpi_req.arr_tail + num_op) * 2;
        info->mpi_req.req_arr = uru_realloc(info->mpi_req.req_arr, info->mpi_req.arr_len * sizeof(MPI_Request), new_size * sizeof(MPI_Request));
        info->mpi_req.op_arr = uru_realloc(info->mpi_req.op_arr, info->mpi_req.arr_len * sizeof(uri_mpi_op_t*), new_size * sizeof(uri_mpi_op_t*));
        info->mpi_req.arr_len = new_size;
    }
    for (int i = 0; i < num_op; i++) {
        info->mpi_req.op_arr[info->mpi_req.arr_tail] = mpi_op[i];
        info->mpi_req.req_arr[info->mpi_req.arr_tail] = mpi_op[i]->mpi_req;
        info->mpi_req.arr_tail++;
    }
    pthread_spin_unlock(&(info->mpi_req.lock));
    return URU_STATUS_OK;
}