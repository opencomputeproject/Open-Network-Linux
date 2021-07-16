#ifndef __IO_DEV_H
#define __IO_DEV_H

#define IO_RESET_DEFAULT (1)
#define IO_LPMODE_DEFAULT (1)
#define IO_MODSEL_DEFAULT (0)
#define IO_TXDISABLE_DEFAULT (0)
#define LAYOUT_MAX_NUM (16)

typedef enum {
    CPLD_ID_1,
    CPLD_ID_2,
    CPLD_ID_NUM,
} cpld_id_t;

typedef enum {
    PCA_UNKOWN_TYPE,
    PCA9555_TYPE,
    PCA9554_TYPE,
    PCA95XX_TYPE_NUM,
} pca95xx_type_t;

typedef enum {
    IOEXP_PRS_TYPE,
    IOEXP_TXFAULT_TYPE,
    IOEXP_RXLOS_TYPE,
    IOEXP_INTR_TYPE,
    IOEXP_TXFAULT2_TYPE,
    IOEXP_RXLOS2_TYPE,
    IOEXP_INPUT_TYPE_NUM,
} ioexp_input_type_t;

typedef enum {
    IOEXP_LPMODE_TYPE,
    IOEXP_RESET_TYPE,
    IOEXP_TXDISABLE_TYPE,
    IOEXP_MODSEL_TYPE,
    IOEXP_TXDISABLE2_TYPE,
    IOEXP_OUTPUT_TYPE_NUM,
} ioexp_output_type_t;
/*refactoring begins*/
struct pin_layout_t {
    ioexp_input_type_t type;
    int port_1st;
    int bit_1st;
    int bit_last;
    bool end_of_layout;
};
struct ioexp_input_t {
    int ioexp_id;
    int ch_id;
    u8 addr;
    struct pin_layout_t layout[LAYOUT_MAX_NUM];
};

struct ioexp_input_map_t {
    struct ioexp_input_t *tbl;
    int size;
};

struct ioexp_output_t {
    int ioexp_id;
    u8 ch_id;
    u8 addr;
    int port_1st;
    int bit_1st;
    int bit_last;  /*reprsent the num of ports*/
    bool end_of_tbl;
};

struct ioexp_config_t {
    int ch_id;
    u8 addr;
    u16 val;
    pca95xx_type_t type;
};

struct ioexp_config_map_t {
    struct ioexp_config_t *tbl;
    int size;
};

struct cpld_i2c_config_t {
    int ch_id;
    u8 addr;
};

struct reg_param_t {
    int cpld_id;
    u8 offset;
};

struct cpld_config_t {
    struct cpld_i2c_config_t i2c_config[CPLD_ID_NUM];
    struct int_vector_t *i2c_ch_map;
    struct reg_param_t intr_st;
    struct reg_param_t intr_en;
    int sff_intr_gpio;
};

struct platform_io_info_t {
    int platform_id;
    struct int_vector_t *i2c_ch_map;
    struct ioexp_config_map_t *config_map;
    struct ioexp_input_map_t *input_map;
    struct ioexp_output_t **output;
    unsigned long *aval_sff_input;
    struct cpld_config_t *cpld_config;
    int mux_rst_gpio;
    bool mux_func_supported;
    struct int_vector_t *mux_ch_2_port_map;
};

extern struct sff_io_driver_t ioexp_sku_sff_io_drv;
extern struct pltfm_func_t ioexp_sku_pltfm_func;

#endif /*__IO_DEV_H*/
