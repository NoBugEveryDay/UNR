#include <uri/api/uri.h>
#include <uru/dev/trace.h>
#include <uru/mpi/mpi.h>
#include <uru/sys/realloc.h>
#include <uru/sys/time.h>
#include <uru/type/flag.h>
#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    void* addr;
    size_t size;
    int32_t rank;
    int32_t idx;
    /* space for uri_mem_h */
} test_mem_t;

pthread_t daemon_thread_h;
volatile char daemon_thread_continue = 1;
volatile char daemon_thread_running = 1;
void* daemon_thread(void* arg) {
    uri_channel_h channel = arg;
    while (daemon_thread_continue) {
        uri_channel_daemon(channel);
    }
    daemon_thread_running = 0;
    return NULL;
}

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

    if (uru_mpi->size != 2) {
        printf("This test should be run with 2 processes\n");
        exit(1);
    }

    uri_channel_h* uri_channel_arr;
    int uri_num_channels;
    uri_query_channel_list(&uri_channel_arr, &uri_num_channels);
    printf("Available URI Channel list:\n");
    for (int i = 0; i < uri_num_channels; i++) {
        uri_channel_attr_t* attr;
        uri_channel_attr_get(uri_channel_arr[i], &attr);
        printf("- %s: %s\n", attr->name, attr->description);

        uri_channel_open(uri_channel_arr[i], sizeof(test_mem_t));
        daemon_thread_continue = 1;
        daemon_thread_running = 1;
        pthread_create(&daemon_thread_h, NULL, daemon_thread, uri_channel_arr[i]);
        size_t uri_mem_size;
        uri_mem_h_size(uri_channel_arr[i], &uri_mem_size);
        for (int mem_size = 2; mem_size <= 65536; mem_size *= 2) {
            test_mem_t* loc_mem_p = (test_mem_t*)uru_malloc(sizeof(test_mem_t) + uri_mem_size);
            test_mem_t* rmt_mem_p = (test_mem_t*)uru_malloc(sizeof(test_mem_t) + uri_mem_size);
            loc_mem_p->addr = uru_malloc(mem_size);
            loc_mem_p->size = mem_size;
            loc_mem_p->rank = uru_mpi->rank;
            uri_mem_h loc_mem_h = (uri_mem_h)(loc_mem_p + 1);
            uri_mem_h rmt_mem_h = (uri_mem_h)(rmt_mem_p + 1);
            uri_mem_reg(uri_channel_arr[i], loc_mem_p->addr, loc_mem_p->size, loc_mem_h);
            MPI_Sendrecv(
                loc_mem_p, sizeof(test_mem_t) + uri_mem_size, MPI_BYTE, 1 - uru_mpi->rank, 0,
                rmt_mem_p, sizeof(test_mem_t) + uri_mem_size, MPI_BYTE, 1 - uru_mpi->rank, 0,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            size_t num_wrt = mem_size / 2;
            uru_flag_t* send_flag = uru_flag_pool_alloc();
            uru_flag_t* recv_flag = uru_flag_pool_alloc();
            uru_flag_t* rmt_flag;
            send_flag->counter = num_wrt;
            recv_flag->counter = num_wrt;
            MPI_Sendrecv(
                &recv_flag, sizeof(uru_flag_t*), MPI_BYTE, 1 - uru_mpi->rank, 0,
                &rmt_flag, sizeof(uru_flag_t*), MPI_BYTE, 1 - uru_mpi->rank, 0,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            char* arr = (char*)loc_mem_p->addr;
            uri_mem_h* loc_mem_h_arr = (uri_mem_h*)uru_malloc(num_wrt * sizeof(uri_mem_h));
            uri_mem_h* rmt_mem_h_arr = (uri_mem_h*)uru_malloc(num_wrt * sizeof(uri_mem_h));
            size_t* loc_offset_arr = (size_t*)uru_malloc(num_wrt * sizeof(size_t));
            size_t* rmt_offset_arr = (size_t*)uru_malloc(num_wrt * sizeof(size_t));
            uru_flag_t** loc_flag_p_arr = (uru_flag_t**)uru_malloc(num_wrt * sizeof(uru_flag_t*));
            uru_flag_t** rmt_flag_p_arr = (uru_flag_t**)uru_malloc(num_wrt * sizeof(uru_flag_t*));
            int64_t* loc_flag_add_arr = (int64_t*)uru_malloc(num_wrt * sizeof(int64_t));
            int64_t* rmt_flag_add_arr = (int64_t*)uru_malloc(num_wrt * sizeof(int64_t));
            uint64_t* dma_size_arr = (uint64_t*)uru_malloc(num_wrt * sizeof(uint64_t));
            uru_transfer_t* dma_type_arr = (uru_transfer_t*)uru_malloc(num_wrt * sizeof(uru_transfer_t));
            for (int i = 0; i < num_wrt; i++) {
                loc_mem_h_arr[i] = loc_mem_h;
                rmt_mem_h_arr[i] = rmt_mem_h;
                loc_offset_arr[i] = i;
                rmt_offset_arr[i] = mem_size / 2 + i;
                loc_flag_p_arr[i] = send_flag;
                rmt_flag_p_arr[i] = rmt_flag;
                loc_flag_add_arr[i] = -1;
                rmt_flag_add_arr[i] = -1;
                dma_size_arr[i] = 1;
                if (i % 2 == 0) {
                    dma_type_arr[i] = URU_TRANSFER_TYPE_DMA_PUT;
                    arr[i] = i % 97;
                } else {
                    dma_type_arr[i] = URU_TRANSFER_TYPE_DMA_GET;
                    arr[i + mem_size / 2] = i % 97;
                }
            }
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Barrier(MPI_COMM_WORLD);
            uri_plan_h plan_h;
            uri_rdma_plan_create(
                uri_channel_arr[i], 0, num_wrt, dma_type_arr,
                loc_mem_h_arr, loc_offset_arr, loc_flag_p_arr, loc_flag_add_arr, dma_size_arr,
                rmt_mem_h_arr, rmt_offset_arr, rmt_flag_p_arr, rmt_flag_add_arr,
                &plan_h);
            uri_rdma_plan_start(uri_channel_arr[i], plan_h);

            for (uru_cpu_cyc_t start = uru_cpu_cyc_now(), now = start;
                 recv_flag->counter != 0 || send_flag->counter != 0;
                 now = uru_cpu_cyc_now()) {
                if (uru_time_diff(start, now) > 20) {
                    printf("Timeout @ mem_size = %d!\n", mem_size);
                    return 1;
                }
            }
            for (int i = 0; i < num_wrt; i++) {
                if (arr[i] != arr[mem_size / 2 + i]) {
                    printf("Error: arr[%d] = %d, arr[%d] = %d\n", i, arr[i], mem_size / 2 + i, arr[mem_size / 2 + i]);
                    return 1;
                }
            }
            
            MPI_Barrier(MPI_COMM_WORLD);
            /* It may cause segment fault in destroy if the RDMA write communication is not finished before destroy */
            MPI_Barrier(MPI_COMM_WORLD);
            uri_rdma_plan_destroy(uri_channel_arr[i], plan_h);
        }

        daemon_thread_continue = 0;
        while (daemon_thread_running) {
            /* Do nothing and wait */
        };
        uri_channel_close(uri_channel_arr[i]);
    }
    MPI_Finalize();
    return 0;
}