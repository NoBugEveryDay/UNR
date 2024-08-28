#include <uri/mpi/uri_mpi.h>
#include <stdint.h>

uru_status_t uri_mpi_rdma_plan_create(
    uri_channel_t* channel_p, uint8_t once_now, size_t num_wrt, uru_transfer_t* dma_type_arr,
    uri_mem_t** loc_mem_pp, size_t* loc_offset_arr, uru_flag_t** loc_flag_p_arr, int64_t* loc_flag_add_arr, uint64_t* dma_size_arr,
    uri_mem_t** rmt_mem_pp, size_t* rmt_offset_arr, uru_flag_t** rmt_flag_p_arr, int64_t* rmt_flag_add_arr,
    uri_plan_t** plan_pp) {

    uri_mpi_channel_t* info = (uri_mpi_channel_t*)channel_p->info;
    URU_ASSERT(once_now==0 || once_now==1);

    uri_plan_t* plan_p = (uri_plan_t*)uru_malloc(sizeof(uri_plan_t) + sizeof(uri_mpi_plan_t));
    *plan_pp = plan_p;
    uri_mpi_plan_t* mpi_plan_p = (uri_mpi_plan_t*)(plan_p + 1);

    mpi_plan_p->send_batch = (uri_mpi_op_batch_t*)uru_calloc(1, sizeof(uri_mpi_op_batch_t) + sizeof(uri_mpi_op_t) * num_wrt);
    mpi_plan_p->send_batch->num_op = num_wrt;
    mpi_plan_p->send_batch->batch_id = -1;
    uri_mpi_op_t* recv_op_arr = (uri_mpi_op_t*)uru_calloc(num_wrt, sizeof(uri_mpi_op_t));
    uri_mpi_op_t** mpi_op = (uri_mpi_op_t**)uru_calloc(num_wrt*((size_t)once_now), sizeof(uri_mpi_op_t*));

    for (int i = 0; i < num_wrt; i++) {
        uri_mpi_op_t* send_op = ((uri_mpi_op_t*)(mpi_plan_p->send_batch + 1)) + i;
        uri_mpi_op_t* recv_op = recv_op_arr + i;

        void* send_mem_h_addr;
        void* recv_mem_h_addr;
        size_t send_mem_h_size, recv_mem_h_size;
        int src_rank, dst_rank;
        uri_mem_get_info(channel_p, loc_mem_pp[i], &send_mem_h_addr, &send_mem_h_size, &src_rank);
        uri_mem_get_info(channel_p, rmt_mem_pp[i], &recv_mem_h_addr, &recv_mem_h_size, &dst_rank);
        URU_ASSERT(src_rank == uru_mpi->rank);
        // URU_ASSERT(dst_rank != uru_mpi->rank);
        if (loc_offset_arr[i] + dma_size_arr[i] > send_mem_h_size) {
            URU_PRINT_INT64("loc_offset_arr[i]", loc_offset_arr[i]);
            URU_PRINT_INT64("dma_size_arr[i]", dma_size_arr[i]);
            URU_PRINT_INT64("send_mem_h_size", send_mem_h_size);
        }
        URU_ASSERT(loc_offset_arr[i] + dma_size_arr[i] <= send_mem_h_size);
        URU_ASSERT(rmt_offset_arr[i] + dma_size_arr[i] <= recv_mem_h_size);
        send_op->addr = send_mem_h_addr + loc_offset_arr[i];
        recv_op->addr = recv_mem_h_addr + rmt_offset_arr[i];
        send_op->size = dma_size_arr[i];
        recv_op->size = dma_size_arr[i];
        send_op->flag_p = loc_flag_p_arr[i];
        recv_op->flag_p = rmt_flag_p_arr[i];
        send_op->flag_add = loc_flag_add_arr[i];
        recv_op->flag_add = rmt_flag_add_arr[i];
        send_op->rank = dst_rank;
        recv_op->rank = dst_rank;
        if (dma_type_arr[i] == URU_TRANSFER_TYPE_DMA_PUT) {
            send_op->op_type = OP_SEND;
            recv_op->op_type = OP_RECV;
        }
        else if (dma_type_arr[i] == URU_TRANSFER_TYPE_DMA_GET) {
            send_op->op_type = OP_RECV;
            recv_op->op_type = OP_SEND;
        }
        else {
            exit(URU_MPI_CHANNEL_ERROR_EXIT);
        }
        uri_mpi_int_pool_get(info->mpi_tag + dst_rank, &(send_op->tag));
        recv_op->tag = send_op->tag;

        if (once_now) {
            mpi_op[i] = (uri_mpi_op_t*)uru_malloc(sizeof(uri_mpi_op_t));
            memcpy(mpi_op[i], send_op, sizeof(uri_mpi_op_t));
            mpi_op[i]->flag = FREE_OP | FREE_TAG;
            if (dma_type_arr[i] == URU_TRANSFER_TYPE_DMA_PUT) {
                MPI_Isend(mpi_op[i]->addr, mpi_op[i]->size, MPI_BYTE, mpi_op[i]->rank, mpi_op[i]->tag, info->data_comm, &(mpi_op[i]->mpi_req));
            }
            else {
                MPI_Irecv(mpi_op[i]->addr, mpi_op[i]->size, MPI_BYTE, mpi_op[i]->rank, mpi_op[i]->tag, info->data_comm, &(mpi_op[i]->mpi_req));
            }
            recv_op->flag = FREE_OP;
        } else {
            send_op->flag = KEEP_OP | KEEP_REQ;
            recv_op->flag = KEEP_OP | KEEP_REQ;
            if (dma_type_arr[i] == URU_TRANSFER_TYPE_DMA_PUT) {
                MPI_Send_init(send_op->addr, send_op->size, MPI_BYTE, send_op->rank, send_op->tag, info->data_comm, &(send_op->mpi_req));
            }
            else {
                MPI_Recv_init(send_op->addr, send_op->size, MPI_BYTE, send_op->rank, send_op->tag, info->data_comm, &(send_op->mpi_req));
            }
        }
        URU_ASSERT((recv_op->flag & FREE_TAG) == 0);
    }

    qsort(recv_op_arr, num_wrt, sizeof(uri_mpi_op_t), uri_mpi_op_cmp);
    mpi_plan_p->num_receiver = 1;
    for (int i = 1; i < num_wrt; i++) {
        if (recv_op_arr[i].rank != recv_op_arr[i - 1].rank) {
            mpi_plan_p->num_receiver++;
        }
    }
    mpi_plan_p->recv_batch = (uri_mpi_op_batch_t**)uru_calloc(mpi_plan_p->num_receiver, sizeof(uri_mpi_op_batch_t*));
    mpi_op = uru_realloc(mpi_op, 
        sizeof(uri_mpi_op_t*) * (num_wrt*((size_t)once_now)),
        sizeof(uri_mpi_op_t*) * (num_wrt*((size_t)once_now) + mpi_plan_p->num_receiver));
    mpi_op += num_wrt*((size_t)once_now);

    for (int i = 0, st = 0, en = st + 1; i < mpi_plan_p->num_receiver; i++, st = en, en++) {
        while (en < num_wrt && recv_op_arr[en].rank == recv_op_arr[st].rank) {
            en++;
        }
        int num_op = en - st;
        mpi_plan_p->recv_batch[i] = (uri_mpi_op_batch_t*)uru_malloc(sizeof(uri_mpi_op_batch_t) + sizeof(uri_mpi_op_t) * num_op);
        mpi_plan_p->recv_batch[i]->num_op = num_op;
        int dst_rank = recv_op_arr[st].rank;
        uri_mpi_op_t* copy_from = recv_op_arr + st;
        uri_mpi_op_t* copy_to = (uri_mpi_op_t*)(mpi_plan_p->recv_batch[i] + 1);
        memcpy(copy_to, copy_from, sizeof(uri_mpi_op_t) * num_op);

        uri_mpi_ctrl_msg_t* ctrl_msg = (uri_mpi_ctrl_msg_t*)uru_malloc(sizeof(uri_mpi_ctrl_msg_t) + sizeof(uri_mpi_op_batch_t) + sizeof(uri_mpi_op_t) * num_op);
        if (once_now) {
            mpi_plan_p->recv_batch[i]->batch_id = -1;
            ctrl_msg->data.create_op_batch.batch_id = -1;
            ctrl_msg->type = execute_op_batch;
        } else {
            uri_mpi_int_pool_get(info->batch_id + dst_rank, &(mpi_plan_p->recv_batch[i]->batch_id));
            ctrl_msg->data.create_op_batch.batch_id = mpi_plan_p->recv_batch[i]->batch_id;
            ctrl_msg->type = create_op_batch;
        }
        memcpy(ctrl_msg + 1, mpi_plan_p->recv_batch[i], sizeof(uri_mpi_op_batch_t) + sizeof(uri_mpi_op_t) * num_op);
        uri_mpi_op_t* op = (uri_mpi_op_t*)(((uri_mpi_op_batch_t*)(ctrl_msg + 1)) + 1);
        for (int j = 0; j < num_op; j++) {
            op[j].rank = uru_mpi->rank;
        }
        mpi_op[i] = (uri_mpi_op_t*)uru_calloc(1, sizeof(uri_mpi_op_t));
        mpi_op[i]->addr = ctrl_msg;
        mpi_op[i]->size = sizeof(uri_mpi_ctrl_msg_t) + sizeof(uri_mpi_op_batch_t) + sizeof(uri_mpi_op_t) * num_op;
        mpi_op[i]->rank = dst_rank;
        mpi_op[i]->flag = FREE_OP | FREE_ADDR;
        MPI_Isend(mpi_op[i]->addr, mpi_op[i]->size, MPI_BYTE, mpi_op[i]->rank, mpi_op[i]->tag, info->ctrl_comm, &(mpi_op[i]->mpi_req));
    }
    mpi_op -= num_wrt*((size_t)once_now);
    uri_mpi_req_que_app(channel_p, num_wrt*((size_t)once_now) + mpi_plan_p->num_receiver, mpi_op);
    free(recv_op_arr);
    free(mpi_op);

    if (once_now) {
        free(mpi_plan_p->send_batch);
        for (int i = 0; i < mpi_plan_p->num_receiver; i++) {
            free(mpi_plan_p->recv_batch[i]);
        }
        free(mpi_plan_p->recv_batch);
        free(plan_p);
    }

    return URU_STATUS_OK;
}

