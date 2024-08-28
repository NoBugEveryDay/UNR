target_include_directories(unr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/unr.h
    DESTINATION include)