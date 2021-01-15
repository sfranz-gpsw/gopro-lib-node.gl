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
export TARGET_OS  ?= $(shell uname -s)

ifeq ($(TARGET_OS),Windows)
PYTHON     ?= python.exe
PREFIX     ?= nodegl-env
W_PWD = $(shell wslpath -wa .)
W_PREFIX ?= $(W_PWD)\$(PREFIX)
$(info PYTHON: $(PYTHON))
$(info PREFIX: $(PREFIX))
$(info W_PREFIX: $(W_PREFIX))
else
PYTHON     ?= python$(if $(shell which python$(PYTHON_MAJOR) 2> /dev/null),$(PYTHON_MAJOR),)
PREFIX     ?= $(PWD)/nodegl-env
endif

DEBUG_GL    ?= no
DEBUG_MEM   ?= no
DEBUG_SCENE ?= no
TESTS_SUITE ?=
V           ?=
NODEGL_DEFAULT_LIBRARY ?= shared

ifeq ($(TARGET_OS), Windows)
# Initialize VCVARS64 to a default value
# Note: the user should override this environment variable to point to the correct path containing vcvars64.bat
VCVARS64 ?= "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
ACTIVATE = $(VCVARS64) \&\& $(PREFIX)\\Scripts\\activate.bat
# Initialize VCPKG_DIR to a default value
# Note: the user should override this environment variable to point to the correct path containing vcpkg
VCPKG_DIR ?= C:\\vcpkg
PKG_CONF_DIR = external\\pkgconf\\build
# Set PKG_CONFIG and PKG_CONFIG_PATH environment variables when invoking command shell
CMD = PKG_CONFIG="$(PREFIX)\\Scripts\\pkg-config.exe" PKG_CONFIG_PATH="$(W_PREFIX)\\Lib\\pkgconfig" WSLENV=PKG_CONFIG/w:PKG_CONFIG_PATH/w cmd.exe /C
else
ACTIVATE = . $(PREFIX)/bin/activate
CMD =
endif

ifneq ($(shell $(CMD) $(PYTHON) -c "import sys;print(sys.version_info.major)"),$(PYTHON_MAJOR))
$(error "Python $(PYTHON_MAJOR) not found")
endif

RPATH_LDFLAGS ?= -Wl,-rpath,$(PREFIX)/lib

ifeq ($(TARGET_OS),Windows)
MESON_SETUP   = meson setup --backend vs \
  --prefix="$(W_PREFIX)" --bindir="$(W_PREFIX)\\Scripts" --includedir="$(W_PREFIX)\\Include" \
  --libdir="$(W_PREFIX)\\Lib" --pkg-config-path=$(PREFIX)\\Lib\\pkgconfig -Drpath=true
MESON_SETUP_NINJA   = meson setup --backend ninja \
  --prefix="$(W_PREFIX)" --bindir="$(W_PREFIX)\\Scripts" --includedir="$(W_PREFIX)\\Include" \
  --libdir="$(W_PREFIX)\\Lib" --pkg-config-path=$(PREFIX)\\Lib\\pkgconfig -Drpath=true
else
MESON_SETUP   = meson setup --prefix=$(PREFIX) --pkg-config-path=$(PREFIX)/lib/pkgconfig -Drpath=true
endif
# MAKEFLAGS= is a workaround for the issue described here:
# https://github.com/ninja-build/ninja/issues/1139#issuecomment-724061270
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
MESON_SETUP += --buildtype=debugoptimized
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
	($(CMD) $(ACTIVATE) \&\& $(MESON_SETUP) ngl-tools builddir\\ngl-tools \&\& $(MESON_COMPILE) -C builddir\\ngl-tools \&\& $(MESON_INSTALL) -C builddir\\ngl-tools)
else
	($(CMD) $(ACTIVATE) && $(MESON_SETUP) ngl-tools builddir/ngl-tools && $(MESON_COMPILE) -C builddir/ngl-tools && $(MESON_INSTALL) -C builddir/ngl-tools)
endif

pynodegl-utils-install: pynodegl-utils-deps-install
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& pip -v install -e pynodegl-utils)
else
	($(CMD) $(ACTIVATE) && pip -v install -e ./pynodegl-utils)
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
	($(CMD) $(ACTIVATE) && pip install -r ./pynodegl-utils/requirements.txt)
