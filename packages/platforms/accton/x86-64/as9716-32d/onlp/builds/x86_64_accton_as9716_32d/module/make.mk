###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_accton_as9716_32d_INCLUDES := -I $(THIS_DIR)inc
x86_64_accton_as9716_32d_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_accton_as9716_32d_DEPENDMODULE_ENTRIES := init:x86_64_accton_as9716_32d ucli:x86_64_accton_as9716_32d

