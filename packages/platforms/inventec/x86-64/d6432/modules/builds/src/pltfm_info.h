#ifndef __PLTFM_CONFIG
#define __PLTFM_CONFIG

#include "port_info/port_info.h"
#include "sff_eeprom/sff_eeprom.h"
#include "sku/sku_info.h"

struct platform_info_t {
    int id;
    char *name;
    struct port_info_table_t *tbl_1st;
    struct port_info_table_t *tbl_2nd;
    struct sff_io_driver_t *io_drv;
    struct sff_eeprom_driver_t *eeprom_drv;
    struct lc_func_t *lc_func;
    struct pltfm_func_t *pltfm_func;
    int lc_num;
    struct i2c_recovery_feature_t i2c_recovery_feature;
    bool sff_power_supported;
    bool fpga_i2c_supported;
};

struct platform_info_t platform_info_tbl[] = {

    {
        .id = PLATFORM_MAPLE,
        .name = "MAPLE",
        .tbl_1st = &maple_port_info_table,
        .tbl_2nd = NULL,
        .io_drv = &ioexp_sku_sff_io_drv,
        .eeprom_drv = &sff_eeprom_drv_func,
        .pltfm_func = &ioexp_sku_pltfm_func,
        .lc_func = NULL, 
        .lc_num = 1,
        .i2c_recovery_feature = { .en = true, .method = MUX_DRV_METHOD},
        .sff_power_supported = false,
        .fpga_i2c_supported = false
    },

    {
        .id = PLATFORM_BANYAN,
        .name = "BANYAN",
        .tbl_1st = &banyan_port_info_table,
        .tbl_2nd = NULL,
        .io_drv = &ioexp_sku_sff_io_drv,
        .eeprom_drv = &sff_eeprom_drv_func,
        .pltfm_func = &ioexp_sku_pltfm_func,
        .lc_func = NULL, 
        .lc_num = 1,
        .i2c_recovery_feature = { .en = true, .method = NATIVE_METHOD},
        .sff_power_supported = false,
        .fpga_i2c_supported = false
    },
    {
        .id = PLATFORM_4U,
        .name = "BANYAN_4U",
        .tbl_1st = &port_info_4U_table1,
        .tbl_2nd = &port_info_4U_table2,
        .io_drv = &banyan4u_sku_sff_io_drv,
        .eeprom_drv = &sff_eeprom_drv_func,
        .pltfm_func = &banyan4u_sku_pltfm_func,
        .lc_func = &banyan4u_sku_lc_func, 
        .lc_num = 4,
        .i2c_recovery_feature = { .en = true, .method = MUX_DRV_METHOD},
        .sff_power_supported = true,
        .fpga_i2c_supported = false
    },
    {
        .id = PLATFORM_BANYAN_8T_SKU1,
        .name = "BANYAN_8T_SKU1",
        .tbl_1st = &banyan8T_sku1_port_info_table,
        .tbl_2nd = NULL,
        .io_drv = &banyan8T_sku1_sff_io_drv,
        .eeprom_drv = &sff_eeprom_drv_func,
        .pltfm_func = &banyan8T_sku1_pltfm_func,
        .lc_func = NULL, 
        .lc_num = 1,
        .i2c_recovery_feature = { .en = false},
        .sff_power_supported = true,
        .fpga_i2c_supported = true
    },
    {
        .id = PLATFORM_BANYAN_8T_SKU2,
        .name = "BANYAN_8T_SKU2",
        .tbl_1st = &banyan8T_sku2_port_info_table,
        .tbl_2nd = NULL,
        .io_drv = &banyan8T_sku2_sff_io_drv,
        .eeprom_drv = &sff_eeprom_drv_func,
        .pltfm_func = &banyan8T_sku2_pltfm_func,
        .lc_func = NULL, 
        .lc_num = 1,
        .i2c_recovery_feature = { .en = true, .method = MUX_DRV_METHOD},
        .sff_power_supported = true,
        .fpga_i2c_supported = false

    },
    {
        .id = PLATFORM_BOCELLI,
        .name = "BOCELLI",
        .tbl_1st = &bocelli_port_info_table,
        .tbl_2nd = NULL,
        .io_drv = &ioexp_sku_sff_io_drv,
        .eeprom_drv = &sff_eeprom_drv_func,
        .pltfm_func = &ioexp_sku_pltfm_func,
        .lc_func = NULL, 
        .lc_num = 1,
        .i2c_recovery_feature = { .en = true, .method = NATIVE_METHOD},
        .sff_power_supported = false,
        .fpga_i2c_supported = false
    },

    {.id = PLATFORM_END, }, /*keep this at the end of table*/
};
#endif /*__PLTFM_CONFIG*/
