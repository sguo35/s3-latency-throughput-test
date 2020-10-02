cmake_minimum_required(VERSION 3.5)
project(async_client)

set(CMAKE_CXX_STANDARD 17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(BOOST_INCLUDE_DIR ~/boost_1_73_0)
find_package(Boost 1.73 COMPONENTS program_options REQUIRED)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

add_subdirectory(asio_http)

include_directories(asio_http)
link_directories(asio_http)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(async_client async_client.cpp)

# list all deps for static linking
target_link_libraries(async_client LINK_PUBLIC ${Boost_LIBRARIES} Threads::Threads ssl crypto asio_http)
target_compile_options(async_client PRIVATE)