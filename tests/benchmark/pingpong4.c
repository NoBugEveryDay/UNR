#include <unr.h>
// #include <uru/sys/realloc.h>
// #include <uru/sys/time.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define warmup 1000
#define iters 10000
#define max_msg_size 4194304

void* send_buf[max_msg_size];
void* recv_buf[max_msg_size];
unr_mem_h send_mem_h, recv_mem_h;
unr_blk_h send_blk_h, recv_blk_h, rmt_blk_h;
unr_sig_h recv_sig[4], rmt_sig[4];
unr_plan_h plan_h[4];

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

double generateStandardGaussianRandom();
double generateGaussianRandom(double mu, double sigma);

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
    srand(time(NULL));

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
        recv_sig, 4, MPI_UNR_SIG_H, rank ^ 2, 0,
        rmt_sig, 4, MPI_UNR_SIG_H, rank ^ 2, 0,
        MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Barrier(MPI_COMM_WORLD);
    printf("warmup=%d, iters=%d, max_msg_size=%d\n", warmup, iters, max_msg_size);
    MPI_Barrier(MPI_COMM_WORLD);
    for (int mi = 0, msg_size = 1; msg_size <= max_msg_size; msg_size *= 2, mi++) {
        double start_time, end_time;
        for (int i = 0; i < 4; i++) {
            unr_blk_part_rdma_plan(send_blk_h, UNR_NO_SIGNAL, 0, msg_size, rmt_blk_h, rmt_sig[i ^ 2], 0, URU_TRANSFER_TYPE_DMA_PUT, plan_h+i);
        }
        unr_plan_start(plan_h[2]);
        unr_plan_start(plan_h[3]);
        for (int i = 0; i < warmup + iters; i++) {
            if (i == warmup) {
                start_time = MPI_Wtime();
            }
            for (int j = 0; j < 4; j++) {
                unr_sig_wait(recv_sig[j]);
                unr_sig_reset(recv_sig[j]);
                double rand_lat = generateGaussianRandom(1, 0.3);
                // double rand_lat = 1.0;
                for (double time_start = MPI_Wtime(); MPI_Wtime() - time_start < lat[mi] * rand_lat/1000000;);
                unr_plan_start(plan_h[j]);
            }
        }
        unr_sig_wait(recv_sig[0]);
        unr_sig_reset(recv_sig[0]);
        unr_sig_wait(recv_sig[1]);
        unr_sig_reset(recv_sig[1]);
        end_time = MPI_Wtime();
        double time = end_time - start_time;
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        for (int i = 0; i < 4; i++) {
            unr_plan_destroy(plan_h[i]);
        }
        if (rank == 0) {
            printf("msg_size=%d, time=%lf, expect=%lf\n", msg_size, time, lat[mi]*iters*4/1000000);
            fflush(stdout);
        }
    }

    unr_finalize();
    MPI_Finalize();
    return 0;
}

#include <math.h>

/* 生成标准正态分布随机数 */
double generateStandardGaussianRandom() {
    double u1, u2, w, mult;
    static double x1, x2;
    static int call = 0;

    if (call == 1) {
        call = !call;
        return x2;
    }

    do {
        u1 = -1 + ((double)rand() / RAND_MAX) * 2; // 生成-1到1之间的随机数
        u2 = -1 + ((double)rand() / RAND_MAX) * 2; // 同上
        w = pow(u1, 2) + pow(u2, 2);
    } while (w >= 1 || w == 0); // 保证在单位圆内

    mult = sqrt((-2 * log(w)) / w);
    x1 = u1 * mult;
    x2 = u2 * mult;

    call = !call;

    return x1; // 返回生成的随机数
}

/* 生成均值为mu，标准差为sigma的正态分布随机数 */
double generateGaussianRandom(double mu, double sigma) {
    double standardGaussian = generateStandardGaussianRandom(); // 生成标准正态分布随机数
    return sigma * standardGaussian + mu; // 线性变换
}