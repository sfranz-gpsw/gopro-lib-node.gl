#!/bin/bash
set -xu
OS=$1
echo "OS: [$OS]"
TMP_DIR=/tmp/gopro-pkg/external

if [ $OS == "win64" ]; then
    mkdir -p win64

    mkdir -p $TMP_DIR/pkg-config && wget -nc -q https://github.com/jmoguillansky-gpsw/gopro-pkg/blob/master/external/win64/pkg-config.tgz?raw=true -O $TMP_DIR/pkg-config/pkg-config.tgz
    (cd win64 && tar xzf $TMP_DIR/pkg-config/pkg-config.tgz)

    mkdir -p $TMP_DIR/ffmpeg && wget -nc -q https://github.com/jmoguillansky-gpsw/gopro-pkg/blob/master/external/win64/ffmpeg_x64-windows.zip?raw=true -O $TMP_DIR/ffmpeg/ffmpeg_x64-windows.zip
    (cd win64 && unzip -q $TMP_DIR/ffmpeg/ffmpeg_x64-windows.zip -d ffmpeg_x64-windows)

    mkdir -p $TMP_DIR/pthreads && wget -nc -q https://github.com/jmoguillansky-gpsw/gopro-pkg/blob/master/external/win64/pthreads_x64-windows.zip?raw=true -O $TMP_DIR/pthreads/pthreads_x64-windows.zip
    (cd win64 && unzip -q $TMP_DIR/pthreads/pthreads_x64-windows.zip -d pthreads_x64-windows)

    wget -nc -q https://www.khronos.org/registry/OpenGL/api/GL/glext.h -P $TMP_DIR/gl/include/GL
    wget -nc -q https://www.khronos.org/registry/OpenGL/api/GL/glcorearb.h -P $TMP_DIR/gl/include/GL
    wget -nc -q https://www.khronos.org/registry/OpenGL/api/GL/wglext.h -P $TMP_DIR/gl/include/GL
    wget -nc -q https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h -P $TMP_DIR/gl/include/KHR
    mkdir -p win64/gl/include && cp -rf $TMP_DIR/gl/include/* win64/gl/include/.

    mkdir -p $TMP_DIR/sdl2 && wget -nc -q https://github.com/jmoguillansky-gpsw/gopro-pkg/blob/master/external/win64/sdl2_x64-windows.zip?raw=true -O $TMP_DIR/sdl2/sdl2_x64-windows.zip
    (cd win64 && unzip -q $TMP_DIR/sdl2/sdl2_x64-windows.zip -d sdl2_x64-windows)

fi
