#include <uri/verbs/uri_verbs.h>

uri_channel_func_ptrs_t uri_verbs_channel_func_ptrs;

void uri_verbs_channel_init(uri_channel_t* channel_p, int device_id, int num_devices) {
    static int func_ptrs_initialized = 0;
    if (!func_ptrs_initialized) {
        func_ptrs_initialized = 1;
        uri_verbs_channel_func_ptrs.channel_open = &uri_verbs_channel_open;
        uri_verbs_channel_func_ptrs.channel_daemon = &uri_verbs_channel_daemon;
        uri_verbs_channel_func_ptrs.channel_close = &uri_verbs_channel_close;
        uri_verbs_channel_func_ptrs.mem_h_size = &uri_verbs_mem_h_size;
        uri_verbs_channel_func_ptrs.mem_reg = &uri_verbs_mem_reg;
        uri_verbs_channel_func_ptrs.rdma_plan_create = &uri_verbs_rdma_plan_create;
        uri_verbs_channel_func_ptrs.rdma_plan_start = &uri_verbs_rdma_plan_start;
        uri_verbs_channel_func_ptrs.rdma_plan_destroy = &uri_verbs_rdma_plan_destroy;
    }
    if (num_devices == 1) {
        channel_p->attr.name = "verbs";
        channel_p->attr.description = "This is used for Verbs supported network.";
    } else {
        channel_p->attr.name = (char*)uru_malloc(sizeof(char) * 6);
        sprintf(channel_p->attr.name, "verbs%d", device_id);
        channel_p->attr.description = (char*)uru_malloc(sizeof(char) * 57);
        sprintf(channel_p->attr.description, "This is used for Verbs supported network, using the %d-th nic.", device_id);
    }
    channel_p->attr.type = "verbs";
    channel_p->attr.blocksize = 4096;
    channel_p->device_id = device_id;
    channel_p->num_devices = num_devices;
    channel_p->func_ptrs = &uri_verbs_channel_func_ptrs;
    channel_p->info = (void*)uru_malloc(sizeof(uri_verbs_channel_t));
    memset(channel_p->info, 0, sizeof(uri_verbs_channel_t));
}

int uri_verbs_get_num_devices() {
    int num_device;
    struct ibv_device** dev_list;
    URU_ASSERT(dev_list = ibv_get_device_list(&num_device));
    URU_ASSERT(num_device);

    // { /* Print device list */
    //     printf("Found %d device(s)\n", num_device);
    //     for (int i = 0; i < num_device; i++) {
    //         printf("    Device %d name : %s\n", i, ibv_get_device_name(dev_list[i]));
    //     }
    // }

    /* URI_VERBS_DEVICE_NUM is used for verify the correctness of multi-channel when multiple NICs is not available. */
    {
        char* env = getenv("URI_VERBS_DEVICE_NUM");
        if (env != NULL) {
            return atoi(env);
        }
    }

    {
        char* env = getenv("TARGET_SYSTEM");
        if (env && strcmp(env, "TH2K") == 0) {
            return 1;
        }
    }

    ibv_free_device_list(dev_list);
    return num_device;
}

