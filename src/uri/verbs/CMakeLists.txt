set(VERBS_PATH "/usr" CACHE PATH "Path to VERBS library")
find_library(VERBS_LIB_PATH ibverbs ${VERBS_PATH}/lib64/)
find_path(VERBS_HEADER_PATH infiniband/verbs.h ${VERBS_PATH}/include/)


if (VERBS_LIB_PATH AND VERBS_HEADER_PATH)
    option(USE_VERBS "Option for use Verbs RDMA interface." ON)
    if (USE_VERBS)
        message(STATUS "[IMPORTANT CONFIGURATION] Build UNR with VERBS RDMA interface: " ${USE_VERBS})
        message(STATUS "VERBS library path: " ${VERBS_LIB_PATH})
        message(STATUS "VERBS header path: " ${VERBS_HEADER_PATH}/infiniband/verbs.h)
        
        target_sources(unr PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}/uri_verbs_channel.c
            ${CMAKE_CURRENT_SOURCE_DIR}/uri_verbs_mem.c
            ${CMAKE_CURRENT_SOURCE_DIR}/uri_verbs_rdma.c
            ${CMAKE_CURRENT_SOURCE_DIR}/uri_verbs_utils.c)
        target_include_directories(unr PRIVATE ${VERBS_HEADER_PATH})
        target_link_libraries(unr ${VERBS_LIB_PATH})
        target_compile_options(unr INTERFACE "-DCAR_CHANNEL_VERBS")
        set(URI_CHANNEL_FIND_DEFS "${URI_CHANNEL_FIND_DEFS} -DURI_CHANNEL_VERBS" CACHE INTERNAL "")

    endif()
else()
    option(USE_VERBS "Option for use Verbs RDMA interface." OFF)
    if (USE_VERBS)
        if(NOT VERBS_LIB_PATH)
            message(FATAL_ERROR "VERBS library not found")
        endif()
        if(NOT VERBS_HEADER_PATH)
            message(FATAL_ERROR "VERBS header not found")
        endif()
    endif()
endif()