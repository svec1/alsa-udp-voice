cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

set(AUV alsa_udp_voice)

project(ALSA_TCP_VOICE)
set(CMAKE_CXX_STANDARD 23)
file(GLOB_RECURSE SRC_FILES src/*.cpp)
add_executable(${AUV} ${SRC_FILES})

target_link_libraries(${AUV} PRIVATE asound)
target_include_directories(${AUV} PRIVATE src/)
