add_library(unr SHARED)

target_link_libraries(unr Threads::Threads)

add_subdirectory(uru)
add_subdirectory(uri)
add_subdirectory(unr)
add_subdirectory(tools)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/uri/base/uri_channel.c
    ${CMAKE_CURRENT_SOURCE_DIR}/uri/base/uri_mem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/uri/base/uri_rdma.c
    PROPERTIES COMPILE_DEFINITIONS "URI_API_AND_URI_DEF_EXCEPTION")

if (TARGET_SYSTEM STREQUAL "TH2A")
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/uru/sys/realloc.c
        PROPERTIES COMPILE_DEFINITIONS "CUSTOMIZE_REALLOC")
    message(STATUS "realloc.c use customize realloc implementation.")
else()
    message(STATUS "realloc.c use system realloc.")
endif()

install(TARGETS unr)