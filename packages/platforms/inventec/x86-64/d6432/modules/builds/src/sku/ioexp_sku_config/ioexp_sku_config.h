#ifndef __IOEXP_SKU_CONFIG
#define __IOEXP_SKU_CONFIG

#include "ioexp_sku_config_banyan.h"
#include "ioexp_sku_config_maple.h"
#include "ioexp_sku_config_bocelli.h"

struct platform_io_info_t platform_io_info_tbl[] = {
    {
        .platform_id = PLATFORM_MAPLE,
        .i2c_ch_map = &maple_ioexp_i2c_ch_map,
        .config_map = &maple_ioexp_config_map,
        .input_map = &maple_ioexp_input_map,
        .aval_sff_input = maple_ioexp_aval_sff_input,
        .output = maple_ioexp_output_tbl,
        .cpld_config = &maple_cpld_config,
        .mux_rst_gpio = MAPLE_MUX_RST_GPIO,
        .mux_ch_2_port_map = &maple_mux_ch_2_port_map,
        .mux_func_supported = true    
    },
    {
        .platform_id = PLATFORM_BANYAN,
        .i2c_ch_map = &banyan_ioexp_i2c_ch_map,
        .config_map = &banyan_ioexp_config_map,
        .input_map = &banyan_ioexp_input_map,
        .aval_sff_input = banyan_ioexp_aval_sff_input,
        .output = banyan_ioexp_output_tbl,
        .cpld_config = &banyan_cpld_config,
        .mux_rst_gpio = BANYAN_MUX_RST_GPIO,
        .mux_func_supported = false
    },
    {
        .platform_id = PLATFORM_BOCELLI,
        .i2c_ch_map = &bocelli_ioexp_i2c_ch_map,
        .config_map = &bocelli_ioexp_config_map,
        .input_map = &bocelli_ioexp_input_map,
        .aval_sff_input = bocelli_ioexp_aval_sff_input,
        .output = bocelli_ioexp_output_tbl,
        .cpld_config = NULL,
        .mux_rst_gpio = BOCELLI_MUX_RST_GPIO,
    },
    {
        .platform_id = PLATFORM_END,
    },

};
#endif /*__IOEXP_SKU_CONFIG*/
