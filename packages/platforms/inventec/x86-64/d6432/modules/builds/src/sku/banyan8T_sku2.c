/*banyan8T_sku2.c*/
/* handle sff io pin control , mux gpio .. cpld  etc*/
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/err.h>
#include "inv_pca954x_data.h"
#include "sku_common.h"

#define PRS_INTR_GPIO_NUM (26)
#define RXLOS_INTR_GPIO_NUM (44)
#define MUX_RST_GPIO_NUM (69)
#define INV_MUX_NAME ("i2c-INVMUX-0")
#define REG_PORT_NUM (8)
#define SKU_PORT_NUM (56)
#define SKU_SFP_DD_PORT_FIRST (0)
#define SKU_SFP_DD_PORT_LAST (47)
#define SKU_QSFP_DD_PORT_FIRST (48)
#define SKU_QSFP_DD_PORT_LAST (55)

unsigned long pre_rxlos_intr_lv = 1;
unsigned long prs_intr_cnt = 0;
unsigned long rxlos_intr_cnt = 0;

typedef enum {
    CPLD_SFF_PRS_TYPE,
    CPLD_SFF_INTR_TYPE,
    CPLD_SFF_RXLOS_TYPE,
    CPLD_SFF_RXLOS2_TYPE,
    CPLD_SFF_TXFAULT_TYPE,
    CPLD_SFF_TXFAULT2_TYPE,
    CPLD_SFF_LPMODE_TYPE,
    CPLD_SFF_RESET_TYPE,
    CPLD_SFF_MODSEL_TYPE,
    CPLD_SFF_TXDISABLE_TYPE,
    CPLD_SFF_TXDISABLE2_TYPE,
    CPLD_SFF_IO_TYPE_NUM,

} cpld_sff_type_t;

enum {
    SFPDD_GRP0_INT_ID,
    SFPDD_GRP1_INT_ID,
    QSFPDD_GRP0_INT_ID,
    SFPDD_RXLOS_GRP0_INT_ID,
    SFPDD_RXLOS_GRP1_INT_ID,
    QSFPDD_RXLOS_GRP0_INT_ID,
    SFF_INT_ID_NUM
};

struct cpld_reg_t {
    int cpld_id;
    u8 offset;
    u8 val; /*only for write data*/     
};


struct i2c_config_t {
    int ch_id;
    u8 addr;
};
#define CPLD_ID_NUM (3)
#define CH_ID_NUM (2)

struct sff_io_st_t {
    unsigned long bitmap;
    unsigned long valid_mask; 
};

struct mux_func_t {
    struct device *dev;
    struct pca954x *pca;
    const struct int_vector_t *ch_2_port;
};

struct sku_t {
    struct i2c_config_t cpld_i2c_config[CPLD_ID_NUM];
    struct i2c_config_t mux_i2c_config;
    struct inv_i2c_client_t inv_i2c_client[CH_ID_NUM];
    struct sff_io_st_t sff_io_st[CPLD_SFF_IO_TYPE_NUM];
    int prs_intr_gpio;
    int rxlos_intr_gpio;
    int mux_rst_gpio;
    struct mux_func_t mux_func;
};

struct sff_intr_event_t {
    struct cpld_reg_t st_clear_reg;    
    int (*intr_hdlr)(struct sff_intr_event_t *func_t, struct sku_t *sku); 
};

struct sff_reg_t {

    int cpld_id;
    u8 offset;
    int size; /*unit byte*/
    int port_1st;
    int port_last;
    bool is_end;
};
enum {
    SFP_DD_PRS_GRP0,
    SFP_DD_PRS_GRP1,
    QSFP_DD_PRS_GRP0,
    PRS_GRP_NUM
};

enum {
    QSFP_DD_INTR_GRP0,
    INTR_GRP_NUM
};

enum {
    SFP_DD_RXLOS_GRP0,
    SFP_DD_RXLOS_GRP1,
    RXLOS_GRP_NUM
};

enum {
    SFP_DD_RXLOS2_GRP0,
    SFP_DD_RXLOS2_GRP1,
    RXLOS2_GRP_NUM
};

enum {
    SFP_DD_TXFAULT_GRP0,
    SFP_DD_TXFAULT_GRP1,
    TXFAULT_GRP_NUM
};

enum {
    SFP_DD_TXFAULT2_GRP0,
    SFP_DD_TXFAULT2_GRP1,
    TXFAULT2_GRP_NUM
};
#define BITMASK_NUM (5)
/*idx : byte size*/
static const unsigned long bitmask[BITMASK_NUM] = {
    [0] = 0, /*dummy*/        
    [1] = 0xffL,
    [2] = 0xffffL,
    [3] = 0xffffffL,
    [4] = 0xffffffffL
};
static int banyan8T_mux_ch_2_port[] = {
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
    [32] = 32,
    [33] = 33,
    [34] = 34,
    [35] = 35,
    [36] = 36,
    [37] = 37,
    [38] = 38,
    [39] = 39,
    [40] = 40,
    [41] = 41,
    [42] = 42,
    [43] = 43,
    [44] = 44,
    [45] = 45,
    [46] = 46,
    [47] = 47,
    [48] = 48,
    [49] = 49,
    [50] = 50,
    [51] = 51,
    [52] = 52,
    [53] = 53,
    [54] = 54,
    [55] = 55
};

static const struct int_vector_t banyan8T_mux_ch_2_port_map = {
	.tbl = banyan8T_mux_ch_2_port,
	.size = ARRAY_SIZE(banyan8T_mux_ch_2_port),
};

/*bit0 : idx1 bit1 idx1  the value is consitent with valid sizeof sff_reg_t table*/
const static unsigned long sff_io_st_valid_mask[CPLD_SFF_IO_TYPE_NUM] = {

    [CPLD_SFF_PRS_TYPE] = 0x7,
    [CPLD_SFF_INTR_TYPE] = 0x1,
    [CPLD_SFF_RXLOS_TYPE] = 0x3,
    [CPLD_SFF_RXLOS2_TYPE] = 0x3,
#if 0 /*reserved*/
    [CPLD_SFF_TXFAULT_TYPE] = 0x3,
    [CPLD_SFF_TXFAULT2_TYPE] = 0x3
#endif        
};
/*<TBD> since we may not verify sfp-dd optic , so force it do hw init*/
const static unsigned long sff_io_def_val[CPLD_SFF_IO_TYPE_NUM] = {
    [CPLD_SFF_LPMODE_TYPE] =  0xffffffffffffffL,
    [CPLD_SFF_RESET_TYPE] =   0xffffffffffffffL,
    [CPLD_SFF_MODSEL_TYPE] =  0x00000000000000L,
};
#define SFF_POWER_DEF_VAL (0xffffffffffffffL)
const static struct sff_reg_t sff_power_reg_tbl[] = {

    [0] = {.cpld_id = 0, .offset = 0x2c, .size = 1, .port_1st = 0, .port_last = 15},
    [1] = {.cpld_id = 1, .offset = 0x1a, .size = 2, .port_1st = 16, .port_last = 47},
    [2] = {.cpld_id = 1, .offset = 0x11, .size = 1, .port_1st = 48, .port_last = 55},
    {.is_end = true}
};

