if (TARGET_SYSTEM STREQUAL "TH2A")
    set(LAUNCH_1_CMD
        /BIGDATA1/app/MPI/mpich/3.2.1-icc-18.0.0-dynamic/bin/mpirun
        -n 1
        -launcher ssh -launcher-exec /usr/bin/nss_yhpc_ssh
        -hosts cn7859)
    set(LAUNCH_2_CMD
        /BIGDATA1/app/MPI/mpich/3.2.1-icc-18.0.0-dynamic/bin/mpirun
        -n 2 -ppn 1
        -launcher ssh -launcher-exec /usr/bin/nss_yhpc_ssh
        -hosts cn7859,cn7860)
    message(STATUS "Test on TH2A with LAUNCH_1_CMD =" ${LAUNCH_1_CMD})
    message(STATUS "Test on TH2A with LAUNCH_2_CMD =" ${LAUNCH_2_CMD})
elseif (TARGET_SYSTEM STREQUAL "TH2K")
    set(LAUNCH_1_CMD
        /app/compiler/intel/18.0.1/compilers_and_libraries_2018.1.163/linux/mpi/intel64/bin/mpirun
        -n 1
        -hosts cpn111)
    set(LAUNCH_2_CMD
        /app/compiler/intel/18.0.1/compilers_and_libraries_2018.1.163/linux/mpi/intel64/bin/mpirun
        -n 2 -ppn 1
        -hosts cpn111,cpn253)
    message(STATUS "Test on TH2K with LAUNCH_1_CMD =" ${LAUNCH_1_CMD})
    message(STATUS "Test on TH2K with LAUNCH_2_CMD =" ${LAUNCH_2_CMD})
elseif (TARGET_SYSTEM STREQUAL "THXY")
    set(JOBID 2047782)
    set(LAUNCH_1_CMD
        srun -N 1 -n 1
        --jobid=${JOBID})
    set(LAUNCH_2_CMD
        srun -N 2 -n 2
        --jobid=${JOBID})
    message(STATUS "Test on THXY with LAUNCH_1_CMD =" ${LAUNCH_1_CMD})
    message(STATUS "Test on THXY with LAUNCH_2_CMD =" ${LAUNCH_2_CMD})
elseif (TARGET_SYSTEM STREQUAL "THAN")
    set(LAUNCH_1_CMD
        /GLOBALFS/sysu_hpcscc_2/xjb/software/nvhpc/Linux_x86_64/23.11/comm_libs/mpi/bin/mpirun
        -n 1)
    set(LAUNCH_2_CMD
        /GLOBALFS/sysu_hpcscc_2/xjb/software/nvhpc/Linux_x86_64/23.11/comm_libs/mpi/bin/mpirun
        -x PATH -x LD_LIBRARY_PATH -x UNR_INTER -x UNR_INTRA
        -n 2 -host an28:1,an29:1)
    message(STATUS "Test on THXY with LAUNCH_1_CMD =" ${LAUNCH_1_CMD})
    message(STATUS "Test on THXY with LAUNCH_2_CMD =" ${LAUNCH_2_CMD})
else()
    set(LAUNCH_2_CMD
        /usr/bin/mpirun -n 2)
    message(STATUS "LAUNCH_1_CMD is empty")
    message(STATUS "LAUNCH_2_CMD =" mpirun -n 2)
endif()

add_subdirectory(channel)
add_subdirectory(uri)
add_subdirectory(unr)
add_subdirectory(benchmark)