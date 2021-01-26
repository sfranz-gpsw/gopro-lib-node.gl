#
# Copyright 2016 GoPro Inc.
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

PYTHON_MAJOR = 3

#
# User configuration
#
DEBUG      ?= no
COVERAGE   ?= no
CURL       ?= curl
TAR        ?= tar
TARGET_OS  ?= $(shell uname -s)

ifeq ($(TARGET_OS),Windows)
PYTHON     ?= python.exe
PREFIX     ?= nodegl-env
W_PWD = $(shell wslpath -aw .)
W_PREFIX ?= $(W_PWD)\$(PREFIX)
$(info PYTHON: $(PYTHON))
$(info PREFIX: $(PREFIX))
$(info W_PREFIX: $(W_PREFIX))
CMAKE_GENERATOR ?= "Visual Studio 16 2019"
NGFX_GRAPHICS_BACKEND ?= "NGFX_GRAPHICS_BACKEND_DIRECT3D12"
NGFX_WINDOW_BACKEND ?= "NGFX_WINDOW_BACKEND_WINDOWS"
else
PYTHON     ?= python$(if $(shell which python$(PYTHON_MAJOR) 2> /dev/null),$(PYTHON_MAJOR),)
PREFIX     ?= $(PWD)/nodegl-env
ifeq ($(TARGET_OS),Linux)
NGFX_GRAPHICS_BACKEND ?= "NGFX_GRAPHICS_BACKEND_VULKAN"
NGFX_WINDOW_BACKEND ?= "NGFX_WINDOW_BACKEND_GLFW"
CMAKE_GENERATOR ?= "CodeBlocks - Ninja"
else ifeq ($(TARGET_OS),Darwin)
NGFX_GRAPHICS_BACKEND ?= "NGFX_GRAPHICS_BACKEND_METAL"
NGFX_WINDOW_BACKEND ?= "NGFX_WINDOW_BACKEND_APPKIT"
CMAKE_GENERATOR ?= "Xcode"
endif
endif

ifeq ($(DEBUG),yes)
CMAKE_BUILD_TYPE = "Debug"
CMAKE_BUILD_DIR = cmake-build-debug
else
CMAKE_BUILD_TYPE = "Release"
CMAKE_BUILD_DIR = cmake-build-release
endif

DEBUG_GL    ?= no
DEBUG_MEM   ?= no
DEBUG_SCENE ?= no
TESTS_SUITE ?=
V           ?=

ifneq ($(shell $(PYTHON) -c "import sys;print(sys.version_info.major)"),$(PYTHON_MAJOR))
$(error "Python $(PYTHON_MAJOR) not found")
endif

SXPLAYER_VERSION ?= 9.6.0
MOLTENVK_VERSION ?= 1.1.0
SHADERC_VERSION  ?= 2020.3

NODEGL_SETUP_OPTS =

