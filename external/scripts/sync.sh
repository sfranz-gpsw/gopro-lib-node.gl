OS=$1
TOKEN=`cat token.cfg`
echo "OS: [$OS]"
TMP_DIR=/tmp/gopro-pkg/external

if [ $OS == "win64" ]; then
    mkdir -p win64

    mkdir -p $TMP_DIR/pkg-config && wget -nc https://github.com/jmoguillansky-gpsw/gopro-nodegl-thirdparty-binaries/blob/master/external/win64/pkg-config.tgz?raw=true -O $TMP_DIR/pkg-config/pkg-config.tgz -P $TMP_DIR/pkg-config
    cd win64 && tar xzf $TMP_DIR/pkg-config/pkg-config.tgz; cd -

    mkdir -p $TMP_DIR/ffmpeg && wget --header "Authorization: token $TOKEN" -nc https://api.github.com/repos/jmoguillansky-gpsw/gopro-pkg/actions/artifacts/28753508/zip -O $TMP_DIR/ffmpeg/ffmpeg_x64-windows.zip -P $TMP_DIR/ffmpeg
    cd win64 && unzip $TMP_DIR/ffmpeg/ffmpeg_x64-windows.zip -d ffmpeg_x64-windows; cd -

    mkdir -p $TMP_DIR/pthreads && wget --header "Authorization: token $TOKEN" -nc https://api.github.com/repos/jmoguillansky-gpsw/gopro-pkg/actions/artifacts/28753514/zip -O $TMP_DIR/pthreads/pthreads_x64-windows.zip -P $TMP_DIR/pthreads
    cd win64 && unzip $TMP_DIR/pthreads/pthreads_x64-windows.zip -d pthreads_x64-windows; cd -

    wget -nc https://www.khronos.org/registry/OpenGL/api/GL/glext.h -P $TMP_DIR/gl/include/GL
    wget -nc https://www.khronos.org/registry/OpenGL/api/GL/glcorearb.h -P $TMP_DIR/gl/include/GL
    wget -nc https://www.khronos.org/registry/OpenGL/api/GL/wglext.h -P $TMP_DIR/gl/include/GL
    wget -nc https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h -P $TMP_DIR/gl/include/KHR
    mkdir -p win64/gl/include && cp -rf $TMP_DIR/gl/include/* win64/gl/include/.

fi
