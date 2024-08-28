target_sources(unr PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/status.c
    ${CMAKE_CURRENT_SOURCE_DIR}/queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/flag.c)

target_include_directories(unr PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR})

install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/uru_status.h
    DESTINATION include)