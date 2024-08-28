link_libraries(unr)

add_executable(alltoallv alltoallv.c)
install(TARGETS alltoallv)

add_executable(pingpong pingpong.c)
install(TARGETS pingpong)

add_executable(pingpong2 pingpong2.c)
install(TARGETS pingpong2)

add_executable(pingpong4 pingpong4.c)
target_link_libraries(pingpong4 m)
install(TARGETS pingpong4)