###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_accton_as5915_18x_INCLUDES := -I $(THIS_DIR)inc
x86_64_accton_as5915_18x_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_accton_as5915_18x_DEPENDMODULE_ENTRIES := init:x86_64_accton_as5915_18x ucli:x86_64_accton_as5915_18x

