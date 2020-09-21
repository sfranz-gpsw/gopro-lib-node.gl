python3 -m venv nodegl-env
install -d nodegl-env/share
install -d nodegl-env/share/nodegl
install -m 644 libnodegl/nodegl.h nodegl-env/include/
install -m 644 libnodegl/nodes.specs nodegl-env/share/nodegl/
install -m 644 cmake-build-debug/libnodegl/libnodegl.a nodegl-env/lib/nodegl.lib
install -m 644 cmake-build-debug/external/sxplayer-9.5.1/libsxplayer.a nodegl-env/lib/
#install -m 644 external/win64/ffmpeg-20200831-4a11a6f-win64-dev/lib/*.lib nodegl-env/Lib/
source nodegl-env/bin/activate && python3 -m pip install -r pynodegl/requirements.txt; deactivate
source nodegl-env/bin/activate && PKG_CONFIG_PATH=$PWD/nodegl-env/lib/pkgconfig LDFLAGS=-Wl,-rpath,$PWD/nodegl-env/lib python3 -m pip -v install -e pynodegl; deactivate
source nodegl-env/bin/activate && python3 -m pip install -r pynodegl-utils/requirements.txt; deactivate
source nodegl-env/bin/activate && PKG_CONFIG_PATH=$PWD/nodegl-env/lib/pkgconfig LDFLAGS=-Wl,-rpath,$PWD/nodegl-env/lib python3 -m pip -v install -e pynodegl-utils; deactivate
