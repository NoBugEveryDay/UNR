#include <unr/base/unr_def.h>
#include <string.h>

uru_status_t unr_blk_reg(unr_mem_h mem_h, size_t offset, size_t size, unr_sig_h send_sig_h, unr_sig_h recv_sig_h, unr_blk_h* blk_h) {
    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    URU_ASSERT(size > 0);
    unr_mem_t* mem_p = (unr_mem_t*)mem_h;
    unr_blk_t* unr_blk = (unr_blk_t*)blk_h;
    unr_sig_t* send_sig_p = (unr_sig_t*)&send_sig_h;
    unr_sig_t* recv_sig_p = (unr_sig_t*)&recv_sig_h;

    memset(blk_h, 0, sizeof(unr_blk_t));
    unr_blk->dst_rank = uru_mpi->rank;
    unr_blk->mem_idx = mem_p->idx;
    unr_blk->offset = offset;
    unr_blk->size = size;
    if (send_sig_p->dst_rank != UNR_NO_SIGNAL_DST_RANK) {
        URU_ASSERT_REASON(uru_mpi->rank == send_sig_p->dst_rank, "The send signal should be a local signal (create on the same process).");
        unr_blk->send_flag_p = send_sig_p->flag_p;
    }
    if (recv_sig_p->dst_rank != UNR_NO_SIGNAL_DST_RANK) {
        URU_ASSERT_REASON(uru_mpi->rank == recv_sig_p->dst_rank, "The recv signal should be a local signal (create on the same process).");
        unr_blk->recv_flag_p = recv_sig_p->flag_p;
    }
    return URU_STATUS_OK;
}

uru_status_t unr_blk_rdma_start(
    unr_blk_h loc_blk_h, unr_sig_h loc_sig_h,
    unr_blk_h rmt_blk_h, unr_sig_h rmt_sig_h, uru_transfer_t dma_type) {

    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    unr_plan_h plan_h;
    unr_rdma_plan_create(
        0, 1, 1, 1,
        (unr_blk_t*)&loc_blk_h, (unr_sig_t*)&loc_sig_h, NULL, NULL,
        (unr_blk_t*)&rmt_blk_h, (unr_sig_t*)&rmt_sig_h, NULL, &dma_type,
        (unr_plan_t**)(&plan_h));
    return URU_STATUS_OK;
}

uru_status_t unr_blk_rdma_plan(
    unr_blk_h loc_blk_h, unr_sig_h loc_sig_h,
    unr_blk_h rmt_blk_h, unr_sig_h rmt_sig_h, uru_transfer_t dma_type,
    unr_plan_h* plan_h) {

    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    unr_rdma_plan_create(
        0, 0, 0, 1,
        (unr_blk_t*)&loc_blk_h, (unr_sig_t*)&loc_sig_h, NULL, NULL,
        (unr_blk_t*)&rmt_blk_h, (unr_sig_t*)&rmt_sig_h, NULL, &dma_type,
        (unr_plan_t**)plan_h);
    return URU_STATUS_OK;
}

uru_status_t unr_blk_part_rdma_start(
    unr_blk_h loc_blk_h, unr_sig_h loc_sig_h, size_t loc_offset, size_t size,
    unr_blk_h rmt_blk_h, unr_sig_h rmt_sig_h, size_t rmt_offset, uru_transfer_t dma_type) {

    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    unr_plan_h plan_h;
    unr_rdma_plan_create(
        0, 1, 1, 1,
        (unr_blk_t*)&loc_blk_h, (unr_sig_t*)&loc_sig_h, &loc_offset, &size,
        (unr_blk_t*)&rmt_blk_h, (unr_sig_t*)&rmt_sig_h, &rmt_offset, &dma_type,
        (unr_plan_t**)(&plan_h));
    return URU_STATUS_OK;
}

