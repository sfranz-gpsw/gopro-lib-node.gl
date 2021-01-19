# Installation

## Dependencies

Make sure you have installed on your system the essential list of dependencies
for building and running the complete **node.gl** stack:

- **GCC** or **Clang**
- **GNU/Make**
- **FFmpeg** (and its libraries for compilation)
- **Python 3.x** (you will need the package with the build headers as well,
  typically named with a `-devel` suffix on Debian based systems)
- **pip** (Python 3 version)
- **Graphviz**
- **SDL2**

## Quick user installation on Linux and MacOS

The following steps describe how to install `node.gl` and its dependencies in
your home user directory, without polluting your system (aside from the system
dependencies which you should install and remove in sane ways).

For a fast ready-to-go user experience, you can use the default rule of the
root `Makefile`.  Calling `make` in the root directory of node.gl will create a
complete environment (based on Python `venv` module):

```shell
% make
[...]
    Install completed.

    You can now enter the venv with:
        . /home/user/node.gl/nodegl-env/bin/activate
```

Jobbed `make` calls are supported, so you can use `make -jN` where `N` is the
number of parallel processes.

**Note**: to leave the environment, you can use `deactivate`.

## Quick user installation on Windows (MINGW toolchain)

Windows supports two configurations: MINGW toolchain, and Visual Studio toolchain (MSVC).
On Windows / MINGW, the steps to bootstrap the environment:

- install [MSYS2](https://www.msys2.org/)
- run MinGW64 shell (*NOT* MSYS2, "MINGW64" should be visible in the prompt)
- run the following in the shell:
```shell
pacman -Syuu  # and restart the shell
pacman -S git make
pacman -S mingw-w64-x86_64-{toolchain,ffmpeg,python}
pacman -S mingw-w64-x86_64-python-watchdog
pacman -S mingw-w64-x86_64-python3-{pillow,pip}
pacman -S mingw-w64-x86_64-pyside2-qt5
pacman -S mingw-w64-x86_64-meson
make TARGET_OS=MinGW-w64
```

Then you should be able to enter the environment and run the tools.

## Quick user installation on Windows (MSVC toolchain)

We use Windows Subsystem for Linux in order to establish a similar environment
as Linux.

The steps to build:

1.  Install Tools:

a) Install Windows Subsystem for Linux, and Ubuntu (recommend 20.04)
b) Install Microsoft Visual Studio 2019
c) Install CMake
d) Install Python using the Windows Installer (https://www.python.org/downloads/windows/

2.  In Windows Subsystem for Linux:

sudo apt -y update
sudo apt -y install build-essential zip

3.  Install 3rd Party Dependencies:

We use vcpkg package manager to install various 3rd party dependencies.

First download vcpkg from https://github.com/microsoft/vcpkg.
You can install vcpkg anywhere.
Then, install the following packages:

vcpkg.exe install pthreads:x64-windows opengl-registry:x64-windows ffmpeg[ffmpeg,ffprobe]:x64-windows sdl2:x64-windows
vcpkg.exe integrate install

4.  Set Build Environment Variables:

Add C:\vcpkg\installed\x64-windows\tools\ffmpeg to your system %PATH%. The
ffmpeg and ffprobe binaries must be available in order to run the tests.

We use various environment variables as part of the build

VCVARS64: full path to vcvars64.bat
VCPKG_DIR: path to vcpkg directory
TARGET_OS: set to Windows

For example:
export VCVARS64='"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"'
export VCPKG_DIR='C:\vcpkg'
export TARGET_OS=Windows

Please follow Bash syntax rules:
Reference: https://wiki-dev.bash-hackers.org/syntax/quoting

5.  Build:

make -j8 TARGET_OS=Windows

## Installation of `libnodegl` (the core library)

`libnodegl` uses [Meson][meson] for its build system. Its compilation and
installation usually looks like the following:

```sh
meson setup builddir
meson compile -C builddir
meson install -C builddir
```

`meson configure` can be used to list the available options. See the [Meson
documentation][meson-doc] for more information.

[meson]: https://mesonbuild.com/
[meson-doc]: https://mesonbuild.com/Quick-guide.html#compiling-a-meson-project

## Installation of `ngl-tools`

The `node.gl` tools located in the `ngl-tools/` directory are to be built and
installed exactly the same way as `libnodegl`.

## Installation of `pynodegl` (the Python binding)

```shell
pip install -r ./pynodegl/requirements.txt
pip install ./pynodegl
```

## Installation of `pynodegl-utils` (the Python utilities and examples)

```shell
pip install -r ./pynodegl-utils/requirements.txt
pip install ./pynodegl-utils
```
