#These commands were extracted from the output of "python.exe setup.py build --compiler=msvc", then modified to enable debug symbols: 
# change -O2 to -O0
#add -g to compiler flags
mkdir build/temp.linux-x86_64-3.6
mkdir build/lib.linux-x86_64-3.6
x86_64-linux-gnu-gcc -pthread -DNDEBUG -g -fwrapv -O0 -Wall -g -fstack-protector-strong -Wformat -Werror=format-security -Wdate-time -D_FORTIFY_SOURCE=2 -fPIC -g -I/home/jmoguillansky/Documents/2020/Git/gopro-lib-node.gl/nodegl-env/include -I/usr/include/python3.6m -c pynodegl.c -o build/temp.linux-x86_64-3.6/pynodegl.o
x86_64-linux-gnu-gcc -pthread -shared -Wl,-O0 -g -Wl,-Bsymbolic-functions -Wl,-Bsymbolic-functions -Wl,-z,relro -Wl,-rpath,/home/jmoguillansky/Documents/2020/Git/gopro-lib-node.gl/pynodegl/../nodegl-env/lib -g -fstack-protector-strong -Wformat -Werror=format-security -Wdate-time -D_FORTIFY_SOURCE=2 -g build/temp.linux-x86_64-3.6/pynodegl.o -L/home/jmoguillansky/Documents/2020/Git/gopro-lib-node.gl/nodegl-env/lib -lnodegl -o build/lib.linux-x86_64-3.6/pynodegl.cpython-36m-x86_64-linux-gnu.so
cp build/lib.linux-x86_64-3.6/pynodegl.cpython-36m-x86_64-linux-gnu.so .
