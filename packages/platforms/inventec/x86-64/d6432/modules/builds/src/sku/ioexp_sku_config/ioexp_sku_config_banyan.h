#ifndef __IO_CONFIG_BANYAN_H
#define __IO_CONFIG_BANYAN_H

#define CPLD_IOEXP_INTR_N (12) /*GPIO 12*/
#define BANYAN_MUX_RST_GPIO (69)
#define BANYAN_IO_PORT_NUM (32)
/*io expander layout*/
int banyan_ioexp_i2c_ch[] = {
	6,
	7,
    8,
    9,
};

struct int_vector_t banyan_ioexp_i2c_ch_map = {
	.tbl = banyan_ioexp_i2c_ch,
	.size = ARRAY_SIZE(banyan_ioexp_i2c_ch),
};

int banyan_mux_ch_2_port[] = {
    [0] = 0,
    [1] = 1,
    [2] = 2,
    [3] = 3,
    [4] = 4,
    [5] = 5,
    [6] = 6,
    [7] = 7,
    [8] = 8,
    [9] = 9,
    [10] = 10,
    [11] = 11,
    [12] = 12,
    [13] = 13,
    [14] = 14,
    [15] = 15,
    [16] = 16,
    [17] = 17,
    [18] = 18,
    [19] = 19,
    [20] = 20,
    [21] = 21,
    [22] = 22,
    [23] = 23,
    [24] = 24,
    [25] = 25,
    [26] = 26,
    [27] = 27,
    [28] = 28,
    [29] = 29,
    [30] = 30,
    [31] = 31,
    [32] = 32
};

struct int_vector_t banyan_mux_ch_2_port_map = {
       .tbl = banyan_mux_ch_2_port,
       .size = ARRAY_SIZE(banyan_mux_ch_2_port),
};
/*
typedef enum {
    IOEXP_PRS_TYPE,
    IOEXP_TXFAULT_TYPE,
    IOEXP_RXLOS_TYPE,
    IOEXP_INTR_TYPE,
    IOEXP_TXFAULT2_TYPE,
    IOEXP_RXLOS2_TYPE,
    IOEXP_INPUT_TYPE_NUM,
} ioexp_input_type_t;
*/

unsigned long banyan_ioexp_aval_sff_input[IOEXP_INPUT_TYPE_NUM] = {

    [IOEXP_PRS_TYPE] = 0xffffffffL,
    [IOEXP_TXFAULT_TYPE] = 0,
    [IOEXP_RXLOS_TYPE] = 0,
    [IOEXP_INTR_TYPE] = 0xffffffffL,
    [IOEXP_TXFAULT2_TYPE] = 0,
    [IOEXP_RXLOS2_TYPE] = 0,
};

