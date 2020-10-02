rm /tmp/LOG_NGFX
source nodegl-env/bin/activate
cd tests && BACKEND=ngfx make -j16 -k V=1 > /tmp/LOG_NGFX 2>&1; cd -
NUM_TESTS=`cd tests && make tests-list | wc -l`
NUM_PASSED=`cat /tmp/LOG_NGFX | grep passed| wc -l`
echo "$NUM_PASSED out of $NUM_TESTS unit tests passed"
