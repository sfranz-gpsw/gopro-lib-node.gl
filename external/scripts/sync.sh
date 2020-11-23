#TODO: use cmake ExternalProject targets
OS=$1
echo "OS: [$OS]"
TMP_DIR=/tmp/nodegl/external

if [ $OS == "win64" ]; then
    mkdir -p win64

    wget -nc ftp://localhost/pub/nodegl/external/win64/pkg-config.tgz -P $TMP_DIR/pkg-config
    cd win64 && tar xzf $TMP_DIR/pkg-config/pkg-config.tgz; cd -

    wget -nc ftp://localhost/pub/nodegl/external/win64/ffmpeg_x64-windows.tgz -P $TMP_DIR/ffmpeg
    cd win64 && tar xzf $TMP_DIR/ffmpeg/ffmpeg_x64-windows.tgz; cd -

    wget -nc ftp://localhost/pub/nodegl/external/win64/pthreads_x64-windows.tgz -P $TMP_DIR/pthreads
    cd win64 && tar xzf $TMP_DIR/pthreads/pthreads_x64-windows.tgz; cd -

    wget -nc https://www.khronos.org/registry/OpenGL/api/GL/glext.h -P $TMP_DIR/gl/include/GL
    wget -nc https://www.khronos.org/registry/OpenGL/api/GL/glcorearb.h -P $TMP_DIR/gl/include/GL
    wget -nc https://www.khronos.org/registry/OpenGL/api/GL/wglext.h -P $TMP_DIR/gl/include/GL
    wget -nc https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h -P $TMP_DIR/gl/include/KHR
    mkdir -p win64/gl/include && cp -rf $TMP_DIR/gl/include/* win64/gl/include/.

fi
