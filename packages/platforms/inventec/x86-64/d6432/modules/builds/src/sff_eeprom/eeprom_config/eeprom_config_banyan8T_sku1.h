#ifndef __I2C_EEPROM_CONFIG_BANYAN_8T_SKU1
#define __I2C_EEPROM_CONFIG_BANYAN_8T_SKU1

struct eeprom_config_t banyan8T_sku1_eeprom_config_map[] = {
    /*port , i2c_channel*/
    {0, 100},
    {1, 101},
    {2, 102},
    {3, 103},
    {4, 104},
    {5, 105},
    {6, 106},
    {7, 107},

    {8, 108},
    {9, 109},
    {10, 110},
    {11, 111},
    {12, 112},
    {13, 113},
    {14, 114},
    {15, 115},

    {16, 116},
    {17, 117},
    {18, 118},
    {19, 119},
    {20, 120},
    {21, 121},
    {22, 122},
    {23, 123},

    {24, 124},
    {25, 125},
    {26, 126},
    {27, 127},
    {28, 128},
    {29, 129},
    {30, 130},
    {31, 131},

};
struct eeprom_i2c_tbl_t banyan8T_sku1_eeprom_i2c_tbl = {
    .map = banyan8T_sku1_eeprom_config_map,
    .size = ARRAY_SIZE(banyan8T_sku1_eeprom_config_map)
};
#endif /*__I2C_EEPROM_CONFIG_BANYAN_8T_SKU1*/
