#include <uri/mpi/uri_mpi.h>

uru_status_t uri_mpi_mem_h_size(uri_channel_t* channel_p, size_t* size) {
    *size = sizeof(uri_mpi_mem_t);
    return URU_STATUS_OK;
}

uru_status_t uri_mpi_mem_reg(uri_channel_t* channel_p, void* mem_addr, size_t mem_size, uri_mem_t* mem_p) {
    return URU_STATUS_OK;
}