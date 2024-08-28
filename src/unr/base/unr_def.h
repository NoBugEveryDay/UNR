#pragma once

#include <uri/api/uri.h>
#include <unr.h>
#include <uru/dev/dev.h>
#include <uru/dev/trace.h>
#include <uru/mpi/mpi.h>
#include <uru/sys/realloc.h>
#include <uru/type/uru_status.h>
#include <uru/type/flag.h>
#include <uru/type/queue.h>
#include <stddef.h>

#define UNR_NO_SIGNAL_DST_RANK (-2)

typedef struct unr_channels_s unr_channels_t;
typedef struct unr_mem_s unr_mem_t;
typedef struct unr_blk_s unr_blk_t;
typedef struct unr_div_s unr_div_t;
typedef struct unr_div_unit_s unr_div_unit_t;
typedef struct unr_sig_s unr_sig_t;
typedef struct unr_sig_pool_s unr_sig_pool_t;
typedef struct unr_plan_s unr_plan_t;
typedef struct unr_plan_create_para_s unr_plan_create_para_t;

extern unr_channels_t* unr_channels;
extern MPI_Datatype MPI_UNR_MEM_H;

struct unr_channels_s {

    int num_avail_channels;        /**< Number of channels that user selected to use */
    uri_channel_h* avail_channels; /**< Channels that user selected to use */
    int num_inter_channels;        /**< Number of inter-node channels that user selected to use */
    uri_channel_h* inter_channels; /**< Inter-node channels that user selected to use */
    int num_intra_channels;        /**< Number of intra-node channels that user selected to use */
    uri_channel_h* intra_channels; /**< Intra-node channels that user selected to use */
    int* intra_channel_reuse;      /**< The index of inter-node channel that is same as the intra-node channel */

    size_t* inter_channel_blk_size;    /**< The block size of inter-node channel */
    size_t inter_channel_blk_size_sum; /**< The sum of block size of inter-node channel */
    size_t* intra_channel_blk_size;    /**< The block size of intra-node channel */
    size_t intra_channel_blk_size_sum; /**< The sum of block size of intra-node channel */

    size_t unr_mem_t_size;          /**< The REAL size of unr_mem_t */
    size_t* inter_uri_mem_h_offset; /**< The offset of inter-node uri_mem_h in unr_mem_t */
    size_t* intra_uri_mem_h_offset; /**< The offset of intra-node uri_mem_h in unr_mem_t */

    int loc_mem_arr_len;     /**< The length of loc_mem_arr */
    int* rmt_mem_arr_len;    /**< The length of rmt_mem_arr */
    unr_mem_h* loc_mem_arr;  /**< The list of local registered memory */
    unr_mem_h** rmt_mem_arr; /**< The list of remote registered memory */
};

struct unr_mem_s {
    void* addr;
    size_t size;
    int32_t rank;
    int32_t idx;
    /* Space for num_avail_channels x real_sizeof(uri_mem_h) */
};

struct __attribute__((packed)) unr_blk_s {
    int32_t dst_rank;
    int32_t mem_idx;
    size_t offset;
    uint64_t size;
    uru_flag_t* send_flag_p; /**< The flag that will trigger when this block is sent */
    uru_flag_t* recv_flag_p; /**< The flag that will trigger when this block is received */
};

struct __attribute__((packed)) unr_sig_s {
    /**
     * @brief The rank of the process that the signal belongs to.
     *        If src_rank == dst_rank, then the signal is in the local process.
     *        Otherwise, the signal is in a remote process.
     */
    int32_t dst_rank;

    /**
     * @brief The address of the signal info in its local process.
     * @details The higher 5bits represent the width for event counter.
     */
    uru_flag_t* flag_p;
};

struct unr_sig_pool_s {
    uru_queue_t queue;
    int32_t num_sigs;
    unr_sig_t** sig_arr;
};

struct unr_plan_s {
    /**
     * @brief The initialized status of this plan.
     * @details 0: undefined.
     *          1: Only alloc space because memory handles have NOT synced yet.
     *          2: Alloced space and create the whole plan.
     */
    uint8_t status;
    uint8_t num_uri_plan; /**< Length of uri_plan[] */
    uri_plan_h* uri_plan_h_arr;
    uri_channel_h* uri_channel_h_arr; /**< The channel of each uri_plan_h */
    unr_plan_create_para_t* para;
};

struct unr_plan_create_para_s {
    uint8_t need_current_load;
    uint8_t once_now;
    size_t num_sends;
    unr_blk_t* loc_blk_arr;
    unr_sig_t* loc_sig_arr;
    size_t* loc_offset_arr;
    size_t* size_arr;
    unr_blk_t* rmt_blk_arr;
    unr_sig_t* rmt_sig_arr;
    size_t* rmt_offset_arr;
    uru_transfer_t* dma_type_arr;
};

/**
 * @brief Used by unr_blk_(part_)send_(batch_)start/plan()
 * @param[in] create_later      Whether this function is called to create the plan in a delayed model.
 * @param[in] need_current_load Whether needed to consider the current load of each channel.
 * @param[in] num_sends         The number of RDMA writes.
 * @param[in] loc_blk_arr       The local registered blocks.
 * @param[in] loc_sig_arr       The local signals needed to be triggered.
 *                              It can be NULL if all the signal are assigned when block registering.
 * @param[in] loc_offset_arr    The offset of the start addresses corresponding to the blocks.
 *                              It can be NULL if all the operations start from the beginning of the block.
 * @param[in] size_arr          The size of the data to be written in each RDMA write operation.
 *                              It can be NULL if all the operations write the whole block.
 * @param[in] rmt_blk_arr       The remote registered blocks.
 * @param[in] rmt_sig_arr       The remote signals needed to be triggered.
 *                              It can be NULL if all the signal are assigned when block registering.
 * @param[in] rmt_offset_arr    The offset of the start addresses corresponding to the blocks.
 *                              It can be NULL if all the operations start from the beginning of the block.
 * @param[out] plan_pp          The created plan.
 */
uru_status_t unr_rdma_plan_create(
    uint8_t create_later, uint8_t need_current_load, uint8_t once_now, size_t num_sends,
    unr_blk_t* loc_blk_arr, unr_sig_t* loc_sig_arr, size_t* loc_offset_arr, size_t* size_arr,
    unr_blk_t* rmt_blk_arr, unr_sig_t* rmt_sig_arr, size_t* rmt_offset_arr, uru_transfer_t* dma_type_arr,
    unr_plan_t** plan_pp);