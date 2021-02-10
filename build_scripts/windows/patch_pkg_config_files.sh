#!/bin/bash

# Export pthreads pkg-config file

VCPKG_DIR_WSL=$(wslpath $VCPKG_DIR)
VCPKG_DIR_NORMALIZED=$(echo $VCPKG_DIR | sed 's/\\/\//g')
prefix=$VCPKG_DIR_NORMALIZED/installed/x64-windows
read -r -d '' PTHREADS_PKG_CONFIG_CONTENTS << EOM
prefix=$prefix
exec_prefix=${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include

Name: libpthreads
Description: Pthreads library
Version: 3.0.0
Libs: -L${libdir} -lpthreadVC3
Cflags: -I${includedir} -D_TIMESPEC_DEFINED

EOM

echo "$PTHREADS_PKG_CONFIG_CONTENTS" > $VCPKG_DIR_WSL/installed/x64-windows/lib/pkgconfig/libpthreads.pc

# patch pkg-config files: convert cygwin paths to windows paths
find $VCPKG_DIR_WSL/installed/x64-windows/lib/pkgconfig -name '*.pc' | xargs -I{} sed -i 's/\/cygdrive\/\([^\/]\)/\1:/g' {}
