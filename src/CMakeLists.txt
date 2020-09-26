cmake_minimum_required(VERSION 3.5)
project(s3-test)
find_package(AWSSDK REQUIRED COMPONENTS s3)
set(CMAKE_CXX_STANDARD 11)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(s3-test "s3-test.cpp")
# list all deps for static linking
target_link_libraries(s3-test ${AWSSDK_LINK_LIBRARIES} Threads::Threads)
target_compile_options(s3-test PRIVATE "-Wall" "-Werror")