const static struct sff_reg_t sff_prs_reg_tbl[] = {

    [SFP_DD_PRS_GRP0] = {.cpld_id = 0, .offset = 0x28, .size = 2, .port_1st = 0, .port_last = 15},
    [SFP_DD_PRS_GRP1] = {.cpld_id = 1, .offset = 0x16, .size = 4, .port_1st = 16, .port_last = 47},
    [QSFP_DD_PRS_GRP0] = {.cpld_id = 1, .offset = 0x10, .size = 1, .port_1st = 48, .port_last = 55},
    {.is_end = true}
};

const static struct sff_reg_t sff_intr_reg_tbl[] = {

    [QSFP_DD_INTR_GRP0] = {.cpld_id = 1, .offset = 0x14, .size = 1, .port_1st = 48, .port_last = 55},
    {.is_end = true}
};

const struct sff_reg_t sff_rxlos_reg_tbl[] = {

    [SFP_DD_RXLOS_GRP0] = {.cpld_id = 0, .offset = 0x24, .size = 2, .port_1st = 0, .port_last = 15},
    [SFP_DD_RXLOS_GRP1] = {.cpld_id = 1, .offset = 0x20, .size = 4, .port_1st = 16, .port_last = 47},
    {.is_end = true}
};

const static struct sff_reg_t sff_rxlos2_reg_tbl[] = {

    [SFP_DD_RXLOS2_GRP0] = {.cpld_id = 0, .offset = 0x26, .size = 2, .port_1st = 0, .port_last = 15},
    [SFP_DD_RXLOS2_GRP1] = {.cpld_id = 1, .offset = 0x24, .size = 4, .port_1st = 16, .port_last = 47},
    {.is_end = true}
};
#if 0 /*reserved*/
const struct sff_reg_t sff_txfault_reg_tbl[] = {

    [SFP_DD_TXFAULT_GRP0] = {.cpld_id = 0, .offset = 0x20, .size = 2, .port_1st = 0, .port_last = 15},
    [SFP_DD_TXFAULT_GRP1] = {.cpld_id = 1, .offset = 0x28, .size = 4, .port_1st = 16, .port_last = 47},
    {.is_end = true}
};

const struct sff_reg_t sff_txfault2_reg_tbl[] = {

    [SFP_DD_TXFAULT2_GRP0] = {.cpld_id = 0, .offset = 0x22, .size = 2, .port_1st = 0, .port_last = 15},
    [SFP_DD_TXFAULT2_GRP1] = {.cpld_id = 1, .offset = 0x2c, .size = 4, .port_1st = 16, .port_last = 47},
    {.is_end = true}
};
#endif
const static struct sff_reg_t sff_lpmode_reg_tbl[] = {

    {.cpld_id = 0, .offset = 0x2a, .size = 2, .port_1st = 0, .port_last = 15},
    {.cpld_id = 1, .offset = 0x1c, .size = 4, .port_1st = 16, .port_last = 47},
    {.cpld_id = 1, .offset = 0x12, .size = 1, .port_1st = 48, .port_last = 55},
    {.is_end = true}
};

const static struct sff_reg_t sff_reset_reg_tbl[] = {

    {.cpld_id = 1, .offset = 0x15, .size = 1, .port_1st = 48, .port_last = 55},
    {.is_end = true}
};

const static struct sff_reg_t sff_modsel_reg_tbl[] = {

    {.cpld_id = 1, .offset = 0x13, .size = 1, .port_1st = 48, .port_last = 55},
    {.is_end = true}
};

const static struct sff_reg_t *sff_io_reg_tbl[CPLD_SFF_IO_TYPE_NUM] = {
    
    [CPLD_SFF_PRS_TYPE] = sff_prs_reg_tbl,
    [CPLD_SFF_INTR_TYPE] = sff_intr_reg_tbl,
    [CPLD_SFF_RXLOS_TYPE] = sff_rxlos_reg_tbl,
    [CPLD_SFF_RXLOS2_TYPE] = sff_rxlos2_reg_tbl,
    [CPLD_SFF_TXFAULT_TYPE] = NULL,//sff_txfault_reg_tbl,
    [CPLD_SFF_TXFAULT2_TYPE] = NULL, //sff_txfault2_reg_tbl,
    [CPLD_SFF_RESET_TYPE] = sff_reset_reg_tbl,
    [CPLD_SFF_LPMODE_TYPE] = sff_lpmode_reg_tbl,
    [CPLD_SFF_MODSEL_TYPE] = sff_modsel_reg_tbl,
    [CPLD_SFF_TXDISABLE_TYPE] = NULL,
    [CPLD_SFF_TXDISABLE2_TYPE] = NULL
};
/*[cpld_id] = config*/
const static struct i2c_config_t cpld_i2c_config[CPLD_ID_NUM] = {
    [0] = { .ch_id = 0, .addr = 0x77}, /*CPLD1*/
    [1] = { .ch_id = 0, .addr = 0x26}, /*CPLD3*/
    [2] = { .ch_id = 0, .addr = 0x33} /*CPLD2*/
};

const static struct i2c_config_t mux_i2c_config = {
    .ch_id = 1, .addr = 0x71
};

/*[ch_id] = ch */
static const int i2c_ch_tbl[CH_ID_NUM] = {
    [0] = 2,
    [1] = 4
};
#define INTR_EN_VAL (1)
#define CPLD_SFF_INTR_ST_SFPDD_GRP0_BITMASK (0x1)
#define CPLD_SFF_INTR_ST_SFPDD_GRP1_BITMASK (0x2)
#define CPLD_SFF_INTR_ST_QSFPDD_GRP0_BITMASK (0x4)
#define CPLD_SFF_INTR_ST_SFPDD_RXLOS_GRP0_BITMASK (0x20)
#define CPLD_SFF_INTR_ST_SFPDD_RXLOS_GRP1_BITMASK (0x40)
#define CPLD_SFF_INTR_ST_QSFPDD_RXLOS_GRP0_BITMASK (0x80)

const static struct cpld_reg_t intr_st_reg = {
    .cpld_id = 0, .offset = 0x19
};

const static struct cpld_reg_t intr_en_reg = {
    .cpld_id = 0, .offset = 0x2e
};
const static struct cpld_reg_t intr2_en_reg = {
    .cpld_id = 1, .offset = 0x31
};

static struct sku_t banyan8T_sku2;
static struct i2c_client *i2c_client_obj = NULL;

