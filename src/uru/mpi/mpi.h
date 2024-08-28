#pragma once

#include <uru/type/uru_status.h>
#include <mpi.h>

typedef struct {
    int rank;
    int size;
    char* same_node;
} uru_mpi_t;

extern uru_mpi_t* uru_mpi;

uru_status_t uru_mpi_init();