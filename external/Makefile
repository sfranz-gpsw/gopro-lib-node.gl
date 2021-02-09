#
# Copyright 2020 GoPro Inc.
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

TAR  ?= tar
CURL ?= curl
UNZIP ?= unzip

MOLTENVK_VERSION ?= 1.1.1
SHADERC_VERSION  ?= 2020.3
SXPLAYER_COMMIT_ID ?= d2e76248e3e35f0e2f53fbccb5ee3b03e78a7498
PKGCONF_COMMIT_ID ?= dcf529b83d621ed09e99e41fc35fdffd068bd87a

DEPENDENCIES = sxplayer
ifeq ($(TARGET_OS),Windows)
DEPENDENCIES += pkgconf
endif

all: $(DEPENDENCIES)

MoltenVK: MoltenVK-$(MOLTENVK_VERSION)
	ln -snf $< $@

MoltenVK-$(MOLTENVK_VERSION): MoltenVK-$(MOLTENVK_VERSION).tar.gz
	$(TAR) xf $<

MoltenVK-$(MOLTENVK_VERSION).tar.gz:
	$(CURL) -L https://github.com/KhronosGroup/MoltenVK/archive/v$(MOLTENVK_VERSION).tar.gz -o $@

shaderc: shaderc-$(SHADERC_VERSION)
	ln -snf $< $@

shaderc-$(SHADERC_VERSION): shaderc-$(SHADERC_VERSION).tar.gz
	$(TAR) xf $<

shaderc-$(SHADERC_VERSION).tar.gz:
	$(CURL) -L https://github.com/google/shaderc/archive/v$(SHADERC_VERSION).tar.gz -o $@

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
sxplayer: sxplayer-$(SXPLAYER_COMMIT_ID)
	ln -snf $< $@

sxplayer-$(SXPLAYER_COMMIT_ID): sxplayer-$(SXPLAYER_COMMIT_ID).zip
	$(UNZIP) $<

sxplayer-$(SXPLAYER_COMMIT_ID).zip:
	$(CURL) -L https://github.com/jmoguillansky-gpsw/sxplayer/archive/$(SXPLAYER_COMMIT_ID).zip -o $@

pkgconf: pkgconf-$(PKGCONF_COMMIT_ID)
	ln -snf $< $@

pkgconf-$(PKGCONF_COMMIT_ID): pkgconf-$(PKGCONF_COMMIT_ID).zip
	$(UNZIP) $<

pkgconf-$(PKGCONF_COMMIT_ID).zip:
	$(CURL) -L https://codeload.github.com/pkgconf/pkgconf/zip/$(PKGCONF_COMMIT_ID) -o $@

.PHONY: all
