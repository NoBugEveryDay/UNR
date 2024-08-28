#include <uri/base/uri_def.h>
#include <uru/dev/trace.h>
#include <uru/mpi/mpi.h>
#include <uru/sys/realloc.h>
#include <mpi.h>
#include <pthread.h>

extern uri_channel_func_ptrs_t uri_mpi_channel_func_ptrs;

typedef struct uri_mpi_channel_s uri_mpi_channel_t;
typedef struct uri_mpi_mem_s uri_mpi_mem_t;
typedef struct uri_mpi_plan_s uri_mpi_plan_t;
typedef struct uri_mpi_op_s uri_mpi_op_t;
typedef struct uri_mpi_op_batch_s uri_mpi_op_batch_t;
typedef struct uri_mpi_ctrl_msg_s uri_mpi_ctrl_msg_t;
typedef struct uri_mpi_int_pool_s uri_mpi_int_pool_t;

struct uri_mpi_channel_s {
    enum {
        RMA,
        NONBLOCKING_SENDRECV
    } mode;

    MPI_Comm ctrl_comm;
    MPI_Comm data_comm;

    uri_mpi_int_pool_t* mpi_tag;
    uri_mpi_int_pool_t* batch_id;

    struct {
        pthread_spinlock_t lock;
        MPI_Request* req_arr;
        uri_mpi_op_t** op_arr;
        size_t arr_len;
        volatile size_t arr_tail;
    } mpi_req;

    uri_mpi_op_batch_t*** recv_batch;
    int* recv_batch_size;
};

/**
 * @brief A pool that assigned recycles integer with thread safety.
 */
struct uri_mpi_int_pool_s {
    pthread_spinlock_t lock;
    int max;
    int* recycle_stack;
    size_t recycle_top;
    size_t recycle_len;
};

struct uri_mpi_mem_s {
    /* Empty */
};

struct uri_mpi_plan_s {
    uri_mpi_op_batch_t* send_batch;
    uint32_t num_receiver;
    uint8_t padding[4];
    uri_mpi_op_batch_t** recv_batch;
};

struct uri_mpi_op_batch_s {

    int32_t num_op; /**< Number of uri_mpi_op_t */

    /**
     * @brief Represents the index of this batch operation in both sender and receiver side.
     * @details If idx == -1, this batch operation is used for sending.
     *          If idx != -1, this batch operation is used for receiving, and its index in the receiver side is `idx`.
     */
    int32_t batch_id;

    /* Space for num_op * uri_mpi_op_t */
};

struct uri_mpi_op_s {
    void* addr;
    size_t size;
    uru_flag_t *flag_p;
    int64_t flag_add;
    /**
     * @brief In sender process, it represents destination rank.
     *        In receiver process, it represents the source rank.
     */
    int rank;
    int tag;
    MPI_Request mpi_req;

    enum {
        FREE_OP = 1 << 0,   /**< Indicate the the uri_mpi_op_t and its addr should be freed after operation */
        KEEP_OP = 1 << 1,   /**< Indicate the the uri_mpi_op_t and its addr should NOT be freed after operation */
        FREE_ADDR = 1 << 2, /**< Indicate the the addr should be freed after operation */
        KEEP_ADDR = 1 << 3, /**< Indicate the the addr should NOT be freed after operation */
        FREE_REQ = 1 << 4,  /**< Indicate the the MPI_Request should be freed after operation */
        KEEP_REQ = 1 << 5,  /**< Indicate the the MPI_Request should NOT be freed after operation */
        FREE_TAG = 1 << 6,  /**< Indicate the the tag should be freed after operation */
        KEEP_TAG = 1 << 7   /**< Indicate the the tag should NOT be freed after operation */
    } flag; /**< Indicate the operation flag of this operation */

    enum {
        OP_SEND = 1 << 1, /**< Indicate the operation is MPI_Send */
        OP_RECV = 1 << 2  /**< Indicate the operation is MPI_Recv */
    } op_type; /**< Indicate the type of this operation */
};

struct uri_mpi_ctrl_msg_s {
    enum {
        invalid,
        enlarge_max_ctrl_msg_size,
        create_op_batch,
        destroy_op_batch,
        start_op_batch,
        execute_op_batch, /**< = create + start + destroy op_batch */
    } type;
    union {
        struct {
            size_t new_max_size;
        } enlarge_max_ctrl_msg_size;
        struct {
            int batch_id;
        } create_op_batch;
        struct {
            int batch_id;
        } destroy_op_batch;
        struct {
            int batch_id;
        } start_op_batch;
    } data;
};

uru_status_t uri_mpi_channel_open(uri_channel_t* channel_p);

uru_status_t uri_mpi_channel_daemon(uri_channel_t* channel_p);

uru_status_t uri_mpi_channel_close(uri_channel_t* channel_p);

uru_status_t uri_mpi_mem_h_size(uri_channel_t* channel_p, size_t* size);

uru_status_t uri_mpi_mem_reg(uri_channel_t* channel_p, void* mem_addr, size_t mem_size, uri_mem_t* mem_p);

uru_status_t uri_mpi_rdma_plan_create(
    uri_channel_t* channel_p, uint8_t once_now, size_t num_wrt, uru_transfer_t* dma_type_arr,
    uri_mem_t** loc_mem_pp, size_t* loc_offset_arr, uru_flag_t** loc_flag_p_arr, int64_t* loc_flag_add_arr, uint64_t* dma_size_arr,
    uri_mem_t** rmt_mem_pp, size_t* rmt_offset_arr, uru_flag_t** rmt_flag_p_arr, int64_t* rmt_flag_add_arr,
    uri_plan_t** plan_pp);

uru_status_t uri_mpi_rdma_plan_start(uri_channel_t* channel_p, uri_plan_t* plan_p);

/**
 * @attention This function may cause segment fault if corresponding communication is not finished before calling this function.
 */
uru_status_t uri_mpi_rdma_plan_destroy(uri_channel_t* channel_p, uri_plan_t* plan_p);

/* The following functions are defined in uri_mpi_utils.c */

uru_status_t uri_mpi_int_pool_get(uri_mpi_int_pool_t* pool, int* ret);

uru_status_t uri_mpi_int_pool_return(uri_mpi_int_pool_t* pool, int num);

int uri_mpi_op_cmp(const void* a, const void* b);

uru_status_t uri_mpi_req_que_app(uri_channel_t* channel_p, size_t num_op, uri_mpi_op_t** mpi_op);