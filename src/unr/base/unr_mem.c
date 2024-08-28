#include <unr/base/unr_def.h>
#include <uru/sys/realloc.h>
#include <string.h>

uru_status_t unr_mem_reg(void* addr, size_t size, unr_mem_h* mem_h) {
    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    URU_ASSERT_REASON(size > 0, "Invalid size: %zu", size);

    if (unr_channels->loc_mem_arr_len % 128 == 0) {
        size_t new_size = unr_channels->loc_mem_arr_len + 128;
        unr_channels->loc_mem_arr = uru_realloc(unr_channels->loc_mem_arr, 
            sizeof(unr_mem_h) * unr_channels->loc_mem_arr_len,
            sizeof(unr_mem_h) * new_size);
    }
    unr_channels->loc_mem_arr_len++;
    unr_channels->loc_mem_arr[unr_channels->loc_mem_arr_len - 1] = (unr_mem_h)uru_calloc(1, unr_channels->unr_mem_t_size);
    *mem_h = unr_channels->loc_mem_arr[unr_channels->loc_mem_arr_len - 1];
    unr_mem_t* mem_p = (unr_mem_t*)(*mem_h);

    mem_p->addr = addr;
    mem_p->size = size;
    mem_p->rank = uru_mpi->rank;
    mem_p->idx = unr_channels->loc_mem_arr_len - 1;
    
    for (int k = 1; k >= 0; k--) {
        int num_channels = k ? unr_channels->num_inter_channels : unr_channels->num_intra_channels;
        uri_channel_h* channels = k ? unr_channels->inter_channels : unr_channels->intra_channels;
        size_t* offset = k ? unr_channels->inter_uri_mem_h_offset : unr_channels->intra_uri_mem_h_offset;
        for (int i = 0; i < num_channels; i++) {
            if (k || unr_channels->intra_channel_reuse[i] == -1) {
                uri_channel_h channel = channels[i];
                uri_mem_h uri_mem_h = ((void*)mem_p) + offset[i];
                uri_mem_reg(channel, addr, size, uri_mem_h);
            }
        }
    }

    return URU_STATUS_OK;
}

uru_status_t unr_mem_reg_sync() {
    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);

    MPI_Allgather(&unr_channels->loc_mem_arr_len, 1, MPI_INT, unr_channels->rmt_mem_arr_len, 1, MPI_INT, MPI_COMM_WORLD);
    int* displs = (int*)uru_malloc(uru_mpi->size * sizeof(int));
    displs[0] = 0;
    for (int i = 1; i < uru_mpi->size; i++) {
        displs[i] = displs[i - 1] + unr_channels->rmt_mem_arr_len[i - 1];
    }
    size_t total_count = displs[uru_mpi->size - 1] + unr_channels->rmt_mem_arr_len[uru_mpi->size - 1];
    void *send_buf = uru_malloc(unr_channels->loc_mem_arr_len * unr_channels->unr_mem_t_size);
    static void *recv_buf = NULL;
    if (recv_buf) {
        free(recv_buf);
    }
    recv_buf = uru_malloc(total_count * unr_channels->unr_mem_t_size);
    for (int i = 0; i < unr_channels->loc_mem_arr_len; i++) {
        memcpy(send_buf + i * unr_channels->unr_mem_t_size, unr_channels->loc_mem_arr[i], unr_channels->unr_mem_t_size);
    }
    MPI_Allgatherv(
        send_buf, unr_channels->loc_mem_arr_len, MPI_UNR_MEM_H, 
        recv_buf, unr_channels->rmt_mem_arr_len, displs, MPI_UNR_MEM_H, MPI_COMM_WORLD);
    for (int i = 0; i < uru_mpi->size; i++) {
        if (unr_channels->rmt_mem_arr[i]) {
            free(unr_channels->rmt_mem_arr[i]);
        }
        unr_channels->rmt_mem_arr[i] = (unr_mem_h*)uru_malloc(unr_channels->rmt_mem_arr_len[i] * sizeof(unr_mem_h));
        for (int j = 0; j < unr_channels->rmt_mem_arr_len[i]; j++) {
            unr_channels->rmt_mem_arr[i][j] = recv_buf + (displs[i] + j) * unr_channels->unr_mem_t_size;
        }
    }
    free(displs);
    free(send_buf);

    return URU_STATUS_OK;
}