uru_status_t unr_blk_part_rdma_plan(
    unr_blk_h loc_blk_h, unr_sig_h loc_sig_h, size_t loc_offset, size_t size,
    unr_blk_h rmt_blk_h, unr_sig_h rmt_sig_h, size_t rmt_offset, uru_transfer_t dma_type,
    unr_plan_h* plan_h) {

    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    unr_rdma_plan_create(
        0, 0, 0, 1,
        (unr_blk_t*)&loc_blk_h, (unr_sig_t*)&loc_sig_h, &loc_offset, &size,
        (unr_blk_t*)&rmt_blk_h, (unr_sig_t*)&rmt_sig_h, &rmt_offset, &dma_type,
        (unr_plan_t**)plan_h);
    return URU_STATUS_OK;
}

uru_status_t unr_blk_rdma_batch_start(
    size_t num_sends,
    unr_blk_h* loc_blk_h_arr, unr_sig_h* loc_sig_h_arr,
    unr_blk_h* rmt_blk_h_arr, unr_sig_h* rmt_sig_h_arr, uru_transfer_t* dma_type_arr) {

    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    unr_plan_h plan_h;
    unr_rdma_plan_create(
        0, 1, 1, num_sends,
        (unr_blk_t*)loc_blk_h_arr, (unr_sig_t*)loc_sig_h_arr, NULL, NULL,
        (unr_blk_t*)rmt_blk_h_arr, (unr_sig_t*)rmt_sig_h_arr, NULL, dma_type_arr,
        (unr_plan_t**)(&plan_h));
    return URU_STATUS_OK;
}

uru_status_t unr_blk_rdma_batch_plan(
    size_t num_sends,
    unr_blk_h* loc_blk_h_arr, unr_sig_h* loc_sig_h_arr,
    unr_blk_h* rmt_blk_h_arr, unr_sig_h* rmt_sig_h_arr, uru_transfer_t* dma_type_arr,
    unr_plan_h* plan_h) {

    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    unr_rdma_plan_create(
        0, 1, 0, num_sends,
        (unr_blk_t*)loc_blk_h_arr, (unr_sig_t*)loc_sig_h_arr, NULL, NULL,
        (unr_blk_t*)rmt_blk_h_arr, (unr_sig_t*)rmt_sig_h_arr, NULL, dma_type_arr,
        (unr_plan_t**)plan_h);
    return URU_STATUS_OK;
}

uru_status_t unr_blk_part_rdma_batch_start(
    size_t num_sends,
    unr_blk_h* loc_blk_h_arr, unr_sig_h* loc_sig_h_arr, size_t* loc_offset, size_t* size,
    unr_blk_h* rmt_blk_h_arr, unr_sig_h* rmt_sig_h_arr, size_t* rmt_offset, uru_transfer_t* dma_type_arr) {

    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    unr_plan_h plan_h;
    unr_rdma_plan_create(
        0, 1, 1, num_sends,
        (unr_blk_t*)loc_blk_h_arr, (unr_sig_t*)loc_sig_h_arr, loc_offset, size,
        (unr_blk_t*)rmt_blk_h_arr, (unr_sig_t*)rmt_sig_h_arr, rmt_offset, dma_type_arr,
        (unr_plan_t**)(&plan_h));
    return URU_STATUS_OK;
}

uru_status_t unr_blk_part_rdma_batch_plan(
    size_t num_sends,
    unr_blk_h* loc_blk_h_arr, unr_sig_h* loc_sig_h_arr, size_t* loc_offset, size_t* size,
    unr_blk_h* rmt_blk_h_arr, unr_sig_h* rmt_sig_h_arr, size_t* rmt_offset, uru_transfer_t* dma_type_arr,
    unr_plan_h* plan_h) {

    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    unr_rdma_plan_create(
        0, 1, 0, num_sends,
        (unr_blk_t*)loc_blk_h_arr, (unr_sig_t*)loc_sig_h_arr, loc_offset, size,
        (unr_blk_t*)rmt_blk_h_arr, (unr_sig_t*)rmt_sig_h_arr, rmt_offset, dma_type_arr,
        (unr_plan_t**)plan_h);
    return URU_STATUS_OK;
}

/**
 * @details We only support use the first inter/intra-node channel to receive data currently.
 */
