rm /tmp/LOG_GL
cd tests
TARGET_OS=Windows BACKEND=opengl WSLENV=BACKEND/w make -j16 -k > /tmp/LOG_GL 2>&1
cd -
