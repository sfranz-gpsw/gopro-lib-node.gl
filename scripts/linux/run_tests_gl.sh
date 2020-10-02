rm /tmp/LOG_GL
source nodegl-env/bin/activate
cd tests && BACKEND=gl make -j16 -k V=1 > /tmp/LOG_GL 2>&1; cd -
bash scripts/linux/status.sh /tmp/LOG_GL