uru_status_t unr_rdma_plan_create(
    uint8_t create_later, uint8_t need_current_load, uint8_t once_now, size_t num_sends,
    unr_blk_t* loc_blk_arr, unr_sig_t* loc_sig_arr, size_t* loc_offset_arr, size_t* size_arr,
    unr_blk_t* rmt_blk_arr, unr_sig_t* rmt_sig_arr, size_t* rmt_offset_arr, uru_transfer_t* dma_type_arr,
    unr_plan_t** unr_plan_pp) {

    unr_plan_t* unr_plan_p;

    if (create_later) {
        /* If the plan is decided to be fully initialized in the next call of this function in previous call,
           recover all the parameters. */
        unr_plan_p = *unr_plan_pp;
        unr_plan_p->status = 2;
        need_current_load = unr_plan_p->para->need_current_load;
        once_now = unr_plan_p->para->once_now;
        num_sends = unr_plan_p->para->num_sends;
        loc_blk_arr = unr_plan_p->para->loc_blk_arr;
        loc_sig_arr = unr_plan_p->para->loc_sig_arr;
        loc_offset_arr = unr_plan_p->para->loc_offset_arr;
        size_arr = unr_plan_p->para->size_arr;
        rmt_blk_arr = unr_plan_p->para->rmt_blk_arr;
        rmt_sig_arr = unr_plan_p->para->rmt_sig_arr;
        rmt_offset_arr = unr_plan_p->para->rmt_offset_arr;
        dma_type_arr = unr_plan_p->para->dma_type_arr;
    } else {
        /* If it is the first time to call this function for this plan, we alloc space for it */
        unr_plan_p = (unr_plan_t*)uru_calloc(sizeof(unr_plan_t), 1);
        *unr_plan_pp = unr_plan_p;
        unr_plan_p->status = 1;

        /* Check whether the unr_mem_reg_sync() is called before this function */
        int mem_synced = 1;
        for (int i = 0; i < num_sends; i++) {
            if (rmt_blk_arr[i].mem_idx >= unr_channels->rmt_mem_arr_len[rmt_blk_arr[i].dst_rank]) {
                mem_synced = 0;
                break;
            }
        }

        if (mem_synced == 0) {
            /* If the memory handles have NOT synced yet, we make a deep copy of parameters and create plan later */
            if (once_now == 1) {
                printf("You should call unr_mem_reg_sync() before start RDMA Write communication!\n");
                exit(URU_ERROR_EXIT);
            }
            unr_plan_p->para = (unr_plan_create_para_t*)uru_calloc(1, sizeof(unr_plan_create_para_t));
            unr_plan_p->para->need_current_load = need_current_load;
            unr_plan_p->para->once_now = once_now;
            unr_plan_p->para->num_sends = num_sends;
            URU_ASSERT(num_sends > 0);
            unr_plan_p->para->loc_blk_arr = (unr_blk_t*)uru_malloc(sizeof(unr_blk_t) * num_sends);
            unr_plan_p->para->rmt_blk_arr = (unr_blk_t*)uru_malloc(sizeof(unr_blk_t) * num_sends);
            memcpy(unr_plan_p->para->loc_blk_arr, loc_blk_arr, sizeof(unr_blk_t) * num_sends);
            memcpy(unr_plan_p->para->rmt_blk_arr, rmt_blk_arr, sizeof(unr_blk_t) * num_sends);
            if (loc_sig_arr == NULL) {
                unr_plan_p->para->loc_sig_arr = NULL;
            } else {
                unr_plan_p->para->loc_sig_arr = (unr_sig_t*)uru_malloc(sizeof(unr_sig_t) * num_sends);
                memcpy(unr_plan_p->para->loc_sig_arr, loc_sig_arr, sizeof(unr_sig_t) * num_sends);
            }
            if (rmt_sig_arr == NULL) {
                unr_plan_p->para->rmt_sig_arr = NULL;
            } else {
                unr_plan_p->para->rmt_sig_arr = (unr_sig_t*)uru_malloc(sizeof(unr_sig_t) * num_sends);
                memcpy(unr_plan_p->para->rmt_sig_arr, rmt_sig_arr, sizeof(unr_sig_t) * num_sends);
            }
            if (loc_offset_arr == NULL) {
                unr_plan_p->para->loc_offset_arr = NULL;
            } else {
                unr_plan_p->para->loc_offset_arr = (size_t*)uru_malloc(sizeof(size_t) * num_sends);
                memcpy(unr_plan_p->para->loc_offset_arr, loc_offset_arr, sizeof(size_t) * num_sends);
            }
            if (rmt_offset_arr == NULL) {
                unr_plan_p->para->rmt_offset_arr = NULL;
            } else {
                unr_plan_p->para->rmt_offset_arr = (size_t*)uru_malloc(sizeof(size_t) * num_sends);
                memcpy(unr_plan_p->para->rmt_offset_arr, rmt_offset_arr, sizeof(size_t) * num_sends);
            }
            if (size_arr == NULL) {
                unr_plan_p->para->size_arr = NULL;
            } else {
                unr_plan_p->para->size_arr = (size_t*)uru_malloc(sizeof(size_t) * num_sends);
                memcpy(unr_plan_p->para->size_arr, size_arr, sizeof(size_t) * num_sends);
            }
            if (dma_type_arr == URU_TRANSFER_TYPE_DMA_PUT_ALL || dma_type_arr == URU_TRANSFER_TYPE_DMA_GET_ALL) {
                unr_plan_p->para->dma_type_arr = dma_type_arr;
            } else {
                unr_plan_p->para->dma_type_arr = (uru_transfer_t*)uru_malloc(sizeof(uru_transfer_t) * num_sends);
                memcpy(unr_plan_p->para->dma_type_arr, dma_type_arr, sizeof(uru_transfer_t) * num_sends);
            }
            return URU_STATUS_OK;
        } else {
            /* If all the needed memory handles are ready, initialize the plan directly in the following code and NOT return. */
        }
    }

    unr_plan_p->status = 2;
    unr_plan_p->num_uri_plan = unr_channels->num_inter_channels + unr_channels->num_intra_channels;
    unr_plan_p->uri_plan_h_arr = (uri_plan_h*)uru_calloc(unr_plan_p->num_uri_plan, sizeof(uri_plan_h));
    unr_plan_p->uri_channel_h_arr = (uri_channel_h*)uru_calloc(unr_plan_p->num_uri_plan, sizeof(uri_channel_h));

    URU_ASSERT(num_sends > 0);

    size_t next_inter_channel_id = 0, next_intra_channel_id = 0, *next_channel_id;
    size_t* div_plan_wrt_size = (size_t*)uru_calloc(unr_plan_p->num_uri_plan, sizeof(size_t));
    size_t* div_plan_wrt_offset = (size_t*)uru_calloc(unr_plan_p->num_uri_plan, sizeof(size_t));
    size_t* uri_plan_num_wrts = (size_t*)uru_calloc(unr_plan_p->num_uri_plan, sizeof(size_t));
    int64_t* sig_add_arr = (int64_t*)uru_malloc(unr_plan_p->num_uri_plan * sizeof(int64_t));

    uri_mem_h** loc_uri_mem = (uri_mem_h**)uru_malloc(sizeof(uri_mem_h*) * unr_plan_p->num_uri_plan);
    uri_mem_h** rmt_uri_mem = (uri_mem_h**)uru_malloc(sizeof(uri_mem_h*) * unr_plan_p->num_uri_plan);
    size_t** loc_uri_offset = (size_t**)uru_malloc(sizeof(size_t*) * unr_plan_p->num_uri_plan);
    size_t** rmt_uri_offset = (size_t**)uru_malloc(sizeof(size_t*) * unr_plan_p->num_uri_plan);
    size_t** part_size = (size_t**)uru_malloc(sizeof(size_t*) * unr_plan_p->num_uri_plan);
    uru_flag_t*** loc_flag_p_arr = (uru_flag_t***)uru_malloc(sizeof(uru_flag_t**) * unr_plan_p->num_uri_plan);
    uru_flag_t*** rmt_flag_p_arr = (uru_flag_t***)uru_malloc(sizeof(uru_flag_t**) * unr_plan_p->num_uri_plan);
    int64_t** loc_flag_add_arr = (int64_t**)uru_malloc(sizeof(uint64_t*) * unr_plan_p->num_uri_plan);
    int64_t** rmt_flag_add_arr = (int64_t**)uru_malloc(sizeof(uint64_t*) * unr_plan_p->num_uri_plan);
    uru_transfer_t** part_dma_type = (uru_transfer_t**)uru_malloc(sizeof(uru_transfer_t*) * unr_plan_p->num_uri_plan);

    for (int i = 0; i < unr_plan_p->num_uri_plan; i++) {
        loc_uri_mem[i] = (uri_mem_h*)uru_malloc(sizeof(uri_mem_h) * num_sends);
        rmt_uri_mem[i] = (uri_mem_h*)uru_malloc(sizeof(uri_mem_h) * num_sends);
        loc_uri_offset[i] = (size_t*)uru_malloc(sizeof(size_t) * num_sends);
        rmt_uri_offset[i] = (size_t*)uru_malloc(sizeof(size_t) * num_sends);
        part_size[i] = (size_t*)uru_malloc(sizeof(size_t) * num_sends);
        loc_flag_p_arr[i] = (uru_flag_t**)uru_malloc(sizeof(uru_flag_t*) * num_sends);
        rmt_flag_p_arr[i] = (uru_flag_t**)uru_malloc(sizeof(uru_flag_t*) * num_sends);
        loc_flag_add_arr[i] = (int64_t*)uru_malloc(sizeof(uint64_t) * num_sends);
        rmt_flag_add_arr[i] = (int64_t*)uru_malloc(sizeof(uint64_t) * num_sends);
        part_dma_type[i] = (uru_transfer_t*)uru_malloc(sizeof(uru_transfer_t) * num_sends);
    }

    /* Set size_arr if it is NULL */
    char free_size_arr_flag = 0;
    if (size_arr == NULL) {
        free_size_arr_flag = 1;
        size_arr = (size_t*)uru_malloc(sizeof(size_t) * num_sends);
        for (int i = 0; i < num_sends; i++) {
            URU_ASSERT(loc_blk_arr[i].size == rmt_blk_arr[i].size);
            size_arr[i] = loc_blk_arr[i].size;
        }
    }

    /* Set dma_type_arr if it is URU_TRANSFER_TYPE_DMA_PUT_ALL or URU_TRANSFER_TYPE_DMA_GET_ALL */
    char free_dma_type_arr_flag = 0;
    if (dma_type_arr == URU_TRANSFER_TYPE_DMA_PUT_ALL || dma_type_arr == URU_TRANSFER_TYPE_DMA_GET_ALL) {
        uru_transfer_t* tmp = (uru_transfer_t*)uru_malloc(sizeof(uru_transfer_t) * num_sends);
        free_dma_type_arr_flag = 1;
        if (dma_type_arr == URU_TRANSFER_TYPE_DMA_PUT_ALL) {
            for (int i = 0; i < num_sends; i++) {
                tmp[i] = URU_TRANSFER_TYPE_DMA_PUT;
            }
        } else {
            for (int i = 0; i < num_sends; i++) {
                tmp[i] = URU_TRANSFER_TYPE_DMA_GET;
            }
        }
        dma_type_arr = tmp;
    }

    for (int i = 0; i < num_sends; i++) {
        if (size_arr[i] == 0) {
            continue;
        }
        URU_ASSERT(loc_blk_arr[i].dst_rank == uru_mpi->rank);
        URU_ASSERT(rmt_blk_arr[i].dst_rank < uru_mpi->size);
        // URU_ASSERT(rmt_blk_arr[i].dst_rank != uru_mpi->rank);
        URU_ASSERT(loc_blk_arr[i].mem_idx < unr_channels->loc_mem_arr_len);
        if (rmt_blk_arr[i].mem_idx >= unr_channels->rmt_mem_arr_len[rmt_blk_arr[i].dst_rank]) {
            printf("You should call unr_mem_reg_sync() before start RDMA Write communication!\n");
            exit(URU_ERROR_EXIT);
        }
        
        size_t num_channels = uru_mpi->same_node[rmt_blk_arr[i].dst_rank] ? unr_channels->num_intra_channels : unr_channels->num_inter_channels;
        uri_channel_h* uri_channels = uru_mpi->same_node[rmt_blk_arr[i].dst_rank] ? unr_channels->intra_channels : unr_channels->inter_channels;
        size_t* uri_mem_offset = uru_mpi->same_node[rmt_blk_arr[i].dst_rank] ? unr_channels->intra_uri_mem_h_offset : unr_channels->inter_uri_mem_h_offset;
        
        /* Calculate div_plan_wrt_size */
        size_t* channel_blk_size = uru_mpi->same_node[rmt_blk_arr[i].dst_rank] ? unr_channels->intra_channel_blk_size : unr_channels->inter_channel_blk_size;
        size_t channel_blk_size_sum = uru_mpi->same_node[rmt_blk_arr[i].dst_rank] ? unr_channels->intra_channel_blk_size_sum : unr_channels->inter_channel_blk_size_sum;
        next_channel_id = uru_mpi->same_node[rmt_blk_arr[i].dst_rank] ? (&next_intra_channel_id) : (&next_inter_channel_id);
        for (int j = 0; j < num_channels; j++) {
            div_plan_wrt_size[j] = size_arr[i] / channel_blk_size_sum * channel_blk_size[j];
        }
        for (size_t rest = size_arr[i] % channel_blk_size_sum; rest; *next_channel_id = (*next_channel_id + 1) % num_channels){
            if (rest > channel_blk_size[*next_channel_id]) {
                div_plan_wrt_size[*next_channel_id] += channel_blk_size[*next_channel_id];
                rest -= channel_blk_size[*next_channel_id];
            } else {
                div_plan_wrt_size[*next_channel_id] += rest;
                rest = 0;
            }
            if (rest < channel_blk_size[(*next_channel_id + 1) % num_channels]) {
                div_plan_wrt_size[*next_channel_id] += rest;
                rest = 0;
            }
        }
        { /* Check the sum(div_plan_wrt_size) == size_arr[i] */
            size_t div_plan_wrt_size_sum = 0;
            for (int j = 0; j < num_channels; j++) {
                div_plan_wrt_size_sum += div_plan_wrt_size[j];
            }
            URU_ASSERT(div_plan_wrt_size_sum == size_arr[i]);
        }


        { /* Calculate div_plan_wrt_offset */
            for (int j = 1; j < num_channels; j++) {
                div_plan_wrt_offset[j] = div_plan_wrt_offset[j - 1] + div_plan_wrt_size[j - 1];
            }
        }

        /* Calculate sig_add_arr */
        /* We use the higher 32 bits for sub-messages counting.
            The 33-th bit is a overflow detection bit,
            The lower 31 bits is used for messages counting. */
        {
            memset(sig_add_arr, 0, sizeof(size_t) * unr_plan_p->num_uri_plan);
            size_t non_zeros = 0;
            for (int j = 0; j < num_channels; j++) {
                if (div_plan_wrt_size[j] != 0) {
                    non_zeros++;
                }
            }
            if (non_zeros == 1) {
                for (int j = 0; j < num_channels; j++) {
                    if (div_plan_wrt_size[j] != 0) {
                        sig_add_arr[j] = -1;
                        break;
                    }
                }
            } else {
                int j;
                for (j = 0; j < num_channels; j++) {
                    if (div_plan_wrt_size[j] != 0) {
                        sig_add_arr[j] = ((int64_t)(-1)) + (((int64_t)(non_zeros-1)) << 32);
                        break;
                    }
                }
                for (j++; j < num_channels; j++) {
                    if (div_plan_wrt_size[j] != 0) {
                        // sig_add_arr[j] = ((int64_t)(-1)) << 32;
                        sig_add_arr[j] = 0xFFFFFFFF00000000;
                    }
                }
            }
        }

        unr_mem_t* loc_unr_mem = (unr_mem_t*)(unr_channels->loc_mem_arr[loc_blk_arr[i].mem_idx]);
        unr_mem_t* rmt_unr_mem = (unr_mem_t*)(unr_channels->rmt_mem_arr[rmt_blk_arr[i].dst_rank][rmt_blk_arr[i].mem_idx]);
        for (int j = 0; j < num_channels; j++) {
            if (div_plan_wrt_size[j] == 0) {
                continue;
            }

            size_t id = uru_mpi->same_node[rmt_blk_arr[i].dst_rank] ? j + unr_channels->num_inter_channels : j;
            size_t k = uri_plan_num_wrts[id];
            uri_plan_num_wrts[id]++;

            loc_uri_mem[id][k] = ((void*)loc_unr_mem) + uri_mem_offset[j];
            rmt_uri_mem[id][k] = ((void*)rmt_unr_mem) + uri_mem_offset[j];

            loc_uri_offset[id][k] = loc_blk_arr[i].offset;
            rmt_uri_offset[id][k] = rmt_blk_arr[i].offset;
            if (loc_offset_arr) {
                loc_uri_offset[id][k] += loc_offset_arr[i];
            }
            if (rmt_offset_arr) {
                rmt_uri_offset[id][k] += rmt_offset_arr[i];
            }
            loc_uri_offset[id][k] += div_plan_wrt_offset[j];
            rmt_uri_offset[id][k] += div_plan_wrt_offset[j];
            
            part_size[id][k] = div_plan_wrt_size[j];
            part_dma_type[id][k] = dma_type_arr[i];

            if (loc_sig_arr && (loc_sig_arr + i)->dst_rank != UNR_NO_SIGNAL_DST_RANK) {
                URU_ASSERT_REASON(loc_sig_arr[i].dst_rank == uru_mpi->rank,
                                  "The local signal should be created on the local process");
                loc_flag_p_arr[id][k] = loc_sig_arr[i].flag_p;
            } else {
                if (dma_type_arr[i] == URU_TRANSFER_TYPE_DMA_PUT) {
                    loc_flag_p_arr[id][k] = loc_blk_arr[i].send_flag_p;
                } else {
                    loc_flag_p_arr[id][k] = loc_blk_arr[i].recv_flag_p;
                }
            }
            loc_flag_add_arr[id][k] = sig_add_arr[j];

            if (rmt_sig_arr && (rmt_sig_arr + i)->dst_rank != UNR_NO_SIGNAL_DST_RANK) {
                URU_ASSERT_REASON(rmt_sig_arr[i].dst_rank == rmt_blk_arr[i].dst_rank,
                                  "The local signal should be created on the same process as block");
                rmt_flag_p_arr[id][k] = rmt_sig_arr[i].flag_p;
            } else {
                if (dma_type_arr[i] == URU_TRANSFER_TYPE_DMA_PUT) {
                    rmt_flag_p_arr[id][k] = rmt_blk_arr[i].recv_flag_p;
                } else {
                    rmt_flag_p_arr[id][k] = rmt_blk_arr[i].send_flag_p;
                }
            }
            rmt_flag_add_arr[id][k] = sig_add_arr[j];
        }
    }

    for (int i = 0; i < unr_plan_p->num_uri_plan; i++) {
        if (uri_plan_num_wrts[i]) {
            uri_channel_h uri_channel = i < unr_channels->num_inter_channels ? unr_channels->inter_channels[i] : unr_channels->intra_channels[i - unr_channels->num_inter_channels];
            unr_plan_p->uri_channel_h_arr[i] = uri_channel;
            uri_rdma_plan_create(
                uri_channel, once_now, uri_plan_num_wrts[i], part_dma_type[i],
                loc_uri_mem[i], loc_uri_offset[i], loc_flag_p_arr[i], loc_flag_add_arr[i], part_size[i],
                rmt_uri_mem[i], rmt_uri_offset[i], rmt_flag_p_arr[i], rmt_flag_add_arr[i],
                &(unr_plan_p->uri_plan_h_arr[i]));
        }
    }

    for (int i = 0; i < unr_plan_p->num_uri_plan; i++) {
        free(loc_uri_mem[i]);
        free(rmt_uri_mem[i]);
        free(loc_uri_offset[i]);
        free(rmt_uri_offset[i]);
        free(part_size[i]);
        free(loc_flag_p_arr[i]);
        free(rmt_flag_p_arr[i]);
        free(loc_flag_add_arr[i]);
        free(rmt_flag_add_arr[i]);
    }
    free(loc_uri_mem);
    free(rmt_uri_mem);
    free(loc_uri_offset);
    free(rmt_uri_offset);
    free(part_size);
    free(loc_flag_p_arr);
    free(rmt_flag_p_arr);
    free(loc_flag_add_arr);
    free(rmt_flag_add_arr);

    free(div_plan_wrt_size);
    free(div_plan_wrt_offset);
    free(uri_plan_num_wrts);
    free(sig_add_arr);

    if (once_now) {
        /* If the plan is only used once now, destroy it immediate */
        free(unr_plan_p->uri_plan_h_arr);
        free(unr_plan_p->uri_channel_h_arr);
        free(unr_plan_p);
        *unr_plan_pp = NULL;
    }

    if (unr_plan_p->para) {
        if (unr_plan_p->para->loc_blk_arr)
            free(unr_plan_p->para->loc_blk_arr);
        if (unr_plan_p->para->rmt_blk_arr)
            free(unr_plan_p->para->rmt_blk_arr);
        if (unr_plan_p->para->loc_sig_arr)
            free(unr_plan_p->para->loc_sig_arr);
        if (unr_plan_p->para->rmt_sig_arr)
            free(unr_plan_p->para->rmt_sig_arr);
        if (unr_plan_p->para->loc_offset_arr)
            free(unr_plan_p->para->loc_offset_arr);
        if (unr_plan_p->para->rmt_offset_arr)
            free(unr_plan_p->para->rmt_offset_arr);
        if (unr_plan_p->para->size_arr)
            free(unr_plan_p->para->size_arr);
        if (unr_plan_p->para->dma_type_arr != URU_TRANSFER_TYPE_DMA_PUT_ALL && unr_plan_p->para->dma_type_arr != URU_TRANSFER_TYPE_DMA_GET_ALL)
            free(unr_plan_p->para->dma_type_arr);
        free(unr_plan_p->para);
    }

    if (free_size_arr_flag) {
        free(size_arr);
    }
    if (free_dma_type_arr_flag) {
        free(dma_type_arr);
    }

    return URU_STATUS_OK;
}

