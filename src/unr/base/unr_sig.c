#include <unr/base/unr_def.h>
#include <uru/sys/preprocessor.h>
#include <uru/sys/time.h>
#include <string.h>

unr_sig_h UNR_NO_SIGNAL;

URU_STATIC_INIT {
    unr_sig_t* unr_no_signal = (unr_sig_t*)&UNR_NO_SIGNAL;
    memset(unr_no_signal, 0, sizeof(unr_sig_t));
    unr_no_signal->dst_rank = UNR_NO_SIGNAL_DST_RANK;
}

uru_status_t unr_sig_create(unr_sig_h* sig_h, uint64_t num_event) {
    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    URU_ASSERT_REASON(num_event > 0, "The number of events should be greater than 0");
    URU_ASSERT_REASON(num_event < (1<<31), "The number of events should be greater than 2^31");
    unr_sig_t* sig_p = (unr_sig_t*)sig_h;
    uru_flag_t* flag_p = uru_flag_pool_alloc();
    sig_p->dst_rank = uru_mpi->rank;
    sig_p->flag_p = flag_p;
    flag_p->counter = num_event;
    flag_p->queue = NULL;
    flag_p->num_event = num_event;
    flag_p->idx = -1;

    return URU_STATUS_OK;
}

uru_status_t unr_sig_check(unr_sig_h sig_h) {
    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    unr_sig_t* sig_p = (unr_sig_t*)&sig_h;
    uru_flag_t* flag_p = sig_p->flag_p;
    volatile int64_t* counter = &(flag_p->counter);
    if (((*counter) & (1<<31)) != 0) {
        printf("Signal overflow detected @ rank=%d, counter=%lld! It means this signal receives more events than expected.\n", uru_mpi->rank, *counter);
        exit(URU_EVENT_OVERFLOW);
    }
    URU_ASSERT_REASON((*counter & (1<<31)) == 0, "Signal overflow detected! It means this signal receives more events than expected.");
    if (*counter == 0) {
        return URU_SIGNAL_TRIGGERED;
    } else {
        return URU_SIGNAL_WAITING;
    }
}

#undef unr_sig_wait
uru_status_t unr_sig_wait(unr_sig_h sig_h, const char *file, int line, const char *func) {
    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    for (uru_cpu_cyc_t start = uru_cpu_cyc_now(), now = start;
         unr_sig_check(sig_h) != URU_SIGNAL_TRIGGERED;
         now = uru_cpu_cyc_now()) {
        if (uru_time_diff(start, now) > 10) {
            unr_sig_t* sig_p = (unr_sig_t*)(&sig_h);
            printf("Timeout @ %s[%d]-%s(): %ld msg not received\n", file, line, func, sig_p->flag_p->counter);
            fflush(stdout);
            start = uru_cpu_cyc_now();
        }
    }
    return URU_STATUS_OK;
}

uru_status_t unr_sig_reset(unr_sig_h sig_h) {
    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    unr_sig_t* sig_p = (unr_sig_t*)&sig_h;
    uru_flag_t* flag_p = sig_p->flag_p;
    URU_ASSERT_REASON(flag_p->counter == 0, "The signal is not triggered yet");
    flag_p->counter = flag_p->num_event;
    return URU_STATUS_OK;
}

uru_status_t unr_sig_pool_create(unr_sig_pool_h* sig_pool_h, int32_t num_sigs, unr_sig_h* sig_h_arr) {
    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    unr_sig_pool_t* sig_pool_p = (unr_sig_pool_t*)uru_malloc(sizeof(unr_sig_pool_t));
    *sig_pool_h = (unr_sig_pool_h)sig_pool_p;

    uru_queue_init(&(sig_pool_p->queue), num_sigs);
    sig_pool_p->num_sigs = num_sigs;
    sig_pool_p->sig_arr = (unr_sig_t**)uru_malloc(sizeof(unr_sig_t*) * num_sigs);
    for (size_t i = 0; i < num_sigs; i++) {
        sig_pool_p->sig_arr[i] = (unr_sig_t*)(&(sig_h_arr[i]));
        URU_ASSERT_REASON(sig_pool_p->sig_arr[i]->dst_rank == uru_mpi->rank,
                          "You can only use the signal created in the local process in %s.", __func__);
        URU_ASSERT_REASON(sig_pool_p->sig_arr[i]->flag_p->idx == -1, "The signal has been used in another pool");
        sig_pool_p->sig_arr[i]->flag_p->idx = i;
        sig_pool_p->sig_arr[i]->flag_p->queue = &(sig_pool_p->queue);
    }
    return URU_STATUS_OK;
}

uru_status_t unr_sig_pool_detect(unr_sig_pool_h sig_pool_h, int32_t* sig_id) {
    URU_ASSERT_REASON(unr_channels, "unr_init should be called before calling %s", __func__);
    unr_sig_pool_t* sig_pool_p = (unr_sig_pool_t*)sig_pool_h;
    uru_queue_pop(&(sig_pool_p->queue), sig_id, 1);
    if ((*sig_id) < 0) {
        return URU_STATUS_OK;
    }
    unr_sig_t* sig_p = (unr_sig_t*)(sig_pool_p->sig_arr[*sig_id]);
    sig_p->flag_p->counter = sig_p->flag_p->num_event;
    return URU_STATUS_OK;
}