ifeq ($(TARGET_OS), Windows)
#TODO: identify correct path
VCVARS64 ?= "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
EXTERNAL_DIR = $(shell wslpath -w external)
WINDOWS_SDK_DIR ?= C:\\Program Files (x86)\\Windows Kits\\10
VULKAN_SDK_DIR ?= $(shell wslpath -w /mnt/c/VulkanSDK/*)
NODEGL_SETUP_OPTS += -Dvulkan_sdk_dir='$(VULKAN_SDK_DIR)'
ACTIVATE = $(VCVARS64) \&\& $(PREFIX)\\Scripts\\activate.bat
else
ACTIVATE = $(PREFIX)/bin/activate
endif

NODEGL_SETUP_OPTS += -Dngfx_graphics_backend=$(NGFX_GRAPHICS_BACKEND) -Dngfx_window_backend=$(NGFX_WINDOW_BACKEND)


RPATH_LDFLAGS ?= -Wl,-rpath,$(PREFIX)/lib

ifeq ($(TARGET_OS),Windows)
MESON_BACKEND ?= vs
else ifeq ($(TARGET_OS),Darwin)
MESON_BACKEND ?= xcode
else
MESON_BACKEND ?= ninja
endif

MESON_BUILDDIR ?= builddir

ifeq ($(TARGET_OS),Windows)
MESON_SETUP   = meson setup --prefix="$(W_PREFIX)" --pkg-config-path=$(PREFIX)\\Lib\\pkgconfig -Drpath=true
# Set PKG_CONFIG and PKG_CONFIG_PATH environment variables when invoking command shell
CMD = PKG_CONFIG="$(PREFIX)\\Scripts\\pkg-config.exe" PKG_CONFIG_PATH="$(W_PREFIX)\\Lib\\pkgconfig" WSLENV=PKG_CONFIG/w:PKG_CONFIG_PATH/w cmd.exe /C
else
MESON_SETUP   = meson setup --prefix=$(PREFIX) --pkg-config-path=$(PREFIX)/lib/pkgconfig -Drpath=true
endif

ifeq ($(TARGET_OS),Windows)
MESON_COMPILE = meson compile
else
MESON_COMPILE = MAKEFLAGS= meson compile
endif
MESON_INSTALL = meson install
ifeq ($(COVERAGE),yes)
MESON_SETUP += -Db_coverage=true
DEBUG = yes
endif
ifeq ($(DEBUG),yes)
MESON_SETUP += --buildtype=debug
else
MESON_SETUP += --buildtype=release
ifneq ($(TARGET_OS),MinGW-w64)
MESON_SETUP += -Db_lto=true
endif
endif
ifneq ($(V),)
MESON_COMPILE += -v
endif
NODEGL_DEBUG_OPTS-$(DEBUG_GL)    += gl
NODEGL_DEBUG_OPTS-$(DEBUG_VK)    += vk
NODEGL_DEBUG_OPTS-$(DEBUG_MEM)   += mem
NODEGL_DEBUG_OPTS-$(DEBUG_SCENE) += scene
ifneq ($(NODEGL_DEBUG_OPTS-yes),)
NODEGL_DEBUG_OPTS = -Ddebug_opts=$(shell echo $(NODEGL_DEBUG_OPTS-yes) | tr ' ' ',')
endif

# Workaround Debian/Ubuntu bug; see https://github.com/mesonbuild/meson/issues/5925
ifeq ($(TARGET_OS),Linux)
DISTRIB_ID := $(or $(shell lsb_release -si 2>/dev/null),none)
ifeq ($(DISTRIB_ID),$(filter $(DISTRIB_ID),Ubuntu Debian))
MESON_SETUP += --libdir lib
endif
endif

ifneq ($(TESTS_SUITE),)
MESON_TESTS_SUITE_OPTS += --suite $(TESTS_SUITE)
endif

all: ngl-tools-install pynodegl-utils-install
	@echo
	@echo "    Install completed."
	@echo
	@echo "    You can now enter the venv with:"
ifeq ($(TARGET_OS),Windows)
	@echo "        . $(PREFIX)\\Scripts\\activate.bat"
else
	@echo "        . $(ACTIVATE)"
endif
	@echo

ngl-tools-install: nodegl-install
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& $(MESON_SETUP) ngl-tools $(MESON_BUILDDIR)\\ngl-tools)
ifeq ($(DEBUG),yes)
	# Set RuntimeLibrary to MultithreadedDLL using a script
	# Note: MESON doesn't support
	bash build_scripts/win64/patch_vcxproj_files.sh --set-runtime-library MultiThreadedDLL $(MESON_BUILDDIR)/ngl-tools
endif
	($(CMD) $(ACTIVATE) \&\& $(MESON_SETUP) --backend $(MESON_BACKEND) ngl-tools $(MESON_BUILDDIR)\\ngl-tools \&\& $(MESON_COMPILE) -C $(MESON_BUILDDIR)\\ngl-tools \&\& $(MESON_INSTALL) -C $(MESON_BUILDDIR)\\ngl-tools)
else
	(. $(ACTIVATE) && $(MESON_SETUP) --backend $(MESON_BACKEND) ngl-tools $(MESON_BUILDDIR)/ngl-tools && $(MESON_COMPILE) -C $(MESON_BUILDDIR)/ngl-tools && $(MESON_INSTALL) -C $(MESON_BUILDDIR)/ngl-tools)
endif

pynodegl-utils-install: pynodegl-utils-deps-install
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& pip -v install -e pynodegl-utils)
else
	(. $(ACTIVATE) && pip -v install -e ./pynodegl-utils)
endif

#
# pynodegl-install is in dependency to prevent from trying to install pynodegl
# from its requirements. Pulling pynodegl from requirements has two main issue:
# it tries to get it from PyPi (and we want to install the local pynodegl
# version), and it would fail anyway because pynodegl is currently not
# available on PyPi.
#
# We do not pull the requirements on Windows because of various issues:
# - PySide2 can't be pulled (required to be installed by the user outside the
#   Python virtualenv)
# - Pillow fails to find zlib (required to be installed by the user outside the
#   Python virtualenv)
# - ngl-control can not currently work because of:
#     - temporary files handling
#     - subprocess usage, passing fd is not supported on Windows
#     - subprocess usage, Windows cannot execute directly hooks shell scripts
#
# Still, we want the module to be installed so we can access the scene()
# decorator and other related utils.
#
pynodegl-utils-deps-install: pynodegl-install
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& pip install -r pynodegl-utils\\requirements.txt)
else ifneq ($(TARGET_OS),MinGW-w64)
	(. $(ACTIVATE) && pip install -r ./pynodegl-utils/requirements.txt)
endif

pynodegl-install: pynodegl-deps-install
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& pip -v install -e .\\pynodegl)
	#Copy DLLs and EXEs to runtime search path.  TODO: optimize
	(cp external/win64/ffmpeg_x64-windows/bin/*.dll pynodegl/.)
	(cp external/win64/pthreads_x64-windows/bin/*.dll pynodegl/.)
	(cp external/win64/shaderc_x64-windows/bin/*.dll pynodegl/.)
	(cp $(MESON_BUILDDIR)/sxplayer/*.dll pynodegl/.)
	(cp $(MESON_BUILDDIR)/libnodegl/*.dll pynodegl/.)
	(cp external/win64/RenderDoc_1.11_64/renderdoc.dll pynodegl/.)
	(cp $(MESON_BUILDDIR)/sxplayer/*.dll $(PREFIX)/Scripts/.)
	(cp $(MESON_BUILDDIR)/libnodegl/*.dll $(PREFIX)/Scripts/.)
else
	(. $(ACTIVATE) && PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig LDFLAGS=$(RPATH_LDFLAGS) pip -v install -e ./pynodegl)
endif

pynodegl-deps-install: $(PREFIX) nodegl-install
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& pip install -r pynodegl\\requirements.txt)
else
	(. $(ACTIVATE) && pip install -r ./pynodegl/requirements.txt)
endif

nodegl-install: nodegl-setup
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& $(MESON_COMPILE) -C $(MESON_BUILDDIR)\\libnodegl \&\& $(MESON_INSTALL) -C $(MESON_BUILDDIR)\\libnodegl)
	# patch libnodegl.pc TODO: remove
	sed -i -e 's/Libs.private: .*/Libs.private: OpenGL32.lib gdi32.lib/' nodegl-env/Lib/pkgconfig/libnodegl.pc
else
	(. $(ACTIVATE) && $(MESON_COMPILE) -C $(MESON_BUILDDIR)/libnodegl && $(MESON_INSTALL) -C $(MESON_BUILDDIR)/libnodegl)
endif

nodegl-setup: sxplayer-install ngfx-install shader-tools-install
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& $(MESON_SETUP) --backend $(MESON_BACKEND) $(NODEGL_SETUP_OPTS) $(NODEGL_DEBUG_OPTS) --default-library shared libnodegl $(MESON_BUILDDIR)\\libnodegl)
ifeq ($(DEBUG),yes)
	# Set RuntimeLibrary to MultithreadedDLL
	bash build_scripts/win64/patch_vcxproj_files.sh --set-runtime-library MultiThreadedDLL $(MESON_BUILDDIR)/libnodegl
