###############################################################################
#
#
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_inventec_d6356j_INCLUDES := -I $(THIS_DIR)inc
x86_64_inventec_d6356j_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_inventec_d6356j_DEPENDMODULE_ENTRIES := init:x86_64_inventec_d6356j ucli:x86_64_inventec_d6356j

# This error should be addressed in the code.
x86_64_inventec_d6356_BROKEN_CFLAGS += -Wno-stringop-truncation
