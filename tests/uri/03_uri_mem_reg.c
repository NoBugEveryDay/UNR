#include <uri/api/uri.h>
#include <uru/dev/trace.h>
#include <uru/mpi/mpi.h>
#include <uru/sys/realloc.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    void* addr;
    size_t size;
    /* space for uri_mem_h */
} test_mem_t;

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

        uri_channel_open(uri_channel_arr[i], sizeof(void*) + sizeof(size_t));
        size_t uri_mem_size;
        uri_mem_h_size(uri_channel_arr[i], &uri_mem_size);
        for (int mem_size = 1; mem_size <= 65536; mem_size *= 2) {
            test_mem_t* mem_p = (test_mem_t*)uru_malloc(sizeof(test_mem_t) + uri_mem_size);
            mem_p->addr = uru_malloc(mem_size);
            mem_p->size = mem_size;
            uri_mem_h mem_h = (uri_mem_h)(mem_p + 1);
            uri_mem_reg(uri_channel_arr[i], mem_p->addr, mem_p->size, mem_h);
        }
        uri_channel_close(uri_channel_arr[i]);
    }
    MPI_Finalize();
    return 0;
}