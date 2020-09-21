rm /tmp/LOG_GL
source nodegl-env/bin/activate
cd tests
BACKEND=gl make -j16 -k > /tmp/LOG_GL 2>&1
cd -
