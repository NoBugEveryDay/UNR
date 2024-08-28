#include <uri/base/uri_def.h>
#include <uru/mpi/mpi.h>
#include <infiniband/verbs.h>
#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define VERBS_PORT_NUM 1
#define VERBS_OUTSTANDING_SEND_REQUESTS 256
#define VERBS_MAX_INLINE_DATA 96

extern uri_channel_func_ptrs_t uri_verbs_channel_func_ptrs;

typedef struct uri_verbs_channel_s uri_verbs_channel_t;
typedef struct uri_verbs_mem_s uri_verbs_mem_t;
typedef struct uri_verbs_plan_s uri_verbs_plan_t;
typedef struct verbs_imm_protocol_s verbs_imm_protocol_t;
typedef struct verbs_wrid_protocol_s verbs_wrid_protocol_t;

struct uri_verbs_channel_s {
    struct ibv_device* dev;
    struct ibv_context* ctx;
    struct ibv_device_attr dev_attr;
    struct ibv_port_attr port_attr;
    struct ibv_pd* pd;
    struct ibv_srq* srq;
    struct ibv_qp** qp_list;
    struct ibv_cq* shared_cq;
    int gid_idx;
    pthread_spinlock_t send_lock;
    uint64_t* req_submit;
    uint64_t* req_finish;
    uint32_t* qp2rank;
};

struct uri_verbs_mem_s {
    uint32_t lkey;
    uint32_t rkey;
    struct ibv_mr* mr;
};

struct uri_verbs_plan_s {
    size_t num_wr;
    struct ibv_send_wr* wr;
    struct ibv_sge* sg_list;
    int *dst_rank;
};

struct verbs_imm_protocol_s {
    uint32_t flag_p32;
};

struct verbs_wrid_protocol_s {
    verbs_imm_protocol_t rmt_imm_protocol;
    verbs_imm_protocol_t imm_protocol;
};

uru_status_t uri_verbs_channel_open(uri_channel_t* channel_p);

uru_status_t uri_verbs_channel_daemon(uri_channel_t* channel_p);

uru_status_t uri_verbs_channel_close(uri_channel_t* channel_p);

uru_status_t uri_verbs_mem_h_size(uri_channel_t* channel_p, size_t* size);

uru_status_t uri_verbs_mem_reg(uri_channel_t* channel_p, void* mem_addr, size_t mem_size, uri_mem_t* mem_p);

uru_status_t uri_verbs_rdma_plan_create(
    uri_channel_t* channel_p, uint8_t once_now, size_t num_wrt, uru_transfer_t* dma_type_arr,
    uri_mem_t** loc_mem_pp, size_t* loc_offset_arr, uru_flag_t** loc_flag_p_arr, int64_t* loc_flag_add_arr, uint64_t* dma_size_arr,
    uri_mem_t** rmt_mem_pp, size_t* rmt_offset_arr, uru_flag_t** rmt_flag_p_arr, int64_t* rmt_flag_add_arr,
    uri_plan_t** plan_pp);

uru_status_t uri_verbs_rdma_plan_start(uri_channel_t* channel_p, uri_plan_t* plan_p);

uru_status_t uri_verbs_rdma_plan_destroy(uri_channel_t* channel_p, uri_plan_t* plan_p);

uru_status_t uri_verbs_event_fill(uri_verbs_channel_t* info, verbs_imm_protocol_t* event, int dst_rank, uru_flag_t* flag_p, int64_t add_num);

void uri_verbs_event_progress(uri_verbs_channel_t* info, verbs_imm_protocol_t* event);

void verbs_check(int error_code, char* file, int line);

#define verbs_check(x) verbs_check(x, __FILE__, __LINE__)

/* The following functions are defined in uri_glex_utils.c */

void modify_qp_to_init(struct ibv_qp* qp);
void modify_qp_to_rtr(struct ibv_qp* qp, uint32_t remote_qpn, uint16_t dlid, union ibv_gid* dgid, int gid_idx, enum ibv_mtu active_mtu);
void modify_qp_to_rts(struct ibv_qp* qp);
uru_status_t uri_verbs_send_rmt_complete(uri_verbs_channel_t* info, int dst_rank, verbs_imm_protocol_t rmt_imm_protocol);