cmake_minimum_required(VERSION 3.14)
project (shader-tools)
set(CMAKE_CXX_STANDARD 17)

file (GLOB_RECURSE SHADER_TOOLS_SRC_FILES src/*.cpp src/*.h)

add_library(shader_tools SHARED ${SHADER_TOOLS_SRC_FILES})
target_include_directories(shader_tools PUBLIC src ${JSON_INCLUDE_DIRS} ${NGFX_INCLUDE_DIRS})
target_link_libraries(shader_tools PUBLIC stdc++fs)

function(build_tool name)
file (GLOB_RECURSE TOOL_SOURCE_FILES tools/${name}*.cpp tools/${name}*.h)
add_executable(${name} ${TOOL_SOURCE_FILES})
target_include_directories(${name} PUBLIC src)
target_link_libraries(${name} shader_tools)
endfunction()

if (GRAPHICS_BACKEND_VULKAN)
build_tool(compile_shaders_vk)
elseif(GRAPHICS_BACKEND_DIRECTX12)
build_tool(compile_shaders_dx12)
elseif(GRAPHICS_BACKEND_METAL)
build_tool(compile_shaders_mtl)
endif()
