#include <uri/api/uri.h>
#include <uri/base/uri_def.h>

#include <stdio.h>
#include <stdlib.h>

uri_channel_t *uri_channel_arr[URI_MAX_CHANNEL_NUM];
int uri_num_channels = 0;

uru_status_t uri_query_channel_list(uri_channel_h** channel_h_pp, int* num_channels) {
    URU_ASSERT_REASON(uri_num_channels >= 1, "No URI channel is available!");
    *channel_h_pp = (uri_channel_h*)uri_channel_arr;
    *num_channels = uri_num_channels;
    return URU_STATUS_OK;
}

uru_status_t uri_channel_attr_get(uri_channel_h channel_h, uri_channel_attr_t** attr_pp) {
    uri_channel_t* channel_p = (uri_channel_t*)channel_h;
    *attr_pp = (uri_channel_attr_t*)(&(channel_p->attr));
    return URU_STATUS_OK;
}

uru_status_t uri_channel_open(uri_channel_h channel_h, size_t mem_h_offset) {
    uri_channel_t* channel_p = (uri_channel_t*)channel_h;
    URU_ASSERT(channel_p != NULL);
    if (channel_p->opened == 1) {
        URU_LOG("%s channel is already opened!\n", channel_p->attr.name);
        return URU_CHANNEL_OPENED;
    }
    channel_p->opened = 1;
    channel_p->mem_h_offset = mem_h_offset;
    URU_ASSERT_REASON(channel_p->func_ptrs != NULL, "Channel function pointer is not initialized!");
    URU_ASSERT_REASON(channel_p->func_ptrs->channel_open != NULL, "channel_open is not implemented!");
    URU_LOG("%s channel opening...\n", channel_p->attr.name);
    return channel_p->func_ptrs->channel_open(channel_p);
}

uru_status_t uri_channel_daemon(uri_channel_h channel_h) {
    uri_channel_t* channel_p = (uri_channel_t*)channel_h;
    URU_ASSERT_REASON(channel_p->opened == 1, "Channel is not opened!");
    URU_ASSERT_REASON(channel_p->func_ptrs != NULL, "Channel function pointer is not initialized!");
    URU_ASSERT_REASON(channel_p->func_ptrs->channel_daemon != NULL, "channel_daemon is not implemented!");
    return channel_p->func_ptrs->channel_daemon(channel_p);
}

uru_status_t uri_channel_close(uri_channel_h channel_h) {
    uri_channel_t* channel_p = (uri_channel_t*)channel_h;
    URU_ASSERT_REASON(channel_p->opened == 1, "Channel is not opened!");
    channel_p->opened = 0;
    URU_ASSERT_REASON(channel_p->func_ptrs != NULL, "Channel function pointer is not initialized!");
    URU_ASSERT_REASON(channel_p->func_ptrs->channel_close != NULL, "channel_close is not implemented!");
    URU_LOG("%s channel closing...\n", channel_p->attr.name);
    return channel_p->func_ptrs->channel_close(channel_p);
}