endif
	# Enable MultiProcessorCompilation
	bash build_scripts/win64/patch_vcxproj_files.sh --set-multiprocessor-compilation true $(MESON_BUILDDIR)/libnodegl
else
	(. $(ACTIVATE) && $(MESON_SETUP) --backend $(MESON_BACKEND) $(NODEGL_SETUP_OPTS) $(NODEGL_DEBUG_OPTS) libnodegl $(MESON_BUILDDIR)/libnodegl)
endif

shell:
ifeq ($(TARGET_OS),Windows)
	(cmd.exe /K $(ACTIVATE))
endif

sxplayer-install: sxplayer $(PREFIX)
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& $(MESON_SETUP) --backend $(MESON_BACKEND) --default-library shared sxplayer $(MESON_BUILDDIR)\\sxplayer \&\& $(MESON_COMPILE) -C $(MESON_BUILDDIR)\\sxplayer \&\& $(MESON_INSTALL) -C $(MESON_BUILDDIR)\\sxplayer)
else
	(. $(ACTIVATE) && $(MESON_SETUP) --backend $(MESON_BACKEND) sxplayer $(MESON_BUILDDIR)/sxplayer && $(MESON_COMPILE) -C $(MESON_BUILDDIR)/sxplayer && $(MESON_INSTALL) -C $(MESON_BUILDDIR)/sxplayer)
