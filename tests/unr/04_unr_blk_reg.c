#include <unr.h>
#include <mpi.h>
#include <stdio.h>
#include <uru/sys/realloc.h>
#include <uru/sys/time.h>

const int n = 500, m = 100;

int main(int argc, char** argv) {

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

    unr_init();

    unsigned int rand_seed = time(NULL);
    MPI_Bcast(&rand_seed, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    srand(rand_seed);
    printf("rand_seed=%u\n", rand_seed);

    unr_mem_h mem_h[n];

    for (int i = 0; i < n; i++) {
        size_t mem_size;
        if (i <= 16) {
            mem_size = 1 << i;
        } else {
            mem_size = (rand() % 16) + 1;
        }

        void* mem_addr = uru_malloc(mem_size);
        unr_mem_reg(mem_addr, mem_size, &mem_h[i]);

        for (int j = 0; j < m; j++) {
            size_t blk_offset = rand() % mem_size;
            size_t blk_size = rand() % (mem_size - blk_offset) + 1;
            unr_blk_h blk_h;
            unr_blk_reg(mem_h[i], blk_offset, blk_size, UNR_NO_SIGNAL, UNR_NO_SIGNAL, &blk_h);
        }
    }
    unr_mem_reg_sync();

    unr_finalize();
    MPI_Finalize();
    return 0;
}