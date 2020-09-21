rm /tmp/LOG_VK
source nodegl-env/bin/activate
cd tests
BACKEND=vk make -j16 -k > /tmp/LOG_VK 2>&1
cd -
