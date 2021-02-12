############################################################
# <bsn.cl fy=2015 v=onl>
#
#           Copyright 2015 Big Switch Networks, Inc.
#
# Licensed under the Eclipse Public License, Version 1.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#        http://www.eclipse.org/legal/epl-v10.html
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the
# License.
#
# </bsn.cl>
############################################################
#
# Prepare and build a kernel.
#
############################################################

ifndef ARCH
$(error $$ARCH must be set)
endif

#
# The kernel major version
#
ifndef K_MAJOR_VERSION
$(error $$K_MAJOR_VERSION must be set)
endif

#
# The kernel patchlevel.
#
ifndef K_PATCH_LEVEL
$(error $$K_PATCH_LEVEL must be set)
endif

#
# The kernel sublevel
#
ifndef K_SUB_LEVEL
$(error $$K_SUB_LEVEL must be set)
endif

#
# The directory containing the patches to be applied
# to the kernel sources.
#
ifndef K_PATCH_DIR
$(error $$K_PATCH_DIR must be set)
endif

ifndef K_PATCH_SERIES_FILE
    ifndef K_PATCH_SERIES
        K_PATCH_SERIES = series
    endif
    K_PATCH_SERIES_FILE = $(K_PATCH_DIR)/$(K_PATCH_SERIES)
endif

#
# This is the directory that will receive the build targets.
# The kernel build tree is placed in this directory,
# as well as any custom copy targets.
#
ifndef K_TARGET_DIR
$(error $$K_TARGET_DIR not set)
endif

#
# This is the absolute path to the kernel configuration
# that should be used for this build.
#
ifndef K_CONFIG
$(error $$K_CONFIG not set)
endif

#
# This is the build target. bzImage, uImage, etc.
#
ifndef K_BUILD_TARGET
$(error $$K_BUILD_TARGET not set)
endif

############################################################
############################################################
ONL_KERNELS := $(ONL)/packages/base/any/kernels

K_VERSION := $(K_MAJOR_VERSION).$(K_PATCH_LEVEL).$(K_SUB_LEVEL)$(K_SUFFIX)
ifndef K_NAME
K_NAME := linux-$(K_VERSION)
endif
ifndef K_ARCHIVE_EXT
K_ARCHIVE_EXT := tar.xz
endif
ifndef K_ARCHIVE_NAME
K_ARCHIVE_NAME := $(K_NAME).$(K_ARCHIVE_EXT)
endif
K_ARCHIVE_PATH := $(ONL_KERNELS)/archives/$(K_ARCHIVE_NAME)
ifndef K_ARCHIVE_URL
K_ARCHIVE_URL := https://www.kernel.org/pub/linux/kernel/v$(K_MAJOR_VERSION).x/$(K_ARCHIVE_NAME)
endif
K_SOURCE_DIR := $(K_TARGET_DIR)/$(K_NAME)
K_MBUILD_DIR := $(K_SOURCE_DIR)-mbuild
K_INSTALL_MOD_PATH := $(K_TARGET_DIR)
K_DTBS_DIR := $(K_SOURCE_DIR)-dtbs

#
# The kernel source archive. Download if not present.
#
$(K_ARCHIVE_PATH):
	cd $(ONL_KERNELS)/archives && wget $(K_ARCHIVE_URL)


.PHONY : ksource kpatched

#
# The extracted kernel sources
#
$(K_SOURCE_DIR)/Makefile: $(K_ARCHIVE_PATH)
	mkdir -p $(K_TARGET_DIR) && cd $(K_TARGET_DIR) && tar kxf $(K_ARCHIVE_PATH)
	touch -c $(K_SOURCE_DIR)/Makefile
	$(K_MAKE) mrproper

ksource: $(K_SOURCE_DIR)/Makefile

#
# The patched kernel sources
#
$(K_SOURCE_DIR)/.PATCHED: $(K_SOURCE_DIR)/Makefile
	$(ONL)/tools/scripts/apply-patches.sh $(K_SOURCE_DIR) $(K_PATCH_DIR) $(K_PATCH_SERIES_FILE)
	touch $(K_SOURCE_DIR)/.PATCHED

