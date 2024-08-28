#include <uri/mpi/uri_mpi.h>

uri_channel_func_ptrs_t uri_mpi_channel_func_ptrs;

void uri_mpi_channel_init(uri_channel_t* channel_p, int device_id, int num_devices) {
    static int func_ptrs_initialized = 0;
    if (!func_ptrs_initialized) {
        func_ptrs_initialized = 1;
        uri_mpi_channel_func_ptrs.channel_open = &uri_mpi_channel_open;
        uri_mpi_channel_func_ptrs.channel_daemon = &uri_mpi_channel_daemon;
        uri_mpi_channel_func_ptrs.channel_close = &uri_mpi_channel_close;
        uri_mpi_channel_func_ptrs.mem_h_size = &uri_mpi_mem_h_size;
        uri_mpi_channel_func_ptrs.mem_reg = &uri_mpi_mem_reg;
        uri_mpi_channel_func_ptrs.rdma_plan_create = &uri_mpi_rdma_plan_create;
        uri_mpi_channel_func_ptrs.rdma_plan_start = &uri_mpi_rdma_plan_start;
        uri_mpi_channel_func_ptrs.rdma_plan_destroy = &uri_mpi_rdma_plan_destroy;
    }
    if (num_devices != 1) {
        channel_p->attr.name = (char*)uru_malloc(sizeof(char) * 5);
        sprintf(channel_p->attr.name, "mpi%d", device_id);
    } else {
        channel_p->attr.name = "mpi";
    }
    channel_p->attr.type = "mpi";
    channel_p->attr.description = "MPI channel: Use MPI to communicate among processes. It is mainly used in intra-node communication.";
    channel_p->attr.blocksize = 4096;
    channel_p->device_id = device_id;
    channel_p->num_devices = num_devices;
    channel_p->func_ptrs = &uri_mpi_channel_func_ptrs;

    uri_mpi_channel_t* info = (uri_mpi_channel_t*)uru_calloc(1, sizeof(uri_mpi_channel_t));
    channel_p->info = info;
}

int uri_mpi_get_num_devices() {
    /* URI_MPI_DEVICE_NUM is used for verify the correctness of multi-channel when multiple NICs is not available. */
    char* env = getenv("URI_MPI_DEVICE_NUM");
    if (env != NULL) {
        return atoi(env);
    } else {
        return 1;
    }
}

uru_status_t uri_mpi_channel_open(uri_channel_t* channel_p) {
    uri_mpi_channel_t* info = (uri_mpi_channel_t*)channel_p->info;
    info->mode = NONBLOCKING_SENDRECV;

    { /* Test whether MPI is initialized with multiple thread support */
        int provided;
        MPI_Query_thread(&provided);
        URU_ASSERT_REASON(provided == MPI_THREAD_MULTIPLE, "MPI was not initialized with MPI_THREAD_MULTIPLE");
    }

    info->mpi_tag = (uri_mpi_int_pool_t*)uru_calloc(uru_mpi->size, sizeof(uri_mpi_int_pool_t));
    info->batch_id = (uri_mpi_int_pool_t*)uru_calloc(uru_mpi->size, sizeof(uri_mpi_int_pool_t));
    for (int i = 0; i < uru_mpi->size; i++) {
        pthread_spin_init(&(info->mpi_tag[i].lock), PTHREAD_PROCESS_PRIVATE);
        pthread_spin_init(&(info->batch_id[i].lock), PTHREAD_PROCESS_PRIVATE);
        info->mpi_tag[i].max = 1;
        info->batch_id[i].max = 1;
    }

    info->recv_batch = (uri_mpi_op_batch_t***)uru_calloc(uru_mpi->size, sizeof(uri_mpi_op_batch_t**));
    info->recv_batch_size = (int*)uru_calloc(uru_mpi->size, sizeof(int));

    pthread_spin_init(&(info->mpi_req.lock), PTHREAD_PROCESS_PRIVATE);
    info->mpi_req.arr_len = 1024;
    info->mpi_req.arr_tail = 0;
    info->mpi_req.op_arr = (uri_mpi_op_t**)uru_calloc(info->mpi_req.arr_len, sizeof(uri_mpi_op_t*));
    info->mpi_req.req_arr = (MPI_Request*)uru_calloc(info->mpi_req.arr_len, sizeof(MPI_Request));

    MPI_Comm_dup(MPI_COMM_WORLD, &(info->ctrl_comm));
    MPI_Comm_dup(MPI_COMM_WORLD, &(info->data_comm));

    return URU_STATUS_OK;
}