uru_status_t uri_mpi_rdma_plan_start(uri_channel_t* channel_p, uri_plan_t* plan_p) {
    uri_mpi_channel_t* info = (uri_mpi_channel_t*)channel_p->info;
    uri_mpi_plan_t* mpi_plan_p = (uri_mpi_plan_t*)(plan_p + 1);

    uri_mpi_op_t** mpi_op = (uri_mpi_op_t**)uru_malloc(sizeof(uri_mpi_op_t*) * (mpi_plan_p->num_receiver + mpi_plan_p->send_batch->num_op));
    for (int i = 0; i < mpi_plan_p->num_receiver; i++) {
        uri_mpi_op_batch_t* recv_batch = mpi_plan_p->recv_batch[i];
        int dst_rank = ((uri_mpi_op_t*)(recv_batch + 1))->rank;
        uri_mpi_ctrl_msg_t* ctrl_msg = (uri_mpi_ctrl_msg_t*)uru_malloc(sizeof(uri_mpi_ctrl_msg_t));
        ctrl_msg->type = start_op_batch;
        ctrl_msg->data.start_op_batch.batch_id = recv_batch->batch_id;
        mpi_op[i] = (uri_mpi_op_t*)uru_calloc(1, sizeof(uri_mpi_op_t));
        mpi_op[i]->addr = ctrl_msg;
        mpi_op[i]->size = sizeof(uri_mpi_ctrl_msg_t);
        mpi_op[i]->rank = dst_rank;
        mpi_op[i]->flag = FREE_OP | FREE_ADDR;
        MPI_Isend(mpi_op[i]->addr, mpi_op[i]->size, MPI_BYTE, mpi_op[i]->rank, mpi_op[i]->tag, info->ctrl_comm, &(mpi_op[i]->mpi_req));
    }

    for (int i = 0; i < mpi_plan_p->send_batch->num_op; i++) {
        uri_mpi_op_t* send_op = ((uri_mpi_op_t*)(mpi_plan_p->send_batch + 1)) + i;
        MPI_Start(&(send_op->mpi_req));
        mpi_op[mpi_plan_p->num_receiver + i] = send_op;
    }

    uri_mpi_req_que_app(channel_p, mpi_plan_p->num_receiver + mpi_plan_p->send_batch->num_op, mpi_op);
    free(mpi_op);
    return URU_STATUS_OK;
};

