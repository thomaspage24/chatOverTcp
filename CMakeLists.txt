cmake_minimum_required(VERSION 3.16)
project(secure_chat CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

add_compile_options(-Wall -Wextra -g)

find_package(OpenSSL REQUIRED)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(main ${SOURCES})

target_include_directories(main PRIVATE include)
target_link_libraries(main PRIVATE OpenSSL::SSL OpenSSL::Crypto)
