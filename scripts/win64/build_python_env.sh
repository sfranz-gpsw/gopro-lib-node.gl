GREEN='\033[0;32m'
NC='\033[0m' # No Color

function cmd {
	cmd.exe /C $1
if [ $? -ne 0 ]; then exit; fi
}
VULKAN_SDK=/mnt/c/VulkanSDK/1.2.131.2

echo -e "${GREEN}Setting up python virtual environment${NC}"
cmd "python.exe -m venv nodegl-env"

install -C -d nodegl-env/share
install -C -d nodegl-env/share/nodegl
install -C -m 644 libnodegl/nodegl.h nodegl-env/Include/
install -C -m 644 libnodegl/nodes.specs nodegl-env/share/nodegl/
install -C -m 644 cmake-build-debug/libnodegl/RelWithDebInfo/nodegl.lib nodegl-env/Lib/nodegl.lib
install -C -m 644 cmake-build-debug/shader-tools/RelWithDebInfo/shader_tools.lib nodegl-env/Lib/shader_tools.lib
install -C -m 644 cmake-build-debug/ngfx/RelWithDebInfo/ngfx.lib nodegl-env/Lib/ngfx.lib
install -C -m 644 external/win64/pthreads-w32-2-9-1-release/Pre-built.2/lib/x64/pthreadVC2.lib nodegl-env/Lib/
install -C -m 644 cmake-build-debug/external/sxplayer-9.5.1/RelWithDebInfo/sxplayer.lib nodegl-env/Lib/
install -C -m 644 $VULKAN_SDK/Lib/*.lib nodegl-env/Lib/
install -C -m 644 external/win64/ffmpeg-20200831-4a11a6f-win64-dev/lib/*.lib nodegl-env/Lib/

echo -e "${GREEN}Installing pynodegl dependencies${NC}"
cmd "nodegl-env\\Scripts\\activate.bat && pip.exe install -r pynodegl\requirements.txt"
echo -e "${GREEN}Building pynodegl${NC}"
cmd "nodegl-env\\Scripts\\activate.bat && pip.exe -v install -e pynodegl"
echo -e "${GREEN}Installing pynodegl-utils dependencies${NC}"
cmd "nodegl-env\\Scripts\\activate.bat && pip.exe install -r pynodegl-utils\requirements.txt"
echo -e "${GREEN}Building pynodegl-utils${NC}"
cmd "nodegl-env\\Scripts\\activate.bat && pip.exe -v install -e pynodegl-utils"
