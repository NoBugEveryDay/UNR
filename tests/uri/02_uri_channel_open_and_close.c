#include <uri/api/uri.h>
#include <uru/dev/trace.h>
#include <uru/mpi/mpi.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {

    int provided, ret;
    ret = MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Init_thread failed\n");
        exit(EXIT_FAILURE);
    }
    MPI_Query_thread(&provided);
    if (provided != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI was not initialized with MPI_THREAD_MULTIPLE\n");
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }
    uru_mpi_init();

    uri_channel_h* uri_channel_arr;
    int uri_num_channels;
    uri_query_channel_list(&uri_channel_arr, &uri_num_channels);
    printf("Available URI Channel list:\n");
    for (int i = 0; i < uri_num_channels; i++) {
        uri_channel_attr_t* attr;
        uri_channel_attr_get(uri_channel_arr[i], &attr);
        printf("- %s: %s\n", attr->name, attr->description);

        uri_channel_open(uri_channel_arr[i], 0);
        uri_channel_close(uri_channel_arr[i]);
    }
    MPI_Finalize();
    return 0;
}