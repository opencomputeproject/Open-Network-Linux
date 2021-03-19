#ifndef __EEPROM_CONFIG
#define __EEPROM_CONFIG

#include "eeprom_config_nba820.h"

struct platform_eeprom_info_t {
    int platform_id;
    struct eeprom_i2c_tbl_t *tbl;
};

struct platform_eeprom_info_t platform_eeprom_info_tbl[] = {

    {.platform_id = PLATFORM_NBA820, .tbl = &nba820_eeprom_i2c_tbl},
    {.platform_id = PLATFORM_END },
};


#endif /*__EEPROM_CONFIG*/
