#include <uru/dev/dev.h>
#include <uru/mpi/mpi.h>
#include <uru/sys/realloc.h>
#include <stdlib.h>
#include <string.h>

uru_mpi_t* uru_mpi = NULL;

uru_status_t uru_mpi_init() {

    if (uru_mpi != NULL) {
        return URU_STATUS_OK;
    }

    int mpi_initialized;
    MPI_Initialized(&mpi_initialized);
    URU_ASSERT_REASON(mpi_initialized, "MPI should be initialized before calling %s", __func__);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    uru_mpi = (uru_mpi_t*)uru_malloc(sizeof(uru_mpi_t) + sizeof(char) * size);
    uru_mpi->rank = rank;
    uru_mpi->size = size;
    uru_mpi->same_node = (char*)(uru_mpi + 1);

    /* Determine the processes that on the same node */
    char hostname[MPI_MAX_PROCESSOR_NAME];
    int length;
    MPI_Get_processor_name(hostname, &length);
    char* hosts = (char*)uru_malloc(sizeof(char) * MPI_MAX_PROCESSOR_NAME * size);
    MPI_Allgather(hostname, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, hosts, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, MPI_COMM_WORLD);
    for (int i = 0; i < size; i++) {
        if (strcmp(hostname, hosts + i * MPI_MAX_PROCESSOR_NAME) == 0) {
            uru_mpi->same_node[i] = 1;
        } else {
            uru_mpi->same_node[i] = 0;
        }
    }
    free(hosts);

    return URU_STATUS_OK;
}