#include <stdint.h>
#include <time.h>

typedef uint64_t uru_cpu_cyc_t;

uru_cpu_cyc_t uru_cpu_cyc_now();

/**
 * @brief Get time difference in seconds
 */
double uru_time_diff(uru_cpu_cyc_t start, uru_cpu_cyc_t end);