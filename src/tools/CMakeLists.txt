add_executable(uri_info uri_info.c)
target_link_libraries(uri_info unr)
install(TARGETS uri_info)

install(PROGRAMS glex_bind DESTINATION bin)