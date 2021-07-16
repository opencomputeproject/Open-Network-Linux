#ifndef __IO_CONFIG_BOCELLI_H
#define __IO_CONFIG_BOCELLI_H

#define CPLD_IOEXP_INTR_N (12) /*GPIO 12*/
#define BOCELLI_MUX_RST_GPIO (69)
#define BOCELLI_IO_PORT_NUM (14)
/*io expander layout*/
int bocelli_ioexp_i2c_ch[] = {
	6,
	7,
};

struct int_vector_t bocelli_ioexp_i2c_ch_map = {
	.tbl = bocelli_ioexp_i2c_ch,
	.size = ARRAY_SIZE(bocelli_ioexp_i2c_ch),
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

unsigned long bocelli_ioexp_aval_sff_input[IOEXP_INPUT_TYPE_NUM] = {

    [IOEXP_PRS_TYPE] = 0x3fffL,
    [IOEXP_TXFAULT_TYPE] = 0x3000L,
    [IOEXP_RXLOS_TYPE] = 0x3000L,
    [IOEXP_INTR_TYPE] = 0xfffL,
    [IOEXP_TXFAULT2_TYPE] = 0,
    [IOEXP_RXLOS2_TYPE] = 0,
};

struct ioexp_input_t bocelli_ioexp_input_tbl[] = {
	{
     .ch_id = 0,
     .ioexp_id = 0,
	 .addr = 0x20,
 	 .layout = {
				{
				 .type = IOEXP_TXFAULT_TYPE,
				 .port_1st = 12,	 
				 .bit_1st = 8,	 
				 .bit_last = 9,	 
				 },
				{
				 .type = IOEXP_RXLOS_TYPE,
				 .port_1st = 12,	 
				 .bit_1st = 10,	 
				 .bit_last = 11,	 
				 },
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 12,	 
				 .bit_1st = 12,	 
				 .bit_last = 13,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{
     .ch_id = 0,
     .ioexp_id = 2,
	 .addr = 0x22,
 	 .layout = {
				{
				 .type = IOEXP_INTR_TYPE,
				 .port_1st = 0,	 
				 .bit_1st = 4,	 
				 .bit_last = 9,	 
				 },
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 0,	 
				 .bit_1st = 10,	 
				 .bit_last = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
	{
     .ch_id = 1,
     .ioexp_id = 4,
	 .addr = 0x21,
 	 .layout = {
				{
				 .type = IOEXP_INTR_TYPE,
				 .port_1st = 6,	 
				 .bit_1st = 4,	 
				 .bit_last = 9,	 
				 },
				{
				 .type = IOEXP_PRS_TYPE,
				 .port_1st = 6,	 
				 .bit_1st = 10,	 
				 .bit_last = 15,	 
				 },
				 {.end_of_layout = true},
			}
	},
};

struct ioexp_input_map_t bocelli_ioexp_input_map = {
	.tbl =  bocelli_ioexp_input_tbl,
	.size = ARRAY_SIZE(bocelli_ioexp_input_tbl),
};

struct ioexp_output_t bocelli_ioexp_modsel[] = {
    {.ioexp_id = 1, .ch_id = 0,  .addr = 0x21, .port_1st = 0, .bit_1st = 0, .bit_last = 5},
    {.ioexp_id = 3, .ch_id = 1,  .addr = 0x20, .port_1st = 6, .bit_1st = 0, .bit_last = 5},
    {.end_of_tbl = true},
};

struct ioexp_output_t bocelli_ioexp_reset[] = {
    {.ioexp_id = 1, .ch_id = 0,  .addr = 0x21, .port_1st = 0, .bit_1st = 6, .bit_last = 11},
    {.ioexp_id = 3, .ch_id = 1,  .addr = 0x20, .port_1st = 6, .bit_1st = 6, .bit_last = 11},
    {.end_of_tbl = true},
};

struct ioexp_output_t bocelli_ioexp_lpmode[] = {
    {.ioexp_id = 1, .ch_id = 0,  .addr = 0x21, .port_1st = 0, .bit_1st = 12, .bit_last = 15},
    {.ioexp_id = 2, .ch_id = 0, .addr = 0x22, .port_1st = 4, .bit_1st = 0, .bit_last = 1},
    {.ioexp_id = 3, .ch_id = 1,  .addr = 0x20, .port_1st = 6, .bit_1st = 12, .bit_last = 15},
    {.ioexp_id = 4, .ch_id = 1, .addr = 0x21, .port_1st = 10, .bit_1st = 0, .bit_last = 1},
    {.end_of_tbl = true},
};

struct ioexp_output_t bocelli_ioexp_txdisable[] = {
    /*sfp*/
    {.ioexp_id = 0, .ch_id = 0, .addr = 0x20, .port_1st = 0, .bit_1st = 0, .bit_last = 1},
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
struct ioexp_output_t *bocelli_ioexp_output_tbl[IOEXP_OUTPUT_TYPE_NUM] = {

    [IOEXP_LPMODE_TYPE] = bocelli_ioexp_lpmode,
    [IOEXP_RESET_TYPE] = bocelli_ioexp_reset,
    [IOEXP_TXDISABLE_TYPE] = bocelli_ioexp_txdisable,
    [IOEXP_MODSEL_TYPE] = bocelli_ioexp_modsel, 
    [IOEXP_TXDISABLE2_TYPE] = NULL,

};
/*only config input pin for our purpose*/
/*idx stands for ioexp_id*/
struct ioexp_config_t bocelli_ioexp_config[] = {
    /*sfp*/
    [0] = {.ch_id = 0, .addr = 0x20, .val = 0xfffc, .type = PCA9555_TYPE},
    /*qsfp*/
    [1] = {.ch_id = 0, .addr = 0x21, .val = 0x0000, .type = PCA9555_TYPE},
    [2] = {.ch_id = 0, .addr = 0x22, .val = 0xfffc, .type = PCA9555_TYPE},
    [3] = {.ch_id = 1, .addr = 0x20, .val = 0x0000, .type = PCA9555_TYPE},
    [4] = {.ch_id = 1, .addr = 0x21, .val = 0xfffc, .type = PCA9555_TYPE},
};
struct ioexp_config_map_t bocelli_ioexp_config_map = {
	.tbl =  bocelli_ioexp_config,
	.size = ARRAY_SIZE(bocelli_ioexp_config),
};
#if 0
int bocelli_cpld_i2c_ch[] = {
	2,
};

struct int_vector_t bocelli_cpld_i2c_ch_map = {
	.tbl = bocelli_cpld_i2c_ch,
	.size = ARRAY_SIZE(bocelli_cpld_i2c_ch),
};

struct cpld_config_t bocelli_cpld_config = {
    .i2c_config = {
                    [CPLD_ID_1] = {.ch_id = 0, .addr = 0x77}, /*cpld_1*/
                    [CPLD_ID_2] = {.ch_id = 0, .addr = 0x33}  /*cpld_2*/
                  },
    .i2c_ch_map = &bocelli_cpld_i2c_ch_map,
    .intr_st = {.cpld_id = CPLD_ID_2, .offset = 0x35},
    .intr_en = {.cpld_id = CPLD_ID_2, .offset = 0x37},
    .sff_intr_gpio = CPLD_IOEXP_INTR_N,
};
#endif
#endif /*__IO_CONFIG_BOCELLI_H*/
