#include <uri/api/uri.h>
#include <uri/base/uri_def.h>

#include <stdio.h>
#include <stdlib.h>

uru_status_t uri_mem_h_size(uri_channel_h channel_h, size_t* size) {
    uri_channel_t* channel_p = (uri_channel_t*)channel_h;
    URU_ASSERT_REASON(channel_p->opened == 1, "Channel is not opened!");
    URU_ASSERT_REASON(channel_p->func_ptrs != NULL, "Memory function pointer is not initialized!");
    URU_ASSERT_REASON(channel_p->func_ptrs->mem_h_size != NULL, "mem_h_size is not implemented!");
    channel_p->func_ptrs->mem_h_size(channel_p, size);
    *size += sizeof(uri_mem_t);
    return URU_STATUS_OK;
}

uru_status_t uri_mem_get_info(uri_channel_t *channel_p, uri_mem_t* mem_p, void** addr, size_t* size, int32_t *rank) {

    typedef struct {
        void* addr;
        size_t size;
        int32_t rank;
        int32_t idx;
    } unr_mem_t;
    unr_mem_t* unr_mem_p = (unr_mem_t*)(((void*)mem_p) - channel_p->mem_h_offset);
    *addr = unr_mem_p->addr;
    *size = unr_mem_p->size;
    *rank = unr_mem_p->rank;
    
    return URU_STATUS_OK;
}

uru_status_t uri_mem_reg(uri_channel_h channel_h, void* mem_addr, size_t mem_size, uri_mem_h mem_h) {
    uri_channel_t* channel_p = (uri_channel_t*)channel_h;
    uri_mem_t* mem_p = (uri_mem_t*)mem_h;
    URU_ASSERT_REASON(channel_p->opened == 1, "Channel is not opened!");
    URU_ASSERT_REASON(channel_p->func_ptrs != NULL, "Memory function pointer is not initialized!");
    URU_ASSERT_REASON(channel_p->func_ptrs->mem_reg != NULL, "mem_reg is not implemented!");
    return channel_p->func_ptrs->mem_reg(channel_p, mem_addr, mem_size, mem_p);
}