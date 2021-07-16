#ifndef __EEPROM_CONFIG
#define __EEPROM_CONFIG

#include "eeprom_config_banyan.h"
#include "eeprom_config_maple.h"
#include "eeprom_config_4U.h"
#include "eeprom_config_banyan8T_sku1.h"
#include "eeprom_config_banyan8T_sku2.h"
#include "eeprom_config_bocelli.h"

struct platform_eeprom_info_t {
    int platform_id;
    struct eeprom_i2c_tbl_t *tbl;
};

struct platform_eeprom_info_t platform_eeprom_info_tbl[] = {

    {.platform_id = PLATFORM_MAPLE, .tbl = &maple_eeprom_i2c_tbl},
    {.platform_id = PLATFORM_BANYAN, .tbl = &banyan_eeprom_i2c_tbl},
    {.platform_id = PLATFORM_4U, .tbl = &banyan_4U_eeprom_i2c_tbl},
    {.platform_id = PLATFORM_BANYAN_8T_SKU1, .tbl = &banyan8T_sku1_eeprom_i2c_tbl},
    {.platform_id = PLATFORM_BANYAN_8T_SKU2, .tbl = &banyan8T_sku2_eeprom_i2c_tbl},
    {.platform_id = PLATFORM_BOCELLI, .tbl = &bocelli_eeprom_i2c_tbl},
    {.platform_id = PLATFORM_END },
};


#endif /*__EEPROM_CONFIG*/
