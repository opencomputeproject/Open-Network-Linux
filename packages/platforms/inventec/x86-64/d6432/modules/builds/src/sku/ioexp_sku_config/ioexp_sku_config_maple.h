#ifndef __IO_CONFIG_MAPLE_H
#define __IO_CONFIG_MAPLE_H

#define CPLD_IOEXP_INTR_N (12) /*GPIO 12*/
#define MAPLE_MUX_RST_GPIO (69)
#define MAPLE_IO_PORT_NUM (56)
/*io expander layout*/
int maple_ioexp_i2c_ch[] = {
	6,
	7,
    8,
    9,
    10,
    11,
    12,
};

struct int_vector_t maple_ioexp_i2c_ch_map = {
	.tbl = maple_ioexp_i2c_ch,
	.size = ARRAY_SIZE(maple_ioexp_i2c_ch),
};


int maple_mux_ch_2_port[] = {
    [0] = 48,
    [1] = 49,
    [2] = 50,
    [3] = 51,
    [4] = 52,
    [5] = 53,
    [6] = 54,
    [7] = 55,
    [8] = 0,
    [9] = 1,
    [10] = 2,
    [11] = 3,
    [12] = 4,
    [13] = 5,
    [14] = 6,
    [15] = 7,
    [16] = 8,
    [17] = 9,
    [18] = 10,
    [19] = 11,
    [20] = 12,
    [21] = 13,
    [22] = 14,
    [23] = 15,
    [24] = 16,
    [25] = 17,
    [26] = 18,
    [27] = 19,
    [28] = 20,
    [29] = 21,
    [30] = 22,
    [31] = 23,
    [32] = 24,
    [33] = 25,
    [34] = 26,
    [35] = 27,
    [36] = 28,
    [37] = 29,
    [38] = 30,
    [39] = 31,
    [40] = 32,
    [41] = 33,
    [42] = 34,
    [43] = 35,
    [44] = 36,
    [45] = 37,
    [46] = 38,
    [47] = 39,
    [48] = 40,
    [49] = 41,
    [50] = 42,
    [51] = 43,
    [52] = 44,
    [53] = 45,
    [54] = 46,
    [55] = 47
};
    
