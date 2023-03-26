cmake_minimum_required(VERSION 3.5)

project(sntpc C)

add_library(sntpc STATIC ${CMAKE_CURRENT_LIST_DIR}/lib/sntpc.c)
target_include_directories(sntpc PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

add_executable(example ${CMAKE_CURRENT_LIST_DIR}/example.c)
target_link_libraries(example PUBLIC sntpc)
