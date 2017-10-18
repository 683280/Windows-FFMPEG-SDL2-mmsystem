cmake_minimum_required(VERSION 3.8)
project(ffmpeg)

set(CMAKE_CXX_STANDARD 11)
include_directories(include)
link_directories(lib)
set(SOURCE_FILES main.cpp CPcmSpeaker.cpp CPcmSpeaker.h PCM.cpp PCM.h)
add_executable(ffmpeg ${SOURCE_FILES})
target_link_libraries(ffmpeg avformat avutil avcodec swscale SDL2 winmm swresample)
