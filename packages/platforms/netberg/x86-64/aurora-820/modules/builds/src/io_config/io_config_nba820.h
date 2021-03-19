#ifndef __IO_CONFIG_NBA820_H
#define __IO_CONFIG_NBS820_H

#define CPLD_IOEXP_INTR_N (12) /*GPIO 12*/
#define NBA820_MUX_RST_GPIO (69)
#define NBA820_IO_PORT_NUM (32)
/*io expander layout*/
int nba820_ioexp_i2c_ch[] = {
	6,
	7,
    8,
    9,
};

struct i2c_ch_map_t nba820_ioexp_i2c_ch_map = {
	.tbl = nba820_ioexp_i2c_ch,
	.size = ARRAY_SIZE(nba820_ioexp_i2c_ch),
};
/*
typedef enum {
    IOEXP_PRS_TYPE,
    IOEXP_TXFAULT_TYPE,
    IOEXP_RXLOS_TYPE,
    IOEXP_INT_TYPE,
    IOEXP_TXFAULT2_TYPE,
    IOEXP_RXLOS2_TYPE,
    IOEXP_INPUT_TYPE_NUM,
} ioexp_input_type_t;
*/

int nba820_ioexp_input_port_num[IOEXP_INPUT_TYPE_NUM] = {

    [IOEXP_PRS_TYPE] = 32,
    [IOEXP_TXFAULT_TYPE] = 0,
    [IOEXP_RXLOS_TYPE] = 0,
    [IOEXP_INT_TYPE] = 32,
    [IOEXP_TXFAULT2_TYPE] = 0,
    [IOEXP_RXLOS2_TYPE] = 0,
};

