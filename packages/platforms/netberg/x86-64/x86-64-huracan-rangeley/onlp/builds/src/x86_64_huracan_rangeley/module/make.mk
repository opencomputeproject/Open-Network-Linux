###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_huracan_rangeley_INCLUDES := -I $(THIS_DIR)inc
x86_64_huracan_rangeley_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_huracan_rangeley_DEPENDMODULE_ENTRIES := init:x86_64_huracan_rangeley ucli:x86_64_huracan_rangeley

