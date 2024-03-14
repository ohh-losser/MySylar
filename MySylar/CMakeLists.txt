cmake_minimum_required(VERSION 3.15)
project(MySylar)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(HEAD_PATH ${PROJECT_SOURCE_DIR}/include)
set(TEST_EXE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(SRC_ROOT ${PROJECT_SOURCE_DIR}/src)

add_subdirectory(tests)
