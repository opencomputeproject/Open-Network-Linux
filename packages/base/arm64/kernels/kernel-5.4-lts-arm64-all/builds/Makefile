KERNEL_BUILD_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
KERNEL_ARCH := arm64
KERNEL_LTS_VERSION := 5.4
KERNEL_CONFIG := arm64-all
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
include $(ONL)/packages/base/any/kernels/lts/builds/Makefile

setup::
	dpkg -l libelf-dev > /dev/null 2>&1 || sudo apt-get install libelf-dev
