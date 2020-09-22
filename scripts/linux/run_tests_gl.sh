rm /tmp/LOG_GL
source nodegl-env/bin/activate
cd tests && BACKEND=gl make -j16 -k V=1 > /tmp/LOG_GL 2>&1; cd -
NUM_TESTS=`cd tests && make tests-list | wc -l`
NUM_PASSED=`cat /tmp/LOG_GL | grep passed| wc -l`
echo "$NUM_PASSED out of $NUM_TESTS unit tests passed"
