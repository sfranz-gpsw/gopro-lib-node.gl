GREEN='\033[0;32m'
NC='\033[0m' # No Color

function cmd {
	cmd.exe /C $1
if [ $? -ne 0 ]; then exit; fi
}
VULKAN_SDK=/mnt/c/VulkanSDK/1.2.131.2

echo -e "${GREEN}Setting up python virtual environment${NC}"
git clean -fxd nodegl-env
cmd "python.exe -m venv nodegl-env"

install -C -d nodegl-env/share
install -C -d nodegl-env/share/nodegl
install -C -m 644 libnodegl/src/nodegl/core/nodegl.h nodegl-env/Include/
install -C -m 644 libnodegl/nodes.specs nodegl-env/share/nodegl/
install -C -m 644 cmake-build-debug/libnodegl/RelWithDebInfo/nodegl.lib nodegl-env/Lib/nodegl.lib
install -C -m 644 cmake-build-debug/shader-tools/RelWithDebInfo/shader_tools.lib nodegl-env/Lib/shader_tools.lib
install -C -m 644 cmake-build-debug/ngfx/RelWithDebInfo/ngfx.lib nodegl-env/Lib/ngfx.lib
install -C -m 644 external/win64/pthreads-w32-2-9-1-release/Pre-built.2/lib/x64/pthreadVC2.lib nodegl-env/Lib/
install -C -m 644 cmake-build-debug/external/sxplayer-9.5.1/RelWithDebInfo/sxplayer.lib nodegl-env/Lib/
install -C -m 644 $VULKAN_SDK/Lib/VkLayer_utils.lib nodegl-env/Lib/
install -C -m 644 $VULKAN_SDK/Lib/vulkan-1.lib nodegl-env/Lib/
install -C -m 644 external/win64/shaderc/cmake-build-debug/libshaderc/RelWithDebInfo/shaderc_combined.lib nodegl-env/Lib
SPIRV_CROSS_LIBS="spirv-cross-cpp spirv-cross-core spirv-cross-glsl spirv-cross-hlsl spirv-cross-msl spirv-cross-reflect spirv-cross-util spirv-cross-c"
for lib in $SPIRV_CROSS_LIBS
    do install -C -m 644 external/win64/spirv_cross/cmake-build-debug/RelWithDebInfo/$lib.lib nodegl-env/Lib/
done
install -C -m 644 external/win64/ffmpeg-4.3.1-2020-09-21-full_build-shared/lib/*.lib nodegl-env/Lib/

echo -e "${GREEN}Installing pynodegl dependencies${NC}"
cmd "nodegl-env\\Scripts\\activate.bat && pip.exe install -r pynodegl\requirements.txt"
echo -e "${GREEN}Building pynodegl${NC}"
cmd "nodegl-env\\Scripts\\activate.bat && pip.exe -v install -e pynodegl"
echo -e "${GREEN}Installing pynodegl-utils dependencies${NC}"
cmd "nodegl-env\\Scripts\\activate.bat && pip.exe install -r pynodegl-utils\requirements.txt"
echo -e "${GREEN}Building pynodegl-utils${NC}"
cmd "nodegl-env\\Scripts\\activate.bat && pip.exe -v install -e pynodegl-utils"
