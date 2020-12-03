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

PREFIX          = venv
PREFIX_DONE     = .venv-done
export TARGET_OS ?= $(shell uname -s)
ifeq ($(TARGET_OS),Windows)
PREFIX_FULLPATH = $(shell wslpath -wa .)\$(PREFIX)
else
PREFIX_FULLPATH = $(PWD)/$(PREFIX)
endif
PYTHON_MAJOR = 3

DEBUG      ?= no
COVERAGE   ?= no
ifeq ($(TARGET_OS),Windows)
PYTHON     ?= python.exe
else
PYTHON     ?= python$(if $(shell which python$(PYTHON_MAJOR) 2> /dev/null),$(PYTHON_MAJOR),)
endif

ifeq ($(TARGET_OS),Windows)
PIP = $(PREFIX)/Scripts/pip.exe
else
PIP = $(PREFIX)/bin/pip
endif
TARGET_OS_LOWERCASE = $(shell $(PYTHON) -c "print('$(TARGET_OS)'.lower())" )
DEBUG_GL    ?= no
DEBUG_MEM   ?= no
DEBUG_SCENE ?= no
TESTS_SUITE ?=
V           ?=

ifeq ($(TARGET_OS),Windows)
VCPKG_DIR ?= C:\\vcpkg
PKG_CONF_DIR = external\\pkgconf\\build
# General way to call cmd from bash: https://github.com/microsoft/WSL/issues/2835
# Add the character @ after /C
CMD = cmd.exe /C @
else
CMD =
endif

ifneq ($(shell $(CMD) $(PYTHON) -c "import sys;print(sys.version_info.major)"),$(PYTHON_MAJOR))
$(error "Python $(PYTHON_MAJOR) not found")
endif

ifeq ($(TARGET_OS),Windows)
ACTIVATE = "$(PREFIX_FULLPATH)\\Scripts\\activate.bat"
else
ACTIVATE = $(PREFIX_FULLPATH)/bin/activate
endif

RPATH_LDFLAGS = -Wl,-rpath,$(PREFIX_FULLPATH)/lib

ifeq ($(TARGET_OS),Windows)
MESON = meson.exe
MESON_SETUP_PARAMS  = \
    --prefix="$(PREFIX_FULLPATH)" --bindir="$(PREFIX_FULLPATH)\\Scripts" --includedir="$(PREFIX_FULLPATH)\\Include" \
    --libdir="$(PREFIX_FULLPATH)\\Lib" --pkg-config-path="$(VCPKG_DIR)\\installed\x64-$(TARGET_OS_LOWERCASE)\\lib\\pkgconfig;$(PREFIX_FULLPATH)\\Lib\\pkgconfig"
else
MESON = meson
MESON_SETUP_PARAMS = --prefix=$(PREFIX_FULLPATH) --pkg-config-path=$(PREFIX_FULLPATH)/lib/pkgconfig -Drpath=true
endif
MESON_SETUP         = $(MESON) setup --backend $(MESON_BACKEND) $(MESON_SETUP_PARAMS)
MESON_TEST          = $(MESON) test

MESON_COMPILE = MAKEFLAGS= $(MESON) compile -j8
MESON_INSTALL = $(MESON) install
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

ifeq ($(TARGET_OS),Windows)
CMAKE ?= cmake.exe
else
CMAKE ?= cmake
endif

ifeq ($(TARGET_OS),Windows)
CMAKE_GENERATOR ?= "Visual Studio 16 2019"
NGFX_GRAPHICS_BACKEND ?= "NGFX_GRAPHICS_BACKEND_DIRECT3D12"
NGFX_WINDOW_BACKEND ?= "NGFX_WINDOW_BACKEND_WINDOWS"
else ifeq ($(TARGET_OS),Linux)
NGFX_GRAPHICS_BACKEND ?= "NGFX_GRAPHICS_BACKEND_VULKAN"
NGFX_WINDOW_BACKEND ?= "NGFX_WINDOW_BACKEND_GLFW"
CMAKE_GENERATOR ?= "CodeBlocks - Ninja"
else ifeq ($(TARGET_OS),Darwin)
NGFX_GRAPHICS_BACKEND ?= "NGFX_GRAPHICS_BACKEND_METAL"
NGFX_WINDOW_BACKEND ?= "NGFX_WINDOW_BACKEND_APPKIT"
CMAKE_GENERATOR ?= "Xcode"
endif

ifeq ($(DEBUG),yes)
CMAKE_BUILD_TYPE = Debug
CMAKE_BUILD_DIR = cmake-build-debug
else
CMAKE_BUILD_TYPE = Release
CMAKE_BUILD_DIR = cmake-build-release
endif