endif

pynodegl-install: pynodegl-deps-install
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& pip -v install -e .\\pynodegl)
	# Install DLLs / Libs
	$(CMD) xcopy /Y builddir\\sxplayer\\*.dll pynodegl\\.
	$(CMD) xcopy /Y /C builddir\\libnodegl\\*.dll pynodegl\\.
	$(CMD) xcopy /Y builddir\\sxplayer\\*.dll $(PREFIX)\\Scripts\\.
	$(CMD) xcopy /Y /C builddir\\libnodegl\\*.dll $(PREFIX)\\Scripts\\.
	$(CMD) xcopy /Y /C builddir\\libnodegl\\*.lib $(PREFIX)\\Lib\\.
else
	($(CMD) $(ACTIVATE) && PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig LDFLAGS=$(RPATH_LDFLAGS) pip -v install -e ./pynodegl)
endif

pynodegl-deps-install: $(PREFIX) nodegl-install
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& pip install -r pynodegl\\requirements.txt)
else
	($(CMD) $(ACTIVATE) && pip install -r ./pynodegl/requirements.txt)
endif

nodegl-install: nodegl-setup
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& $(MESON_COMPILE) -C builddir\\libnodegl \&\& $(MESON_INSTALL) -C builddir\\libnodegl)
else
	($(CMD) $(ACTIVATE) && $(MESON_COMPILE) -C builddir/libnodegl && $(MESON_INSTALL) -C builddir/libnodegl)
endif

nodegl-setup: sxplayer-install
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& $(MESON_SETUP) $(NODEGL_DEBUG_OPTS) --default-library $(NODEGL_DEFAULT_LIBRARY) libnodegl builddir\\libnodegl)
else
	($(CMD) $(ACTIVATE) && $(MESON_SETUP) $(NODEGL_DEBUG_OPTS) libnodegl builddir/libnodegl)
endif

pkg-config-install: external-download $(PREFIX)
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& $(MESON_SETUP) --buildtype=debug -Dtests=false external\\pkgconf builddir\\pkgconf \&\& $(MESON_COMPILE) -C builddir\\pkgconf \&\& $(MESON_INSTALL) -C builddir\\pkgconf)
	($(CMD) copy $(PREFIX)\\Scripts\\pkgconf.exe $(PREFIX)\\Scripts\\pkg-config.exe)
endif

sxplayer-install: external-download pkg-config-install $(PREFIX)
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& $(MESON_SETUP) --default-library shared external\\sxplayer builddir\\sxplayer \&\& $(MESON_COMPILE) -C builddir\\sxplayer \&\& $(MESON_INSTALL) -C builddir\\sxplayer)
else
	($(CMD) $(ACTIVATE) && $(MESON_SETUP) --default-library shared external/sxplayer builddir/sxplayer && $(MESON_COMPILE) -C builddir/sxplayer && $(MESON_INSTALL) -C builddir/sxplayer)
endif

external-download:
	$(MAKE) -C external

