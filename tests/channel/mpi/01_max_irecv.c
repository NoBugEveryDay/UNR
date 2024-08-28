/**
 * @file 01_max_irecv.c
 * @author Guangnan Feng (nobugday@gamil.com)
 * @brief Some MPI implementation have a limit number of outstanding MPI_Irecv.
 *  This program is used to find the number.
 * @version 1.0
 * @date 2024-01-09
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int a;
    for (int i = 0; i < 262144; i++) {
        MPI_Request req;
        MPI_Irecv(&a, 1, MPI_INT, 0, i, MPI_COMM_WORLD, &req);
    }
    printf("OK @ 262144\n");
    for (int i = 262144; 1; i++) {
        MPI_Request req;
        MPI_Irecv(&a, 1, MPI_INT, 0, i, MPI_COMM_WORLD, &req);
        printf("OK @ %d\n", i);
    }

    return 0;
}