endif

# Note for developers: in order to customize the sxplayer you're building
# against, you can use your own sources post-install:
#
#     % unlink sxplayer
#     % ln -snf /path/to/sxplayer.git sxplayer
#     % touch /path/to/sxplayer.git
#
# The `touch` command makes sure the source target directory is more recent
# than the prerequisite directory of the sxplayer rule. If this isn't true, the
# symlink will be re-recreated on the next `make` call
sxplayer: sxplayer-$(SXPLAYER_VERSION)
ifneq ($(TARGET_OS),MinGW-w64)
	ln -snf $< $@
else
	cp -r $< $@
endif
#TODO: submit PR for sxplayer changes
	bash external/patches/sxplayer/apply_patch.sh

sxplayer-$(SXPLAYER_VERSION): sxplayer-$(SXPLAYER_VERSION).tar.gz
	$(TAR) xf $<

sxplayer-$(SXPLAYER_VERSION).tar.gz:
	$(CURL) -L https://github.com/Stupeflix/sxplayer/archive/v$(SXPLAYER_VERSION).tar.gz -o $@

shaderc-install: SHADERC_LIB_FILENAME = libshaderc_shared.1.dylib
shaderc-install: shaderc-$(SHADERC_VERSION) $(PREFIX)
ifeq ($(TARGET_OS),Windows)
	echo "prebuilt shaderc is automatically downloaded as a configuration step"
else
	(cd $< && ./utils/git-sync-deps)
	cmake -B $</build -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(PREFIX) $<
	ninja -C $</build install
	# XXX: mac only, test on linux without this; also maybe a cmake option
	install_name_tool -id @rpath/$(SHADERC_LIB_FILENAME) $(PREFIX)/lib/$(SHADERC_LIB_FILENAME)
endif

