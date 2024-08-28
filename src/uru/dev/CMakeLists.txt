option(ASSERT_CHECK "Assert check in UNR. Disable it may improve the performance" OFF)
if(ASSERT_CHECK)
    target_compile_definitions(unr PUBLIC URU_ENABLE_ASSERT)
endif(ASSERT_CHECK)
message(STATUS "ASSERT_CHECK: ${ASSERT_CHECK}")

option(PRINT_LOG "Enable logging in UNR" OFF)
if(PRINT_LOG)
    target_compile_definitions(unr PUBLIC URU_ENABLE_LOG)
endif(PRINT_LOG)
message(STATUS "PRINT_LOG: ${PRINT_LOG}")

target_sources(unr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/trace.c)