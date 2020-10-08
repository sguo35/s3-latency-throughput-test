cmake_minimum_required(VERSION 3.5)
project(async_client)

set(CMAKE_CXX_STANDARD 17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Boost 1.73 COMPONENTS program_options REQUIRED)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )


if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -lstdc++")

add_executable(async_client async_client.cpp aws_signer.cpp aws_signer.hpp)
add_executable(async_client_insecure async_client_insecure.cpp aws_signer.cpp aws_signer.hpp)
#add_executable(aws_signer aws_signer.cpp)

# list all deps for static linking
target_link_libraries(async_client LINK_PUBLIC ${Boost_LIBRARIES} Threads::Threads ssl crypto)
target_compile_options(async_client PRIVATE)

#target_link_libraries(aws_signer LINK_PUBLIC ${Boost_LIBRARIES} Threads::Threads ssl crypto)
#target_compile_options(aws_signer PRIVATE)

target_link_libraries(async_client_insecure LINK_PUBLIC ${Boost_LIBRARIES} Threads::Threads ssl crypto)
target_compile_options(async_client_insecure PRIVATE)