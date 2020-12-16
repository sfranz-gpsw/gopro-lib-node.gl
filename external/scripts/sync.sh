#TODO: use cmake ExternalProject targets
set -ex
OS=$1
echo "OS: [$OS]"
TMP_DIR=/tmp/nodegl/external

wget -nc -q https://github.com/Stupeflix/sxplayer/archive/v9.5.1.tar.gz  -P $TMP_DIR/sxplayer
tar xzf $TMP_DIR/sxplayer/v9.5.1.tar.gz
bash scripts/patch_sxplayer.sh

wget -nc -q https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp -P $TMP_DIR/json
mkdir -p json/include && cp -rf $TMP_DIR/json/* json/include/.


if [ $OS == "win64" ]; then
	mkdir win64
	
	wget -nc -q https://github.com/glfw/glfw/releases/download/3.3.2/glfw-3.3.2.bin.WIN64.zip -P $TMP_DIR
	cd win64 && unzip -q $TMP_DIR/glfw-3.3.2.bin.WIN64.zip; cd -
	
	wget -nc -q https://sourceforge.net/projects/pthreads4w/files/pthreads-w32-2-9-1-release.zip -P $TMP_DIR
	cd win64 && unzip -q -d pthreads-w32-2-9-1-release $TMP_DIR/pthreads-w32-2-9-1-release.zip; cd -
	
	
	wget -nc -q https://www.khronos.org/registry/OpenGL/api/GL/glext.h -P $TMP_DIR/gl/include/GL
	wget -nc -q https://www.khronos.org/registry/OpenGL/api/GL/glcorearb.h -P $TMP_DIR/gl/include/GL
	wget -nc -q https://www.khronos.org/registry/OpenGL/api/GL/wglext.h -P $TMP_DIR/gl/include/GL
	wget -nc -q https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h -P $TMP_DIR/gl/include/KHR
	mkdir -p win64/gl/include && cp -rf $TMP_DIR/gl/include/* win64/gl/include/.
	
	wget -nc -q https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0-win32.zip -P $TMP_DIR
	cd win64 && unzip -q -d glew-2.1.0-win32 $TMP_DIR/glew-2.1.0-win32.zip; cd -
	
	wget -nc -q https://www.gyan.dev/ffmpeg/builds/packages/ffmpeg-4.3.1-2020-11-08-full_build-shared.7z -P $TMP_DIR/ffmpeg
	cd win64 && 7z x $TMP_DIR/ffmpeg/ffmpeg-4.3.1-2020-11-08-full_build-shared.7z; cd -
	
	wget -nc -q https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip -P $TMP_DIR/glm
	cd win64 && unzip -q -d glm-0.9.9.8 $TMP_DIR/glm/glm-0.9.9.8.zip; cd -
	
	wget -nc -q https://raw.githubusercontent.com/nothings/stb/master/stb_image.h -P $TMP_DIR/stb
	mkdir -p stb && cp $TMP_DIR/stb/stb_image.h stb/.
	
	wget -nc -q https://raw.githubusercontent.com/microsoft/DirectX-Graphics-Samples/v10.0.17763.0/Libraries/D3DX12/d3dx12.h -P $TMP_DIR/d3dx12
	mkdir -p win64/d3dx12 && cp $TMP_DIR/d3dx12/d3dx12.h win64/d3dx12/.
    
    #TODO: build shaderc
    (cd win64 && ln -s ../../../external/shaderc/ shaderc)
    
    #TODO: build SPIRV-cross
    (cd win64 && ln -s ../../../external/SPIRV-Cross/ spirv_cross)
fi