uru_status_t unr_plan_start(unr_plan_h plan_h) {
    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    unr_plan_t* unr_plan_p = (unr_plan_t*)plan_h;
    URU_ASSERT(unr_plan_p != NULL);
    if (unr_plan_p->status != 2) {
        URU_ASSERT(unr_plan_p->status == 1);
        unr_rdma_plan_create(1, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &unr_plan_p);
    }
    for (int i = 0; i < unr_plan_p->num_uri_plan; i++) {
        if (unr_plan_p->uri_plan_h_arr[i] != NULL) {
            URU_ASSERT(unr_plan_p->uri_channel_h_arr[i] != NULL);
            uri_rdma_plan_start(unr_plan_p->uri_channel_h_arr[i], unr_plan_p->uri_plan_h_arr[i]);
        }
    }
    return URU_STATUS_OK;
}

uru_status_t unr_plan_destroy(unr_plan_h plan_h) {
    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    unr_plan_t* unr_plan_p = (unr_plan_t*)plan_h;
    for (int i = 0; i < unr_plan_p->num_uri_plan; i++) {
        if (unr_plan_p->uri_plan_h_arr[i] != NULL) {
            URU_ASSERT(unr_plan_p->uri_channel_h_arr[i] != NULL);
            uri_rdma_plan_destroy(unr_plan_p->uri_channel_h_arr[i], unr_plan_p->uri_plan_h_arr[i]);
        }
    }
    free(unr_plan_p->uri_plan_h_arr);
    free(unr_plan_p->uri_channel_h_arr);
    free(unr_plan_p);
    return URU_STATUS_OK;
}