#include <uri/verbs/uri_verbs.h>

uru_status_t uri_verbs_mem_h_size(uri_channel_t* channel_p, size_t* size) {
    *size = sizeof(uri_verbs_mem_t);
    return URU_STATUS_OK;
}

uru_status_t uri_verbs_mem_reg(uri_channel_t* channel_p, void* mem_addr, size_t mem_size, uri_mem_t* mem_p) {

    uri_verbs_channel_t* info = (uri_verbs_channel_t*)channel_p->info;
    uri_verbs_mem_t* mem_info = (uri_verbs_mem_t*)(mem_p + 1);

    /* Initialize uri_verbs_mem_t */
    URU_ASSERT(mem_info->mr = ibv_reg_mr(info->pd, mem_addr, mem_size, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE));
    mem_info->lkey = mem_info->mr->lkey;
    mem_info->rkey = mem_info->mr->rkey;
    return URU_STATUS_OK;
}