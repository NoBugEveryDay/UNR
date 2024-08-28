#include <unr.h>
#include <unr/base/unr_def.h>
#include <uru/sys/time.h>
#include <stdio.h>

#define MAX_SIZE (1 << 20)
#define TEST_TIMES 128

int rank, size;

void mpi_alltoallv_test(size_t msg_size) {
    char* send_buf1 = (char*)malloc(size * msg_size);
    char* send_buf2 = (char*)malloc(size * msg_size);
    char* recv_buf1 = (char*)malloc(size * msg_size);
    char* recv_buf2 = (char*)malloc(size * msg_size);
    char* ans_buf = (char*)malloc(size * msg_size);
    int* send_counts = (int*)malloc(size * sizeof(int));
    int* recv_counts = (int*)malloc(size * sizeof(int));
    int* send_displs = (int*)malloc(size * sizeof(int));
    int* recv_displs = (int*)malloc(size * sizeof(int));
    for (int i = 0; i < size; i++) {
        send_counts[i] = msg_size;
        recv_counts[i] = msg_size;
        send_displs[i] = i * msg_size;
        recv_displs[i] = i * msg_size;
        srand(rank * size + i);
        for (int j = i * msg_size; j < (i + 1) * msg_size; j++) {
            int tmp = rand() % 111;
            send_buf1[j] = tmp;
            send_buf2[j] = tmp;
        }
        srand(i * size + rank);
        for (int j = i * msg_size; j < (i + 1) * msg_size; j++) {
            ans_buf[j] = rand() % 111;
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Alltoallv(send_buf1, send_counts, send_displs, MPI_CHAR, recv_buf1, recv_counts, recv_displs, MPI_CHAR, MPI_COMM_WORLD);
    for (int i = 0; i < size * msg_size; i++) {
        if (recv_buf1[i] != ans_buf[i]) {
            printf("MPI_Alltoallv wrong answer\n");
            exit(1);
        }
    }
    MPI_Alltoallv(send_buf2, send_counts, send_displs, MPI_CHAR, recv_buf2, recv_counts, recv_displs, MPI_CHAR, MPI_COMM_WORLD);
    for (int i = 0; i < size * msg_size; i++) {
        if (recv_buf2[i] != ans_buf[i]) {
            printf("MPI_Alltoallv wrong answer\n");
            exit(1);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    uru_cpu_cyc_t start = uru_cpu_cyc_now();
    for (int i = 0; i < TEST_TIMES; i += 2) {
        MPI_Alltoallv(send_buf1, send_counts, send_displs, MPI_CHAR, recv_buf1, recv_counts, recv_displs, MPI_CHAR, MPI_COMM_WORLD);
        MPI_Alltoallv(send_buf2, send_counts, send_displs, MPI_CHAR, recv_buf2, recv_counts, recv_displs, MPI_CHAR, MPI_COMM_WORLD);
    }
    uru_cpu_cyc_t end = uru_cpu_cyc_now();
    double my_time = uru_time_diff(start, end);
    double avg_time, min_time, max_time;
    MPI_Reduce(&my_time, &avg_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&my_time, &min_time, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&my_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        double avg_speed = TEST_TIMES * size * size * msg_size / avg_time / 1e9;
        double min_speed = TEST_TIMES * size * msg_size / max_time / 1e9;
        double max_speed = TEST_TIMES * size * msg_size / min_time / 1e9;
        printf("MPI_Alltoallv: msg_size = %ld, avg_speed = %lf GB/s, min_speed = %lf GB/s, max_speed = %lf GB/s\n", msg_size, avg_speed, min_speed, max_speed);
    }
    free(send_buf1);
    free(send_buf2);
    free(recv_buf1);
    free(recv_buf2);
    free(send_counts);
    free(recv_counts);
    free(send_displs);
    free(recv_displs);
}

void sig_wait(unr_sig_h sig, int line) {
    for (uru_cpu_cyc_t start = uru_cpu_cyc_now(), now = start;
         unr_sig_check(sig) != URU_SIGNAL_TRIGGERED;
         now = uru_cpu_cyc_now()) {
        if (uru_time_diff(start, now) > 10) {
            unr_sig_t* sig_p = (unr_sig_t*)(&sig);
            printf("Timeout @ line[%d]: %ld msg not received\n", line, sig_p->flag_p->counter);
            exit(1);
        }
    }
}

void unr_alltoallv_test(size_t msg_size) {
    char* send_buf1 = (char*)calloc(size, msg_size);
    char* send_buf2 = (char*)calloc(size, msg_size);
    char* recv_buf1 = (char*)calloc(size, msg_size);
    char* recv_buf2 = (char*)calloc(size, msg_size);
    char* ans_buf = (char*)calloc(size, msg_size);
    unr_mem_h send_mem1, send_mem2, recv_mem1, recv_mem2;
    unr_mem_reg(send_buf1, size * msg_size, &send_mem1);
    unr_mem_reg(send_buf2, size * msg_size, &send_mem2);
    unr_mem_reg(recv_buf1, size * msg_size, &recv_mem1);
    unr_mem_reg(recv_buf2, size * msg_size, &recv_mem2);
    unr_mem_reg_sync();
    unr_blk_h send_blk1, send_blk2, recv_blk1, recv_blk2;
    unr_sig_h send_sig1, send_sig2, recv_sig1, recv_sig2;
    unr_sig_create(&send_sig1, size);
    unr_sig_create(&send_sig2, size);
    unr_sig_create(&recv_sig1, size);
    unr_sig_create(&recv_sig2, size);
    unr_blk_reg(send_mem1, 0, size * msg_size, send_sig1, UNR_NO_SIGNAL, &send_blk1);
    unr_blk_reg(send_mem2, 0, size * msg_size, send_sig2, UNR_NO_SIGNAL, &send_blk2);
    unr_blk_reg(recv_mem1, 0, size * msg_size, UNR_NO_SIGNAL, recv_sig1, &recv_blk1);
    unr_blk_reg(recv_mem2, 0, size * msg_size, UNR_NO_SIGNAL, recv_sig2, &recv_blk2);
    size_t* send_counts = (size_t*)malloc(size * sizeof(size_t));
    size_t* recv_counts = (size_t*)malloc(size * sizeof(size_t));
    size_t* send_displs = (size_t*)malloc(size * sizeof(size_t));
    size_t* recv_displs = (size_t*)malloc(size * sizeof(size_t));
    unr_blk_h* send_blk1_arr = (unr_blk_h*)malloc(size * sizeof(unr_blk_h));
    unr_blk_h* send_blk2_arr = (unr_blk_h*)malloc(size * sizeof(unr_blk_h));
    unr_blk_h* recv_blk1_arr = (unr_blk_h*)malloc(size * sizeof(unr_blk_h));
    unr_blk_h* recv_blk2_arr = (unr_blk_h*)malloc(size * sizeof(unr_blk_h));
    for (int i = 0; i < size; i++) {
        send_counts[i] = msg_size;
        recv_counts[i] = msg_size;
        send_displs[i] = i * msg_size;
        recv_displs[i] = rank * msg_size; /* Notice that, it is different! */
        send_blk1_arr[i] = send_blk1;
        send_blk2_arr[i] = send_blk2;
        srand(rank * size + i);
        for (int j = i * msg_size; j < (i + 1) * msg_size; j++) {
            int tmp = rand() % 111;
            send_buf1[j] = tmp;
            send_buf2[j] = tmp;
        }
        srand(i * size + rank);
        for (int j = i * msg_size; j < (i + 1) * msg_size; j++) {
            ans_buf[j] = rand() % 111;
        }
    }
    MPI_Allgather(&recv_blk1, 1, MPI_UNR_BLK_H, recv_blk1_arr, 1, MPI_UNR_BLK_H, MPI_COMM_WORLD);
    MPI_Allgather(&recv_blk2, 1, MPI_UNR_BLK_H, recv_blk2_arr, 1, MPI_UNR_BLK_H, MPI_COMM_WORLD);
    unr_plan_h plan1, plan2;
    unr_blk_part_rdma_batch_plan(size,
                                 send_blk1_arr, NULL, send_displs, send_counts,
                                 recv_blk1_arr, NULL, recv_displs, URU_TRANSFER_TYPE_DMA_PUT_ALL, &plan1);
    unr_blk_part_rdma_batch_plan(size,
                                 send_blk2_arr, NULL, send_displs, send_counts,
                                 recv_blk2_arr, NULL, recv_displs, URU_TRANSFER_TYPE_DMA_PUT_ALL, &plan2);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    unr_plan_start(plan1);
    sig_wait(recv_sig1, __LINE__);
    for (int i = 0; i < size * msg_size; i++) {
        if (recv_buf1[i] != ans_buf[i]) {
            printf("unr_alltoallv1 wrong answer @ %ld->%d[]=%d!=%d\n", i / msg_size, rank, recv_buf1[i], ans_buf[i]);
            exit(1);
        }
    }
    sig_wait(send_sig1, __LINE__);
    unr_sig_reset(recv_sig1);
    unr_sig_reset(send_sig1);
    unr_plan_start(plan2);
    sig_wait(recv_sig2, __LINE__);
    for (int i = 0; i < size * msg_size; i++) {
        if (recv_buf2[i] != ans_buf[i]) {
            printf("unr_alltoallv2 wrong answer\n");
            exit(1);
        }
    }
    sig_wait(send_sig2, __LINE__);
    unr_sig_reset(recv_sig2);
    unr_sig_reset(send_sig2);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    uru_cpu_cyc_t start = uru_cpu_cyc_now();
    for (int i = 0; i < TEST_TIMES; i += 2) {
        unr_plan_start(plan1);
        sig_wait(recv_sig1, __LINE__);
        sig_wait(send_sig1, __LINE__);
        unr_sig_reset(recv_sig1);
        unr_sig_reset(send_sig1);
        unr_plan_start(plan2);
        sig_wait(recv_sig2, __LINE__);
        sig_wait(send_sig2, __LINE__);
        unr_sig_reset(recv_sig2);
        unr_sig_reset(send_sig2);
    }
    uru_cpu_cyc_t end = uru_cpu_cyc_now();
    double my_time = uru_time_diff(start, end);
    double avg_time, min_time, max_time;
    MPI_Reduce(&my_time, &avg_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&my_time, &min_time, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&my_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        double avg_speed = TEST_TIMES * size * size * msg_size / avg_time / 1e9;
        double min_speed = TEST_TIMES * size * msg_size / max_time / 1e9;
        double max_speed = TEST_TIMES * size * msg_size / min_time / 1e9;
        printf("UNR_Alltoallv: msg_size = %ld, avg_speed = %lf GB/s, min_speed = %lf GB/s, max_speed = %lf GB/s\n", msg_size, avg_speed, min_speed, max_speed);
    }
    free(send_buf1);
    free(send_buf2);
    free(recv_buf1);
    free(recv_buf2);
    free(send_counts);
    free(recv_counts);
    free(send_displs);
    free(recv_displs);
}

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
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    for (size_t i = 1; i <= MAX_SIZE; i *= 2) {
        mpi_alltoallv_test(i);
        unr_alltoallv_test(i);
    }

    unr_finalize();
    MPI_Finalize();

    return 0;
}