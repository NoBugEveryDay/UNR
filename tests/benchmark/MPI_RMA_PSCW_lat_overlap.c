/**
 * @file MPI_RMA_PSCW_lat_overlap.c
 * @author Guangnan Feng (nobugday@gamil.com)
 * @brief This file is used to evaluate the latency of MPI-RMA PSCW with overlapping computation and communication.
 * @version 1.0
 * @date 2024-04-09
 * @copyright Copyright (c) 2024
 */

#include <stdio.h>
#include <unistd.h>
#include <mpi.h>

#define warmup 100
#define iters 1000
#define max_msg_size 4194304

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // 必须有两个进程
    if (world_size != 2) {
        fprintf(stderr, "World size must be two for %s\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (int msg_size = 1; msg_size <= max_msg_size; msg_size *= 2) {
        double start_time, end_time, sleep_start, sleep_stop, sleep_time = 0, sleep_time_total;
        MPI_Win win;
        void *ping_pong_buffer;
        MPI_Win_allocate(msg_size, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &ping_pong_buffer, &win);
        MPI_Group comm_group, group;
        MPI_Comm_group(MPI_COMM_WORLD, &comm_group);
        int destrank = 1 - world_rank;
        MPI_Group_incl(comm_group, 1, &destrank, &group);
        MPI_Barrier(MPI_COMM_WORLD);

        for (int i = 0; i < warmup + iters; i++) {
            if (i == warmup) {
                start_time = MPI_Wtime();
            }

            if (world_rank == 0) {
                MPI_Win_start(group, 0, win);
                MPI_Put(ping_pong_buffer, msg_size, MPI_BYTE, destrank, 0, msg_size, MPI_BYTE, win);
                if (i >= warmup) {
                    sleep_start = MPI_Wtime();
                    if (usleep(10)) printf("usleep failed\n");
                    sleep_stop = MPI_Wtime();
                    sleep_time += sleep_stop - sleep_start;
                }
                MPI_Win_complete(win);
                MPI_Win_post(group, 0, win);
                // if (i >= warmup) {
                //     sleep_start = MPI_Wtime();
                //     if (usleep(10)) printf("usleep failed\n");
                //     sleep_stop = MPI_Wtime();
                //     sleep_time += sleep_stop - sleep_start;
                // }
                MPI_Win_wait(win);
            }
            else {
                MPI_Win_post(group, 0, win);
                MPI_Win_wait(win);
                // if (i >= warmup) {
                //     sleep_start = MPI_Wtime();
                //     if (usleep(5)) printf("usleep failed\n");
                //     sleep_stop = MPI_Wtime();
                //     sleep_time += sleep_stop - sleep_start;
                // }
                MPI_Win_start(group, 0, win);
                MPI_Put(ping_pong_buffer, msg_size, MPI_BYTE, destrank, 0, msg_size, MPI_BYTE, win);
                if (i >= warmup) {
                    sleep_start = MPI_Wtime();
                    if (usleep(10)) printf("usleep failed\n");
                    sleep_stop = MPI_Wtime();
                    sleep_time += sleep_stop - sleep_start;
                }
                MPI_Win_complete(win);
            }
        }
        end_time = MPI_Wtime();
        MPI_Allreduce(&sleep_time, &sleep_time_total, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        double time = end_time - start_time;
        time -= sleep_time_total;
        // time -= sleep_time;
        if (world_rank == 0) {
            printf("msg_size=%d, latency=%f\n", msg_size, time*1000000/(iters*2));
            fflush(stdout);
        }
        MPI_Win_free(&win);
    }

    MPI_Finalize();
    return 0;
}
