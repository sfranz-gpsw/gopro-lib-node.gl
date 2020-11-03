cmake.exe -H. -Bcmake-build-debug -DCMAKE_BUILD_TYPE=RelWithDebInfo -G "Visual Studio 16 2019" -DNGFX_GRAPHICS_BACKEND_DIRECT3D12=ON
cmake.exe --build cmake-build-debug --config RelWithDebInfo -j8
bash scripts/win64/copy_dlls.sh
bash scripts/win64/build_shaders.sh d3dBlitOp