kpatched: $(K_SOURCE_DIR)/.PATCHED

#
# Setup the kernel and output directory for the build.
#
setup: $(K_SOURCE_DIR)/.PATCHED
	cp $(K_CONFIG) $(K_SOURCE_DIR)/.config

#
# Kernel build command.
#
K_MAKE    := $(MAKE) -C $(K_SOURCE_DIR)

#
# Build the kernel.
#
build: setup
	+$(K_MAKE) $(K_BUILD_TARGET)
	+$(K_MAKE) modules
	+$(K_MAKE) modules_install INSTALL_MOD_PATH=$(K_INSTALL_MOD_PATH)
	find $(K_INSTALL_MOD_PATH) -type l -name source -delete
	find $(K_INSTALL_MOD_PATH) -type l -name build -delete


ifdef K_COPY_SRC
ifdef K_COPY_DST
ifdef K_COPY_GZIP
	gzip -c $(K_SOURCE_DIR)/$(K_COPY_SRC) > $(K_TARGET_DIR)/$(K_COPY_DST)
else
	cp $(K_SOURCE_DIR)/$(K_COPY_SRC) $(K_TARGET_DIR)/$(K_COPY_DST)
endif
endif
endif


MODSYNCLIST_DEFAULT := .config System.map Module.symvers Makefile include scripts drivers \
			arch/x86/include arch/x86/Makefile \
			arch/powerpc/include arch/powerpc/Makefile arch/powerpc/lib arch/powerpc/boot/dts \
			arch/arm/include arch/arm/Makefile arch/arm/lib arch/arm/boot/dts arch/arm/kernel \
			arch/arm64/include arch/arm64/Makefile arch/arm64/lib arch/arm64/boot/dts arch/arm64/kernel

MODSYNCLIST := $(MODSYNCLIST_DEFAULT) $(MODSYNCLIST_EXTRA) $(K_MODSYNCLIST)

# This file must be preserved for PPC module builds.
MODSYNCKEEP := arch/powerpc/lib/crtsavres.o

mbuild: build
	rm -rf $(K_MBUILD_DIR)
	mkdir -p $(K_MBUILD_DIR)
	$(foreach f,$(MODSYNCLIST),$(ONL)/tools/scripts/tree-copy.sh $(K_SOURCE_DIR) $(f) $(K_MBUILD_DIR);)
	find $(K_MBUILD_DIR) -name "*.o*" -delete

# This breaks the 5.x kernel builds
ifeq ($(shell test $(K_MAJOR_VERSION) -lt 5 && echo 1),1)
	find $(K_MBUILD_DIR) -name "*.c" -delete
endif
	find $(K_MBUILD_DIR) -name "*.ko" -delete

# Remove symlink directory so that build does not fail
# Note this hack needs to be updated before Linux removes the old locations
	find $(K_MBUILD_DIR) -type d -name include-prefixes -exec rm -rf {} +

ifeq ($(ARCH), powerpc)
	$(foreach f,$(MODSYNCKEEP), cp $(K_SOURCE_DIR)/$(f) $(K_MBUILD_DIR)/$(f) || true;)
endif

dtbs: mbuild
ifdef DTS_LIST
	rm -rf $(K_DTBS_DIR)
	mkdir -p $(K_DTBS_DIR)
ifeq ($(ARCH),arm64)
	cp $(K_SOURCE_DIR)/arch/$(ARCH)/boot/dts/freescale/*.dtb $(K_DTBS_DIR)
else
	$(foreach name,$(DTS_LIST),$(K_SOURCE_DIR)/scripts/dtc/dtc -I dts -O dtb -o $(K_DTBS_DIR)/$(name).dtb $(K_SOURCE_DIR)/arch/$(ARCH)/boot/dts/$(name).dts; )
endif
endif

#
# This target can be used to manage the configuration file.
#
configure: setup
	$(K_MAKE) menuconfig
	cp $(K_SOURCE_DIR)/.config $(K_CONFIG)

.DEFAULT_GOAL := dtbs
