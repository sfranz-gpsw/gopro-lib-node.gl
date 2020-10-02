rm /tmp/LOG_VK
source nodegl-env/bin/activate
cd tests && BACKEND=vk make -j16 -k V=1 > /tmp/LOG_VK 2>&1; cd -
bash scripts/linux/status.sh /tmp/LOG_VK

