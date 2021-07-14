###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_alphanetworks_snj60d0_320f_INCLUDES := -I $(THIS_DIR)inc
x86_64_alphanetworks_snj60d0_320f_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_alphanetworks_snj60d0_320f_DEPENDMODULE_ENTRIES := init:x86_64_alphanetworks_snj60d0_320f ucli:x86_64_alphanetworks_snj60d0_320f

