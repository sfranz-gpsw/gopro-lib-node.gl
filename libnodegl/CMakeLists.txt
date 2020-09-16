cmake_minimum_required(VERSION 3.10)
project (nodegl)

file (GLOB_RECURSE NODEGL_SRC_FILES *.c *.h)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Android")
list (FILTER NODEGL_SRC_FILES EXCLUDE REGEX "android_.*$")
endif()
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
list (FILTER NODEGL_SRC_FILES EXCLUDE REGEX "_wgl.*.c$")
endif()
list (FILTER NODEGL_SRC_FILES EXCLUDE REGEX "_ios.*.c$")
list (FILTER NODEGL_SRC_FILES EXCLUDE REGEX "jni_")
list (FILTER NODEGL_SRC_FILES EXCLUDE REGEX "vaapi.*$")
list (FILTER NODEGL_SRC_FILES EXCLUDE REGEX "_darwin.*.c$")
list (FILTER NODEGL_SRC_FILES EXCLUDE REGEX "_mediacodec.*.c$")
if ( NOT (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "Android"))
list (FILTER NODEGL_SRC_FILES EXCLUDE REGEX "_egl.*.c$")
endif()
list (FILTER NODEGL_SRC_FILES EXCLUDE REGEX "gen_specs.c$")
list (FILTER NODEGL_SRC_FILES EXCLUDE REGEX "test_.*.c$")

set (NODEGL_CFLAGS -DBACKEND_GL -DBACKEND_VK -fPIC -g -Wall)
if ( ${CMAKE_SYSTEM_NAME} MATCHES "Linux") 
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
set (NODEGL_CFLAGS ${NODEGL_CFLAGS} -DHAVE_GLPLATFORM_WGL -DTARGET_WINDOWS)
endif()
set(NODEGL_BUILD_TYPE SHARED)
set (CMAKE_C_STANDARD 11)
add_library(nodegl ${NODEGL_BUILD_TYPE} ${NODEGL_SRC_FILES})
target_compile_options(nodegl PUBLIC ${NODEGL_CFLAGS})
target_link_libraries(nodegl PUBLIC 
	${SXPLAYER_LIBS}
	${VULKANSDK_LIBS}
	${SHADERC_LIBS}
	${GL_LIBS}
)
target_link_directories(nodegl PUBLIC 
	${VULKANSDK_LIB_DIR}
)
target_include_directories(nodegl PUBLIC 
	${SXPLAYER_INCLUDE_DIR}
	${VULKANSDK_INCLUDE_DIR}
	${GL_INCLUDE_DIR}
)