shader-tools-install: $(PREFIX) ngfx-install
ifeq ($(TARGET_OS), Windows)
	( cd shader-tools && cmake.exe -H. -B$(CMAKE_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -G $(CMAKE_GENERATOR)  -D$(NGFX_GRAPHICS_BACKEND)=ON )
ifeq ($(DEBUG),yes)
	# Set RuntimeLibrary to MultithreadedDLL
	bash build_scripts/win64/patch_vcxproj_files.sh --set-runtime-library MultiThreadedDLL shader-tools/$(CMAKE_BUILD_DIR)
endif
	( \
	  cd shader-tools && cmake.exe --build $(CMAKE_BUILD_DIR) --config $(CMAKE_BUILD_TYPE) -j8 && \
	  cmake.exe --install $(CMAKE_BUILD_DIR) --config $(CMAKE_BUILD_TYPE) --prefix ../external/win64/shader_tools_x64-windows \
	)
ifeq ($(NGFX_GRAPHICS_BACKEND), NGFX_GRAPHICS_BACKEND_DIRECT3D12)
	($(CMD) $(ACTIVATE) \&\& shader-tools\\$(CMAKE_BUILD_DIR)\\$(CMAKE_BUILD_TYPE)\\compile_shaders_dx12.exe d3dBlitOp)
endif
else ifeq ($(TARGET_OS), Linux)
	( \
	  cd shader-tools && \
	  cmake -H. -B$(CMAKE_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -G $(CMAKE_GENERATOR)  -D$(NGFX_GRAPHICS_BACKEND)=ON && \
	  cmake --build $(CMAKE_BUILD_DIR) -j8 && \
	  cmake --install $(CMAKE_BUILD_DIR) --config $(CMAKE_BUILD_TYPE) --prefix ../external/linux/shader_tools_x64-linux \
	)
	cp external/linux/shader_tools_x64-linux/lib/libshader_tools.so $(PREFIX)/lib
else ifeq ($(TARGET_OS), Darwin)
	( \
	  cd shader-tools && \
	  cmake -H. -B$(CMAKE_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -G $(CMAKE_GENERATOR)  -D$(NGFX_GRAPHICS_BACKEND)=ON && \
	  cmake --build $(CMAKE_BUILD_DIR) -j8 && \
	  cmake --install $(CMAKE_BUILD_DIR) --config $(CMAKE_BUILD_TYPE) --prefix ../external/darwin/shader_tools_x64-darwin \
	)
	cp external/darwin/shader_tools_x64-darwin/lib/libshader_tools.dylib $(PREFIX)/lib
endif

ngfx-install: $(PREFIX)
ifeq ($(TARGET_OS), Windows)
	( cd ngfx && cmake.exe -H. -B$(CMAKE_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -G $(CMAKE_GENERATOR) -D$(NGFX_GRAPHICS_BACKEND)=ON )
ifeq ($(DEBUG),yes)
	# Set RuntimeLibrary to MultithreadedDLL
	bash build_scripts/win64/patch_vcxproj_files.sh --set-runtime-library MultiThreadedDLL ngfx/$(CMAKE_BUILD_DIR)
	# Enable MultiProcessorCompilation
	bash build_scripts/win64/patch_vcxproj_files.sh --set-multiprocessor-compilation true ngfx/$(CMAKE_BUILD_DIR)
endif
	( \
	  cd ngfx && cmake.exe --build $(CMAKE_BUILD_DIR) --config $(CMAKE_BUILD_TYPE) -j8 && \
	  cmake.exe --install $(CMAKE_BUILD_DIR) --config $(CMAKE_BUILD_TYPE) --prefix ../external/win64/ngfx_x64-windows \
	)
	cp external/win64/ngfx_x64-windows/lib/ngfx.lib $(PREFIX)/Lib
else ifeq ($(TARGET_OS), Linux)
	( \
	  cd ngfx && \
	  cmake -H. -B$(CMAKE_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -G $(CMAKE_GENERATOR) -D$(NGFX_GRAPHICS_BACKEND)=ON && \
	  cmake --build $(CMAKE_BUILD_DIR) -j8 && \
	  cmake --install $(CMAKE_BUILD_DIR) --prefix ../external/linux/ngfx_x64-linux \
	)
	cp external/linux/ngfx_x64-linux/lib/libngfx.so $(PREFIX)/lib
else ifeq ($(TARGET_OS), Darwin)
	( \
	  cd ngfx && \
	  cmake -H. -B$(CMAKE_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -G $(CMAKE_GENERATOR) -D$(NGFX_GRAPHICS_BACKEND)=ON && \
	  cmake --build $(CMAKE_BUILD_DIR) -j8 && \
	  cmake --install $(CMAKE_BUILD_DIR) --config $(CMAKE_BUILD_TYPE) --prefix ../external/darwin/ngfx_x64-darwin \
	)
	cp external/darwin/ngfx_x64-darwin/lib/libngfx.dylib $(PREFIX)/lib
endif

ngl-debug-tools: $(PREFIX)
ifeq ($(TARGET_OS), Windows)
	( cd ngl-debug-tools && cmake.exe -H. -B$(CMAKE_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -G $(CMAKE_GENERATOR) )
ifeq ($(DEBUG),yes)
	# Set RuntimeLibrary to MultithreadedDLL
	bash build_scripts/win64/patch_vcxproj_files.sh --set-runtime-library MultiThreadedDLL ngl-debug-tools/$(CMAKE_BUILD_DIR)
endif
	( cd ngl-debug-tools && cmake.exe --build $(CMAKE_BUILD_DIR) --config $(CMAKE_BUILD_TYPE) -j8 )
	(cp external/win64/RenderDoc_1.11_64/renderdoc.dll ngl-debug-tools/$(CMAKE_BUILD_DIR)/$(CMAKE_BUILD_TYPE))
else ifeq ($(TARGET_OS), Linux)
	( \
	  cd ngl-debug-tools && \
	  cmake -H. -B$(CMAKE_BUILD_DIR) -G $(CMAKE_GENERATOR) && \
	  cmake --build $(CMAKE_BUILD_DIR) -j8 \
	)
endif


shaderc-$(SHADERC_VERSION): shaderc-$(SHADERC_VERSION).tar.gz
	$(TAR) xf $<

shaderc-$(SHADERC_VERSION).tar.gz:
	$(CURL) -L https://github.com/google/shaderc/archive/v$(SHADERC_VERSION).tar.gz -o $@

# Note: somehow xcodebuild sets name @rpath/libMoltenVK.dylib automatically
# (according to otool -l) so we don't have to do anything special
MoltenVK-install: MoltenVK $(PREFIX)
	(cd $< && ./fetchDependencies -v --macos)
	$(MAKE) -C $< macos
	install -d $(PREFIX)/include
	install -d $(PREFIX)/lib
	cp -v $</Package/Latest/MoltenVK/dylib/macOS/libMoltenVK.dylib $(PREFIX)/lib
	cp -vr $</Package/Latest/MoltenVK/include $(PREFIX)

MoltenVK: MoltenVK-$(MOLTENVK_VERSION)
	ln -snf $< $@

MoltenVK-$(MOLTENVK_VERSION): MoltenVK-$(MOLTENVK_VERSION).tar.gz
	$(TAR) xf $<

MoltenVK-$(MOLTENVK_VERSION).tar.gz:
	$(CURL) -L https://github.com/KhronosGroup/MoltenVK/archive/v$(MOLTENVK_VERSION).tar.gz -o $@

#
# We do not pull meson from pip on Windows for the same reasons we don't pull
# Pillow and PySide2. We require the users to have it on their system.
#
$(PREFIX):
	(git clean -fxd external)
	(cd external && bash scripts/sync.sh $(TARGET_OS))
ifeq ($(TARGET_OS),Windows)
	$(PYTHON) -m venv $(PREFIX)
	($(CMD) copy external\\win64\\pkg-config\\* nodegl-env\\Scripts\\.)
	($(CMD) mkdir $(PREFIX)\\Lib\\pkgconfig)
	($(CMD) pushd external\\win64\\ffmpeg_x64-windows \&\& python.exe scripts/install.py "$(W_PREFIX)" \& popd)
	($(CMD) pushd external\\win64\\pthreads_x64-windows \&\& python.exe scripts/install.py "$(W_PREFIX)" \& popd)
	($(CMD) pushd external\\win64\\sdl2_x64-windows \&\& python.exe scripts/install.py "$(W_PREFIX)" \& popd)
	($(CMD) pushd external\\win64\\shaderc_x64-windows \&\& python.exe scripts/install.py "$(W_PREFIX)" \& popd)
	(sed -i -e 's/Libs: .*/Libs: -L\${libdir} -lSDL2 -L\${libdir}/manual-link -lSDL2main/' $(PREFIX)/Lib/pkgconfig/sdl2.pc)
	(cp external/win64/ffmpeg_x64-windows/tools/ffmpeg/*.exe $(PREFIX)/Scripts/.)
	(cp external/win64/ffmpeg_x64-windows/bin/*.dll $(PREFIX)/Scripts/.)
	(cp external/win64/pthreads_x64-windows/bin/*.dll $(PREFIX)/Scripts/.)
	(cp external/win64/shaderc_x64-windows/bin/*.dll $(PREFIX)/Scripts/.)
	(cp external/win64/RenderDoc_1.11_64/renderdoc.dll $(PREFIX)/Scripts/.)
else ifeq ($(TARGET_OS),MinGW-w64)
	$(PYTHON) -m venv --system-site-packages $(PREFIX)
else
	$(PYTHON) -m venv $(PREFIX)
endif

tests: nodegl-tests tests-setup
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& meson test $(MESON_TESTS_SUITE_OPTS) -C $(MESON_BUILDDIR)\\tests)
else
	(. $(ACTIVATE) && meson test $(MESON_TESTS_SUITE_OPTS) -C $(MESON_BUILDDIR)/tests)
endif

tests-setup: ngl-tools-install pynodegl-utils-install
ifeq ($(TARGET_OS),Windows)
	# meson test only unsupports ninja backend
	($(CMD) $(ACTIVATE) \&\& $(MESON_SETUP) --backend ninja $(MESON_BUILDDIR)\\tests tests)
else
	(. $(ACTIVATE) && $(MESON_SETUP) --backend ninja $(MESON_BUILDDIR)/tests tests)
endif

nodegl-tests: nodegl-install
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& meson test -C $(MESON_BUILDDIR)\\libnodegl)
else
	(. $(ACTIVATE) && meson test -C $(MESON_BUILDDIR)/libnodegl)
endif

nodegl-%: nodegl-setup
	(. $(ACTIVATE) && $(MESON_COMPILE) -C $(MESON_BUILDDIR)/libnodegl $(subst nodegl-,,$@))

clean_py:
	$(RM) pynodegl/nodes_def.pyx
	$(RM) pynodegl/pynodegl.c
	$(RM) pynodegl/pynodegl.*.so
	$(RM) -r pynodegl/build
	$(RM) -r pynodegl/pynodegl.egg-info
	$(RM) -r pynodegl/.eggs
	$(RM) -r pynodegl-utils/pynodegl_utils.egg-info
	$(RM) -r pynodegl-utils/.eggs

clean: clean_py
	$(RM) -r $(MESON_BUILDDIR)/sxplayer
	$(RM) -r $(MESON_BUILDDIR)/libnodegl
	$(RM) -r $(MESON_BUILDDIR)/ngl-tools
	$(RM) -r $(MESON_BUILDDIR)/tests

# You need to build and run with COVERAGE set to generate data.
# For example: `make clean && make -j8 tests COVERAGE=yes`
# We don't use `meson coverage` here because of
# https://github.com/mesonbuild/meson/issues/7895
coverage-html:
	(. $(ACTIVATE) && ninja -C $(MESON_BUILDDIR)/libnodegl coverage-html)
coverage-xml:
	(. $(ACTIVATE) && ninja -C $(MESON_BUILDDIR)/libnodegl coverage-xml)

.PHONY: all
.PHONY: ngl-tools-install
.PHONY: pynodegl-utils-install pynodegl-utils-deps-install
.PHONY: pynodegl-install pynodegl-deps-install
.PHONY: nodegl-install nodegl-setup
.PHONY: sxplayer-install
.PHONY: shaderc-install
.PHONY: MoltenVK-install
.PHONY: tests tests-setup
.PHONY: clean clean_py
.PHONY: coverage-html coverage-xml
.PHONY: ngfx-install
.PHONY: shell
.PHONY: ngl-debug-tools
