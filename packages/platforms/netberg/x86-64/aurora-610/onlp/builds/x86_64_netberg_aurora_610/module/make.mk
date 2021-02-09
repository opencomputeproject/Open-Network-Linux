###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_netberg_aurora_610_INCLUDES := -I $(THIS_DIR)inc
x86_64_netberg_aurora_610_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_netberg_aurora_610_DEPENDMODULE_ENTRIES := init:x86_64_netberg_aurora_610 ucli:x86_64_netberg_aurora_610

x86_64_netberg_aurora_610_CFLAGS := -Wno-restrict -Wno-format-truncation

