#ifndef __I2C_EEPROM_CONFIG_BOCELLI
#define __I2C_EEPROM_CONFIG_BOCELLI

struct eeprom_config_t bocelli_eeprom_config_map[] = {
    /*port , i2c_channel*/
    {0, 14},
    {1, 15},
    {2, 16},
    {3, 17},
    {4, 18},
    {5, 19},
    {6, 20},
    {7, 21},
    {8, 22},
    {9, 23},
    {10, 24},
    {11, 25},
    
    {12, 26},
    {13, 27},
};
struct eeprom_i2c_tbl_t bocelli_eeprom_i2c_tbl = {
    .map = bocelli_eeprom_config_map,
    .size = ARRAY_SIZE(bocelli_eeprom_config_map)
};
#endif /*__I2C_EEPROM_CONFIG_BOCELLI*/