uru_status_t uri_mpi_rdma_plan_destroy(uri_channel_t* channel_p, uri_plan_t* plan_p) {
    uri_mpi_channel_t* info = (uri_mpi_channel_t*)channel_p->info;
    uri_mpi_plan_t* mpi_plan_p = (uri_mpi_plan_t*)(plan_p + 1);
    for (int i = 0; i < mpi_plan_p->send_batch->num_op; i++) {
        uri_mpi_op_t* send_op = ((uri_mpi_op_t*)(mpi_plan_p->send_batch + 1)) + i;
        uri_mpi_int_pool_return(info->mpi_tag + send_op->rank, send_op->tag);
        MPI_Request_free(&(send_op->mpi_req));
    }
    uri_mpi_op_t** mpi_op = (uri_mpi_op_t**)uru_malloc(sizeof(uri_mpi_op_t*) * mpi_plan_p->num_receiver);
    for (int i = 0; i < mpi_plan_p->num_receiver; i++) {
        uri_mpi_op_batch_t* recv_batch = mpi_plan_p->recv_batch[i];
        uri_mpi_ctrl_msg_t* ctrl_msg = (uri_mpi_ctrl_msg_t*)uru_malloc(sizeof(uri_mpi_ctrl_msg_t));
        ctrl_msg->type = destroy_op_batch;
        ctrl_msg->data.destroy_op_batch.batch_id = recv_batch->batch_id;
        int dst_rank = ((uri_mpi_op_t*)(recv_batch + 1))->rank;
        mpi_op[i] = (uri_mpi_op_t*)uru_calloc(1, sizeof(uri_mpi_op_t));
        mpi_op[i]->addr = ctrl_msg;
        mpi_op[i]->size = sizeof(uri_mpi_ctrl_msg_t);
        mpi_op[i]->rank = dst_rank;
        mpi_op[i]->flag = FREE_OP | FREE_ADDR;
        MPI_Isend(mpi_op[i]->addr, mpi_op[i]->size, MPI_BYTE, mpi_op[i]->rank, mpi_op[i]->tag, info->ctrl_comm, &(mpi_op[i]->mpi_req));
        uri_mpi_int_pool_return(info->batch_id + dst_rank, recv_batch->batch_id);
    }
    uri_mpi_req_que_app(channel_p, mpi_plan_p->num_receiver, mpi_op);
    free(mpi_op);
    free(mpi_plan_p->send_batch);
    for (int i = 0; i < mpi_plan_p->num_receiver; i++) {
        free(mpi_plan_p->recv_batch[i]);
    }
    free(mpi_plan_p->recv_batch);
    free(plan_p);
    return URU_STATUS_OK;
}