uru_status_t uri_verbs_print_device_attr(uri_verbs_channel_t* info) {
    struct ibv_device_attr* dev_attr = &(info->dev_attr);
    struct ibv_port_attr* port_attr = &(info->port_attr);

    URU_PRINT_INT64("max_qp_wr", dev_attr->max_qp_wr);
    URU_PRINT_INT64("max_cqe", dev_attr->max_cqe);
    URU_PRINT_INT64("max_srq_wr", dev_attr->max_srq_wr);
    URU_PRINT_INT64("LID", port_attr->lid);
    if (port_attr->lid == 0) {
        if (uru_mpi->rank == 0) {
            union ibv_gid my_gid;
            verbs_check(ibv_query_gid(info->ctx, VERBS_PORT_NUM, info->gid_idx, &my_gid));
            uint8_t* p = (uint8_t*)&my_gid;
            printf("RoCE used: Rank 0 gid = %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
                   p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
        }
    }
    URU_PRINT_INT64("active_mtu", port_attr->active_mtu);

    return URU_STATUS_OK;
}

uru_status_t uri_verbs_channel_daemon(uri_channel_t* channel_p) {
    uri_verbs_channel_t* info = (uri_verbs_channel_t*)channel_p->info;
    static struct ibv_wc* wc = NULL;
    static struct ibv_recv_wr* wr = NULL;
    if (wc == NULL) {
        wc = (struct ibv_wc*)uru_malloc(sizeof(struct ibv_wc) * info->dev_attr.max_cqe);
        wr = (struct ibv_recv_wr*)uru_calloc(info->dev_attr.max_srq_wr, sizeof(struct ibv_recv_wr));
        for (int i = 0; i < info->dev_attr.max_srq_wr; i++) {
            wr[i].next = wr + i + 1;
        }
    }
    int num_complete;
    do {
        num_complete = ibv_poll_cq(info->shared_cq, info->dev_attr.max_cqe, wc);
        if (num_complete) {
            if (num_complete > info->dev_attr.max_srq_wr / 2) {
                static int warned = 0;
                if (!warned) {
                    warned = 1;
                    printf("[%d/%d] ibverbs WARNING num_complete=%d/%d\n", uru_mpi->rank, uru_mpi->size, num_complete, info->dev_attr.max_srq_wr);
                }
            }
            int num_wr = 0;
            for (int i = 0; i < num_complete; i++) {
                if (wc[i].status != IBV_WC_SUCCESS) {
                    printf("[%d/%d] ibverbs ERROR wc[%d].status=%d\n", uru_mpi->rank, uru_mpi->size, i, wc[i].status);
                    exit(URU_ERROR_EXIT);
                }

                if (wc[i].opcode == IBV_WC_RDMA_WRITE) {
                    /* RDMA WRITE Sender Complete */
                    uint32_t dst_rank = info->qp2rank[wc[i].qp_num];
                    verbs_wrid_protocol_t* wrid = (verbs_wrid_protocol_t*)(&(wc[i].wr_id));
                    info->req_finish[dst_rank]++;
                    uri_verbs_event_progress(info, (verbs_imm_protocol_t*)(&(wrid->imm_protocol)));
                } else if (wc[i].opcode == IBV_WC_RECV_RDMA_WITH_IMM) {
                    /* RDMA WRITE Receiver Complete */
                    uri_verbs_event_progress(info, (verbs_imm_protocol_t*)(&(wc[i].imm_data)));
                    num_wr++;
                } else if (wc[i].opcode == IBV_WC_RDMA_READ) {
                    /* RDMA READ source Complete */
                    uint32_t dst_rank = info->qp2rank[wc[i].qp_num];
                    verbs_wrid_protocol_t* wrid = (verbs_wrid_protocol_t*)(&(wc[i].wr_id));
                    info->req_finish[dst_rank]++;
                    uri_verbs_event_progress(info, (verbs_imm_protocol_t*)(&(wrid->imm_protocol)));
                    uri_verbs_send_rmt_complete(info, dst_rank, wrid->rmt_imm_protocol);
                } else {
                    printf("[%d/%d] ibverbs UNKNOWN wc[%d].opcode=%d\n", uru_mpi->rank, uru_mpi->size, i, wc[i].opcode);
                    exit(URU_ERROR_EXIT);
                }
            }
            if (num_wr) {
                /* Post receive request into SRQ */
                wr[num_wr - 1].next = NULL;
                struct ibv_recv_wr* bad_wr;
                verbs_check(ibv_post_srq_recv(info->srq, wr, &bad_wr));
                wr[num_wr - 1].next = wr + num_wr;
            }
        }
    } while (num_complete);

    return URU_STATUS_OK;
}

uru_status_t uri_verbs_channel_open(uri_channel_t* channel_p) {

    uri_verbs_channel_t* info = (uri_verbs_channel_t*)channel_p->info;

    /* Open device and get attributes */
    {
        int open_dev_id;
        { /* Set open_dev_id */
            open_dev_id = channel_p->device_id;
            if (getenv("URI_VERBS_DEVICE_NUM")) {
                /* URI_VERBS_DEVICE_NUM is used for verify the correctness of multi-channel when multiple NICs is not available. */
                open_dev_id = 0;
            }
            {
                char* env = getenv("TARGET_SYSTEM");
                if (env && strcmp(env, "TH2K") == 0) {
                    open_dev_id = 0; /* CPU[0] GPU[3]:100G EDR IB */
                    // open_dev_id = 2; /* CPU[2] GPU[1]:25G RoCE */
                }
            }
        }

        int num_device;
        struct ibv_device** dev_list;
        URU_ASSERT(dev_list = ibv_get_device_list(&num_device));
        URU_ASSERT(num_device);
        URU_ASSERT(open_dev_id < num_device);
        URU_ASSERT(info->dev = dev_list[open_dev_id]);
        URU_ASSERT(info->ctx = ibv_open_device(info->dev));
        ibv_free_device_list(dev_list);
        ibv_query_device(info->ctx, &(info->dev_attr));
        info->pd = ibv_alloc_pd(info->ctx);
        URU_ASSERT(VERBS_OUTSTANDING_SEND_REQUESTS < info->dev_attr.max_qp_wr);
        verbs_check(ibv_query_port(info->ctx, VERBS_PORT_NUM, &(info->port_attr)));
        // uri_verbs_print_device_attr(info);
    }

    { /* Init QP*/

        { /* Create SRQ(shared received queue) */
            struct ibv_srq_init_attr srq_init_attr;
            memset(&srq_init_attr, 0, sizeof(srq_init_attr));
            srq_init_attr.attr.max_wr = info->dev_attr.max_srq_wr;
            srq_init_attr.attr.max_sge = 0;
            URU_ASSERT(info->srq = ibv_create_srq(info->pd, &srq_init_attr));
        }

        { /* Post receive request into SRQ */
            struct ibv_recv_wr* wr = (struct ibv_recv_wr*)uru_calloc(info->dev_attr.max_srq_wr, sizeof(struct ibv_recv_wr));
            for (int i = 0; i < info->dev_attr.max_srq_wr - 1; i++) {
                wr[i].next = wr + i + 1;
            }
            struct ibv_recv_wr* bad_wr;
            verbs_check(ibv_post_srq_recv(info->srq, wr, &bad_wr));
            free(wr);
        }

        { /* Create shared complete queue */
            URU_ASSERT(info->shared_cq = ibv_create_cq(info->ctx, info->dev_attr.max_cqe, NULL, NULL, 0));
        }

        { /* Create QP */
            struct ibv_qp_init_attr qp_init_attr;
            memset(&qp_init_attr, 0, sizeof(qp_init_attr));
            qp_init_attr.send_cq = info->shared_cq;
            qp_init_attr.recv_cq = info->shared_cq;
            qp_init_attr.srq = info->srq;
            qp_init_attr.cap.max_send_wr = VERBS_OUTSTANDING_SEND_REQUESTS;
            qp_init_attr.cap.max_recv_wr = 0; /* This value is ignored because QP is associated with an SRQ */
            qp_init_attr.cap.max_send_sge = 1;
            qp_init_attr.cap.max_recv_sge = 1;
            qp_init_attr.cap.max_inline_data = VERBS_MAX_INLINE_DATA;
            qp_init_attr.qp_type = IBV_QPT_RC;
            qp_init_attr.sq_sig_all = 0;
            info->qp_list = (struct ibv_qp**)uru_calloc(uru_mpi->size, sizeof(struct ibv_qp*));
            for (int i = 0; i < uru_mpi->size; i++) {
                URU_ASSERT(info->qp_list[i] = ibv_create_qp(info->pd, &qp_init_attr));
                modify_qp_to_init(info->qp_list[i]);
            }
        }

        info->qp2rank = (uint32_t*)uru_malloc((1<<24) * sizeof(uint32_t));
        memset(info->qp2rank, 255, (1<<24) * sizeof(uint32_t));
        uint32_t *qp_num_list_send, *qp_num_list_recv;
        { /* Exchange peer number */
            qp_num_list_send = (uint32_t*)malloc(sizeof(uint32_t) * uru_mpi->size);
            qp_num_list_recv = (uint32_t*)malloc(sizeof(uint32_t) * uru_mpi->size);
            for (int i = 0; i < uru_mpi->size; i++) {
                qp_num_list_send[i] = info->qp_list[i]->qp_num;
                URU_ASSERT_REASON(info->qp_list[i]->qp_num < (1<<24), "QP number bigger than 1<<24");
                info->qp2rank[info->qp_list[i]->qp_num] = i;
            }

            MPI_Alltoall(qp_num_list_send, 1, MPI_UINT32_T, qp_num_list_recv, 1, MPI_UINT32_T, MPI_COMM_WORLD);

        }

        uint16_t* lid_list;
        union ibv_gid* gid_list = NULL;
        { /* Exchange LID and GID */
            lid_list = (uint16_t*)malloc(sizeof(uint16_t) * uru_mpi->size);
            MPI_Allgather(&info->port_attr.lid, 1, MPI_UINT16_T, lid_list, 1, MPI_UINT16_T, MPI_COMM_WORLD);
            if (info->port_attr.lid == 0) {
                { /* Get gid index */
                    URU_ASSERT_REASON(getenv("UNR_DIR"), "UNR_DIR is not set. Please `source fast/env.sh`");
                    info->gid_idx = system("${UNR_DIR}/scripts/get_gid_idx.sh") >> 8;
                    // printf("Rank %d - RoCE used: gid_idx = %d\n", uru_mpi->rank, info->gid_idx); fflush(stdout);
                }
                union ibv_gid my_gid;
                verbs_check(ibv_query_gid(info->ctx, VERBS_PORT_NUM, info->gid_idx, &my_gid));
                gid_list = (union ibv_gid*)malloc(sizeof(union ibv_gid) * uru_mpi->size);
                MPI_Allgather(&my_gid, 16, MPI_BYTE, gid_list, 16, MPI_BYTE, MPI_COMM_WORLD);
            }
        }

        int my_mtu, max_mtu;
        { /* Exchange MTU */
            my_mtu = info->port_attr.active_mtu;
            MPI_Allreduce(&my_mtu, &max_mtu, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        }

        { /* Change QP state */
            for (int i = 0; i < uru_mpi->size; i++) {
                modify_qp_to_rtr(info->qp_list[i], qp_num_list_recv[i], lid_list[i], &(gid_list[i]), info->gid_idx, (enum ibv_mtu)max_mtu);
                modify_qp_to_rts(info->qp_list[i]);
            }
        }

        { /* Initialize qp counter */
            info->req_submit = (uint64_t*)uru_calloc(uru_mpi->size, sizeof(uint64_t));
            info->req_finish = (uint64_t*)uru_calloc(uru_mpi->size, sizeof(uint64_t));
            for (int i = 0; i < uru_mpi->size; i++) {
                info->req_submit[i] = 0;
                info->req_finish[i] = 0;
            }
        }

        { /* Free space*/
            free(qp_num_list_send);
            free(qp_num_list_recv);
            free(lid_list);
            if (gid_list) {
                free(gid_list);
            }
        }

    }

    pthread_spin_init(&(info->send_lock), PTHREAD_PROCESS_PRIVATE);

    init_flag_pool();

    /* Make sure all the process are initialized */
    {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
    }
    return URU_STATUS_OK;
}

uru_status_t uri_verbs_channel_close(uri_channel_t* channel_p) {
    uri_verbs_channel_t* info = (uri_verbs_channel_t*)channel_p->info;
    ibv_close_device(info->ctx);
    free(info->qp_list);
    free(info->req_submit);
    free(info->req_finish);
    return URU_STATUS_OK;
}

URI_CHANNEL_REGISTER(uri_verbs_get_num_devices, uri_verbs_channel_init);

#undef verbs_check
void verbs_check(int error_code, char* file, int line) {
    if (error_code) {
        printf("[%d/%d] ibverbs ERROR error_code=%d errno=[%s] at %s:%d\n", uru_mpi->rank, uru_mpi->size, error_code, strerror(errno), file, line);
        exit(0);
    }
}