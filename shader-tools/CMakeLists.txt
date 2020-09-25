cmake_minimum_required(VERSION 3.14)
project (shader-tools)
set(CMAKE_CXX_STANDARD 17)

file (GLOB_RECURSE SHADER_TOOLS_SRC_FILES src/*.cpp src/*.h)

add_library(shader_tools SHARED ${SHADER_TOOLS_SRC_FILES})
target_include_directories(shader_tools PUBLIC . ${JSON_INCLUDE_DIRS})
target_link_libraries(shader_tools PUBLIC stdc++fs)