/*function declaration*/
static struct inv_i2c_client_t *inv_i2c_client_find(struct sku_t *sku, struct i2c_config_t *config);
static int sff_io_set(struct sku_t *sku, cpld_sff_type_t type, unsigned long bitmap);
static int sku_sff_power_set(struct sku_t *sku, unsigned long bitmap);
static int cpld_reg_get(struct sku_t *sku, int ch_id, u8 offset, u8 *buf, int size);
static int cpld_reg_set(struct sku_t *sku, int ch_id, u8 offset, const u8 *buf, int size);
static int sku_init(int platform_id, int io_no_init);
static void sku_deinit(void);
static int sku_hdlr(void);
static int sku_sff_get_ready_action(int lc_id, int port);
static int sku_sff_detected_action(int lc_id, int port);
static int cpld_sff_io_st_update(struct sku_t *sku, cpld_sff_type_t type, int idx);
static int cpld_io_set(cpld_sff_type_t type, int lc_id, unsigned long bitmap);
static int cpld_io_get(cpld_sff_type_t type, int lc_id, unsigned long *bitmap);
static int sfpdd_grp0_intr_hdlr(struct sff_intr_event_t *event, struct sku_t *sku);
static int sfpdd_grp1_intr_hdlr(struct sff_intr_event_t *event, struct sku_t *sku);
static int qsfpdd_grp0_intr_hdlr(struct sff_intr_event_t *event, struct sku_t *sku);
static int sfpdd_rxlos_grp0_intr_hdlr(struct sff_intr_event_t *event, struct sku_t *sku);
static int sfpdd_rxlos_grp1_intr_hdlr(struct sff_intr_event_t *event, struct sku_t *sku);
static int qsfpdd_rxlos_grp0_intr_hdlr(struct sff_intr_event_t *event, struct sku_t *sku);
static int mux_reset_get(int lc_id, int *val);
static int mux_reset_set(int lc_id, int value);
static bool mux_is_alive(int lc_id);
/*sff io*/
static int cpld_prs_get(int lc_id, unsigned long *bitmap);
static int cpld_intr_get(int lc_id, unsigned long *bitmap);
static int cpld_rxlos_get(int lc_id, unsigned long *bitmap);
static int cpld_rxlos2_get(int lc_id, unsigned long *bitmap);
//static int cpld_txfault_get(int lc_id, unsigned long *bitmap);
//static int cpld_txfault2_get(int lc_id, unsigned long *bitmap);
static int cpld_reset_set(int lc_id, unsigned long bitmap);
static int cpld_reset_get(int lc_id, unsigned long *bitmap);
static int cpld_lpmode_set(int lc_id, unsigned long bitmap);
static int cpld_lpmode_get(int lc_id, unsigned long *bitmap);
static int cpld_modsel_set(int lc_id, unsigned long bitmap);
static int cpld_modsel_get(int lc_id, unsigned long *bitmap);
/*sff extra io*/
static int cpld_power_set(int lc_id, unsigned long bitmap);
static int cpld_power_get(int lc_id, unsigned long *bitmap);

/*mux operation*/
static int sku_mux_failed_ch_get(int lc_id, unsigned long *ch);
static int sku_mux_blocked_ch_set(int lc_id, unsigned long ch);
static int sku_mux_fail_set(int lc_id, bool is_fail);
static int sku_mux_fail_get(int lc_id, bool *is_fail);
static int sku_mux_ch_to_port(int lc_id, int ch);
static int sku_mux_port_to_ch(int lc_id, int port);

struct sff_intr_event_t sff_intr_event[SFF_INT_ID_NUM] = {
    [SFPDD_GRP0_INT_ID] = { 
                            .st_clear_reg = {.cpld_id = 0, .offset = 0x2d, .val = 1},  
                            .intr_hdlr = sfpdd_grp0_intr_hdlr},
    [SFPDD_GRP1_INT_ID] = { 
                            .st_clear_reg = {.cpld_id = 1, .offset = 0x30, .val = 0x1},  
                            .intr_hdlr = sfpdd_grp1_intr_hdlr},
    [QSFPDD_GRP0_INT_ID] = { 
                            .st_clear_reg = {.cpld_id = 1, .offset = 0x30, .val = 0x1},  
                            .intr_hdlr = qsfpdd_grp0_intr_hdlr},
    [SFPDD_RXLOS_GRP0_INT_ID] = { 
                            .st_clear_reg = {0},  
                            .intr_hdlr = sfpdd_rxlos_grp0_intr_hdlr},
    [SFPDD_RXLOS_GRP1_INT_ID] = { 
                            .st_clear_reg = {0},  
                            .intr_hdlr = sfpdd_rxlos_grp1_intr_hdlr},
    [QSFPDD_RXLOS_GRP0_INT_ID] = { 
                            .st_clear_reg = {0},  
                            .intr_hdlr = qsfpdd_rxlos_grp0_intr_hdlr}

};

struct sff_io_driver_t banyan8T_sku2_sff_io_drv = {
    .prs = {
        .set = NULL,
        .get = cpld_prs_get
    },

    .intr = {
        .set = NULL,
        .get = cpld_intr_get
    },
    .rxlos = {
        .set = NULL,
        .get = cpld_rxlos_get
    },
    .rxlos2 = {
        .set = NULL,
        .get = cpld_rxlos2_get
    },
    .txfault = {
        .set = NULL,
        .get = NULL
    },
    .txfault2 = {
        .set = NULL,
        .get = NULL
    },
    .reset = {
        .set = cpld_reset_set,
        .get = cpld_reset_get
    },
    .lpmode = {
        .set = cpld_lpmode_set,
        .get = cpld_lpmode_get
    },
    .modsel = {
        .set = cpld_modsel_set,
        .get = cpld_modsel_get
    },
    .txdisable = {
        .set = NULL,
        .get = NULL
    },
    .txdisable2 = {
        .set = NULL,
        .get = NULL
    },
};

struct pltfm_func_t banyan8T_sku2_pltfm_func = {
    .init = sku_init,
    .deinit = sku_deinit,
    .io_hdlr = sku_hdlr,
    .mux_reset_set = mux_reset_set,
    .mux_reset_get = mux_reset_get,
    .i2c_is_alive = mux_is_alive,
    .sff_get_ready_action = sku_sff_get_ready_action,
    .sff_detected_action = sku_sff_detected_action,
    .sff_power = {
        .set = cpld_power_set,
        .get = cpld_power_get
    },
    .mux_fail_set = sku_mux_fail_set,
    .mux_fail_get = sku_mux_fail_get,
    .mux_failed_ch_get =sku_mux_failed_ch_get,
    .mux_blocked_ch_set = sku_mux_blocked_ch_set,
    .mux_ch_to_port = sku_mux_ch_to_port,
    .mux_port_to_ch = sku_mux_port_to_ch
};
/*check if mux layer is alive*/ 
static bool mux_is_alive(int lc_id)
{
/*method1: checking mux status by read it's register*/
#if 1 
    int ret = 0;
    struct inv_i2c_client_t *inv_i2c_client = NULL;
    struct sku_t *sku = &banyan8T_sku2;
    (void)lc_id;

    if (!p_valid(inv_i2c_client = inv_i2c_client_find(sku, &(sku->mux_i2c_config)))) {
        return -EBADRQC;
    }

    mutex_lock(&inv_i2c_client->lock);
    ret = i2c_smbus_read_byte(inv_i2c_client->client);
    mutex_unlock(&inv_i2c_client->lock);
    if (ret < 0) {
        SKU_LOG_ERR("not alive\n");
        return false;
    }
    
    return true;
#else 

struct mux_func_t *func = &(banyan8T_sku2.mux_func); 
bool fail = false;
(void)lc_id;
func->pca->mux_fail_get(func->dev, &fail);
return (fail ? false : true); 
#endif    
}

