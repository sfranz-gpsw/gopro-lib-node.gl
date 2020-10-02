rm /tmp/LOG_NGFX
source nodegl-env/bin/activate
cd tests && BACKEND=ngfx make -j16 -k V=1 > /tmp/LOG_NGFX 2>&1; cd -
bash scripts/linux/status.sh /tmp/LOG_NGFX
