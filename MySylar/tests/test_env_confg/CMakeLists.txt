cmake_minimum_required(VERSION 3.15)
project(TESTENVANDCONFIG)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated -Wno-deprecated-declarations")

aux_source_directory(./ SRC1)
file(GLOB_RECURSE SRC2 ${SRC_ROOT}/*.cpp)
include_directories(${HEAD_PATH})

find_package(Threads REQUIRED)
find_package(yaml-cpp REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})
add_executable(test_env_config.exe ${SRC1} ${SRC2})
target_link_libraries(test_env_config.exe yaml-cpp)
target_link_libraries(test_env_config.exe Threads::Threads)
