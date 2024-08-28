#pragma once
#include <mpi.h>
#define URU_TRACE_SWITCH
#define URU_MPI_CHECK() /*int rank; MPI_Comm_rank(MPI_COMM_WORLD,&rank); if (rank==0)*/

#ifdef URU_TRACE_SWITCH
    extern int uru_trace_stack_depth;
    #define URU_TAB() {URU_MPI_CHECK(){ int i; for (i = 0; i < uru_trace_stack_depth; i++) printf("    "); fflush(stdout);}}
    #define URU_TRACE() {URU_MPI_CHECK(){ URU_TAB(); printf("\033[31m>>>URU_TRACE<<<\033[0m %s[%d] : %s()\n", __FILE__, __LINE__, __func__); fflush(stdout);}}
    #define URU_PRINT_INT(VAR_NAME, VAR) {URU_MPI_CHECK(){ URU_TAB(); printf("\033[31m>>>URU_PRINT_INT<<<\033[0m %s[%d] : %s() - %s=%d\n", __FILE__, __LINE__, __func__, VAR_NAME, (int)(VAR)); fflush(stdout);}}
    #define URU_PRINT_INT64(VAR_NAME, VAR) {URU_MPI_CHECK(){ URU_TAB(); printf("\033[31m>>>URU_PRINT_INT<<<\033[0m %s[%d] : %s() - %s=%lld\n", __FILE__, __LINE__, __func__, VAR_NAME, (long long int)(VAR)); fflush(stdout);}}
    #define URU_PRINT_DOUBLE(VAR_NAME, VAR) {URU_MPI_CHECK(){ URU_TAB(); printf("\033[31m>>>URU_PRINT_INT<<<\033[0m %s[%d] : %s() - %s=%lf\n", __FILE__, __LINE__, __func__, VAR_NAME, (double)(VAR)); fflush(stdout);}}
    #define URU_PRINT_STR(VAR_NAME, VAR) {URU_MPI_CHECK(){ URU_TAB(); printf("\033[31m>>>URU_PRINT_STR<<<\033[0m %s[%d] : %s() - %s=\"%s\"\n", __FILE__, __LINE__, __func__, VAR_NAME, VAR); fflush(stdout);}}
    #define URU_PRINT_LINE() {URU_MPI_CHECK(){ URU_TAB(); printf("\033[31m---------------------------------------------------------------------\033[0m\n"); fflush(stdout);}}
    #define URU_STEP_IN_FUNC() {URU_MPI_CHECK(){ uru_trace_stack_depth++; URU_TAB(); printf("\033[31m>>>URU_STEP_IN_FUNC<<<\033[0m %s[%d] : %s()\n", __FILE__, __LINE__, __func__); fflush(stdout);}}
    #define URU_STEP_OUT_FUNC() {URU_MPI_CHECK(){ URU_TAB(); uru_trace_stack_depth--; printf("\033[31m>>>URU_STEP_OUT_FUNC<<<\033[0m %s[%d] : %s()\n", __FILE__, __LINE__, __func__); fflush(stdout);}}
    #define URU_EXIT() {printf("\033[31m>>>URU_EXIT<<<\033[0m %s[%d] : %s()\n", __FILE__, __LINE__, __func__); fflush(stdout); exit(0);}
#else
    #define URU_TAB()
    #define URU_TRACE()
    #define URU_PRINT_INT(VAR_NAME, VAR)
    #define URU_PRINT_INT64(VAR_NAME, VAR)
    #define URU_PRINT_DOUBLE(VAR_NAME, VAR)
    #define URU_PRINT_STR(VAR_NAME, VAR)
    #define URU_PRINT_LINE()
    #define URU_STEP_IN_FUNC()
    #define URU_STEP_OUT_FUNC()
    #define URU_EXIT()
#endif
