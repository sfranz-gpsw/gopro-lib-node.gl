rm /tmp/LOG_NGFX
source nodegl-env/bin/activate
cd tests && BACKEND=ngfx make -k V=1 -j $NUM_THREADS > /tmp/LOG_NGFX 2>&1; cd -
bash scripts/linux/status.sh /tmp/LOG_NGFX
