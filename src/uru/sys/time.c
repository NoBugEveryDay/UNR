#include <uru/sys/time.h>
#include <uru/sys/preprocessor.h>
#include <uru/dev/dev.h>
#include <time.h>
#include <stdint.h>

#if defined(__x86_64__)
static inline uint64_t get_cpu_cycle_count() {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}
#elif defined(__aarch64__)
static inline uint64_t get_cpu_cycle_count() {
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r" (val));
    return val;
}
#else
#error "Unsupported architecture"
#endif

uru_cpu_cyc_t uru_cpu_cyc_now() {
    return get_cpu_cycle_count();
}

double cpu_frequency = 0;

URU_STATIC_INIT {
    struct timespec start_time, end_time;
    URU_ASSERT(clock_gettime(CLOCK_MONOTONIC, &start_time) != -1);
    uru_cpu_cyc_t start_cyc = uru_cpu_cyc_now(), end_cyc;
    double time_diff;
    do {
        URU_ASSERT(clock_gettime(CLOCK_MONOTONIC, &end_time) != -1);
        end_cyc = uru_cpu_cyc_now();
        time_diff = (end_time.tv_sec - start_time.tv_sec) * 1000000000;
        time_diff += (end_time.tv_nsec - start_time.tv_nsec);
    } while (time_diff < 100000000); /* 0.1s */
    cpu_frequency = ((double)(end_cyc - start_cyc)) / time_diff * 1000000000;
}

double uru_time_diff(uru_cpu_cyc_t start, uru_cpu_cyc_t end) {
    return ((double)(end - start)) / cpu_frequency;
}