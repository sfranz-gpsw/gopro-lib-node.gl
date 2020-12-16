git clone https://github.com/google/shaderc
cd shaderc
cmake.exe -H. -Bcmake-build-debug -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake.exe --build cmake-build-debug -j8 --config RelWithDebInfo