struct int_vector_t maple_mux_ch_2_port_map = {
	.tbl = maple_mux_ch_2_port,
	.size = ARRAY_SIZE(maple_mux_ch_2_port),
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

unsigned long maple_ioexp_aval_sff_input[IOEXP_INPUT_TYPE_NUM] = {

    [IOEXP_PRS_TYPE] = 0xffffffffffffffL,
    [IOEXP_TXFAULT_TYPE] = 0xffffffffffffL,
    [IOEXP_RXLOS_TYPE] = 0xffffffffffffL,
    [IOEXP_INTR_TYPE] = 0xff000000000000L,
    [IOEXP_TXFAULT2_TYPE] = 0,
    [IOEXP_RXLOS2_TYPE] = 0,
};

struct ioexp_input_t maple_ioexp_input_tbl[] = {
	{
     .ch_id = 0,
     .ioexp_id = 2,
	 .addr = 0x22,
 	 .layout = {
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 48,	 
				 .bit_1st = 8,	 
				 .bit_last = 15,	 
				 },
				{
				 .type = IOEXP_INTR_TYPE,
				 .port_1st = 48,	 
				 .bit_1st = 0,	 
				 .bit_last = 7,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{
     .ch_id = 1,
     .ioexp_id = 3,
	 .addr = 0x20,
 	 .layout = {
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 0,	 
				 .bit_1st = 4,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_TXFAULT_TYPE,
				 .port_1st = 0,	 
				 .bit_1st = 0,	 
				 .bit_last = 3,	 
				 },
				{
				 .type = IOEXP_RXLOS_TYPE,
				 .port_1st = 0,	 
				 .bit_1st = 12,	 
				 .bit_last = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{.ch_id = 1,
     .ioexp_id = 4,
	 .addr = 0x21,
 	 .layout = {
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 4,	 
				 .bit_1st = 4,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_TXFAULT_TYPE,
				 .port_1st = 4,	 
				 .bit_1st = 0,	 
				 .bit_last = 3,	 
				 },
				{
				 .type = IOEXP_RXLOS_TYPE,
				 .port_1st = 4,	 
				 .bit_1st = 12,	 
				 .bit_last = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{.ch_id = 2,
     .ioexp_id = 6,
	 .addr = 0x20,
 	 .layout = {
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 8,	 
				 .bit_1st = 4,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_TXFAULT_TYPE,
				 .port_1st = 8,	 
				 .bit_1st = 0,	 
				 .bit_last = 3,	 
				 },
				{
				 .type = IOEXP_RXLOS_TYPE,
				 .port_1st = 8,	 
				 .bit_1st = 12,	 
				 .bit_last = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{.ch_id = 2,
     .ioexp_id = 7,
	 .addr = 0x21,
 	 .layout = {
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 12,	 
				 .bit_1st = 4,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_TXFAULT_TYPE,
				 .port_1st = 12,	 
				 .bit_1st = 0,	 
				 .bit_last = 3,	 
				 },
				{
				 .type = IOEXP_RXLOS_TYPE,
				 .port_1st = 12,	 
				 .bit_1st = 12,	 
				 .bit_last = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{.ch_id = 3,
     .ioexp_id = 9,
	 .addr = 0x20,
 	 .layout = {
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 16,	 
				 .bit_1st = 4,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_TXFAULT_TYPE,
				 .port_1st = 16,	 
				 .bit_1st = 0,	 
				 .bit_last = 3,	 
				 },
				{
				 .type = IOEXP_RXLOS_TYPE,
				 .port_1st = 16,	 
				 .bit_1st = 12,	 
				 .bit_last = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{.ch_id = 3,
     .ioexp_id = 10,
	 .addr = 0x21,
 	 .layout = {
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 20,	 
				 .bit_1st = 4,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_TXFAULT_TYPE,
				 .port_1st = 20,	 
				 .bit_1st = 0,	 
				 .bit_last = 3,	 
				 },
				{
				 .type = IOEXP_RXLOS_TYPE,
				 .port_1st = 20,	 
				 .bit_1st = 12,	 
				 .bit_last = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{.ch_id = 4,
     .ioexp_id = 12,
	 .addr = 0x20,
 	 .layout = {
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 24,	 
				 .bit_1st = 4,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_TXFAULT_TYPE,
				 .port_1st = 24,	 
				 .bit_1st = 0,	 
				 .bit_last = 3,	 
				 },
				{
				 .type = IOEXP_RXLOS_TYPE,
				 .port_1st = 24,	 
				 .bit_1st = 12,	 
				 .bit_last = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{.ch_id = 4,
     .ioexp_id = 13,
	 .addr = 0x21,
 	 .layout = {
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 28,	 
				 .bit_1st = 4,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_TXFAULT_TYPE,
				 .port_1st = 28,	 
				 .bit_1st = 0,	 
				 .bit_last = 3,	 
				 },
				{
				 .type = IOEXP_RXLOS_TYPE,
				 .port_1st = 28,	 
				 .bit_1st = 12,	 
				 .bit_last = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{.ch_id = 5,
     .ioexp_id = 15,
	 .addr = 0x20,
 	 .layout = {
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 32,	 
				 .bit_1st = 4,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_TXFAULT_TYPE,
				 .port_1st = 32,	 
				 .bit_1st = 0,	 
				 .bit_last = 3,	 
				 },
				{
				 .type = IOEXP_RXLOS_TYPE,
				 .port_1st = 32,	 
				 .bit_1st = 12,	 
				 .bit_last = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{.ch_id = 5,
     .ioexp_id = 16,
	 .addr = 0x21,
 	 .layout = {
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 36,	 
				 .bit_1st = 4,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_TXFAULT_TYPE,
				 .port_1st = 36,	 
				 .bit_1st = 0,	 
				 .bit_last = 3,	 
				 },
				{
				 .type = IOEXP_RXLOS_TYPE,
				 .port_1st = 36,	 
				 .bit_1st = 12,	 
				 .bit_last = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{.ch_id = 6,
     .ioexp_id = 18,
	 .addr = 0x20,
 	 .layout = {
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 40,	 
				 .bit_1st = 4,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_TXFAULT_TYPE,
				 .port_1st = 40,	 
				 .bit_1st = 0,	 
				 .bit_last = 3,	 
				 },
				{
				 .type = IOEXP_RXLOS_TYPE,
				 .port_1st = 40,	 
				 .bit_1st = 12,	 
				 .bit_last = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{.ch_id = 6,
     .ioexp_id = 19,
	 .addr = 0x21,
 	 .layout = {
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 44,	 
				 .bit_1st = 4,	 
				 .bit_last = 7,	 
				 },
				{
				 .type = IOEXP_TXFAULT_TYPE,
				 .port_1st = 44,	 
				 .bit_1st = 0,	 
				 .bit_last = 3,	 
				 },
				{
				 .type = IOEXP_RXLOS_TYPE,
				 .port_1st = 44,	 
				 .bit_1st = 12,	 
				 .bit_last = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
};

struct ioexp_input_map_t maple_ioexp_input_map = {
	.tbl =  maple_ioexp_input_tbl,
	.size = ARRAY_SIZE(maple_ioexp_input_tbl),
};

struct ioexp_output_t maple_ioexp_modsel[] = {
    {.ioexp_id = 0, .ch_id = 0,  .addr = 0x20, .port_1st = 48, .bit_1st = 0, .bit_last = 7},
    {.end_of_tbl = true},
};

struct ioexp_output_t maple_ioexp_reset[] = {
    {.ioexp_id = 0, .ch_id = 0,  .addr = 0x20, .port_1st = 48, .bit_1st = 8, .bit_last = 15},
    {.end_of_tbl = true},
};

struct ioexp_output_t maple_ioexp_lpmode[] = {
    {.ioexp_id = 1, .ch_id = 0, .addr = 0x21, .port_1st = 48, .bit_1st = 0, .bit_last = 7},
    {.end_of_tbl = true},
};

struct ioexp_output_t maple_ioexp_txdisable[] = {
    /*sfp*/
    {.ioexp_id = 3, .ch_id = 1, .addr = 0x20, .port_1st = 0, .bit_1st = 8, .bit_last = 11},
    {.ioexp_id = 4, .ch_id = 1, .addr = 0x21, .port_1st = 4, .bit_1st = 8, .bit_last = 11},
    {.ioexp_id = 6, .ch_id = 2, .addr = 0x20, .port_1st = 8, .bit_1st = 8, .bit_last = 11},
    {.ioexp_id = 7, .ch_id = 2, .addr = 0x21, .port_1st = 12, .bit_1st = 8, .bit_last = 11},
    {.ioexp_id = 9, .ch_id = 3, .addr = 0x20, .port_1st = 16, .bit_1st = 8, .bit_last = 11},
    {.ioexp_id = 10, .ch_id = 3, .addr = 0x21, .port_1st = 20, .bit_1st = 8, .bit_last = 11},
    {.ioexp_id = 12, .ch_id = 4, .addr = 0x20, .port_1st = 24, .bit_1st = 8, .bit_last = 11},
    {.ioexp_id = 13, .ch_id = 4, .addr = 0x21, .port_1st = 28, .bit_1st = 8, .bit_last = 11},
    {.ioexp_id = 15, .ch_id = 5, .addr = 0x20, .port_1st = 32, .bit_1st = 8, .bit_last = 11},
    {.ioexp_id = 16, .ch_id = 5, .addr = 0x21, .port_1st = 36, .bit_1st = 8, .bit_last = 11},
    {.ioexp_id = 18, .ch_id = 6, .addr = 0x20, .port_1st = 40, .bit_1st = 8, .bit_last = 11},
    {.ioexp_id = 19, .ch_id = 6, .addr = 0x21, .port_1st = 44, .bit_1st = 8, .bit_last = 11},
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
struct ioexp_output_t *maple_ioexp_output_tbl[IOEXP_OUTPUT_TYPE_NUM] = {

    [IOEXP_LPMODE_TYPE] = maple_ioexp_lpmode,
    [IOEXP_RESET_TYPE] = maple_ioexp_reset,
    [IOEXP_TXDISABLE_TYPE] = maple_ioexp_txdisable,
    [IOEXP_MODSEL_TYPE] = maple_ioexp_modsel, 
    [IOEXP_TXDISABLE2_TYPE] = NULL,

};
/*only config input pin for our purpose*/
struct ioexp_config_t maple_ioexp_config[] = {
    /*qsfp*/
    {.ch_id = 0, .addr = 0x20, .val = 0x0000, .type = PCA9555_TYPE},
    {.ch_id = 0, .addr = 0x21, .val = 0xff00, .type = PCA9555_TYPE},
    {.ch_id = 0, .addr = 0x22, .val = 0xffff, .type = PCA9555_TYPE},
    /*sfp*/
    {.ch_id = 1, .addr = 0x20, .val = 0xf0ff, .type = PCA9555_TYPE},
    {.ch_id = 1, .addr = 0x21, .val = 0xf0ff, .type = PCA9555_TYPE},
    {.ch_id = 1, .addr = 0x22, .val = 0x0000, .type = PCA9555_TYPE},
    {.ch_id = 2, .addr = 0x20, .val = 0xf0ff, .type = PCA9555_TYPE},
    {.ch_id = 2, .addr = 0x21, .val = 0xf0ff, .type = PCA9555_TYPE} ,
    {.ch_id = 2, .addr = 0x22, .val = 0x0000, .type = PCA9555_TYPE},
    {.ch_id = 3, .addr = 0x20, .val = 0xf0ff, .type = PCA9555_TYPE},
    {.ch_id = 3, .addr = 0x21, .val = 0xf0ff, .type = PCA9555_TYPE},
    {.ch_id = 3, .addr = 0x22, .val = 0x0000, .type = PCA9555_TYPE},
    {.ch_id = 4, .addr = 0x20, .val = 0xf0ff, .type = PCA9555_TYPE},
    {.ch_id = 4, .addr = 0x21, .val = 0xf0ff, .type = PCA9555_TYPE},
    {.ch_id = 4, .addr = 0x22, .val = 0x0000, .type = PCA9555_TYPE},
    {.ch_id = 5, .addr = 0x20, .val = 0xf0ff, .type = PCA9555_TYPE},
    {.ch_id = 5, .addr = 0x21, .val = 0xf0ff, .type = PCA9555_TYPE},
    {.ch_id = 5, .addr = 0x22, .val = 0x0000, .type = PCA9555_TYPE},
    {.ch_id = 6, .addr = 0x20, .val = 0xf0ff, .type = PCA9555_TYPE},
    {.ch_id = 6, .addr = 0x21, .val = 0xf0ff, .type = PCA9555_TYPE},
    {.ch_id = 6, .addr = 0x22, .val = 0x0000, .type = PCA9555_TYPE},
};
struct ioexp_config_map_t maple_ioexp_config_map = {
	.tbl =  maple_ioexp_config,
	.size = ARRAY_SIZE(maple_ioexp_config),
};

int maple_cpld_i2c_ch[] = {
	2,
};

struct int_vector_t maple_cpld_i2c_ch_map = {
	.tbl = maple_cpld_i2c_ch,
	.size = ARRAY_SIZE(maple_cpld_i2c_ch),
};

struct cpld_config_t maple_cpld_config = {
    .i2c_config = {
                    [CPLD_ID_1] = {.ch_id = 0, .addr = 0x77}, /*cpld_1*/
                    [CPLD_ID_2] = {.ch_id = 0, .addr = 0x33}  /*cpld_2*/
                  },
    .i2c_ch_map = &maple_cpld_i2c_ch_map,
    .intr_st = {.cpld_id = CPLD_ID_2, .offset = 0x35},
    .intr_en = {.cpld_id = CPLD_ID_2, .offset = 0x37},
    .sff_intr_gpio = CPLD_IOEXP_INTR_N,
};

#endif /*__IO_CONFIG_MAPLE_H*/
