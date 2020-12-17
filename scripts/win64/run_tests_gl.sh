rm /tmp/LOG_GL
TESTS=`cd tests && make tests-list | grep test-$FILTER | sed /rtt-feature/d` 
cd tests && TARGET_OS=Windows BACKEND=gl WSLENV=BACKEND/w PYTHONPATH=../pynodegl:../pynodegl-utils WSLENV=BACKEND/w:PYTHONPATH/p make -k V=1 -j $NUM_THREADS $TESTS > /tmp/LOG_GL 2>&1
cd -