static int mux_reset_get(int lc_id, int *val)
{
    int lv = 0;
    (void)lc_id;
    lv = gpio_get_value(banyan8T_sku2.mux_rst_gpio);
    *val = lv;
    return 0;
}

/*io mux control*/
static int mux_reset_set(int lc_id, int value)
{
    (void)lc_id;
    gpio_set_value(banyan8T_sku2.mux_rst_gpio, value);
    return 0;
}

static inline bool ch_id_valid(int id)
{
    return ((id >= 0 && id < CH_ID_NUM) ? true : false);
}

static struct inv_i2c_client_t *inv_i2c_client_find(struct sku_t *sku, struct i2c_config_t *config)
{
    struct inv_i2c_client_t *inv_i2c_client = NULL;
    int ch_id = 0;
    u8 addr = 0;

    if (!p_valid(sku)) {
        return NULL;
    } 
    if (!p_valid(config)) {
        return NULL;
    } 
    ch_id = config->ch_id;
    addr = config->addr;

    if (!ch_id_valid(ch_id)) {
        SKU_LOG_ERR("cant find ch_id:%d\n", ch_id);
        return NULL;
    }

    inv_i2c_client = &(sku->inv_i2c_client[ch_id]);
    if (!p_valid(inv_i2c_client)) {
        SKU_LOG_ERR("NULL inv_i2c_client ch_id:%d\n", ch_id);
        return NULL;
    }

    if (!p_valid(inv_i2c_client->client)) {
        SKU_LOG_ERR("NULL inv_i2c_client ch_id:%d\n", ch_id);
        return NULL;
    }
    inv_i2c_client->client->addr = addr;

    return inv_i2c_client;
}

static int inv_i2c_smbus_read_block_data(struct inv_i2c_client_t *inv_i2c_client, u8 offset, int len, u8 *buf)
{
    int ret = 0;

    check_p(inv_i2c_client);
    check_p(buf);
    mutex_lock(&inv_i2c_client->lock);
    ret = i2c_smbus_read_i2c_block_data_retry(inv_i2c_client->client, offset, len, buf);
    mutex_unlock(&inv_i2c_client->lock);

    return ret;
}

static int inv_i2c_smbus_write_block_data(struct inv_i2c_client_t *inv_i2c_client, u8 offset, int len, const u8 *buf)
{
    int ret = 0;

    check_p(inv_i2c_client);
    check_p(buf); 
    mutex_lock(&inv_i2c_client->lock);
    ret = i2c_smbus_write_i2c_block_data_retry(inv_i2c_client->client, offset, len, buf);
    mutex_unlock(&inv_i2c_client->lock);

    return ret;
}

/*it's used by cpld_sff_io_st_update_by_grp*/ 
static int ioin_update_byIdx(struct sku_t *sku, const struct sff_reg_t *sff_reg, unsigned long *bitmap)
{
    int ret = 0;
    u8 *buf = NULL;
    int buf_offset = 0;

    check_p(sku);
    check_p(sff_reg);
    check_p(bitmap);
    
    buf = (u8 *)(bitmap);
    buf_offset = sff_reg->port_1st / REG_PORT_NUM;

    if ((ret = cpld_reg_get(sku, sff_reg->cpld_id, sff_reg->offset, buf+buf_offset, sff_reg->size)) < 0) {
        return ret;
    }

    return 0;
}

static int cpld_sff_io_st_update_all(struct sku_t *sku, cpld_sff_type_t type)
{
    int ret = 0;
    int idx =0 ;
    struct sff_io_st_t *input_st = &(sku->sff_io_st[type]);
    const struct sff_reg_t *sff_reg_tbl = sff_io_reg_tbl[type];
    check_p(sku);
    check_p(sff_reg_tbl); 
    
    for (idx = 0; true != sff_reg_tbl[idx].is_end; idx++) {
        if ((ret = ioin_update_byIdx(sku, sff_reg_tbl+idx, &(input_st->bitmap))) < 0) {
            input_st->valid_mask = 0;
            return ret;
        } 
        set_bit(idx, &(input_st->valid_mask));
    }
    return 0;
}

static int cpld_sff_ioin_init(struct sku_t *sku)
{
    int ret = 0;

    check_p(sku);
    if ((ret = cpld_sff_io_st_update_all(sku, CPLD_SFF_PRS_TYPE)) < 0) {
        return ret;
    }
    if ((ret = cpld_sff_io_st_update_all(sku, CPLD_SFF_INTR_TYPE)) < 0) {
        return ret;
    }
    if ((ret = cpld_sff_io_st_update_all(sku, CPLD_SFF_RXLOS_TYPE)) < 0) {
        return ret;
    }
    if ((ret = cpld_sff_io_st_update_all(sku, CPLD_SFF_RXLOS2_TYPE)) < 0) {
        return ret;
    }
    return 0;
}    

static int cpld_sff_ioout_init(bool need_init, struct sku_t *sku)
{
    int ret = 0;

    check_p(sku);
    if (!need_init) {
        return 0;
    }
    if ((ret = sff_io_set(sku, CPLD_SFF_LPMODE_TYPE, sff_io_def_val[CPLD_SFF_LPMODE_TYPE])) < 0) {
        return ret;
    }
    
    if ((ret = sff_io_set(sku, CPLD_SFF_RESET_TYPE, sff_io_def_val[CPLD_SFF_RESET_TYPE])) < 0) {
        return ret;
    }
    
    if ((ret = sff_io_set(sku, CPLD_SFF_MODSEL_TYPE, sff_io_def_val[CPLD_SFF_MODSEL_TYPE])) < 0) {
        return ret;
    }
    if ((ret =  sku_sff_power_set(sku, SFF_POWER_DEF_VAL)) < 0) {
        return ret;
    }
    return 0;
}    

static int sfpdd_grp0_intr_hdlr(struct sff_intr_event_t *event, struct sku_t *sku)
{
    int ret = 0;
    struct cpld_reg_t *reg = NULL;
   
    check_p(sku);
    check_p(event);
    reg = &(event->st_clear_reg);
    if ((ret = cpld_sff_io_st_update(sku, CPLD_SFF_PRS_TYPE, SFP_DD_PRS_GRP0)) < 0) {
        return ret;
    }
    if ((ret = cpld_reg_set(sku, reg->cpld_id, reg->offset, &(reg->val), sizeof(reg->val))) < 0) {
        return ret;
    }
    return 0;
}    

static int sfpdd_grp1_intr_hdlr(struct sff_intr_event_t *event, struct sku_t *sku)
{
    int ret = 0;
    struct cpld_reg_t *reg = NULL;
   
    check_p(sku);
    check_p(event);
    reg = &(event->st_clear_reg);
    if ((ret = cpld_sff_io_st_update(sku, CPLD_SFF_PRS_TYPE, SFP_DD_PRS_GRP1)) < 0) {
        return ret;
    }
    if ((ret = cpld_reg_set(sku, reg->cpld_id, reg->offset, &(reg->val), sizeof(reg->val))) < 0) {
        return ret;
    }
    return 0;
     
}    

