link_libraries(unr)

set_source_files_properties(01_uri_channel_find.c PROPERTIES COMPILE_FLAGS "${URI_CHANNEL_FIND_DEFS}")
get_source_file_property(compile_defs ${CMAKE_SOURCE_DIR}/tests/uri/01_uri_channel_find.c COMPILE_FLAGS)
message(STATUS "Compile definitions for 01_uri_channel_find.c: ${compile_defs}")

add_executable(01_uri_channel_find 01_uri_channel_find.c)
install(TARGETS 01_uri_channel_find)
add_test(NAME uri_channel_find COMMAND ${LAUNCH_1_CMD} 01_uri_channel_find)

add_executable(02_uri_channel_open_and_close 02_uri_channel_open_and_close.c)
install(TARGETS 02_uri_channel_open_and_close)
add_test(NAME uri_channel_open_and_close COMMAND ${LAUNCH_1_CMD} 02_uri_channel_open_and_close)

add_executable(03_uri_mem_reg 03_uri_mem_reg.c)
install(TARGETS 03_uri_mem_reg)
add_test(NAME uri_mem_reg COMMAND ${LAUNCH_1_CMD} 03_uri_mem_reg)

add_executable(04_uri_rdma_plan_create_0 04_uri_rdma_plan_create_0.c)
install(TARGETS 04_uri_rdma_plan_create_0)
add_test(NAME uri_rdma_plan_create_0 COMMAND ${LAUNCH_2_CMD} 04_uri_rdma_plan_create_0)

add_executable(05_uri_rdma_plan_create_1 05_uri_rdma_plan_create_1.c)
install(TARGETS 05_uri_rdma_plan_create_1)
add_test(NAME uri_rdma_plan_create_1 COMMAND ${LAUNCH_2_CMD}  05_uri_rdma_plan_create_1)

add_executable(06_uri_rdma_plan_start 06_uri_rdma_plan_start.c)
install(TARGETS 06_uri_rdma_plan_start)
add_test(NAME uri_rdma_plan_start COMMAND ${LAUNCH_2_CMD}  06_uri_rdma_plan_start)

add_executable(07_uri_rdma_plan_destroy 07_uri_rdma_plan_destroy.c)
install(TARGETS 07_uri_rdma_plan_destroy)
add_test(NAME uri_rdma_plan_destroy COMMAND ${LAUNCH_2_CMD}  07_uri_rdma_plan_destroy)
