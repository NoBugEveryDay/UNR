#include <unr.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define warmup 1000
#define iters 10000
#define max_msg_size 4194304

void* send_buf[max_msg_size];
void* recv_buf[max_msg_size];
unr_mem_h send_mem_h, recv_mem_h;
unr_blk_h send_blk_h, recv_blk_h, rmt_blk_h;
unr_sig_h recv_sig[2], rmt_sig[2];
unr_plan_h plan_h[2];

double lat[23] = {
    2.357436,
    2.27529,
    2.272836,
    2.259317,
    2.260311,
    2.26145,
    2.268455,
    2.724365,
    2.740242,
    2.79768,
    2.907723,
    3.040638,
    3.271607,
    3.793507,
    4.552476,
    6.291646,
    9.73556,
    16.403941,
    29.938766,
    56.957344,
    111.006456,
    219.053097,
    435.266066
};

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
    srand(0);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size != 4) {
        fprintf(stderr, "This test requires exactly 4 processes\n");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < 4; i++) {
        unr_sig_create(&recv_sig[i], 1);
    }
    unr_mem_reg(send_buf, max_msg_size, &send_mem_h);
    unr_mem_reg(recv_buf, max_msg_size, &recv_mem_h);
    unr_mem_reg_sync();
    unr_blk_reg(send_mem_h, 0, max_msg_size, UNR_NO_SIGNAL, UNR_NO_SIGNAL, &send_blk_h);
    unr_blk_reg(recv_mem_h, 0, max_msg_size, UNR_NO_SIGNAL, UNR_NO_SIGNAL, &recv_blk_h);
    MPI_Sendrecv(
        &recv_blk_h, 1, MPI_UNR_BLK_H, rank ^ 2, 0,
        &rmt_blk_h, 1, MPI_UNR_BLK_H, rank ^ 2, 0,
        MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Sendrecv(
        recv_sig, 2, MPI_UNR_SIG_H, rank ^ 2, 0,
        rmt_sig, 2, MPI_UNR_SIG_H, rank ^ 2, 0,
        MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Barrier(MPI_COMM_WORLD);
    printf("warmup=%d, iters=%d, max_msg_size=%d\n", warmup, iters, max_msg_size);
    MPI_Barrier(MPI_COMM_WORLD);
    for (int mi = 0, msg_size = 1; msg_size <= max_msg_size; msg_size *= 2, mi++) {
        double start_time, end_time;
        unr_blk_part_rdma_plan(send_blk_h, UNR_NO_SIGNAL, 0, msg_size, rmt_blk_h, rmt_sig[1], 0, URU_TRANSFER_TYPE_DMA_PUT, plan_h+0);
        unr_blk_part_rdma_plan(send_blk_h, UNR_NO_SIGNAL, 0, msg_size, rmt_blk_h, rmt_sig[0], 0, URU_TRANSFER_TYPE_DMA_PUT, plan_h+1);
        unr_plan_start(plan_h[1]);
        for (int i = 0; i < warmup + iters; i++) {
            if (i == warmup) {
                start_time = MPI_Wtime();
            }
            for (int j = 0; j < 2; j++) {
                unr_sig_wait(recv_sig[j]);
                unr_sig_reset(recv_sig[j]);
                for (double time_start = MPI_Wtime(); MPI_Wtime() - time_start < lat[mi]/1000000;);
                unr_plan_start(plan_h[j]);
            }
        }
        unr_sig_wait(recv_sig[0]);
        unr_sig_reset(recv_sig[0]);
        end_time = MPI_Wtime();
        double time = end_time - start_time;
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        unr_plan_destroy(plan_h[0]);
        unr_plan_destroy(plan_h[1]);
        if (rank == 0) {
            printf("msg_size=%d, time=%lf, expect=%lf\n", msg_size, time, lat[mi]*iters*4/1000000);
            fflush(stdout);
        }
    }

    unr_finalize();
    MPI_Finalize();
    return 0;
}