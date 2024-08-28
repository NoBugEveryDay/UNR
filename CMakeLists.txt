cmake_minimum_required(VERSION 3.16)
project(UNR LANGUAGES C)

include(CTest)
list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")

set(CMAKE_POSITION_INDEPENDENT_CODE YES)

find_package(MPI REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Threads REQUIRED)
find_package(Doxygen)
if (DOXYGEN_FOUND)
    add_subdirectory(doc)
else()
    message(WARNING "Doxygen is needed to build the documentation.")
endif()

set(TARGET_SYSTEM $ENV{TARGET_SYSTEM})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src)
add_subdirectory(tests)