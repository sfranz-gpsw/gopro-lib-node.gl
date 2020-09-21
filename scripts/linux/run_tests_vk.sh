rm /tmp/LOG_VK
cd tests
TARGET_OS=Windows BACKEND=vk WSLENV=BACKEND/w make -j16 -k > /tmp/LOG_VK 2>&1
cd -