static int qsfpdd_grp0_intr_hdlr(struct sff_intr_event_t *event, struct sku_t *sku)
{
    int ret = 0;
    struct cpld_reg_t *reg = NULL;
   
    check_p(sku);
    check_p(event);
    reg = &(event->st_clear_reg);
    if ((ret = cpld_sff_io_st_update(sku, CPLD_SFF_PRS_TYPE, QSFP_DD_PRS_GRP0)) < 0) {
        return ret;
    }
    if ((ret = cpld_reg_set(sku, reg->cpld_id, reg->offset, &(reg->val), sizeof(reg->val))) < 0) {
        return ret;
    }
    SKU_LOG_DBG("done!\n");
    return 0;
}    

static int sfpdd_rxlos_grp0_intr_hdlr(struct sff_intr_event_t *event, struct sku_t *sku)
{
    int ret = 0;
   
    check_p(sku);
    check_p(event);
    if ((ret = cpld_sff_io_st_update(sku, CPLD_SFF_RXLOS_TYPE, SFP_DD_RXLOS_GRP0)) < 0) {
        return ret;
    }
    if ((ret = cpld_sff_io_st_update(sku, CPLD_SFF_RXLOS2_TYPE, SFP_DD_RXLOS2_GRP0)) < 0) {
        return ret;
    }
    return 0;
}    

static int sfpdd_rxlos_grp1_intr_hdlr(struct sff_intr_event_t *event, struct sku_t *sku)
{
    int ret = 0;
   
    check_p(sku);
    check_p(event);

    if ((ret = cpld_sff_io_st_update(sku, CPLD_SFF_RXLOS_TYPE, SFP_DD_RXLOS_GRP1)) < 0) {
        return ret;
    }
    if ((ret = cpld_sff_io_st_update(sku, CPLD_SFF_RXLOS2_TYPE, SFP_DD_RXLOS2_GRP1)) < 0) {
        return ret;
    }
    return 0;
     
}    

static int qsfpdd_rxlos_grp0_intr_hdlr(struct sff_intr_event_t *event, struct sku_t *sku)
{
    int ret = 0;
   
    check_p(sku);
    check_p(event);
    
    if ((ret = cpld_sff_io_st_update(sku, CPLD_SFF_INTR_TYPE, QSFP_DD_INTR_GRP0)) < 0) {
        return ret;
    }
    //SKU_LOG_DBG("done!\n");
    return 0;
}    

static int cpld_sff_io_st_update(struct sku_t *sku, cpld_sff_type_t type, int idx)
{
    int ret = 0;
    struct sff_io_st_t *input_st = NULL;
    const struct sff_reg_t *sff_reg_tbl = sff_io_reg_tbl[type];
    
    check_p(sku);
    input_st = &(sku->sff_io_st[type]);
    check_p(sff_reg_tbl); 
    
    if ((ret = ioin_update_byIdx(sku, sff_reg_tbl+idx, &(input_st->bitmap))) < 0) {
        input_st->valid_mask = 0;
        return ret;
    } 

    set_bit(idx, &(input_st->valid_mask));
    SKU_LOG_DBG("type %d 0x%lx\n", type, input_st->bitmap);
    return 0;
}

static int cpld_ioin_get_cache(cpld_sff_type_t type, int lc_id, unsigned long *bitmap)
{
    (void)lc_id;
    
    check_p(bitmap);
    if (sff_io_st_valid_mask[type] != banyan8T_sku2.sff_io_st[type].valid_mask) {
        return -EBADRQC;
    }
    *bitmap = banyan8T_sku2.sff_io_st[type].bitmap;
    return 0;
}

static int sff_io_set(struct sku_t *sku, cpld_sff_type_t type, unsigned long bitmap)
{
    int ret = 0;
    const struct sff_reg_t *sff_reg = sff_io_reg_tbl[type];
    int i = 0;
    u8 *buf = NULL;
    unsigned long val = 0;
    check_p(sku);
    check_p(sff_reg);
    
    for (i = 0; true != sff_reg[i].is_end; i++) {
        
        val = (bitmap >> sff_reg[i].port_1st) & bitmask[sff_reg[i].size];
        buf = (u8 *)(&val);
        if ((ret = cpld_reg_set(sku, sff_reg[i].cpld_id,
                   sff_reg[i].offset,
                   buf,
                   sff_reg[i].size)) < 0) {
            return ret;
        }
    }

    return 0;
}
static int sku_sff_power_set(struct sku_t *sku, unsigned long bitmap)
{
    int ret = 0;
    const struct sff_reg_t *sff_reg = sff_power_reg_tbl;
    int i = 0;
    int bit = 0;
    int port = 0;
    u8 *buf = NULL;
    unsigned long remap = 0; 
    bool is_set0 = false;
    bool is_set1 = false;
    int offset = 0;
    check_p(sku);
    check_p(sff_reg);
    
    SKU_LOG_DBG("bitmap:0x%lx\n", bitmap);
/*need to do remapping here since two sfp-dd module share one power switch, qsfp-dd module has its own power switch*/
    for (bit = 0, port = SKU_SFP_DD_PORT_FIRST; port <= SKU_SFP_DD_PORT_LAST; port+=2) {
        is_set0 = test_bit(port, &bitmap);
        is_set1 = test_bit(port+1, &bitmap);
        
        if (is_set0 && is_set1) {
            set_bit(bit, &remap);
        }
        if (!is_set0 && !is_set1) {
            clear_bit(bit, &remap);
        }
        bit++;
    }
    for (port = SKU_QSFP_DD_PORT_FIRST; port <= SKU_QSFP_DD_PORT_LAST; port++) {

        if (test_bit(port, &bitmap)) {
            set_bit(bit, &remap);
        } else {
            clear_bit(bit, &remap);
        }
        bit++;
    }
     
    SKU_LOG_DBG("remap:%lx\n", remap);
    for (i = 0; true != sff_reg[i].is_end; i++) {
        buf = (u8 *)(&remap);
        if ((ret = cpld_reg_set(sku, sff_reg[i].cpld_id,
                   sff_reg[i].offset,
                   buf+offset,
                   sff_reg[i].size)) < 0) {
            return ret;
        }
        offset += sff_reg[i].size;
    }

    return 0;
}

static int cpld_io_set(cpld_sff_type_t type, int lc_id, unsigned long bitmap)
{
    (void)lc_id;
    return sff_io_set(&banyan8T_sku2, type, bitmap);
}

static int cpld_reg_set(struct sku_t *sku, int cpld_id, u8 offset, const u8 *buf, int size)
{
    int ret = 0;
    struct inv_i2c_client_t *inv_i2c_client = NULL;
   
    check_p(sku);
    check_p(buf);
    if (!p_valid(inv_i2c_client = inv_i2c_client_find(sku, (sku->cpld_i2c_config)+cpld_id))) {
        return -EBADRQC;
    }
    if ((ret = inv_i2c_smbus_write_block_data(inv_i2c_client,
               offset,
               size,
               buf)) < 0) {
        return ret;
    }
    return 0;
}    