uru_status_t uri_mpi_process_ctrl_msg(uri_channel_t* channel_p, int src_rank, uri_mpi_ctrl_msg_t* ctrl_msg) {
    uri_mpi_channel_t* info = (uri_mpi_channel_t*)channel_p->info;

    switch (ctrl_msg->type) {
        case create_op_batch: {
            int batch_id = ctrl_msg->data.create_op_batch.batch_id;
            if (batch_id + 1 > info->recv_batch_size[src_rank]) {
                size_t new_size = batch_id * 2 + 16;
                info->recv_batch[src_rank] = uru_realloc(info->recv_batch[src_rank],
                                                         sizeof(uri_mpi_op_batch_t*) * info->recv_batch_size[src_rank],
                                                         sizeof(uri_mpi_op_batch_t*) * new_size);
                info->recv_batch_size[src_rank] = new_size;
            }
            uri_mpi_op_batch_t* recv_batch = (uri_mpi_op_batch_t*)(ctrl_msg + 1);
            URU_ASSERT(info->recv_batch[src_rank][batch_id] == NULL);
            info->recv_batch[src_rank][batch_id] = (uri_mpi_op_batch_t*)uru_malloc(sizeof(uri_mpi_op_batch_t) + sizeof(uri_mpi_op_t) * recv_batch->num_op);
            memcpy(info->recv_batch[src_rank][batch_id], recv_batch, sizeof(uri_mpi_op_batch_t) + sizeof(uri_mpi_op_t) * recv_batch->num_op);
            recv_batch = info->recv_batch[src_rank][batch_id];
            for (int j = 0; j < recv_batch->num_op; j++) {
                uri_mpi_op_t* recv_op = ((uri_mpi_op_t*)(recv_batch + 1)) + j;
                if (recv_op->op_type & OP_RECV) {
                    MPI_Recv_init(recv_op->addr, recv_op->size, MPI_BYTE, recv_op->rank, recv_op->tag, info->data_comm, &(recv_op->mpi_req));
                }
                else if (recv_op->op_type & OP_SEND){
                    MPI_Send_init(recv_op->addr, recv_op->size, MPI_BYTE, recv_op->rank, recv_op->tag, info->data_comm, &(recv_op->mpi_req));
                }
                else {
                    exit(URU_MPI_CHANNEL_ERROR_EXIT);
                }
            }
            break;
        }
        case destroy_op_batch: {
            int batch_id = ctrl_msg->data.destroy_op_batch.batch_id;
            uri_mpi_op_batch_t* recv_batch = info->recv_batch[src_rank][batch_id];
            for (int j = 0; j < recv_batch->num_op; j++) {
                uri_mpi_op_t* recv_op = ((uri_mpi_op_t*)(recv_batch + 1)) + j;
                MPI_Request_free(&(recv_op->mpi_req));
            }
            free(info->recv_batch[src_rank][batch_id]);
            info->recv_batch[src_rank][batch_id] = NULL;
            break;
        }
        case start_op_batch: {
            int batch_id = ctrl_msg->data.start_op_batch.batch_id;
            URU_ASSERT(batch_id < info->recv_batch_size[src_rank]);
            URU_ASSERT(info->recv_batch[src_rank][batch_id] != NULL);
            uri_mpi_op_batch_t* recv_batch = info->recv_batch[src_rank][batch_id];
            uri_mpi_op_t** mpi_op = (uri_mpi_op_t**)uru_malloc(sizeof(uri_mpi_op_t*) * recv_batch->num_op);
            for (int j = 0; j < recv_batch->num_op; j++) {
                uri_mpi_op_t* recv_op = ((uri_mpi_op_t*)(recv_batch + 1)) + j;
                mpi_op[j] = recv_op;
                MPI_Start(&(recv_op->mpi_req));
            }
            uri_mpi_req_que_app(channel_p, recv_batch->num_op, mpi_op);
            free(mpi_op);
            break;
        }
        case execute_op_batch: {
            uri_mpi_op_batch_t* recv_batch = (uri_mpi_op_batch_t*)(ctrl_msg + 1);
            uri_mpi_op_t** mpi_op = (uri_mpi_op_t**)uru_malloc(sizeof(uri_mpi_op_t*) * recv_batch->num_op);
            for (int j = 0; j < recv_batch->num_op; j++) {
                uri_mpi_op_t* recv_op = ((uri_mpi_op_t*)(recv_batch + 1)) + j;
                mpi_op[j] = (uri_mpi_op_t*)uru_malloc(sizeof(uri_mpi_op_t));
                memcpy(mpi_op[j], recv_op, sizeof(uri_mpi_op_t));
                if (recv_op->op_type & OP_RECV) {
                    MPI_Irecv(mpi_op[j]->addr, mpi_op[j]->size, MPI_BYTE, mpi_op[j]->rank, mpi_op[j]->tag, info->data_comm, &(mpi_op[j]->mpi_req));
                }
                else if (recv_op->op_type & OP_SEND){
                    MPI_Isend(mpi_op[j]->addr, mpi_op[j]->size, MPI_BYTE, mpi_op[j]->rank, mpi_op[j]->tag, info->data_comm, &(mpi_op[j]->mpi_req));
                }
                else {
                    exit(URU_MPI_CHANNEL_ERROR_EXIT);
                }
                URU_ASSERT((mpi_op[j]->flag & FREE_TAG) == 0);
            }
            uri_mpi_req_que_app(channel_p, recv_batch->num_op, mpi_op);
            free(mpi_op);
            break;
        }
        default: {
            URU_ASSERT_REASON(0, "Unknown ctrl_msg type: %d", ctrl_msg->type);
            break;
        }
    }
    return URU_STATUS_OK;
}

