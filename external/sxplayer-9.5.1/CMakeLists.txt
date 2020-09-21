cmake_minimum_required(VERSION 3.14)
project (sxplayer)

file (GLOB_RECURSE SXPLAYER_SRC_FILES *.c *.h)
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
list (FILTER SXPLAYER_SRC_FILES EXCLUDE REGEX "decoder_vt.c")
endif()
list (FILTER SXPLAYER_SRC_FILES EXCLUDE REGEX "test-prog.c")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
set(SXPLAYER_BUILD_TYPE STATIC)
else()
set(SXPLAYER_BUILD_TYPE SHARED)
endif()

add_library(sxplayer ${SXPLAYER_BUILD_TYPE} ${SXPLAYER_SRC_FILES})
target_include_directories(sxplayer PUBLIC 
        ${PTHREADS_INCLUDE_DIRS}
        ${FFMPEG_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
)
set_target_properties(sxplayer PROPERTIES COMPILE_FLAGS
    "${PTHREADS_CFLAGS} ${FFMPEG_CFLAGS} ${GLEW_CFLAGS} ${GLFW_CFLAGS}"
)
target_link_libraries(sxplayer PUBLIC 
        ${PTHREADS_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        ${GLEW_LIBRARIES}
        ${GLFW_LIBRARIES}
)
target_link_directories(sxplayer PUBLIC 
        ${PTHREADS_LIBRARY_DIRS}
        ${FFMPEG_LIBRARY_DIRS}
        ${GLEW_LIBRARY_DIRS}
        ${GLFW_LIBRARY_DIRS}
)
