###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_alphanetworks_spx70d0_082f_INCLUDES := -I $(THIS_DIR)inc
x86_64_alphanetworks_spx70d0_082f_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_alphanetworks_spx70d0_082f_DEPENDMODULE_ENTRIES := init:x86_64_alphanetworks_spx70d0_082f ucli:x86_64_alphanetworks_spx70d0_082f

