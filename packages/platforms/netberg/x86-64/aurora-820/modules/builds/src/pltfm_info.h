#ifndef __PLTFM_CONFIG
#define __PLTFM_CONFIG

#include "port_info/port_info_nba820.h"

struct platform_info_t {
    int id;
    char *name;
    struct port_info_table_t *tbl_1st;
    struct port_info_table_t *tbl_2nd;
    int lc_num;
};

struct platform_info_t platform_info_tbl[] = {

    {.id = PLATFORM_NBA820, 
     .name = "NBA820",
     .tbl_1st = &nba820_port_info_table, 
     .tbl_2nd = NULL, 
     .lc_num = 1},
    
    {.id = PLATFORM_END, }, /*keep this at the end of table*/
};
#endif /*__PLTFM_CONFIG*/