static int cpld_reg_get(struct sku_t *sku, int cpld_id, u8 offset, u8 *buf, int size)
{
    int ret = 0;
    struct inv_i2c_client_t *inv_i2c_client = NULL;
    
    check_p(sku);
    check_p(buf);
    if (!p_valid(inv_i2c_client = inv_i2c_client_find(sku, (sku->cpld_i2c_config)+cpld_id))) {
        return -EBADRQC;
    }
    if ((ret = inv_i2c_smbus_read_block_data(inv_i2c_client,
               offset,
               size,
               buf)) < 0) {
        return ret;
    }
    return 0;
}    

static int sku_sff_power_get(struct sku_t *sku, unsigned long *bitmap)
{
    int ret = 0;
    const struct sff_reg_t *sff_reg = sff_power_reg_tbl;
    int i = 0;
    int port = 0;
    int bit = 0;
    u8 *buf = NULL;
    int buf_offset = 0;
    unsigned long remap = 0;
    
    check_p(sku);
    check_p(bitmap);
    check_p(sff_reg);
    
    buf = (u8 *)(&remap);
    for (i = 0; true != sff_reg[i].is_end; i++) {

        if ((ret = cpld_reg_get(sku, sff_reg[i].cpld_id, sff_reg[i].offset, buf+buf_offset, sff_reg[i].size)) < 0) {
            return ret;
        }
        buf_offset += sff_reg[i].size;
    }
    SKU_LOG_DBG("remap:0x%lx\n", remap);
    /*need to do remapping here since two sfp-dd module share one power switch, qsfp-dd module has its own power switch*/
    for (bit = 0, port = SKU_SFP_DD_PORT_FIRST; port <= SKU_SFP_DD_PORT_LAST; port+=2) {
        
        if (test_bit(bit, &remap)) {
            set_bit(port, bitmap);
            set_bit(port+1, bitmap);
        } else {
            clear_bit(port, bitmap);
            clear_bit(port+1, bitmap);
        }
        bit++;
    }     
    for (port = SKU_QSFP_DD_PORT_FIRST; port <= SKU_QSFP_DD_PORT_LAST; port++) {
        
        if (test_bit(bit, &remap)) {
            set_bit(port, bitmap);
        } else {
            clear_bit(port, bitmap);
        }
        bit++;
    }     
    SKU_LOG_DBG("bitmap:0x%lx\n", *bitmap);
    return 0;
}

static int sff_io_get(struct sku_t *sku, cpld_sff_type_t type, unsigned long *bitmap)
{
    int ret = 0;
    const struct sff_reg_t *sff_reg = sff_io_reg_tbl[type];
    int i = 0;
    u8 *buf = NULL;
    int buf_offset = 0;
    unsigned long val = 0;
    buf = (u8 *)(&val);
    
    check_p(sku);
    check_p(bitmap);
    check_p(sff_reg);
    
    for (i = 0; true != sff_reg[i].is_end; i++) {

        buf_offset = sff_reg[i].port_1st / REG_PORT_NUM;
        if ((ret = cpld_reg_get(sku, sff_reg[i].cpld_id, sff_reg[i].offset, buf+buf_offset, sff_reg[i].size)) < 0) {
            return ret;
        }
    }

    *bitmap = val;
    return 0;
}

static int cpld_io_get(cpld_sff_type_t type, int lc_id, unsigned long *bitmap)
{
    (void)lc_id;
    return sff_io_get(&banyan8T_sku2, type, bitmap);
}

static int cpld_prs_get(int lc_id, unsigned long *bitmap)
{
    return cpld_ioin_get_cache(CPLD_SFF_PRS_TYPE, lc_id, bitmap);
}

static int cpld_intr_get(int lc_id, unsigned long *bitmap)
{
    return cpld_ioin_get_cache(CPLD_SFF_INTR_TYPE, lc_id, bitmap);
}

static int cpld_rxlos_get(int lc_id, unsigned long *bitmap)
{
    return cpld_ioin_get_cache(CPLD_SFF_RXLOS_TYPE, lc_id, bitmap);
}

static int cpld_rxlos2_get(int lc_id, unsigned long *bitmap)
{
    return cpld_ioin_get_cache(CPLD_SFF_RXLOS2_TYPE, lc_id, bitmap);
}
#if 0 /*reserved*/
static int cpld_txfault_get(int lc_id, unsigned long *bitmap)
{
    return cpld_ioin_get_cache(CPLD_SFF_TXFAULT_TYPE, lc_id, bitmap);
}

static int cpld_txfault2_get(int lc_id, unsigned long *bitmap)
{
    return cpld_ioin_get_cache(CPLD_SFF_TXFAULT2_TYPE, lc_id, bitmap);
}
#endif
static int cpld_reset_set(int lc_id, unsigned long bitmap)
{
    return cpld_io_set(CPLD_SFF_RESET_TYPE, lc_id, bitmap);
}

static int cpld_lpmode_set(int lc_id, unsigned long bitmap)
{
    return cpld_io_set(CPLD_SFF_LPMODE_TYPE, lc_id, bitmap);
}

static int cpld_modsel_set(int lc_id, unsigned long bitmap)
{
    return cpld_io_set(CPLD_SFF_MODSEL_TYPE, lc_id, bitmap);
}

static int cpld_reset_get(int lc_id, unsigned long *bitmap)
{
    return cpld_io_get(CPLD_SFF_RESET_TYPE, lc_id, bitmap);
}

static int cpld_lpmode_get(int lc_id, unsigned long *bitmap)
{
    return cpld_io_get(CPLD_SFF_LPMODE_TYPE, lc_id, bitmap);
}

static int cpld_modsel_get(int lc_id, unsigned long *bitmap)
{
    return cpld_io_get(CPLD_SFF_MODSEL_TYPE, lc_id, bitmap);
}
/*sff extra io*/
static int cpld_power_get(int lc_id, unsigned long *bitmap)
{
    return sku_sff_power_get(&banyan8T_sku2, bitmap);
}

static int cpld_power_set(int lc_id, unsigned long bitmap)
{
    return sku_sff_power_set(&banyan8T_sku2, bitmap);
}

static int inv_i2c_client_init(int ch,  struct i2c_client **client)
{
    struct i2c_adapter *adap = NULL;

    check_p(client);
    adap = i2c_get_adapter(ch);
    if (!p_valid(adap)) {
        SKU_LOG_ERR("get adapter fail ch:%d\n", ch);
        return -EBADRQC;
    }

    SKU_LOG_DBG("get adapter ok ch:%d\n", ch);
    (*client)->adapter = adap;

    return 0;
}

static void inv_i2c_client_deinit_all(struct sku_t *self)
{
    struct i2c_client *client = NULL;
    int id = 0;
    
    for (id = 0; id < CH_ID_NUM; id++) {
        client = self->inv_i2c_client[id].client;
        if (p_valid(client)) {
            if (p_valid(client->adapter)) {
                i2c_put_adapter(client->adapter);
                SKU_LOG_DBG("put_adapter:%d\n", id);
            }
        }
    }
}
static void inv_i2c_clients_destroy(struct sku_t *self)
{
    int ch_id = 0;

    if (p_valid(i2c_client_obj)) {
        kfree(i2c_client_obj);
    }
    for (ch_id = 0; ch_id < CH_ID_NUM; ch_id++) {
        self->inv_i2c_client[ch_id].client = NULL;
    }
}

