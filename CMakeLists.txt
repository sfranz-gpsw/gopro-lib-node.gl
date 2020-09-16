cmake_minimum_required(VERSION 3.10)
project (gopro-nodegl)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set (SXPLAYER_LIBS sxplayer avformat avfilter avcodec avutil pthread)
set (SXPLAYER_VERSION 9.5.1)
set (EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
set (PTHREADS_INCLUDE_DIR ${EXTERNAL_DIR}/win64/pthreads-w32-2-9-1-release/Pre-built.2/include)
set (PTHREAD_CFLAGS -DHAVE_STRUCT_TIMESPEC)
set (FFMPEG_INCLUDE_DIR ${EXTERNAL_DIR}/win64/ffmpeg-20200831-4a11a6f-win64-dev/include)
endif()

add_subdirectory(external/sxplayer-${SXPLAYER_VERSION})
add_subdirectory(libnodegl)
