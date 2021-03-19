###############################################################################
#
#
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_netberg_aurora_820_INCLUDES := -I $(THIS_DIR)inc
x86_64_netberg_aurora_820_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_netberg_aurora_820_DEPENDMODULE_ENTRIES := init:x86_64_netberg_aurora_820 ucli:x86_64_netberg_aurora_820

x86_64_netberg_aurora_820_BROKEN_CFLAGS += -Wno-restrict -Wno-format-truncation