struct ioexp_input_t banyan_ioexp_input_tbl[] = {
	{
     .ch_id = 0,
     .ioexp_id = 2,
	 .addr = 0x22,
 	 .layout = {
				{
				 .type = IOEXP_INTR_TYPE,
				 .port_1st = 0,	 
				 .bit_1st = 0,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 0,	 
				 .bit_1st = 8,	 
				 .bit_last = 15,	 
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
				 .type = IOEXP_INTR_TYPE,
				 .port_1st = 8,	 
				 .bit_1st = 0,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 8,	 
				 .bit_1st = 8,	 
				 .bit_last = 15,	 
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
				 .type = IOEXP_INTR_TYPE,
				 .port_1st = 16,	 
				 .bit_1st = 0,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 16,	 
				 .bit_1st = 8,	 
				 .bit_last = 15,	 
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
				 .type = IOEXP_INTR_TYPE,
				 .port_1st = 24,	 
				 .bit_1st = 0,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 24,	 
				 .bit_1st = 8,	 
				 .bit_last = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
};

struct ioexp_input_map_t banyan_ioexp_input_map = {
	.tbl =  banyan_ioexp_input_tbl,
	.size = ARRAY_SIZE(banyan_ioexp_input_tbl),
};

struct ioexp_output_t banyan_ioexp_modsel[] = {
    {.ioexp_id = 0, .ch_id = 0,  .addr = 0x20, .port_1st = 0, .bit_1st = 0, .bit_last = 7},
    {.ioexp_id = 3, .ch_id = 1,  .addr = 0x20, .port_1st = 8, .bit_1st = 0, .bit_last = 7},
    {.ioexp_id = 6, .ch_id = 2,  .addr = 0x20, .port_1st = 16, .bit_1st = 0, .bit_last = 7},
    {.ioexp_id = 9, .ch_id = 3,  .addr = 0x20, .port_1st = 24, .bit_1st = 0, .bit_last = 7},
    {.end_of_tbl = true},
};

struct ioexp_output_t banyan_ioexp_reset[] = {
    {.ioexp_id = 0, .ch_id = 0,  .addr = 0x20, .port_1st = 0, .bit_1st = 8, .bit_last = 15},
    {.ioexp_id = 3, .ch_id = 1,  .addr = 0x20, .port_1st = 8, .bit_1st = 8, .bit_last = 15},
    {.ioexp_id = 6, .ch_id = 2,  .addr = 0x20, .port_1st = 16, .bit_1st = 8, .bit_last = 15},
    {.ioexp_id = 9, .ch_id = 3,  .addr = 0x20, .port_1st = 24, .bit_1st = 8, .bit_last = 15},
    {.end_of_tbl = true},
};


struct ioexp_output_t banyan_ioexp_lpmode[] = {
    {.ioexp_id = 1, .ch_id = 0, .addr = 0x21, .port_1st = 0, .bit_1st = 0, .bit_last = 7},
    {.ioexp_id = 4, .ch_id = 1, .addr = 0x21, .port_1st = 8, .bit_1st = 0, .bit_last = 7},
    {.ioexp_id = 7, .ch_id = 2, .addr = 0x21, .port_1st = 16, .bit_1st = 0, .bit_last = 7},
    {.ioexp_id = 10, .ch_id = 3, .addr = 0x21, .port_1st = 24, .bit_1st = 0, .bit_last = 7},
    {.end_of_tbl = true},
};


/*[note]
typedef enum {
    IOEXP_LPMODE_TYPE,
    IOEXP_RESET_TYPE,
    IOEXP_TXDISABLE_TYPE,
    IOEXP_MODSEL_TYPE,
    IOEXP_TXDISABLE2_TYPE,
    IOEXP_OUTPUT_TYPE_NUM,
} ioexp_output_type_t;
*/
struct ioexp_output_t *banyan_ioexp_output_tbl[IOEXP_OUTPUT_TYPE_NUM] = {

    [IOEXP_LPMODE_TYPE] = banyan_ioexp_lpmode,
    [IOEXP_RESET_TYPE] = banyan_ioexp_reset,
    [IOEXP_TXDISABLE_TYPE] = NULL,
    [IOEXP_MODSEL_TYPE] = banyan_ioexp_modsel, 
    [IOEXP_TXDISABLE2_TYPE] = NULL,

};

struct ioexp_config_t banyan_ioexp_config[] = {
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
struct ioexp_config_map_t banyan_ioexp_config_map = {
	.tbl =  banyan_ioexp_config,
	.size = ARRAY_SIZE(banyan_ioexp_config),
};

int banyan_cpld_i2c_ch[] = {
	2,
};

struct int_vector_t banyan_cpld_i2c_ch_map = {
	.tbl = banyan_cpld_i2c_ch,
	.size = ARRAY_SIZE(banyan_cpld_i2c_ch),
};

struct cpld_config_t banyan_cpld_config = {
    .i2c_config = {
                    [CPLD_ID_1] = {.ch_id = 0, .addr = 0x77}, /*cpld_1*/
                    [CPLD_ID_2] = {.ch_id = 0, .addr = 0x33}  /*cpld_2*/
                  },
    .i2c_ch_map = &banyan_cpld_i2c_ch_map,
    .intr_st = {.cpld_id = CPLD_ID_1, .offset = 0x1d},
    .intr_en = {.cpld_id = CPLD_ID_1, .offset = 0x37},
    .sff_intr_gpio = CPLD_IOEXP_INTR_N,
};

#endif /*__IO_CONFIG_BANYAN_H*/
