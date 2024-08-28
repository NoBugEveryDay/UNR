#include <stdio.h>
#include <uri/api/uri.h>

int main() {
    uri_channel_h *uri_channel_arr;
    int uri_num_channels;
    uri_query_channel_list(&uri_channel_arr, &uri_num_channels);
    printf("Available URI Channel list:\n");
    for (int i = 0; i < uri_num_channels; i++) {
        uri_channel_attr_t *attr;
        uri_channel_attr_get(uri_channel_arr[i], &attr);
        printf("- %s: %s\n", attr->name, attr->description);
    }
    return 0;
}