uru_status_t uri_mpi_channel_daemon(uri_channel_t* channel_p) {

    uri_mpi_channel_t* info = (uri_mpi_channel_t*)channel_p->info;

    { /* Probe control message */
        static size_t max_ctrl_msg_size = 0;
        static void* ctrl_msg_buf = NULL;
        int ctrl_msg_recv_flag;
        do {
            MPI_Status status;
            URU_ASSERT(MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, info->ctrl_comm, &ctrl_msg_recv_flag, &status) == MPI_SUCCESS);
            if (ctrl_msg_recv_flag) {
                int msg_size;
                MPI_Get_count(&status, MPI_BYTE, &msg_size);
                int src_rank = status.MPI_SOURCE;
                if (max_ctrl_msg_size < msg_size) {
                    ctrl_msg_buf = uru_realloc(ctrl_msg_buf, max_ctrl_msg_size, msg_size * 2);
                    max_ctrl_msg_size = msg_size * 2;
                }
                memset(ctrl_msg_buf, 0, max_ctrl_msg_size);
                URU_ASSERT(MPI_Recv(ctrl_msg_buf, msg_size, MPI_BYTE, src_rank, status.MPI_TAG, info->ctrl_comm, MPI_STATUS_IGNORE) == MPI_SUCCESS);
                uri_mpi_process_ctrl_msg(channel_p, src_rank, (uri_mpi_ctrl_msg_t*)ctrl_msg_buf);
            }
        } while (ctrl_msg_recv_flag);
    }

    int idx_count = 0;
    static int* idx = NULL;
    static MPI_Request* req_arr_copy = NULL;
    static uri_mpi_op_t** op_arr_copy = NULL;
    static size_t arr_max_len = 0;
    size_t req_arr_tail_copy = info->mpi_req.arr_tail;
    if (arr_max_len < req_arr_tail_copy) {
        size_t new_size = req_arr_tail_copy * 2 + 1;
        idx = uru_realloc(idx, sizeof(int) * arr_max_len, sizeof(int) * new_size);
        req_arr_copy = uru_realloc(req_arr_copy, sizeof(MPI_Request) * arr_max_len, sizeof(MPI_Request) * new_size);
        op_arr_copy = uru_realloc(op_arr_copy, sizeof(uri_mpi_op_t*) * arr_max_len, sizeof(uri_mpi_op_t*) * new_size);
        arr_max_len = new_size;
    }
    if (req_arr_tail_copy) {
        pthread_spin_lock(&(info->mpi_req.lock));
        memcpy(req_arr_copy, info->mpi_req.req_arr, sizeof(MPI_Request) * req_arr_tail_copy);
        memcpy(op_arr_copy, info->mpi_req.op_arr, sizeof(uri_mpi_op_t*) * req_arr_tail_copy);
        pthread_spin_unlock(&(info->mpi_req.lock));
        MPI_Testsome(req_arr_tail_copy, req_arr_copy, &idx_count, idx, MPI_STATUSES_IGNORE);
        // #ifdef URU_ENABLE_ASSERT
        //     for (int i = 0; i < req_arr_tail_copy; i++) {
        //         int flag;
        //         MPI_Test(req_arr_copy + i, &flag, MPI_STATUS_IGNORE);
        //         if (flag) {
        //             idx[idx_count] = i;
        //             idx_count++;
        //         }
        //     }
        // #else
        //     MPI_Testsome(req_arr_tail_copy, req_arr_copy, &idx_count, idx, MPI_STATUSES_IGNORE);
        // #endif
    }

    URU_ASSERT(idx_count != MPI_UNDEFINED);
