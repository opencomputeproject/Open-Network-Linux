include $(ONL)/make/any.mk

MODULE := onlp-snmpd
include $(BUILDER)/standardinit.mk

DEPENDMODULES := onlp_snmp AIM OS snmp_subagent IOF onlplib cjson cjson_util
DEPENDMODULE_HEADERS := onlp

include $(BUILDER)/dependmodules.mk

BINARY := onlp-snmpd
$(BINARY)_LIBRARIES := $(LIBRARY_TARGETS)
include $(BUILDER)/bin.mk

include $(BUILDER)/targets.mk

GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MODULES_INIT=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MAIN=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_PVS_SYSLOG=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_DAEMONIZE=1
GLOBAL_CFLAGS += -DAIM_CONFIG_AIM_MAIN_FUNCTION=snmp_subagent_main
GLOBAL_CFLAGS += -DSNMP_SUBAGENT_CONFIG_INCLUDE_MAIN=1
GLOBAL_CFLAGS += -DSNMP_SUBAGENT_CONFIG_AGENT_NAME=\"onlp-snmpd\"
GLOBAL_CFLAGS += -DSNMP_SUBAGENT_CONFIG_AGENT_DESCRIPTION="\"ONLP SNMP Agent\""
GLOBAL_CFLAGS += -g

LIBONLP := $(shell $(ONLPM) --find-file onlp:$(ARCH) libonlp.so)

GLOBAL_LINK_LIBS += -lm -lpthread $(LIBONLP)
GLOBAL_LINK_LIBS += -Wl,--unresolved-symbols=ignore-in-shared-libs

.DEFAULT_GOAL := onlp-snmpd