struct ioexp_input_t nba820_ioexp_input_tbl[] = {
	{
     .ch_id = 0,
     .ioexp_id = 2,
	 .addr = 0x22,
 	 .layout = {
				{
				 .type = IOEXP_INT_TYPE,
				 .port_min = 0,	 
				 .bit_min = 0,	 
				 .bit_max = 7,	 
				 },
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_min = 0,	 
				 .bit_min = 8,	 
				 .bit_max = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{
     .ch_id = 1,
     .ioexp_id = 5,
	 .addr = 0x22,
 	 .layout = {
				{
				 .type = IOEXP_INT_TYPE,
				 .port_min = 8,	 
				 .bit_min = 0,	 
				 .bit_max = 7,	 
				 },
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_min = 8,	 
				 .bit_min = 8,	 
				 .bit_max = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{
     .ch_id = 2,
     .ioexp_id = 8,
	 .addr = 0x22,
 	 .layout = {
				{
				 .type = IOEXP_INT_TYPE,
				 .port_min = 16,	 
				 .bit_min = 0,	 
				 .bit_max = 7,	 
				 },
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_min = 16,	 
				 .bit_min = 8,	 
				 .bit_max = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{
     .ch_id = 3,
     .ioexp_id = 11,
	 .addr = 0x22,
 	 .layout = {
				{
				 .type = IOEXP_INT_TYPE,
				 .port_min = 24,	 
				 .bit_min = 0,	 
				 .bit_max = 7,	 
				 },
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_min = 24,	 
				 .bit_min = 8,	 
				 .bit_max = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
};

struct ioexp_input_map_t nba820_ioexp_input_map = {
	.tbl =  nba820_ioexp_input_tbl,
	.size = ARRAY_SIZE(nba820_ioexp_input_tbl),
};

struct ioexp_output_t nba820_ioexp_modesel[] = {
    {.ioexp_id = 0, .ch_id = 0,  .addr = 0x20, .port_min = 0, .bit_min = 0, .bit_max = 7},
    {.ioexp_id = 3, .ch_id = 1,  .addr = 0x20, .port_min = 8, .bit_min = 0, .bit_max = 7},
    {.ioexp_id = 6, .ch_id = 2,  .addr = 0x20, .port_min = 16, .bit_min = 0, .bit_max = 7},
    {.ioexp_id = 9, .ch_id = 3,  .addr = 0x20, .port_min = 24, .bit_min = 0, .bit_max = 7},
    {.end_of_tbl = true},
};

struct ioexp_output_t nba820_ioexp_reset[] = {
    {.ioexp_id = 0, .ch_id = 0,  .addr = 0x20, .port_min = 0, .bit_min = 8, .bit_max = 15},
    {.ioexp_id = 3, .ch_id = 1,  .addr = 0x20, .port_min = 8, .bit_min = 8, .bit_max = 15},
    {.ioexp_id = 6, .ch_id = 2,  .addr = 0x20, .port_min = 16, .bit_min = 8, .bit_max = 15},
    {.ioexp_id = 9, .ch_id = 3,  .addr = 0x20, .port_min = 24, .bit_min = 8, .bit_max = 15},
    {.end_of_tbl = true},
};


struct ioexp_output_t nba820_ioexp_lpmode[] = {
    {.ioexp_id = 1, .ch_id = 0, .addr = 0x21, .port_min = 0, .bit_min = 0, .bit_max = 7},
    {.ioexp_id = 4, .ch_id = 1, .addr = 0x21, .port_min = 8, .bit_min = 0, .bit_max = 7},
    {.ioexp_id = 7, .ch_id = 2, .addr = 0x21, .port_min = 16, .bit_min = 0, .bit_max = 7},
    {.ioexp_id = 10, .ch_id = 3, .addr = 0x21, .port_min = 24, .bit_min = 0, .bit_max = 7},
    {.end_of_tbl = true},
};


/*[note]
typedef enum {
    IOEXP_LPMODE_TYPE,
    IOEXP_RESET_TYPE,
    IOEXP_TXDISABLE_TYPE,
    IOEXP_MODESEL_TYPE,
    IOEXP_TXDISABLE2_TYPE,
    IOEXP_OUTPUT_TYPE_NUM,
} ioexp_output_type_t;
*/
struct ioexp_output_t *nba820_ioexp_output_tbl[IOEXP_OUTPUT_TYPE_NUM] = {

    [IOEXP_LPMODE_TYPE] = nba820_ioexp_lpmode,
    [IOEXP_RESET_TYPE] = nba820_ioexp_reset,
    [IOEXP_TXDISABLE_TYPE] = NULL,
    [IOEXP_MODESEL_TYPE] = nba820_ioexp_modesel, 
    [IOEXP_TXDISABLE2_TYPE] = NULL,

};

struct ioexp_config_t nba820_ioexp_config[] = {
    {.ch_id = 0, .addr = 0x20, .val = 0x0000, .type = PCA9555_TYPE},
    {.ch_id = 0, .addr = 0x21, .val = 0xff00, .type = PCA9555_TYPE},
    {.ch_id = 0, .addr = 0x22, .val = 0xffff, .type = PCA9555_TYPE},
    {.ch_id = 1, .addr = 0x20, .val = 0x0000, .type = PCA9555_TYPE},
    {.ch_id = 1, .addr = 0x21, .val = 0xff00, .type = PCA9555_TYPE},
    {.ch_id = 1, .addr = 0x22, .val = 0xffff, .type = PCA9555_TYPE},
    {.ch_id = 2, .addr = 0x20, .val = 0x0000, .type = PCA9555_TYPE},
    {.ch_id = 2, .addr = 0x21, .val = 0xff00, .type = PCA9555_TYPE},
    {.ch_id = 2, .addr = 0x22, .val = 0xffff, .type = PCA9555_TYPE},
    {.ch_id = 3, .addr = 0x20, .val = 0x0000, .type = PCA9555_TYPE},
    {.ch_id = 3, .addr = 0x21, .val = 0xff00, .type = PCA9555_TYPE},
    {.ch_id = 3, .addr = 0x22, .val = 0xffff, .type = PCA9555_TYPE},
};
struct ioexp_config_map_t nba820_ioexp_config_map = {
	.tbl =  nba820_ioexp_config,
	.size = ARRAY_SIZE(nba820_ioexp_config),
};

int nba820_cpld_i2c_ch[] = {
	2,
};

struct i2c_ch_map_t nba820_cpld_i2c_ch_map = {
	.tbl = nba820_cpld_i2c_ch,
	.size = ARRAY_SIZE(nba820_cpld_i2c_ch),
};

struct cpld_config_t nba820_cpld_config = {
    .i2c_config = {
                    [CPLD_ID_1] = {.ch_id = 0, .addr = 0x77}, /*cpld_1*/
                    [CPLD_ID_2] = {.ch_id = 0, .addr = 0x33}  /*cpld_2*/
                  },
    .i2c_ch_map = &nba820_cpld_i2c_ch_map,
    .intr_st = {.cpld_id = CPLD_ID_1, .offset = 0x1d},
    .intr_en = {.cpld_id = CPLD_ID_1, .offset = 0x37},
    .sff_intr_gpio = CPLD_IOEXP_INTR_N,
};

#endif /*__IO_CONFIG_NBA820_H*/
