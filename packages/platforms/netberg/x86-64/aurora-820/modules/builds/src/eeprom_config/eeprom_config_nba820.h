//port info

//i2c_eeprom_config_cypress
#ifndef __I2C_EEPROM_CONFIG_NBA820
#define __I2C_EEPROM_CONFIG_NBA820

struct eeprom_config_t nba820_eeprom_config_map[] = {
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
    {14, 28},
    {15, 29},

    {16, 30},
    {17, 31},
    {18, 32},
    {19, 33},
    {20, 34},
    {21, 35},
    {22, 36},
    {23, 37},

    {24, 38},
    {25, 39},
    {26, 40},
    {27, 41},
    {28, 42},
    {29, 43},
    {30, 44},
    {31, 45},

};
struct eeprom_i2c_tbl_t nba820_eeprom_i2c_tbl = {
    .map = nba820_eeprom_config_map,
    .size = ARRAY_SIZE(nba820_eeprom_config_map)
};
#endif /*__I2C_EEPROM_CONFIG_NBA820*/
