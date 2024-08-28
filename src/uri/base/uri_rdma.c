#include <uri/api/uri.h>
#include <uri/base/uri_def.h>

#include <stdio.h>
#include <stdlib.h>

uru_status_t uri_rdma_plan_create(
    uri_channel_h channel_h, uint8_t once_now, size_t num_wrt, uru_transfer_t* dma_type_arr,
    uri_mem_h* loc_mem_h_arr, size_t* loc_offset_arr, uru_flag_t** loc_flag_p_arr, int64_t* loc_flag_add_arr, uint64_t* dma_size_arr,
    uri_mem_h* rmt_mem_h_arr, size_t* rmt_offset_arr, uru_flag_t** rmt_flag_p_arr, int64_t* rmt_flag_add_arr,
    uri_plan_h* plan_h) {
    
    uri_channel_t* channel_p = (uri_channel_t*)channel_h;
    URU_ASSERT_REASON(channel_p->opened == 1, "Channel is not opened!");
    URU_ASSERT_REASON(channel_p->func_ptrs != NULL, "Memory function pointer is not initialized!");
    URU_ASSERT_REASON(channel_p->func_ptrs->rdma_plan_create != NULL, "rdma_plan_create is not implemented!");
    URU_ASSERT_REASON(dma_type_arr != 0, "dma_type_arr is NULL!");
    return channel_p->func_ptrs->rdma_plan_create(
        channel_p, once_now, num_wrt, dma_type_arr,
        (uri_mem_t**)loc_mem_h_arr, loc_offset_arr, loc_flag_p_arr, loc_flag_add_arr, dma_size_arr,
        (uri_mem_t**)rmt_mem_h_arr, rmt_offset_arr, rmt_flag_p_arr, rmt_flag_add_arr,
        (uri_plan_t**)plan_h);
}

uru_status_t uri_rdma_plan_start(uri_channel_h channel_h, uri_plan_h plan_h) {
    uri_channel_t* channel_p = (uri_channel_t*)channel_h;
    URU_ASSERT_REASON(channel_p->opened == 1, "Channel is not opened!");
    URU_ASSERT_REASON(channel_p->func_ptrs != NULL, "Memory function pointer is not initialized!");
    URU_ASSERT_REASON(channel_p->func_ptrs->rdma_plan_start != NULL, "rdma_plan_start is not implemented!");
    return channel_p->func_ptrs->rdma_plan_start(channel_p, (uri_plan_t*)plan_h);
}

uru_status_t uri_rdma_plan_destroy(uri_channel_h channel_h, uri_plan_h plan_h) {
    uri_channel_t* channel_p = (uri_channel_t*)channel_h;
    URU_ASSERT_REASON(channel_p->opened == 1, "Channel is not opened!");
    URU_ASSERT_REASON(channel_p->func_ptrs != NULL, "Memory function pointer is not initialized!");
    URU_ASSERT_REASON(channel_p->func_ptrs->rdma_plan_destroy != NULL, "rdma_plan_destroy is not implemented!");
    return channel_p->func_ptrs->rdma_plan_destroy(channel_p, (uri_plan_t*)plan_h);
}