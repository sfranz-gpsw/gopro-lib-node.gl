cmake_minimum_required(VERSION 3.14)
project (debug-tools)

file (GLOB_RECURSE PYSHELL_SRC_FILES *.cpp *.h)

function(build_tool name)
file (GLOB_RECURSE TEST_SOURCE_FILES tools/${name}/*.cpp tools/${name}/*.h tools/${name}/*.mm)
add_executable(${name} ${TEST_SOURCE_FILES})
target_include_directories(${name} PUBLIC ${Python3_INCLUDE_DIRS})
target_link_libraries(${name} PUBLIC ${Python3_LIBRARIES})
target_link_directories(${name} PUBLIC ${Python3_LIBRARY_DIRS})
target_compile_options(${name} PUBLIC -DTESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../tests" )
endfunction()

build_tool(pyshell)
build_tool(run_test)