static int inv_i2c_clients_create(struct sku_t *self)
{
    int ch_id = 0;
    struct i2c_client *client = NULL;


    check_p(self);
    client = kzalloc(sizeof(struct i2c_client)*CH_ID_NUM, GFP_KERNEL);
    if (!p_valid(client)) {
        return -EBADRQC;
    }
    i2c_client_obj = client;
    /*build a link*/
    for (ch_id = 0; ch_id < CH_ID_NUM; ch_id++) {

        self->inv_i2c_client[ch_id].client = &i2c_client_obj[ch_id];
    }
    return 0;
}
static int inv_i2c_client_init_all(struct sku_t *self)
{
    int ch_id = 0;
    struct inv_i2c_client_t *inv_i2c_client = NULL;
    int ret = 0;

    check_p(self);
    for (ch_id = 0; ch_id < CH_ID_NUM; ch_id++) {

        inv_i2c_client = &(self->inv_i2c_client[ch_id]);
        if ((ret = inv_i2c_client_init(i2c_ch_tbl[ch_id], &(inv_i2c_client->client))) < 0) {
            break;
        }
        mutex_init(&(inv_i2c_client->lock));

    }
    if (ret < 0) {
        inv_i2c_client_deinit_all(self);
        return ret;
    }

    return 0;
}
static void config_load(struct sku_t *sku)
{
    int cpld_id = 0;

    for (cpld_id = 0; cpld_id < CPLD_ID_NUM; cpld_id++) {
        memcpy(sku->cpld_i2c_config+cpld_id, cpld_i2c_config+cpld_id, sizeof(struct i2c_config_t));
    }
    memcpy(&sku->mux_i2c_config, &mux_i2c_config, sizeof(struct i2c_config_t));
    sku->prs_intr_gpio = PRS_INTR_GPIO_NUM;
    sku->rxlos_intr_gpio = RXLOS_INTR_GPIO_NUM;
    sku->mux_rst_gpio = MUX_RST_GPIO_NUM;
}
/*dir 1:in 0:out */
static int cpld_gpio_init(int gpio_no)
{
    int ret = 0;

    if ((ret = gpio_is_valid(gpio_no)) < 0) {

        SKU_LOG_ERR("gpio:%d is invalid ret:%d\n", gpio_no, ret);
        return ret;
    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}

static int sff_intr_enable(struct sku_t *sku, bool on)
{
    int ret = 0;
    u8 enable = 0;
    check_p(sku);
    if (on) {
        enable = INTR_EN_VAL;
    }
    if ((ret = cpld_reg_set(sku, 
                            intr_en_reg.cpld_id,
                            intr_en_reg.offset,
                            &(enable),
                            sizeof(enable))) < 0) {

        return ret;
    }
    if ((ret = cpld_reg_set(sku, 
                            intr2_en_reg.cpld_id,
                            intr2_en_reg.offset,
                            &(enable),
                            sizeof(enable))) < 0) {

        return ret;
    }
    return 0;
}    

static int sku_mux_failed_ch_get(int lc_id, unsigned long *ch)
{
    u64 failed_ch = 0;
    struct mux_func_t *func = &(banyan8T_sku2.mux_func); 
    (void)lc_id;
    check_p(ch);
    func->pca->current_channel_get(func->dev, &failed_ch);
    *ch = (unsigned long)failed_ch;
    return 0;
}    

static int sku_mux_blocked_ch_set(int lc_id, unsigned long ch)
{
    struct mux_func_t *func = &(banyan8T_sku2.mux_func); 
    (void)lc_id;
    func->pca->block_channel_set(func->dev, (u64)ch);
    return 0;
}    

static int sku_mux_fail_set(int lc_id, bool is_fail)
{
    struct mux_func_t *func = &(banyan8T_sku2.mux_func); 
    (void)lc_id;
    func->pca->mux_fail_set(func->dev, is_fail);
    return 0;
}

static int sku_mux_fail_get(int lc_id, bool *is_fail)
{
    struct mux_func_t *func = &(banyan8T_sku2.mux_func); 
    (void)lc_id;
    check_p(is_fail);
    func->pca->mux_fail_get(func->dev, is_fail);
    return 0;
}

static int sku_mux_ch_to_port(int lc_id, int ch)
{
    struct mux_func_t *func = &(banyan8T_sku2.mux_func); 
    (void)lc_id;
    
    if (ch >= func->ch_2_port->size) {
        SKU_LOG_ERR("ch out of range %d\n", ch);
        return -1;
    }
    return func->ch_2_port->tbl[ch];
}    

static int sku_mux_port_to_ch(int lc_id, int port)
{
    int ch = 0;
    struct mux_func_t *func = &(banyan8T_sku2.mux_func); 
    int ch_num = func->ch_2_port->size;
    (void)lc_id;
    
    for (ch = 0; ch < ch_num; ch++) {
        if (func->ch_2_port->tbl[ch] == port) {
            break;
        }            
    }
    if (ch >= ch_num) {
        SKU_LOG_ERR("cant find ch\n");
        return -1;
    }
    return ch;
}
static int mux_func_load(struct mux_func_t *func)
{
    struct device *dev = NULL;
    struct i2c_client *client = NULL;
    struct i2c_mux_core *muxc = NULL;
    struct pca954x *pca = NULL;
    
    if (!p_valid(dev = find_dev(INV_MUX_NAME, &i2c_bus_type))) {
        return -EBADRQC;
    }

    client = i2c_verify_client(dev);
    muxc = i2c_get_clientdata(client);
    if (!p_valid(pca = i2c_mux_priv(muxc))) {
        return -EBADRQC;
    }
    
    func->dev = dev;
    func->pca = pca;
    if (!p_valid(func->pca->mux_fail_get)) {
        SKU_LOG_ERR("mux func NOT found\n");
        return -EBADRQC;
    }

    func->ch_2_port = &banyan8T_mux_ch_2_port_map;  
    return 0;
}    

static int sku_init(int platform_id, int io_no_init)
{
    int ret = 0;
    int gpio_base = 0;
    bool ioout_need_init = false;
    (void)platform_id;
    ioout_need_init = ((io_no_init == 0) ? true : false);
    config_load(&banyan8T_sku2);

    if (gpio_base_find(&gpio_base) < 0) {
        SKU_LOG_ERR("find gpio name fail\n");
        goto err_exit;
    }
    SKU_LOG_DBG("gpio_base:%d\n", gpio_base);
    banyan8T_sku2.prs_intr_gpio += gpio_base;
    banyan8T_sku2.rxlos_intr_gpio += gpio_base;
    banyan8T_sku2.mux_rst_gpio += gpio_base;
    
    if (mux_func_load(&banyan8T_sku2.mux_func) < 0) {
        SKU_LOG_ERR("mux_func_load fail\n");
        goto err_exit;
    }
    if (inv_i2c_clients_create(&banyan8T_sku2) < 0) {
        SKU_LOG_ERR("inv_i2c_clients_create fail\n");
        goto err_exit;
    }

    if (inv_i2c_client_init_all(&banyan8T_sku2) < 0) {
        SKU_LOG_ERR("inv_i2c_client_init_all fail\n");
        goto kfree_inv_i2c_client;
    }

    if (cpld_gpio_init(banyan8T_sku2.prs_intr_gpio) < 0) {
        goto deinit_inv_i2c_client;
    }
    
    if (cpld_gpio_init(banyan8T_sku2.rxlos_intr_gpio) < 0) {
        goto deinit_inv_i2c_client;
    }

    if (cpld_gpio_init(banyan8T_sku2.mux_rst_gpio) < 0) {
        goto deinit_inv_i2c_client;
    }
    if (cpld_sff_ioin_init(&banyan8T_sku2) < 0) {
        SKU_LOG_ERR("cpld_sff_ioin_init fail\n");
        goto deinit_inv_i2c_client;
    }
    if (cpld_sff_ioout_init(ioout_need_init, &banyan8T_sku2) < 0) {
        SKU_LOG_ERR("cpld_sff_ioout_init fail\n");
        goto deinit_inv_i2c_client;
    }
    if (sff_intr_enable(&banyan8T_sku2, true) < 0) {
        SKU_LOG_ERR("sff_intr_enable fail\n");
        goto deinit_inv_i2c_client;
    }
    SKU_LOG_INFO("ok\n");
    return 0;
deinit_inv_i2c_client:
    inv_i2c_client_deinit_all(&banyan8T_sku2);
kfree_inv_i2c_client:
    inv_i2c_clients_destroy(&banyan8T_sku2);
err_exit:
    ret = -EBADRQC;
    return ret;

}

static void sku_deinit(void)
{
    sff_intr_enable(&banyan8T_sku2, false); 
    inv_i2c_client_deinit_all(&banyan8T_sku2);
    inv_i2c_clients_destroy(&banyan8T_sku2);
    SKU_LOG_INFO("ok\n");
}

static inline bool intr_is_asserted(int gpio_no)
{
    int val = 0;
    val = gpio_get_value(gpio_no);
    return ((val == 0) ? true : false);
}    

static int prs_intr_hdlr(struct sku_t *sku)
{
    int ret = 0;
    u8 intr_st = 0;
    struct sff_intr_event_t *event = NULL;
    /*no interrupt events*/ 
    if (!intr_is_asserted(sku->prs_intr_gpio)) {
        return 0;
    }
    prs_intr_cnt++;
    SKU_LOG_DBG("intr occurs: %ld\n", prs_intr_cnt);
    if ((ret = cpld_reg_get(sku, 
                            intr_st_reg.cpld_id,
                            intr_st_reg.offset,
                            &intr_st,
                            sizeof(intr_st))) < 0) {

        return ret;
    }
    /*reverse negative logic to positive logic*/    
    intr_st = ~intr_st;
    if (intr_st & CPLD_SFF_INTR_ST_SFPDD_GRP0_BITMASK) {
        event = &sff_intr_event[SFPDD_GRP0_INT_ID];
        if ((ret = event->intr_hdlr(event, sku)) < 0) {
            return ret;
        }
    }
    if (intr_st & CPLD_SFF_INTR_ST_SFPDD_GRP1_BITMASK) {
        event = &sff_intr_event[SFPDD_GRP1_INT_ID];
        if ((ret = event->intr_hdlr(event, sku)) < 0) {
            return ret;
        }
    }
    if (intr_st & CPLD_SFF_INTR_ST_QSFPDD_GRP0_BITMASK) {
        event = &sff_intr_event[QSFPDD_GRP0_INT_ID];
        if ((ret = event->intr_hdlr(event, sku)) < 0) {
            return ret;
        }
    }
    
    return 0;
}

static int rxlos_intr_hdlr(struct sku_t *sku)
{
    int ret = 0;
    u8 intr_st = 0;
    int cur_lv = 0;
    struct sff_intr_event_t *event = NULL;
    rxlos_intr_cnt++;
    cur_lv = gpio_get_value(sku->rxlos_intr_gpio);

    if (cur_lv == 1) {
        if (pre_rxlos_intr_lv == 0) {
            event = &sff_intr_event[SFPDD_RXLOS_GRP0_INT_ID];
            if ((ret = event->intr_hdlr(event, sku)) < 0) {
                return ret;
            }
            event = &sff_intr_event[SFPDD_RXLOS_GRP1_INT_ID];
            if ((ret = event->intr_hdlr(event, sku)) < 0) {
                return ret;
            }
            event = &sff_intr_event[QSFPDD_RXLOS_GRP0_INT_ID];
            if ((ret = event->intr_hdlr(event, sku)) < 0) {
                return ret;
            }
            SKU_LOG_DBG("update rxlos bitmap, intr deasserted %ld\n", rxlos_intr_cnt);
        }

    } else {    
        SKU_LOG_DBG("intr occurs: %ld\n", rxlos_intr_cnt);
        if ((ret = cpld_reg_get(sku, 
                                intr_st_reg.cpld_id,
                                intr_st_reg.offset,
                                &intr_st,
                                sizeof(intr_st))) < 0) {

            return ret;
        }
        /*reverse negative logic to positive logic*/    
        intr_st = ~intr_st;
        if (intr_st & CPLD_SFF_INTR_ST_SFPDD_RXLOS_GRP0_BITMASK) {
            event = &sff_intr_event[SFPDD_RXLOS_GRP0_INT_ID];
            if ((ret = event->intr_hdlr(event, sku)) < 0) {
                return ret;
            }
        }
        if (intr_st & CPLD_SFF_INTR_ST_SFPDD_RXLOS_GRP1_BITMASK) {
            event = &sff_intr_event[SFPDD_RXLOS_GRP1_INT_ID];
            if ((ret = event->intr_hdlr(event, sku)) < 0) {
                return ret;
            }
        }
        if (intr_st & CPLD_SFF_INTR_ST_QSFPDD_RXLOS_GRP0_BITMASK) {
            event = &sff_intr_event[QSFPDD_RXLOS_GRP0_INT_ID];
            if ((ret = event->intr_hdlr(event, sku)) < 0) {
                return ret;
            }
        }
    }
    pre_rxlos_intr_lv = cur_lv;
    return 0;
}

static int sku_hdlr(void)
{
    int ret = 0;
    struct sku_t *sku = &banyan8T_sku2;

    if ((ret = prs_intr_hdlr(sku)) < 0) {
        return ret;
    }
    if ((ret = rxlos_intr_hdlr(sku)) < 0) {
        return ret;
    }
    return 0;
}

static int sku_sff_get_ready_action(int lc_id, int port)
{
    (void)lc_id;
    /*do nothing*/
    return 0;
}

static int sku_sff_detected_action(int lc_id, int port)
{
    (void)lc_id;
    /*do nothing*/
    return 0;
}
