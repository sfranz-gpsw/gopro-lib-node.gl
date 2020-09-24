cmake_minimum_required(VERSION 3.14)
project (shader-tools)
set(CMAKE_CXX_STANDARD 17)

file (GLOB_RECURSE SHADER_TOOLS_SRC_FILES src/*.cpp src/*.h)

add_executable(shader_tools ${SHADER_TOOLS_SRC_FILES})
target_include_directories(shader_tools PUBLIC . ${JSON_INCLUDE_DIRS})
