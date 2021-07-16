#ifndef __PORT_INFO_CONFIG_BOCELLI
#define __PORT_INFO_CONFIG_BOCELLI

struct port_info_map_t bocelli_port_info_map[] = {
    {0, QSFP_DD_TYPE, "port1"},
    {1, QSFP_DD_TYPE, "port2"},
    {2, QSFP_DD_TYPE, "port3"},
    {3, QSFP_DD_TYPE, "port4"},
    {4, QSFP_DD_TYPE, "port5"},
    {5, QSFP_DD_TYPE, "port6"},
    {6, QSFP_DD_TYPE, "port7"},
    {7, QSFP_DD_TYPE, "port8"},
    {8, QSFP_DD_TYPE, "port9"},
    {9, QSFP_DD_TYPE, "port10"},
    {10, QSFP_DD_TYPE, "port11"},
    {11, QSFP_DD_TYPE, "port12"},
    {12, SFP_TYPE, "port13"},
    {13, SFP_TYPE, "port14"},
};

struct port_info_table_t bocelli_port_info_table = {
    .map =  bocelli_port_info_map,
    .size = ARRAY_SIZE(bocelli_port_info_map),
};

#endif /*__PORT_INFO_CONFIG_BOCELLI*/
