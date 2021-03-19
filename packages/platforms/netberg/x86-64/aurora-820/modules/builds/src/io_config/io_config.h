#ifndef __IO_CONFIG
#define __IO_CONFIG

#include "io_config_nba820.h"

struct platform_io_info_t platform_io_info_tbl[] = {
    {
        .platform_id = PLATFORM_NBA820,
        .i2c_ch_map = &nba820_ioexp_i2c_ch_map,
        .config_map = &nba820_ioexp_config_map,
        .input_map = &nba820_ioexp_input_map,
        .input_port_num = nba820_ioexp_input_port_num,
        .output = nba820_ioexp_output_tbl,
        .cpld_config = &nba820_cpld_config,
        .mux_rst_gpio = NBA820_MUX_RST_GPIO,
    },
    {
        .platform_id = PLATFORM_END,
    },

};
#endif /*__IO_CONFIG*/
