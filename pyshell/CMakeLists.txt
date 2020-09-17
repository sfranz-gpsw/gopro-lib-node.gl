cmake_minimum_required(VERSION 3.10)
project (pyshell)

file (GLOB_RECURSE PYSHELL_SRC_FILES *.cpp)

add_executable(pyshell ${PYSHELL_SRC_FILES})
target_include_directories(pyshell PUBLIC ${PYTHON_INCLUDE_DIR})
target_link_libraries(pyshell PUBLIC ${PYTHON_LIBS})
target_link_directories(pyshell PUBLIC ${PYTHON_LIB_DIR})