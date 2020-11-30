OS=$1
echo "OS: [$OS]"
TMP_DIR=/tmp/nodegl/external

if [ $OS == "win64" ]; then
    mkdir -p win64

    mkdir -p $TMP_DIR/pkg-config && wget -nc https://github.com/jmoguillansky-gpsw/gopro-nodegl-thirdparty-binaries/blob/master/external/win64/pkg-config.tgz?raw=true -O $TMP_DIR/pkg-config/pkg-config.tgz -P $TMP_DIR/pkg-config
    cd win64 && tar xzf $TMP_DIR/pkg-config/pkg-config.tgz; cd -

    mkdir -p $TMP_DIR/ffmpeg && wget -nc https://github.com/jmoguillansky-gpsw/gopro-nodegl-thirdparty-binaries/blob/master/external/win64/ffmpeg_x64-windows.tgz?raw=true -O $TMP_DIR/ffmpeg/ffmpeg_x64-windows.tgz -P $TMP_DIR/ffmpeg
    cd win64 && tar xzf $TMP_DIR/ffmpeg/ffmpeg_x64-windows.tgz; cd -

    mkdir $TMP_DIR/pthreads && wget -nc https://github.com/jmoguillansky-gpsw/gopro-nodegl-thirdparty-binaries/blob/master/external/win64/pthreads_x64-windows.tgz?raw=true -O $TMP_DIR/pthreads/pthreads_x64-windows.tgz -P $TMP_DIR/pthreads
    cd win64 && tar xzf $TMP_DIR/pthreads/pthreads_x64-windows.tgz; cd -

    wget -nc https://www.khronos.org/registry/OpenGL/api/GL/glext.h -P $TMP_DIR/gl/include/GL
    wget -nc https://www.khronos.org/registry/OpenGL/api/GL/glcorearb.h -P $TMP_DIR/gl/include/GL
    wget -nc https://www.khronos.org/registry/OpenGL/api/GL/wglext.h -P $TMP_DIR/gl/include/GL
    wget -nc https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h -P $TMP_DIR/gl/include/KHR
    mkdir -p win64/gl/include && cp -rf $TMP_DIR/gl/include/* win64/gl/include/.

fi
