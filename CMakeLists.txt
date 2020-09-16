cmake_minimum_required(VERSION 3.10)
project (gopro-nodegl)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set (SXPLAYER_LIBS sxplayer avformat avfilter avcodec avutil pthread)
set (EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")

set (PTHREADS_DIR ${EXTERNAL_DIR}/win64/pthreads-w32-2-9-1-release/Pre-built.2)
set (PTHREADS_INCLUDE_DIR ${PTHREADS_DIR}/include)
set (PTHREADS_CFLAGS -DHAVE_STRUCT_TIMESPEC)
set (PTHREADS_LIB_DIR ${PTHREADS_DIR}/lib/x64)
set (PTHREADS_LIBS pthreadVC2.lib)

set (FFMPEG_DIR ${EXTERNAL_DIR}/win64/ffmpeg-20200831-4a11a6f-win64-dev)
set (FFMPEG_INCLUDE_DIR ${FFMPEG_DIR}/include)
set (FFMPEG_LIB_DIR ${FFMPEG_DIR}/lib)
set (FFMPEG_LIBS avcodec.lib avdevice.lib avformat.lib avfilter.lib avutil.lib)

set (GLEW_DIR ${EXTERNAL_DIR}/win64/glew-2.1.0-win32/glew-2.1.0)
set (GLEW_INCLUDE_DIR ${GLEW_DIR}/include)
set (GLEW_LIB_DIR ${GLEW_DIR}/lib/Release/x64)
set (GLEW_LIBS glew32.lib)

set (GLFW_DIR ${EXTERNAL_DIR}/win64/glfw-3.3.2.bin.WIN64)
set (GLFW_INCLUDE_DIR ${GLFW_DIR}/include)
set (GLFW_LIB_DIR ${GLFW_DIR}/lib-vc2019)
set (GLFW_LIBS glfw3.lib OpenGL32.lib)

endif()

add_subdirectory(external/sxplayer-9.5.1)
add_subdirectory(libnodegl)