$(PREFIX):
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(PYTHON) -m venv $(PREFIX))
	($(CMD) copy $(VCPKG_DIR)\\packages\\ffmpeg_x64-windows\\tools\\ffmpeg\\*.exe $(PREFIX)\\Scripts\\.)
	($(CMD) copy $(VCPKG_DIR)\\packages\\ffmpeg_x64-windows\\bin\\*.dll pynodegl\\.)
	($(CMD) copy $(VCPKG_DIR)\\packages\\pthreads_x64-windows\\bin\\*.dll pynodegl\\.)
	($(CMD) copy $(VCPKG_DIR)\\packages\\ffmpeg_x64-windows\\bin\\*.dll $(PREFIX)\\Scripts\\.)
	($(CMD) copy $(VCPKG_DIR)\\packages\\pthreads_x64-windows\\bin\\*.dll $(PREFIX)\\Scripts\\.)
	($(CMD) copy $(VCPKG_DIR)\\packages\\pthreads_x64-windows\\lib\\*.lib $(PREFIX)\\Lib\\.)
	($(CMD) mkdir $(PREFIX)\\Lib\\pkgconfig)
	# copy and patch ffmpeg pkg-config files
	($(CMD) copy ${VCPKG_DIR}\\packages\\ffmpeg_x64-windows\\lib\\pkgconfig\\*.pc $(PREFIX)\\Lib\\pkgconfig\\.)
	ffmpeg_pc_files="libavcodec.pc libavdevice.pc libavfilter.pc libavformat.pc libavresample.pc libavutil.pc libswresample.pc libswscale.pc" ;\
	for pc_file in $$ffmpeg_pc_files ;\
		do \
		sed -i -e 's/\/cygdrive\/c/C:/g' $(PREFIX)/Lib/pkgconfig/$$pc_file ;\
		sed -i -e 's/\/cygdrive\/d/D:/g' $(PREFIX)/Lib/pkgconfig/$$pc_file ;\
		sed -i -e "s/prefix=[^\n]*/prefix=$${VCPKG_DIR}\\\\packages\\\\ffmpeg_x64-windows/" nodegl-env/Lib/pkgconfig/$$pc_file ;\
	done;
	# copy and patch SDL2 pkg-config files
	($(CMD) copy ${VCPKG_DIR}\\packages\\sdl2_x64-windows\\lib\\pkgconfig\\*.pc $(PREFIX)\\Lib\\pkgconfig\\.)
	(sed -i -e 's/prefix=.*/prefix=$(VCPKG_DIR)\/packages\/sdl2_x64-windows/' $(PREFIX)/Lib/pkgconfig/sdl2.pc)
	(sed -i -e 's/Libs: .*/Libs: -L\${libdir} -lSDL2 -L\${libdir}/manual-link -lSDL2main/' $(PREFIX)/Lib/pkgconfig/sdl2.pc)
	(sed -i -e 's/\\/\//g' $(PREFIX)/Lib/pkgconfig/sdl2.pc)
	($(CMD) $(ACTIVATE) \&\& pip install meson ninja)
else ifeq ($(TARGET_OS),MinGW-w64)
	#
	# We do not pull meson from pip on mingw for the same reasons we don't pull
	# Pillow and PySide2. We require the users to have it on their system.
	#
	$(PYTHON) -m venv --system-site-packages $(PREFIX)
else
	$(PYTHON) -m venv $(PREFIX)
	($(CMD) $(ACTIVATE) && pip install meson ninja)
endif

tests: nodegl-tests tests-setup
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& meson test $(MESON_TESTS_SUITE_OPTS) -C builddir\\tests)
else
	($(CMD) $(ACTIVATE) && meson test $(MESON_TESTS_SUITE_OPTS) -C builddir/tests)
endif

tests-setup: ngl-tools-install pynodegl-utils-install
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& $(MESON_SETUP_NINJA) builddir\\tests tests)
else
	($(CMD) $(ACTIVATE) && $(MESON_SETUP) builddir/tests tests)
endif

nodegl-tests: nodegl-install
ifeq ($(TARGET_OS),Windows)
	($(CMD) $(ACTIVATE) \&\& meson test -C builddir\\libnodegl)
else
	($(CMD) $(ACTIVATE) && meson test -C builddir/libnodegl)
endif

nodegl-%: nodegl-setup
	($(CMD) $(ACTIVATE) && $(MESON_COMPILE) -C builddir/libnodegl $(subst nodegl-,,$@))

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
	$(RM) -r builddir/sxplayer
	$(RM) -r builddir/libnodegl
	$(RM) -r builddir/ngl-tools
	$(RM) -r builddir/tests

# You need to build and run with COVERAGE set to generate data.
# For example: `make clean && make -j8 tests COVERAGE=yes`
# We don't use `meson coverage` here because of
# https://github.com/mesonbuild/meson/issues/7895
coverage-html:
	($(CMD) $(ACTIVATE) && ninja -C builddir/libnodegl coverage-html)
coverage-xml:
	($(CMD) $(ACTIVATE) && ninja -C builddir/libnodegl coverage-xml)

.PHONY: all
.PHONY: ngl-tools-install
.PHONY: pynodegl-utils-install pynodegl-utils-deps-install
.PHONY: pynodegl-install pynodegl-deps-install
.PHONY: nodegl-install nodegl-setup
.PHONY: sxplayer-install
.PHONY: tests tests-setup
.PHONY: clean clean_py
.PHONY: coverage-html coverage-xml
.PHONY: external-download
