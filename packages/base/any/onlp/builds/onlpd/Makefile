############################################################
# <bsn.cl fy=2014 v=onl>
#
#           Copyright 2014 BigSwitch Networks, Inc.
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
#
#
############################################################
include $(ONL)/make/any.mk

MODULE := onlpd-module
include $(BUILDER)/standardinit.mk

DEPENDMODULES := $(DEPENDMODULES) AIM IOF sff cjson cjson_util timer_wheel OS uCli BigList ELS
DEPENDMODULE_HEADERS := $(DEPENDMODULE_HEADERS) AIM IOF sff cjson cjson_util timer_wheel OS uCli BigList ELS

include $(BUILDER)/dependmodules.mk

BINARY := onlpd
$(BINARY)_LIBRARIES := $(LIBRARY_TARGETS)
include $(BUILDER)/bin.mk

LIBONLP_SO := ../onlp/$(BUILD_DIR)/bin/libonlp.so

GLOBAL_CFLAGS += -DAIM_CONFIG_AIM_MAIN_FUNCTION=onlpdump_main
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MODULES_INIT=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MAIN=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_DAEMONIZE=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_PVS_SYSLOG=1
GLOBAL_CFLAGS += -DUCLI_CONFIG_INCLUDE_ELS_LOOP=1
GLOBAL_CFLAGS += -DONLP_CONFIG_INCLUDE_UCLI=1

GLOBAL_LINK_LIBS += $(LIBONLP_SO) -Wl,--unresolved-symbols=ignore-in-shared-libs
GLOBAL_LINK_LIBS += -lpthread -lm -lrt -ledit

include $(BUILDER)/targets.mk