#ifdef URU_ENABLE_ASSERT
    for (int i = 1; i < idx_count; i++) {
        URU_ASSERT(idx[i - 1] < idx[i]);
    }
#endif

    for (int i = 0; i < idx_count; i++) {
        uri_mpi_op_t* op = op_arr_copy[idx[i]];
        URU_ASSERT(op);
        if (op->flag_p != NULL) {
            op->flag_p->counter += op->flag_add;
            URU_ASSERT(op->flag_p->counter >= 0);
            if (op->flag_p->counter == 0 && op->flag_p->queue) {
                uru_queue_push(op->flag_p->queue, op->flag_p->idx, 1);
            }
        }
        if (op->flag & FREE_ADDR) {
            URU_ASSERT((op->flag & KEEP_ADDR) == 0);
            free(op->addr);
        }
        if (op->flag & FREE_REQ) {
            URU_ASSERT((op->flag & KEEP_REQ) == 0);
            MPI_Request_free(&(op->mpi_req));
        }
        if (op->flag & FREE_TAG) {
            URU_ASSERT((op->flag & KEEP_TAG) == 0);
            uri_mpi_int_pool_return(info->mpi_tag + op->rank, op->tag);
        }
        if (op->flag & FREE_OP) {
            URU_ASSERT((op->flag & KEEP_OP) == 0);
            free(op);
        }
    }

    if (idx_count) {
        pthread_spin_lock(&(info->mpi_req.lock));

        /* Remove all the received request */
        for (int i = 0; i < idx_count; i++) {
            uri_mpi_op_t* op = info->mpi_req.op_arr[idx[i]];
            while (i < idx_count && idx[idx_count - 1] == info->mpi_req.arr_tail - 1) {
                idx_count--;
                info->mpi_req.arr_tail--;
            }
            if (i < idx_count) {
                /* Move the last one to the front */
                info->mpi_req.arr_tail--;
                info->mpi_req.req_arr[idx[i]] = info->mpi_req.req_arr[info->mpi_req.arr_tail];
                info->mpi_req.op_arr[idx[i]] = info->mpi_req.op_arr[info->mpi_req.arr_tail];
            }
        }

        /* Remove all the received request (Another method) */
        // int i = 0, j = 0, k = 0;
        // for (; j < info->mpi_req.arr_tail; i++, j++) {
        //     while (k < idx_count && idx[k] == j) {
        //         k++;
        //         j++;
        //     }
        //     if (j >= info->mpi_req.arr_tail) {
        //         break;
        //     }
        //     info->mpi_req.req_arr[i] = info->mpi_req.req_arr[j];
        //     info->mpi_req.op_arr[i] = info->mpi_req.op_arr[j];
        // }
        // URU_ASSERT(i == info->mpi_req.arr_tail - idx_count);
        // info->mpi_req.arr_tail -= idx_count;

        pthread_spin_unlock(&(info->mpi_req.lock));
    }
    return URU_STATUS_OK;
}

uru_status_t uri_mpi_channel_close(uri_channel_t* channel_p) {
    uri_mpi_channel_t* info = (uri_mpi_channel_t*)channel_p->info;

    /* Make sure all the messages are sent */
    u_int64_t max_arr_tail, self_arr_tail = info->mpi_req.arr_tail;
    do {
        MPI_Allreduce(&self_arr_tail, &max_arr_tail, 1, MPI_UINT64_T, MPI_MAX, info->ctrl_comm);
        uri_mpi_channel_daemon(channel_p);
        self_arr_tail = info->mpi_req.arr_tail;
    } while (max_arr_tail);

    MPI_Comm_free(&(info->ctrl_comm));
    MPI_Comm_free(&(info->data_comm));
    return URU_STATUS_OK;
}

URI_CHANNEL_REGISTER(uri_mpi_get_num_devices, uri_mpi_channel_init);