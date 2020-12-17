#!/bin/bash
set -xu
OS=$1
echo "OS: [$OS]"
TMP_DIR=/tmp/gopro-pkg/external

if [ $OS == "win64" ]; then
    mkdir -p win64

    mkdir -p $TMP_DIR/pkg-config && wget -nc https://github.com/jmoguillansky-gpsw/gopro-pkg/blob/master/external/win64/pkg-config.7z?raw=true -O $TMP_DIR/pkg-config/pkg-config.7z
    (cd win64 && 7z x $TMP_DIR/pkg-config/pkg-config.7z)

    mkdir -p $TMP_DIR/ffmpeg && wget -nc https://github.com/jmoguillansky-gpsw/gopro-pkg/blob/master/external/win64/ffmpeg_x64-windows.7z?raw=true -O $TMP_DIR/ffmpeg/ffmpeg_x64-windows.7z
    (cd win64 && 7z x $TMP_DIR/ffmpeg/ffmpeg_x64-windows.7z)

    mkdir -p $TMP_DIR/pthreads && wget -nc https://github.com/jmoguillansky-gpsw/gopro-pkg/blob/master/external/win64/pthreads_x64-windows.7z?raw=true -O $TMP_DIR/pthreads/pthreads_x64-windows.7z
    (cd win64 && 7z x $TMP_DIR/pthreads/pthreads_x64-windows.7z)

    wget -nc https://www.khronos.org/registry/OpenGL/api/GL/glext.h -P $TMP_DIR/gl/include/GL
    wget -nc https://www.khronos.org/registry/OpenGL/api/GL/glcorearb.h -P $TMP_DIR/gl/include/GL
    wget -nc https://www.khronos.org/registry/OpenGL/api/GL/wglext.h -P $TMP_DIR/gl/include/GL
    wget -nc https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h -P $TMP_DIR/gl/include/KHR
    mkdir -p win64/gl/include && cp -rf $TMP_DIR/gl/include/* win64/gl/include/.

    mkdir -p $TMP_DIR/sdl2 && wget -nc https://github.com/jmoguillansky-gpsw/gopro-pkg/blob/master/external/win64/sdl2_x64-windows.7z?raw=true -O $TMP_DIR/sdl2/sdl2_x64-windows.7z
    (cd win64 && 7z x $TMP_DIR/sdl2/sdl2_x64-windows.7z)
    
    mkdir -p $TMP_DIR/shaderc && wget -nc https://github.com/jmoguillansky-gpsw/gopro-pkg/blob/master/external/win64/shaderc_x64-windows.7z?raw=true -O $TMP_DIR/shaderc/shaderc_x64-windows.7z
    (cd win64 && 7z x $TMP_DIR/shaderc/shaderc_x64-windows.7z)
    
    wget -nc -q https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip -P $TMP_DIR/glm
    (cd win64 && unzip -q -d glm-0.9.9.8 $TMP_DIR/glm/glm-0.9.9.8.zip)
    
    wget -nc -q https://raw.githubusercontent.com/nothings/stb/master/stb_image.h -P $TMP_DIR/stb
	mkdir -p stb && cp $TMP_DIR/stb/stb_image.h stb/.
fi
