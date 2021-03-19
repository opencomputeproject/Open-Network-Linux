#ifndef __IO_DEV_H
#define __IO_DEV_H

#define IO_RESET_DEFAULT (1)
#define IO_LPMODE_DEFAULT (1)
#define IO_MODESEL_DEFAULT (0)
#define IO_TXDISABLE_DEFAULT (1)
#define LAYOUT_MAX_NUM (16)

typedef enum {
    CPLD_ID_1,
    CPLD_ID_2,
    CPLD_ID_NUM,
}cpld_id_t;

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
    IOEXP_INT_TYPE,
    IOEXP_TXFAULT2_TYPE,
    IOEXP_RXLOS2_TYPE,
    IOEXP_INPUT_TYPE_NUM,
} ioexp_input_type_t;

typedef enum {
    IOEXP_LPMODE_TYPE,
    IOEXP_RESET_TYPE,
    IOEXP_TXDISABLE_TYPE,
    IOEXP_MODESEL_TYPE,
    IOEXP_TXDISABLE2_TYPE,
    IOEXP_OUTPUT_TYPE_NUM,
} ioexp_output_type_t;
/*refactoring begins*/
struct pin_layout_t {
	ioexp_input_type_t type;
	int port_min;
	int bit_min;
	int bit_max;
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
    int port_min;
    int bit_min;
    int bit_max;  /*reprsent the num of ports*/
	bool end_of_tbl;
};

struct ldata_format_t {

	unsigned long bitmap;
	unsigned long valid;
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

struct i2c_ch_map_t {
    int *tbl;
    int size; 
};

struct io_dev_client_t {
    struct i2c_client *client;
    struct mutex lock;
};

struct ioexp_dev_t {
    struct i2c_ch_map_t *i2c_ch_map;
    struct ioexp_config_map_t *config_map;
    struct ioexp_input_map_t *input_map;
    int *input_port_num;
    struct ioexp_output_t **output;
    struct ldata_format_t input_st[IOEXP_INPUT_TYPE_NUM];
    struct io_dev_client_t *ioexp_client;
    int (*input_hdlr)(struct ioexp_dev_t *self);
    //int port_num;
    //int sfp_port_num;
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
	struct i2c_ch_map_t *i2c_ch_map;
	struct reg_param_t intr_st; 
	struct reg_param_t intr_en; 
	int sff_intr_gpio;
};

struct cpld_io_t {
	struct cpld_config_t *config;
    struct io_dev_client_t *cpld_client;
    int (*intr_st_get)(struct cpld_io_t *cpld_io, u32 *reg);
};

struct platform_io_info_t {
	int platform_id;
    struct i2c_ch_map_t *i2c_ch_map;
    struct ioexp_config_map_t *config_map;
    struct ioexp_input_map_t *input_map;
    struct ioexp_output_t **output;
//	int port_num;
//	int sfp_port_num;
    int *input_port_num;
	struct cpld_config_t *cpld_config;
	int mux_rst_gpio;
};
struct io_dev_t {
    int pltfm_id;
    struct ioexp_dev_t ioexp_dev;
    bool intr_mode_supported;
    struct cpld_io_t cpld_io;
    int mux_rst_gpio;
};	

struct sff_io_driver_t *sff_io_drv_get_iodev(void);

int io_dev_mux_reset_set(int lc_id, int val);
int io_dev_mux_reset_get(int lc_id, int *val);
bool ioexp_is_channel_ready(int lc_id);
bool ioexp_is_i2c_ready(void);
int io_dev_init(int platform_id, int io_no_init);
void io_dev_deinit(void);
int io_dev_hdlr(void);
#endif /*__IO_DEV_H*/
