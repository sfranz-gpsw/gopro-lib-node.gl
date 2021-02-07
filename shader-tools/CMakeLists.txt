cmake_minimum_required(VERSION 3.14)
project (shader-tools)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set (EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
set (NGFX_DIR ${EXTERNAL_DIR}/windows/ngfx_x64-windows)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set (NGFX_DIR ${EXTERNAL_DIR}/linux/ngfx_x64-linux)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set (NGFX_DIR ${EXTERNAL_DIR}/darwin/ngfx_x64-darwin)
endif()
set (NGFX_INCLUDE_DIRS ${NGFX_DIR} ${NGFX_DIR}/include)
set (NGFX_LIBRARY_DIRS ${NGFX_DIR}/lib)

set (JSON_DIR ${EXTERNAL_DIR}/json)
set (JSON_INCLUDE_DIRS ${JSON_DIR}/include)

find_package (PkgConfig REQUIRED)
pkg_check_modules(SHADERC REQUIRED shaderc)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")

set (SPIRV_CROSS_DIR ${EXTERNAL_DIR}/windows/spirv_cross_x64-windows)
set (SPIRV_CROSS_INCLUDE_DIRS ${SPIRV_CROSS_DIR}/include)
set (SPIRV_CROSS_LIBRARY_DIRS ${SPIRV_CROSS_DIR}/lib)
set (SPIRV_CROSS_LIBRARIES spirv-cross-cpp spirv-cross-core spirv-cross-glsl spirv-cross-hlsl spirv-cross-msl spirv-cross-reflect spirv-cross-util spirv-cross-c)

else()

set (SPIRV_CROSS_DIR /usr/local)
set (SPIRV_CROSS_INCLUDE_DIRS ${SPIRV_CROSS_DIR}/include)
set (SPIRV_CROSS_LIBRARY_DIRS ${SPIRV_CROSS_DIR}/lib)
set (SPIRV_CROSS_LIBRARIES spirv-cross-cpp spirv-cross-core spirv-cross-glsl spirv-cross-hlsl spirv-cross-msl spirv-cross-reflect spirv-cross-util spirv-cross-c)


endif()

file (GLOB_RECURSE SHADER_TOOLS_SRC_FILES src/*.cpp src/*.h)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
set(SHADER_TOOLS_BUILD_TYPE STATIC)
else()
set(SHADER_TOOLS_BUILD_TYPE SHARED)
endif()
add_library(shader_tools ${SHADER_TOOLS_BUILD_TYPE} ${SHADER_TOOLS_SRC_FILES})
target_compile_options(shader_tools PUBLIC ${NGFX_CFLAGS})
target_include_directories(shader_tools PUBLIC src
    ${JSON_INCLUDE_DIRS}
    ${NGFX_INCLUDE_DIRS}
    ${SHADERC_INCLUDE_DIRS}
    ${SPIRV_CROSS_INCLUDE_DIRS}
)

target_link_libraries(shader_tools PUBLIC ngfx
    ${SHADERC_LIBRARIES}
    ${SPIRV_CROSS_LIBRARIES}
)
target_link_directories(shader_tools PUBLIC
    ${NGFX_LIBRARY_DIRS}
    ${SHADERC_LIBRARY_DIRS}
    ${SPIRV_CROSS_LIBRARY_DIRS}
)
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries(shader_tools PUBLIC stdc++fs)
endif()

function(build_tool name)
file (GLOB_RECURSE TOOL_SOURCE_FILES tools/${name}*.cpp tools/${name}*.h)
add_executable(${name} ${TOOL_SOURCE_FILES})
target_include_directories(${name} PUBLIC src)
target_link_libraries(${name} shader_tools)
endfunction()

if (NGFX_GRAPHICS_BACKEND_VULKAN)
#build_tool(compile_shaders_vk)
elseif(NGFX_GRAPHICS_BACKEND_DIRECT3D12)
build_tool(compile_shaders_dx12)
elseif(NGFX_GRAPHICS_BACKEND_METAL)
#build_tool(compile_shaders_mtl)
endif()

install(TARGETS shader_tools 
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
function (install_headers folder)
file (GLOB_RECURSE SHADER_TOOLS_HEADER_FILES src/${folder}/*.h)
install (FILES ${SHADER_TOOLS_HEADER_FILES} DESTINATION include/${folder})
endfunction()
install_headers(ngfx/core)
install_headers(ngfx/graphics)
install_headers(ngfx/regex)


