#TODO: use cmake ExternalProject targets
TMP_DIR=/tmp/nodegl/external
wget -nc https://github.com/Stupeflix/sxplayer/archive/v9.5.1.tar.gz  -P $TMP_DIR/sxplayer
wget -nc https://sourceforge.net/projects/pthreads4w/files/pthreads-w32-2-9-1-release.zip -P $TMP_DIR
wget -nc https://github.com/glfw/glfw/releases/download/3.3.2/glfw-3.3.2.bin.WIN64.zip -P $TMP_DIR
wget -nc https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0-win32.zip -P $TMP_DIR
wget -nc https://www.khronos.org/registry/OpenGL/api/GL/glext.h -P $TMP_DIR/gl/include/GL
wget -nc https://www.khronos.org/registry/OpenGL/api/GL/glcorearb.h -P $TMP_DIR/gl/include/GL
wget -nc https://www.khronos.org/registry/OpenGL/api/GL/wglext.h -P $TMP_DIR/gl/include/GL
wget -nc https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h -P $TMP_DIR/gl/include/KHR
wget -nc https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp -P $TMP_DIR/json
wget -nc https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip -P $TMP_DIR/glm
wget -nc https://raw.githubusercontent.com/nothings/stb/master/stb_image.h -P $TMP_DIR/stb
wget -nc https://raw.githubusercontent.com/microsoft/DirectX-Graphics-Samples/v10.0.17763.0/Libraries/D3DX12/d3dx12.h -P $TMP_DIR/d3dx12
tar xzf $TMP_DIR/sxplayer/v9.5.1.tar.gz
mkdir -p json/include && cp -rf $TMP_DIR/json/* json/include/.
bash scripts/apply_patches.sh
mkdir win64
cd win64 && unzip $TMP_DIR/glfw-3.3.2.bin.WIN64.zip; cd -
cd win64 && unzip -d pthreads-w32-2-9-1-release $TMP_DIR/pthreads-w32-2-9-1-release.zip; cd -
mkdir -p win64/gl/include && cp -rf $TMP_DIR/gl/include/* win64/gl/include/.
cd win64 && unzip -d glew-2.1.0-win32 $TMP_DIR/glew-2.1.0-win32.zip; cd -
cd win64 && tar xzf ../archive/ffmpeg-20200831-4a11a6f-win64-dev.tgz; cd -
cd win64 && unzip -d glm-0.9.9.8 $TMP_DIR/glm/glm-0.9.9.8.zip
mkdir -p stb && cp $TMP_DIR/stb/stb_image.h stb/.
mkdir -p win64/d3dx12 && cp $TMP_DIR/d3dx12/d3dx12.h win64/d3dx12/.