CMAKE_SETUP = $(CMAKE) -H. -B$(CMAKE_BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -G $(CMAKE_GENERATOR)
CMAKE_COMPILE = $(CMAKE) --build $(CMAKE_BUILD_DIR) --config $(CMAKE_BUILD_TYPE) -j8
CMAKE_INSTALL = $(CMAKE) --install $(CMAKE_BUILD_DIR) --config $(CMAKE_BUILD_TYPE)

NODEGL_SETUP_OPTS =

ifeq ($(TARGET_OS), Windows)
EXTERNAL_DIR = $(shell wslpath -w external)
WINDOWS_SDK_DIR ?= C:\\Program Files (x86)\\Windows Kits\\10
VULKAN_SDK_DIR ?= $(shell wslpath -w /mnt/c/VulkanSDK/*)
NODEGL_SETUP_OPTS += -Dvulkan_sdk_dir='$(VULKAN_SDK_DIR)'
endif

NODEGL_SETUP_OPTS += -Dngfx_graphics_backend=$(NGFX_GRAPHICS_BACKEND) -Dngfx_window_backend=$(NGFX_WINDOW_BACKEND)

ifeq ($(TARGET_OS),Windows)
MESON_BACKEND ?= vs
else ifeq ($(TARGET_OS),Darwin)
MESON_BACKEND ?= xcode
else
MESON_BACKEND ?= ninja
endif

MESON_BUILDDIR ?= builddir

all: ngl-tools-install pynodegl-utils-install
	@echo
	@echo "    Install completed."
	@echo
	@echo "    You can now enter the venv with:"
	@echo "        $(ACTIVATE)"
	@echo

ngl-tools-install: nodegl-install
	($(MESON_SETUP) --backend $(MESON_BACKEND) ngl-tools $(MESON_BUILDDIR)/ngl-tools)
ifeq ($(TARGET_OS),Windows)
ifeq ($(DEBUG),yes)
	# Set RuntimeLibrary to MultithreadedDLL using a script
	# Note: Meson doesn't support
	bash build_scripts/$(TARGET_OS_LOWERCASE)/patch_vcxproj_files.sh --set-runtime-library MultiThreadedDLL $(MESON_BUILDDIR)/ngl-tools
endif
endif
	($(MESON_COMPILE) -C $(MESON_BUILDDIR)/ngl-tools && $(MESON_INSTALL) -C $(MESON_BUILDDIR)/ngl-tools)

pynodegl-utils-install: pynodegl-utils-deps-install
	($(PIP) install -e ./pynodegl-utils)

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
	($(PIP) install -r ./pynodegl-utils/requirements.txt)

pynodegl-install: pynodegl-deps-install
	($(PIP) -v install -e ./pynodegl)

pynodegl-deps-install: $(PREFIX_DONE) nodegl-install
	($(PIP) install -r ./pynodegl/requirements.txt)

nodegl-install: nodegl-setup
	($(MESON_COMPILE) -C $(MESON_BUILDDIR)/libnodegl && $(MESON_INSTALL) -C $(MESON_BUILDDIR)/libnodegl)

nodegl-setup: sxplayer-install ngfx-install shader-tools-install
	($(MESON_SETUP) --backend $(MESON_BACKEND) $(NODEGL_SETUP_OPTS) $(NODEGL_DEBUG_OPTS) --default-library shared libnodegl $(MESON_BUILDDIR)/libnodegl)
ifeq ($(TARGET_OS),Windows)
ifeq ($(DEBUG),yes)
	# Set RuntimeLibrary to MultithreadedDLL
	bash build_scripts/$(TARGET_OS_LOWERCASE)/patch_vcxproj_files.sh --set-runtime-library MultiThreadedDLL $(MESON_BUILDDIR)/libnodegl
endif
	# Enable MultiProcessorCompilation
	bash build_scripts/$(TARGET_OS_LOWERCASE)/patch_vcxproj_files.sh --set-multiprocessor-compilation true $(MESON_BUILDDIR)/libnodegl
endif

pkg-config-install: external-download $(PREFIX_DONE)
ifeq ($(TARGET_OS),Windows)
	($(MESON_SETUP) -Dtests=false external/pkgconf $(MESON_BUILDDIR)/pkgconf && $(MESON_COMPILE) -C builddir/pkgconf && $(MESON_INSTALL) -C $(MESON_BUILDDIR)/pkgconf)
endif

sxplayer-install: external-download pkg-config-install $(PREFIX_DONE)
	($(MESON_SETUP) external/sxplayer $(MESON_BUILDDIR)/sxplayer && $(MESON_COMPILE) -C $(MESON_BUILDDIR)/sxplayer && $(MESON_INSTALL) -C $(MESON_BUILDDIR)/sxplayer)

external-download:
	$(MAKE) -C external
ifeq ($(TARGET_OS),Darwin)
	$(MAKE) -C external MoltenVK shaderc
endif

ifeq ($(TARGET_OS),Darwin)
external-install: sxplayer-install MoltenVK-install shaderc-install
else
external-install: sxplayer-install
endif

shader-tools-install: $(PREFIX_DONE) ngfx-install
	(cd shader-tools && $(CMAKE_SETUP) -D$(NGFX_GRAPHICS_BACKEND)=ON)
ifeq ($(TARGET_OS), Windows)
ifeq ($(DEBUG),yes)
	# Set RuntimeLibrary to MultithreadedDLL
	bash build_scripts/$(TARGET_OS_LOWERCASE)/patch_vcxproj_files.sh --set-runtime-library MultiThreadedDLL shader-tools/$(CMAKE_BUILD_DIR)
endif
	(cd shader-tools && $(CMAKE_COMPILE) && $(CMAKE_INSTALL) --prefix ../external/$(TARGET_OS_LOWERCASE)/shader_tools_x64-$(TARGET_OS_LOWERCASE))
ifeq ($(NGFX_GRAPHICS_BACKEND), NGFX_GRAPHICS_BACKEND_DIRECT3D12)
	(shader-tools/$(CMAKE_BUILD_DIR)/$(CMAKE_BUILD_TYPE)/compile_shaders_dx12.exe d3dBlitOp)
endif
else ifeq ($(TARGET_OS), Linux)
	(cd shader-tools && $(CMAKE_COMPILE) && $(CMAKE_INSTALL) --prefix ../external/$(TARGET_OS_LOWERCASE)/shader_tools_x64-$(TARGET_OS_LOWERCASE))
	cp external/$(TARGET_OS_LOWERCASE)/shader_tools_x64-$(TARGET_OS_LOWERCASE)/lib/libshader_tools.so $(PREFIX)/lib
else ifeq ($(TARGET_OS), Darwin)
	(cd shader-tools && $(CMAKE_COMPILE) && $(CMAKE_INSTALL) --prefix ../external/$(TARGET_OS_LOWERCASE)/shader_tools_x64-$(TARGET_OS_LOWERCASE))
	cp external/$(TARGET_OS_LOWERCASE)/shader_tools_x64-$(TARGET_OS_LOWERCASE)/lib/libshader_tools.dylib $(PREFIX)/lib
endif


ngfx-install: $(PREFIX_DONE)
ifeq ($(TARGET_OS), Windows)
	( cd ngfx && $(CMAKE_SETUP) -D$(NGFX_GRAPHICS_BACKEND)=ON )
ifeq ($(DEBUG),yes)
	# Set RuntimeLibrary to MultithreadedDLL
	bash build_scripts/$(TARGET_OS_LOWERCASE)/patch_vcxproj_files.sh --set-runtime-library MultiThreadedDLL ngfx/$(CMAKE_BUILD_DIR)
	# Enable MultiProcessorCompilation
	bash build_scripts/$(TARGET_OS_LOWERCASE)/patch_vcxproj_files.sh --set-multiprocessor-compilation true ngfx/$(CMAKE_BUILD_DIR)
endif
	( cd ngfx && $(CMAKE_COMPILE) && $(CMAKE_INSTALL) --prefix ../external/$(TARGET_OS_LOWERCASE)/ngfx_x64-$(TARGET_OS_LOWERCASE) )
	cp external/$(TARGET_OS_LOWERCASE)/ngfx_x64-$(TARGET_OS_LOWERCASE)/lib/ngfx.lib $(PREFIX)/Lib
else ifeq ($(TARGET_OS), Linux)
	( \
	  cd ngfx && \
	  $(CMAKE_SETUP) -D$(NGFX_GRAPHICS_BACKEND)=ON && \
	  $(CMAKE_COMPILE) && \
	  $(CMAKE_INSTALL) --prefix ../external/$(TARGET_OS_LOWERCASE)/ngfx_x64-$(TARGET_OS_LOWERCASE) \
	)
	cp external/$(TARGET_OS_LOWERCASE)/ngfx_x64-$(TARGET_OS_LOWERCASE)/lib/libngfx.so $(PREFIX)/lib
else ifeq ($(TARGET_OS), Darwin)
	( \
	  cd ngfx && \
	  $(CMAKE_SETUP) -D$(NGFX_GRAPHICS_BACKEND)=ON && \
	  $(CMAKE_COMPILE) && \
	  $(CMAKE_INSTALL) --prefix ../external/$(TARGET_OS_LOWERCASE)/ngfx_x64-$(TARGET_OS_LOWERCASE) \
	)
	cp external/$(TARGET_OS_LOWERCASE)/ngfx_x64-$(TARGET_OS_LOWERCASE)/lib/libngfx.dylib $(PREFIX)/lib
endif

ngl-debug-tools: $(PREFIX_DONE)
	( cd ngl-debug-tools && $(CMAKE_SETUP) )
ifeq ($(TARGET_OS), Windows)
ifeq ($(DEBUG),yes)
	# Set RuntimeLibrary to MultithreadedDLL
	bash build_scripts/$(TARGET_OS_LOWERCASE)/patch_vcxproj_files.sh --set-runtime-library MultiThreadedDLL ngl-debug-tools/$(CMAKE_BUILD_DIR)
endif
	( cd ngl-debug-tools && $(CMAKE_COMPILE) )
	(cp external/$(TARGET_OS_LOWERCASE)/RenderDoc_1.11_64/renderdoc.dll ngl-debug-tools/$(CMAKE_BUILD_DIR)/$(CMAKE_BUILD_TYPE))
else
	( cd ngl-debug-tools && $(CMAKE_COMPILE) )
endif

shaderc-install: SHADERC_LIB_FILENAME = libshaderc_shared.1.dylib
shaderc-install: external-download $(PREFIX_DONE)
	cd external/shaderc && ./utils/git-sync-deps
	$(CMAKE_SETUP) && $(CMAKE_COMPILE) && $(CMAKE_INSTALL) --prefix $(PREFIX)
	install_name_tool -id @rpath/$(SHADERC_LIB_FILENAME) $(PREFIX)/lib/$(SHADERC_LIB_FILENAME)

# Note: somehow xcodebuild sets name @rpath/libMoltenVK.dylib automatically
# (according to otool -l) so we don't have to do anything special
MoltenVK-install: external-download $(PREFIX_DONE)
	cd external/MoltenVK && ./fetchDependencies -v --macos
	$(MAKE) -C external/MoltenVK macos
	install -d $(PREFIX)/include
	install -d $(PREFIX)/lib
	cp -v external/MoltenVK/Package/Latest/MoltenVK/dylib/macOS/libMoltenVK.dylib $(PREFIX)/lib
	cp -vr external/MoltenVK/Package/Latest/MoltenVK/include $(PREFIX)

$(PREFIX_DONE):
	(cd external && bash scripts/sync.sh $(TARGET_OS))
ifeq ($(TARGET_OS),Windows)
	($(PYTHON) -m venv $(PREFIX))
	(mkdir $(PREFIX)/Lib/pkgconfig)
	(cp external/$(TARGET_OS_LOWERCASE)/RenderDoc_1.11_64/renderdoc.dll $(PREFIX)/Scripts/.)
else ifeq ($(TARGET_OS),MinGW-w64)
	$(PYTHON) -m venv --system-site-packages  $(PREFIX)
else
	$(PYTHON) -m venv $(PREFIX)
endif
	touch $(PREFIX_DONE)

$(PREFIX): $(PREFIX_DONE)

tests: nodegl-tests tests-setup
	($(MESON) test $(MESON_TESTS_SUITE_OPTS) -C $(MESON_BUILDDIR)/tests)

tests-setup: ngl-tools-install pynodegl-utils-install
	$(MESON_SETUP) --backend ninja $(MESON_BUILDDIR)/tests tests

nodegl-tests: nodegl-install
	$(MESON_TEST) -C $(MESON_BUILDDIR)/libnodegl

compile-%:
	$(MESON_COMPILE) -C $(MESON_BUILDDIR)/$(subst compile-,,$@)

install-%: compile-%
	$(MESON_INSTALL) -C $(MESON_BUILDDIR)/$(subst install-,,$@)

nodegl-%: nodegl-setup
	$(MESON_COMPILE) -C $(MESON_BUILDDIR)/libnodegl $(subst nodegl-,,$@)

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
	(ninja -C $(MESON_BUILDDIR)/libnodegl coverage-html)
coverage-xml:
	(ninja -C $(MESON_BUILDDIR)/libnodegl coverage-xml)

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
.PHONY: external-download external-install
.PHONY: ngfx-install
.PHONY: ngl-debug-tools
