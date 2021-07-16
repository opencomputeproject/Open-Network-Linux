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
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/err.h>
#include "sku_common.h"

#define INTR_MODE

static const char *lc_led_str[LC_LED_CTRL_NUM] = {

    [LC_LED_CTRL_GREEN_ON] = "LC_LED_GREEN_ON",
    [LC_LED_CTRL_RED_ON] = "LC_LED_RED_ON",
};
const char *lc_type_name[LC_TYPE_NUM] = {
    "UNKNOWN_TYPE",
    "LC_100G",
    "LC_400G",
};

enum {
    LC_100G_ID = 0x1,
    LC_400G_ID,
};

#define PHY_NUM_PER_LC  (8)
#define PHY_READY_BITMASK ((1L << PHY_NUM_PER_LC) - 1)

#define CARD_NUM (4)
#define PORT_GROUP_NUM (2) /*per lc_id*/
#define TMP_DEV_NUM (2)    /*per lc_id*/
#define MUX_L2_NUM (4)     /*per lc_id*/

#define SFF_DATA_VALID_MASK (0x3uL)

#define SWITCH_CPLD_ADDR (0x33)
#define CPLD1_ADDR (0x33)
#define CPLD2_ADDR  (0x66)
#define IOEXP1_ADDR (0x20)
#define IOEXP2_ADDR (0x21)
#define TMP1_ADDR (0x4a)
#define TMP2_ADDR (0x4d)
/*pca9546*/
#define MUX_L1_ADDR (0x71)
/*pca9548*/
#define MUX_L2_ADDR (0x72)
//#define PCA9548_CONTROL_OFFSET (0)

/*INTR gpio*/
//#define LC_INTR_GPIO (12)
#define LC_INTR_GPIO (45)
#define LC1_SFF_INTR_GPIO (44)
#define LC2_SFF_INTR_GPIO (61)
#define LC3_SFF_INTR_GPIO (26)
#define LC4_SFF_INTR_GPIO (27)

/*io expander*/
#define IOEXP_INPUT_OFFSET (0x0)

/*switch cpld*/
#define SWITCH_CPLD_LC_ST_INTR_OFFSET (0x37)
#define SWITCH_CPLD_LC1_ST_OFFSET (0x38)
#define SWITCH_CPLD_LC2_ST_OFFSET (0x3a)
#define SWITCH_CPLD_LC3_ST_OFFSET (0x3c)
#define SWITCH_CPLD_LC4_ST_OFFSET (0x3e)

#define SWITCH_CPLD_LC_TEMP_INTR_N_BIT (6)
#define SWITCH_CPLD_LC_PRS_INTR_N_BIT (3)
#define SWITCH_CPLD_LC_MUX_INTR_N_BIT (2)
#define SWITCH_CPLD_LC_EJ_L_N_BIT (1)
#define SWITCH_CPLD_LC_EJ_R_N_BIT (0)

#define SWITCH_CPLD_LC1_CTRL_OFFSET (0x39)
#define SWITCH_CPLD_LC2_CTRL_OFFSET (0x3b)
#define SWITCH_CPLD_LC3_CTRL_OFFSET (0x3d)
#define SWITCH_CPLD_LC4_CTRL_OFFSET (0x3f)
#define SWITCH_CPLD_INTR_ST_OFFSET (0x44)
#define SWITCH_CPLD_INTR_EN_OFFSET (0x45)

#define SWICH_CPLD_LC_PWR_CONTROL_BIT (1)
#define SWICH_CPLD_LC_CPU_PLTRST_N_BIT (0)

/*line lc_id cpld reg offset*/
#define PCB_VER_OFFSET  (0x0)
#define LC_ID_BIT_S   (4)
#define LC_ID_BIT_NUM   (4)

#define I2C_MUX_RST_OFFSET (0x0b)

#define RST_PCA9548_D_OFFSET (0x0b)

#define RST_PCA9548_1_N_BIT (0)
#define RST_PCA9548_2_N_BIT (1)
#define RST_PCA9548_3_N_BIT (2)
#define RST_PCA9548_4_N_BIT (3)
#define RST_PCA9546_N_BIT (4)

#define LC_CPLD_PHY_RST_OFFSET  (0x0a)
#define LC_CPLD_LED_DIAG_OFFSET (0xf)
#define LC_CPLD_LED_BOOT_AMBER_BIT (0)
#define LC_CPLD_SFF_RESET_OFFSET   (0x12)
#define LC_CPLD_SFF_INTR_OFFSET  (0x14)
#define LC_CPLD_SFF_PRS_OFFSET  (0x16)
#define LC_CPLD_SFF_LPMODE_OFFSET   (0x18)
#define LC_CPLD_SFF_POWER_OFFSET    (0x1f)
#define LC_CPLD_SFF_OC_OFFSET   (0x21)

#define LC_ST_LED_OFFSET (0x0d)

/*lc power status check*/
#define LC_CPLD_DC_DC_ST1_OFFSET (0x1d)
#define LC_CPLD_DC_DC_ST2_OFFSET (0x1e)

/*cpld1 lc power control*/
#define LC_CPLD_MISC_CONTROL1_OFFSET (0x27)
#define LC_CPLD_UCD_PMBUS_CNTRL_R_BIT (3)

#define LC_CPLD_MISC_CONTROL2_OFFSET (0x28)
#define LC_CPLD_LC1_PWR_CONTROL_BIT (1)

/*cpld transceiver interrupt*/
#define LC_CPLD_SFF_INTR_ST_OFFSET (0x2a)
#define LC_CPLD_SFF_INTR_CPLD2_PRS_BIT (0)
#define LC_CPLD_SFF_INTR_CPLD2_INTR_BIT (1)
#define LC_CPLD_SFF_INTR_CPLD2_OC_BIT (2)
#define LC_CPLD_SFF_INTR_RESERVED1 (3)
#define LC_CPLD_SFF_INTR_CPLD1_PRS_BIT (4)
#define LC_CPLD_SFF_INTR_CPLD1_INTR_BIT (5)
#define LC_CPLD_SFF_INTR_CPLD1_OC_BIT (6)
#define LC_CPLD_SFF_INTR_RESERVED2 (7)
#define LC_CPLD_SFF_INTR_NUM (8)
/*cpld clear sff prs interrupt status*/
#define LC_CPLD_SFF_PRS_INTR_CLEAR_OFFSET (0x2d)


#define PORT_IDX_NUM (16)
#define U16_MASK (((1 << PORT_IDX_NUM) - 1))
/*for register <= 32 bits*/
#define inv_set_bit(bit, reg)  ((reg) | (1 << bit))
#define inv_clear_bit(bit, reg)  ((reg) & (~(1 << bit)))

/*side band signaling output default value*/
#define LC_CPLD_DEF_RESET_VAL   ((1 << PORT_IDX_NUM) - 1)
#define LC_CPLD_DEF_LPMODE_VAL   ((1 << PORT_IDX_NUM) - 1)
#define LC_CPLD_DEF_POWER_VAL   ((1 << PORT_IDX_NUM) - 1)

typedef enum {
    LC_SWITCH_CH_ID,
    LC1_SFF_CH_ID,
    LC2_SFF_CH_ID,
    LC3_SFF_CH_ID,
    LC4_SFF_CH_ID,
    LC1_TMP_CH_ID,
    LC2_TMP_CH_ID,
    LC3_TMP_CH_ID,
    LC4_TMP_CH_ID,
    LC1_MUX_L1_CH_ID,
    LC1_MUX_L2_1_CH_ID,
    LC1_MUX_L2_2_CH_ID,
    LC1_MUX_L2_3_CH_ID,
    LC1_MUX_L2_4_CH_ID,
    LC2_MUX_L1_CH_ID,
    LC2_MUX_L2_1_CH_ID,
    LC2_MUX_L2_2_CH_ID,
    LC2_MUX_L2_3_CH_ID,
    LC2_MUX_L2_4_CH_ID,
    LC3_MUX_L1_CH_ID,
    LC3_MUX_L2_1_CH_ID,
    LC3_MUX_L2_2_CH_ID,
    LC3_MUX_L2_3_CH_ID,
    LC3_MUX_L2_4_CH_ID,
    LC4_MUX_L1_CH_ID,
    LC4_MUX_L2_1_CH_ID,
    LC4_MUX_L2_2_CH_ID,
    LC4_MUX_L2_3_CH_ID,
    LC4_MUX_L2_4_CH_ID,
    LC_CH_ID_NUM,
} lc_ch_id_t;

int lcChTbl[LC_CH_ID_NUM] = {
    [LC_SWITCH_CH_ID] = 41,
    [LC1_SFF_CH_ID] = 6,
    [LC2_SFF_CH_ID] = 7,
    [LC3_SFF_CH_ID] = 8,
    [LC4_SFF_CH_ID] = 9,
    [LC1_TMP_CH_ID] = 50,
    [LC2_TMP_CH_ID] = 51,
    [LC3_TMP_CH_ID] = 52,
    [LC4_TMP_CH_ID] = 53,
    [LC1_MUX_L1_CH_ID] = 81,
    [LC1_MUX_L2_1_CH_ID] = 85,
    [LC1_MUX_L2_2_CH_ID] = 86,
    [LC1_MUX_L2_3_CH_ID] = 87,
    [LC1_MUX_L2_4_CH_ID] = 88,
    [LC2_MUX_L1_CH_ID] = 82,
    [LC2_MUX_L2_1_CH_ID] = 89,
    [LC2_MUX_L2_2_CH_ID] = 90,
    [LC2_MUX_L2_3_CH_ID] = 91,
    [LC2_MUX_L2_4_CH_ID] = 92,
    [LC3_MUX_L1_CH_ID] = 83,
    [LC3_MUX_L2_1_CH_ID] = 93,
    [LC3_MUX_L2_2_CH_ID] = 94,
    [LC3_MUX_L2_3_CH_ID] = 95,
    [LC3_MUX_L2_4_CH_ID] = 96,
    [LC4_MUX_L1_CH_ID] = 84,
    [LC4_MUX_L2_1_CH_ID] = 97,
    [LC4_MUX_L2_2_CH_ID] = 98,
    [LC4_MUX_L2_3_CH_ID] = 99,
    [LC4_MUX_L2_4_CH_ID] = 100,
};
#define LM75_REG_TEMP       (0x00)
#define LM75_REG_CONF       (0x01)
#define LM75_REG_LOW       (0x02)
#define LM75_REG_HIGH        (0x03)
#define LM75_RESOLUTION     (9)

#define TEMP_CHECK_NUM (5)

typedef enum {
    TEMP_UNKNOWN,
    TEMP_INPUT,
    TEMP_TH_L,
    TEMP_TH_H,
} temp_conf_type_t;

struct lc_config_t {
    int lc_ch_id;
    u8 addr;
};

typedef union {

    u16 reg[4];
    unsigned long data;
} u64_format_t;

struct io_data_t {

    u32 buf;
    u8 valid;
};
typedef enum {
    LC_ST_PRS_TYPE,
    LC_ST_OVER_TEMP_TYPE,
    LC_ST_EJ_R_TYPE,
    LC_ST_EJ_L_TYPE,
    LC_ST_TYPE_NUM,

} lc_st_type_t;
static const char *lc_st_name[LC_ST_TYPE_NUM] = {
    [LC_ST_PRS_TYPE] = "LC_ST_PRS_TYPE",
    [LC_ST_OVER_TEMP_TYPE] = "LC_ST_OVER_TEMP_TYPE",
    [LC_ST_EJ_R_TYPE] = "LC_ST_EJ_R_TYPE",
    [LC_ST_EJ_L_TYPE] = "LC_ST_EJ_L_TYPE",
};
typedef enum {
    SFF_PRS_TYPE,
    SFF_INTR_TYPE,
    SFF_OC_TYPE,
    SFF_INPUT_TYPE_NUM,

} sff_input_type_t;

const char *sff_input_name[SFF_INPUT_TYPE_NUM] = {
    "SFF_PRS_TYPE",
    "SFF_INTR_TYPE",
    "SFF_OC_TYPE",
};
typedef enum {
    SFF_LPMODE_TYPE,
    SFF_RESET_TYPE,
    SFF_POWER_TYPE,
    SFF_OUTPUT_TYPE_NUM,

} lc_sff_output_type_t;
const char *lc_sff_output_name[SFF_OUTPUT_TYPE_NUM] = {
    "SFF_LPMODE_TYPE",
    "SFF_RESET_TYPE",
    "SFF_POWER_TYPE",
};

u8 lc_sff_input_reg[SFF_INPUT_TYPE_NUM] = {
    [SFF_PRS_TYPE] = LC_CPLD_SFF_PRS_OFFSET,/*use ioexp for temp  not PRS_OFFSET,*/
    [SFF_INTR_TYPE] = LC_CPLD_SFF_INTR_OFFSET,
    [SFF_OC_TYPE] = LC_CPLD_SFF_OC_OFFSET
};
u8 lc_sff_output_reg[SFF_OUTPUT_TYPE_NUM] = {
    [SFF_LPMODE_TYPE] = LC_CPLD_SFF_LPMODE_OFFSET,
    [SFF_RESET_TYPE] = LC_CPLD_SFF_RESET_OFFSET,
    [SFF_POWER_TYPE] = LC_CPLD_SFF_POWER_OFFSET,
};
u16 lc_sff_output_def[SFF_OUTPUT_TYPE_NUM] = {
    [SFF_LPMODE_TYPE] = LC_CPLD_DEF_LPMODE_VAL,
    [SFF_RESET_TYPE] = LC_CPLD_DEF_RESET_VAL,
    [SFF_POWER_TYPE] = LC_CPLD_DEF_POWER_VAL,
};
u8 lc_st_reg[CARD_NUM] = {
    SWITCH_CPLD_LC1_ST_OFFSET,
    SWITCH_CPLD_LC2_ST_OFFSET,
    SWITCH_CPLD_LC3_ST_OFFSET,
    SWITCH_CPLD_LC4_ST_OFFSET
};
u8 lc_control_reg[CARD_NUM] = {
    SWITCH_CPLD_LC1_CTRL_OFFSET,
    SWITCH_CPLD_LC2_CTRL_OFFSET,
    SWITCH_CPLD_LC3_CTRL_OFFSET,
    SWITCH_CPLD_LC4_CTRL_OFFSET,
};
int lc_st_bit[LC_ST_TYPE_NUM] = {
    [LC_ST_PRS_TYPE] = SWITCH_CPLD_LC_PRS_INTR_N_BIT,
    [LC_ST_OVER_TEMP_TYPE] = SWITCH_CPLD_LC_TEMP_INTR_N_BIT,
    [LC_ST_EJ_R_TYPE] = SWITCH_CPLD_LC_EJ_R_N_BIT,
    [LC_ST_EJ_L_TYPE] = SWITCH_CPLD_LC_EJ_L_N_BIT,
};

const int rst_mux_l2_bit[MUX_L2_NUM] = {
    RST_PCA9548_1_N_BIT,
    RST_PCA9548_2_N_BIT,
    RST_PCA9548_3_N_BIT,
    RST_PCA9548_4_N_BIT,
};
enum {

    CPLD1_ID = 0,
    CPLD2_ID
};

struct lc_dev_obj_t {
    struct lc_config_t cpld[PORT_GROUP_NUM];
    struct lc_config_t ioexp[PORT_GROUP_NUM];
    struct lc_config_t tmp_dev[TMP_DEV_NUM];
    struct lc_config_t mux_l1;
    struct lc_config_t mux_l2[MUX_L2_NUM];
    struct io_data_t input[SFF_INPUT_TYPE_NUM];
    int sff_intr_gpio;
};

struct sku_t {
    struct lc_config_t switch_cpld;
    struct inv_i2c_client_t lc_client[LC_CH_ID_NUM];
    struct lc_dev_obj_t obj[CARD_NUM];
    int lc_intr_gpio;
    struct ldata_format_t lc_st[LC_ST_TYPE_NUM];
    int lc_id_num;
};
struct sku_t banyan4u_sku;
struct i2c_client *lcI2cClient = NULL;

/*<TBD>*/
struct lc_config_t switch_cpld_config = {
    .lc_ch_id = LC_SWITCH_CH_ID,
    .addr = SWITCH_CPLD_ADDR,
};
const struct lc_config_t cpld_config_tbl[CARD_NUM][PORT_GROUP_NUM] = {

    [0] = {
        { .lc_ch_id = LC1_SFF_CH_ID, .addr = CPLD1_ADDR},
        { .lc_ch_id = LC1_SFF_CH_ID, .addr = CPLD2_ADDR},
    },
    [1] = {
        { .lc_ch_id = LC2_SFF_CH_ID, .addr = CPLD1_ADDR},
        { .lc_ch_id = LC2_SFF_CH_ID, .addr = CPLD2_ADDR},
    },
    [2] = {
        { .lc_ch_id = LC3_SFF_CH_ID, .addr = CPLD1_ADDR},
        { .lc_ch_id = LC3_SFF_CH_ID, .addr = CPLD2_ADDR},
    },
    [3] = {
        { .lc_ch_id = LC4_SFF_CH_ID, .addr = CPLD1_ADDR},
        { .lc_ch_id = LC4_SFF_CH_ID, .addr = CPLD2_ADDR},
    },

};
const struct lc_config_t ioexp_config_tbl[CARD_NUM][PORT_GROUP_NUM] = {

    [0] = {
        { .lc_ch_id = LC1_SFF_CH_ID, .addr = IOEXP1_ADDR},
        { .lc_ch_id = LC1_SFF_CH_ID, .addr = IOEXP2_ADDR},
    },
    [1] = {
        { .lc_ch_id = LC2_SFF_CH_ID, .addr = IOEXP1_ADDR},
        { .lc_ch_id = LC2_SFF_CH_ID, .addr = IOEXP2_ADDR},
    },
    [2] = {
        { .lc_ch_id = LC3_SFF_CH_ID, .addr = IOEXP1_ADDR},
        { .lc_ch_id = LC3_SFF_CH_ID, .addr = IOEXP2_ADDR},
    },
    [3] = {
        { .lc_ch_id = LC4_SFF_CH_ID, .addr = IOEXP1_ADDR},
        { .lc_ch_id = LC4_SFF_CH_ID, .addr = IOEXP2_ADDR},
    },

};
const struct lc_config_t tmp_dev_config_tbl[CARD_NUM][TMP_DEV_NUM] = {

    [0] = {
        { .lc_ch_id = LC1_TMP_CH_ID, .addr = TMP1_ADDR},
        { .lc_ch_id = LC1_TMP_CH_ID, .addr = TMP2_ADDR},
    },
    [1] = {
        { .lc_ch_id = LC2_TMP_CH_ID, .addr = TMP1_ADDR},
        { .lc_ch_id = LC2_TMP_CH_ID, .addr = TMP2_ADDR},
    },
    [2] = {
        { .lc_ch_id = LC3_TMP_CH_ID, .addr = TMP1_ADDR},
        { .lc_ch_id = LC3_TMP_CH_ID, .addr = TMP2_ADDR},
    },
    [3] = {
        { .lc_ch_id = LC4_TMP_CH_ID, .addr = TMP1_ADDR},
        { .lc_ch_id = LC4_TMP_CH_ID, .addr = TMP2_ADDR},
    },
};
const struct lc_config_t mux_l1_config_tbl[CARD_NUM] = {

    [0] = {
        .lc_ch_id = LC1_MUX_L1_CH_ID, .addr = MUX_L1_ADDR,
    },
    [1] = {
        .lc_ch_id = LC2_MUX_L1_CH_ID, .addr = MUX_L1_ADDR,
    },
    [2] = {
        .lc_ch_id = LC3_MUX_L1_CH_ID, .addr = MUX_L1_ADDR,
    },
    [3] = {
        .lc_ch_id = LC4_MUX_L1_CH_ID, .addr = MUX_L1_ADDR,
    },
};
const struct lc_config_t mux_l2_config_tbl[CARD_NUM][MUX_L2_NUM] = {

    [0] = {
        {.lc_ch_id = LC1_MUX_L2_1_CH_ID, .addr = MUX_L2_ADDR},
        {.lc_ch_id = LC1_MUX_L2_2_CH_ID, .addr = MUX_L2_ADDR},
        {.lc_ch_id = LC1_MUX_L2_3_CH_ID, .addr = MUX_L2_ADDR},
        {.lc_ch_id = LC1_MUX_L2_3_CH_ID, .addr = MUX_L2_ADDR},
    },
    [1] = {
        {.lc_ch_id = LC2_MUX_L2_1_CH_ID, .addr = MUX_L2_ADDR},
        {.lc_ch_id = LC2_MUX_L2_2_CH_ID, .addr = MUX_L2_ADDR},
        {.lc_ch_id = LC2_MUX_L2_3_CH_ID, .addr = MUX_L2_ADDR},
        {.lc_ch_id = LC2_MUX_L2_3_CH_ID, .addr = MUX_L2_ADDR},
    },
    [2] = {
        {.lc_ch_id = LC3_MUX_L2_1_CH_ID, .addr = MUX_L2_ADDR},
        {.lc_ch_id = LC3_MUX_L2_2_CH_ID, .addr = MUX_L2_ADDR},
        {.lc_ch_id = LC3_MUX_L2_3_CH_ID, .addr = MUX_L2_ADDR},
        {.lc_ch_id = LC3_MUX_L2_3_CH_ID, .addr = MUX_L2_ADDR},
    },
    [3] = {
        {.lc_ch_id = LC4_MUX_L2_1_CH_ID, .addr = MUX_L2_ADDR},
        {.lc_ch_id = LC4_MUX_L2_2_CH_ID, .addr = MUX_L2_ADDR},
        {.lc_ch_id = LC4_MUX_L2_3_CH_ID, .addr = MUX_L2_ADDR},
        {.lc_ch_id = LC4_MUX_L2_3_CH_ID, .addr = MUX_L2_ADDR},
    },
};
const int sff_intr_gpio[CARD_NUM] = {
    LC1_SFF_INTR_GPIO,
    LC2_SFF_INTR_GPIO,
    LC3_SFF_INTR_GPIO,
    LC4_SFF_INTR_GPIO,
};
/*function declaration*/
int lc_dev_lc_cpld_init(int lc_id);
int lc_dev_power_set(int lc_id, bool on);
int lc_dev_power_ready(int lc_id, bool *ready);
void lc_dev_phy_ready(unsigned long bitmap, bool *ready);
int lc_dev_prs_get(unsigned long *bitmap);
int lc_dev_reset_set(int lc_id, u8 lv);
int lc_dev_reset_get(int lc_id, u8 *lv);
int lc_dev_type_get(int lc_id, lc_type_t *type);
int lc_dev_type_get_text(int lc_id, char *buf, int size);
int lc_dev_mux_l1_reset(int lc_id, u8 lv);
bool lc_dev_mux_l1_is_alive(int lc_id);
int lc_dev_mux_l2_reset(int lc_id, int mux_l2_id, u8 lv);
int lc_dev_mux_reset_set(int lc_id, int lv);
int lc_dev_led_set(int lc_id, lc_led_ctrl_t ctrl);
int lc_dev_led_st_get(int lc_id, lc_led_ctrl_t *ctrl);
int lc_dev_init(int platform_id, int io_no_init);
void lc_dev_deinit(void);
int lc_io_hdlr(void);
int lc_dev_over_temp_asserted(int lc_id, bool *asserted);
int lc_dev_over_temp_deasserted(int lc_id, bool *deasserted);
int lc_dev_temp_get_text(int lc_id, char *buf, int size);
int lc_dev_temp_th_get_text(int lc_id, char *buf, int size);
int lc_dev_phy_reset_set(int lc_id, u8 val);
int lc_sff_intr_hdlr_byCard(int lc_id);
int lc_dev_led_boot_amber_set(int lc_id, bool on);
int lc_dev_temp_th_set(int lc_id, int temp);
int lc_dev_ej_r_get(unsigned long *bitmap);
int lc_dev_ej_l_get(unsigned long *bitmap);

static int lc_dev_intr_enable(bool on);
static int lc_dev_intr_st_update(void);
static int lc_dev_intr_st_clear(void);
static int lc_sff_output_bitmap_set(lc_sff_output_type_t type, int lc_id, int port_grp, u16 val);
/*sff io*/
static int lc_sff_prs_get(int lc_id, unsigned long *bitmap);
static int lc_sff_intr_get(int lc_id, unsigned long *bitmap);
static int lc_sff_lpmode_set(int lc_id, unsigned long bitmap);
static int lc_sff_lpmode_get(int lc_id, unsigned long *bitmap);
static int lc_sff_reset_set(int lc_id, unsigned long bitmap);
static int lc_sff_reset_get(int lc_id, unsigned long *bitmap);
/*sff extra io*/
static int lc_sff_power_set(int lc_id, unsigned long bitmap);
static int lc_sff_power_get(int lc_id, unsigned long *bitmap);
static int lc_sff_oc_get(int lc_id, unsigned long *bitmap);

static int lc_sff_prs_update_byGroup(int lc_id, int port_grp);
static int lc_sff_intr_update_byGroup(int lc_id, int port_grp);
static int lc_sff_oc_update_byGroup(int lc_id, int port_grp);
static int lc_sff_prs_update_1stGroup(int lc_id);
static int lc_sff_prs_update_2ndGroup(int lc_id);
static int lc_sff_intr_update_1stGroup(int lc_id);
static int lc_sff_intr_update_2ndGroup(int lc_id);
static int lc_sff_oc_update_1stGroup(int lc_id);
static int lc_sff_oc_update_2ndGroup(int lc_id);

struct sff_io_driver_t banyan4u_sku_sff_io_drv = {
    .prs = {
        .set = NULL,
        .get = lc_sff_prs_get
    },
    .intr = {
        .set = NULL,
        .get = lc_sff_intr_get
    },
    .reset = {
        .set = lc_sff_reset_set,
        .get = lc_sff_reset_get
    },
    .lpmode = {
        .set = lc_sff_lpmode_set,
        .get = lc_sff_lpmode_get
    },
};

static void dummy_phy_ready(unsigned long bitmap, bool *ready)
{
    if (0xff == bitmap) {
        *ready = true;
    } else {
        *ready = false;
    }
}
static int lc_dummy_get(int lc_id, int *lv)
{
    *lv = 1;
    return 0;
}

struct pltfm_func_t banyan4u_sku_pltfm_func = {
    .init = lc_dev_init,
    .deinit = lc_dev_deinit,
    .io_hdlr = lc_io_hdlr,
    .i2c_is_alive = lc_dev_mux_l1_is_alive,
    .mux_reset_set = lc_dev_mux_reset_set,
    .mux_reset_get = lc_dummy_get,
    .sff_power = {
        .set = lc_sff_power_set,
        .get = lc_sff_power_get
    },
    
    .sff_oc = {
        .set = NULL,
        .get = lc_sff_oc_get
    },
};

struct lc_func_t banyan4u_sku_lc_func = {
    .cpld_init = lc_dev_lc_cpld_init,
    .power_set = lc_dev_power_set,
    .power_ready = lc_dev_power_ready,
    //.phy_ready = lc_dev_phy_ready,
    .phy_ready = dummy_phy_ready,
    .prs_get = lc_dev_prs_get,
    .reset_set = lc_dev_reset_set,
    .type_get = lc_dev_type_get,
    .type_get_text = lc_dev_type_get_text,
    .led_set = lc_dev_led_set,
    .led_set = lc_dev_led_set,
    .over_temp_asserted = lc_dev_over_temp_asserted,
    .over_temp_deasserted = lc_dev_over_temp_deasserted,
    .temp_get = lc_dev_temp_get_text,
    .phy_reset_set = lc_dev_phy_reset_set,
    .led_boot_amber_set = lc_dev_led_boot_amber_set,
    .temp_th_set = lc_dev_temp_th_set,
    .intr_hdlr = lc_sff_intr_hdlr_byCard,
    .ej_r_get = lc_dev_ej_r_get,    
    .ej_l_get = lc_dev_ej_l_get    
};

int (*sff_intr_funcs[LC_CPLD_SFF_INTR_NUM])(int lc_id) = {

    [LC_CPLD_SFF_INTR_CPLD2_PRS_BIT] = lc_sff_prs_update_2ndGroup,
    [LC_CPLD_SFF_INTR_CPLD2_INTR_BIT] = lc_sff_intr_update_2ndGroup,
    [LC_CPLD_SFF_INTR_CPLD2_OC_BIT] = lc_sff_oc_update_2ndGroup,
    [LC_CPLD_SFF_INTR_RESERVED1] = NULL,
    [LC_CPLD_SFF_INTR_CPLD1_PRS_BIT] = lc_sff_prs_update_1stGroup,
    [LC_CPLD_SFF_INTR_CPLD1_INTR_BIT] = lc_sff_intr_update_1stGroup,
    [LC_CPLD_SFF_INTR_CPLD1_OC_BIT] = lc_sff_oc_update_1stGroup,
    [LC_CPLD_SFF_INTR_RESERVED2] = NULL,
};
static bool grp_valid(int port_grp)
{
    if (port_grp < PORT_GROUP_NUM && port_grp >= 0) {
        return true;
    } else {
        SKU_LOG_ERR("invalid port_grp:%d\n", port_grp);
        return false;
    }
}
static bool lc_id_valid(int lc_id)
{
    if (lc_id < CARD_NUM && lc_id >= 0) {
        return true;
    } else {
        SKU_LOG_ERR("invalid lc_id:%d\n", lc_id);
        return false;
    }
}

static void config_load(void)
{
    int lc_id = 0;
    int port_grp = 0;
    int tmp_dev = 0;
    int mux_l2_i = 0;

    memcpy(&(banyan4u_sku.switch_cpld), &switch_cpld_config, sizeof(struct lc_config_t));
    banyan4u_sku.lc_intr_gpio = LC_INTR_GPIO;
    for (lc_id = 0; lc_id < CARD_NUM; lc_id++) {

        banyan4u_sku.obj[lc_id].sff_intr_gpio = sff_intr_gpio[lc_id];

        for (port_grp = 0; port_grp < PORT_GROUP_NUM; port_grp++) {

            memcpy(&banyan4u_sku.obj[lc_id].cpld[port_grp], &cpld_config_tbl[lc_id][port_grp], sizeof(struct lc_config_t));
            memcpy(&banyan4u_sku.obj[lc_id].ioexp[port_grp], &ioexp_config_tbl[lc_id][port_grp], sizeof(struct lc_config_t));
        }
        for (tmp_dev = 0; tmp_dev < TMP_DEV_NUM; tmp_dev++) {
            memcpy(&banyan4u_sku.obj[lc_id].tmp_dev[tmp_dev], &tmp_dev_config_tbl[lc_id][tmp_dev], sizeof(struct lc_config_t));
        }
        /*mux config*/
        memcpy(&banyan4u_sku.obj[lc_id].mux_l1, &mux_l1_config_tbl[lc_id], sizeof(struct lc_config_t));
        for (mux_l2_i = 0; mux_l2_i < MUX_L2_NUM;  mux_l2_i++) {
            memcpy(&banyan4u_sku.obj[lc_id].mux_l2[mux_l2_i], &mux_l2_config_tbl[lc_id][mux_l2_i], sizeof(struct lc_config_t));
        }
    }
}
int lc_i2c_client_init(int lc_ch,  struct i2c_client **client)
{
    struct i2c_adapter *adap = NULL;

    if (!p_valid(*client)) {
        return -EBADRQC;
    }
    adap = i2c_get_adapter(lc_ch);
    if (!p_valid(adap)) {
        SKU_LOG_ERR("get adapter fail ch:%d\n", lc_ch);
        return -EBADRQC;
    }

    SKU_LOG_DBG("get adapter ok ch:%d\n", lc_ch);
    (*client)->adapter = adap;

    return 0;

}
static void lc_i2c_client_deinit_all(struct sku_t *self)
{
    struct i2c_client *client = NULL;
    int id = 0;
    if (!self) {
        return;
    }

    for (id = 0; id < LC_CH_ID_NUM; id++) {
        client = self->lc_client[id].client;
        if (p_valid(client)) {
            if (p_valid(client->adapter)) {
                i2c_put_adapter(client->adapter);
                SKU_LOG_DBG("put_adapter:%d\n", id);
            }
        }
    }
}
void lc_i2c_clients_destroy(struct sku_t *self)
{
    int lc_ch_id = 0;

    if (p_valid(lcI2cClient)) {
        kfree(lcI2cClient);
    }
    for (lc_ch_id = 0; lc_ch_id < LC_CH_ID_NUM; lc_ch_id++) {

        self->lc_client[lc_ch_id].client = NULL;
    }
}
int lc_i2c_clients_create(struct sku_t *self)
{
    int lc_ch_id = 0;
    struct i2c_client *client = NULL;


    client = kzalloc(sizeof(struct i2c_client)*LC_CH_ID_NUM, GFP_KERNEL);
    if (!p_valid(client)) {
        return -EBADRQC;
    }
    lcI2cClient = client;
    /*build a link*/
    for (lc_ch_id = 0; lc_ch_id < LC_CH_ID_NUM; lc_ch_id++) {

        self->lc_client[lc_ch_id].client = &lcI2cClient[lc_ch_id];
    }
    return 0;
}
int lc_i2c_client_init_all(struct sku_t *self)
{
    int lc_ch_id = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    int ret = 0;

    for (lc_ch_id = 0; lc_ch_id < LC_CH_ID_NUM; lc_ch_id++) {

        lc_client = &(self->lc_client[lc_ch_id]);
        if ((ret = lc_i2c_client_init(lcChTbl[lc_ch_id], &(lc_client->client))) < 0) {
            break;
        }
        mutex_init(&(lc_client->lock));

    }
    if (ret < 0) {
        lc_i2c_client_deinit_all(self);
        return ret;
    }

    return 0;
}
static bool ch_id_valid(int id)
{
    return ((id >= 0 && id < LC_CH_ID_NUM) ? true : false);
}
struct inv_i2c_client_t *lc_client_find(struct lc_config_t *config)
{
    struct inv_i2c_client_t *lc_client = NULL;
    int lc_ch_id = 0;
    u8 addr = 0;
    if (!config) {
        return NULL;
    }
    lc_ch_id = config->lc_ch_id;
    addr = config->addr;

    if (!ch_id_valid(lc_ch_id)) {
        SKU_LOG_ERR("cant find lc_ch_id:%d\n", lc_ch_id);
        return NULL;
    }

    lc_client = &(banyan4u_sku.lc_client[lc_ch_id]);
    if (!p_valid(lc_client)) {
        SKU_LOG_ERR("NULL lc_client lc_ch_id:%d\n", lc_ch_id);
        return NULL;
    }

    if (!p_valid(lc_client->client)) {
        SKU_LOG_ERR("NULL lc_client lc_ch_id:%d\n", lc_ch_id);
        return NULL;
    }
    lc_client->client->addr = addr;

    //SKU_LOG_DBG("lc_ch_id:%d addr: 0x%x\n", lc_ch_id, addr);
    return lc_client;
}

static int lc_i2c_smbus_write_byte_data(struct inv_i2c_client_t *lc_client, u8 offset, u8 buf)
{
    int ret = 0;

    if (!lc_client) {
        return -EINVAL;
    }

    mutex_lock(&lc_client->lock);
    ret = i2c_smbus_write_byte_data_retry(lc_client->client, offset, buf);
    mutex_unlock(&lc_client->lock);

    return ret;
}
int lc_i2c_smbus_read_byte_data(struct inv_i2c_client_t *lc_client, u8 offset)
{
    int ret = 0;

    if (!lc_client) {
        return -EINVAL;
    }

    mutex_lock(&lc_client->lock);
    ret = i2c_smbus_read_byte_data_retry(lc_client->client, offset);
    mutex_unlock(&lc_client->lock);

    return ret;
}
int lc_i2c_smbus_read_byte(struct inv_i2c_client_t *lc_client)
{
    int ret = 0;

    if (!lc_client) {
        return -EINVAL;
    }

    mutex_lock(&lc_client->lock);
    ret = i2c_smbus_read_byte(lc_client->client);
    mutex_unlock(&lc_client->lock);

    return ret;
}

int lc_i2c_smbus_read_word_data(struct inv_i2c_client_t *lc_client, u8 offset)
{
    int ret = 0;

    if (!lc_client) {
        return -EINVAL;
    }

    mutex_lock(&lc_client->lock);
    ret = i2c_smbus_read_word_data_retry(lc_client->client, offset);
    mutex_unlock(&lc_client->lock);

    return ret;
}

int lc_i2c_smbus_write_word_data(struct inv_i2c_client_t *lc_client, u8 offset, u16 data)
{
    int ret = 0;

    if (!lc_client) {
        return -EINVAL;
    }

    mutex_lock(&lc_client->lock);
    ret = i2c_smbus_write_word_data_retry(lc_client->client, offset, data);
    mutex_unlock(&lc_client->lock);

    return ret;
}

/* description: get lc object instance
 *
 * input:
 *      lc_id: lc id number
 *return:
        0: success
 *  */
inline struct lc_dev_obj_t *lc_obj_get(int lc_id)
{
    if (!lc_id_valid(lc_id)) {
        SKU_LOG_ERR("fail to get lc obj:%d\n", lc_id);
        return NULL;
    }
    return &(banyan4u_sku.obj[lc_id]);
}
int lc_sff_input_get(sff_input_type_t type, int lc_id, unsigned long *bitmap)
{
    struct lc_dev_obj_t *obj = NULL;

    if (!bitmap) {
        return -EINVAL;
    }

    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }

    if (SFF_DATA_VALID_MASK != obj->input[type].valid) {
        return -EBADRQC;
    }

    *bitmap = (unsigned long)(obj->input[type].buf);
    //SKU_LOG_DBG("lc_id:%d %s: %lx\n", lc_id, sff_input_name[type], *bitmap);
    return 0;
}

static int lc_sff_prs_get(int lc_id, unsigned long *bitmap)
{
    return lc_sff_input_get(SFF_PRS_TYPE, lc_id, bitmap);
}
static int lc_sff_oc_get(int lc_id, unsigned long *bitmap)
{
    return lc_sff_input_get(SFF_OC_TYPE, lc_id, bitmap);
}
static int lc_sff_intr_get(int lc_id, unsigned long *bitmap)
{
    return lc_sff_input_get(SFF_INTR_TYPE, lc_id, bitmap);
}
/*update part of input data by port_group
 *input data contains 32 bits which is divided into 2 group of sub_data(16bit)
 * */
static void subdata_update(u32 *data, u32 sub, int grp)
{
    u32 mask = (((1 << PORT_IDX_NUM) - 1) << (grp * PORT_IDX_NUM));
    u32 shift = grp * PORT_IDX_NUM;
    (*data) = ((*data) & (~mask)) | (sub << shift);
}
static int lc_sff_input_update_byGroup(sff_input_type_t type, int lc_id, int port_grp)
{
    struct inv_i2c_client_t *lc_client = NULL;
    int ret = 0;
    struct lc_dev_obj_t *obj = NULL;
    struct lc_config_t *config = NULL;
    u32 old_input = 0;
    u32 input_change = 0;
    int port = 0;
    u8 cur_st = 0;

    if (!grp_valid(port_grp) ||
            !lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }
    config = &(obj->cpld[port_grp]);
    if (!(lc_client = lc_client_find(config))) {
        return -EBADRQC;
    }
    if ((ret = lc_i2c_smbus_read_word_data(lc_client, lc_sff_input_reg[type])) < 0) {
        obj->input[type].valid = 0;
        return ret;
    }
    old_input = obj->input[type].buf;
    subdata_update(&(obj->input[type].buf), ret, port_grp);
    obj->input[type].valid |= 1 << port_grp;
    SKU_LOG_DBG("lc_id:%d group:%d %s input:0x%x\n", lc_id, port_grp, sff_input_name[type], obj->input[type].buf);
    /*debugging*/
    input_change = old_input ^ (obj->input[type].buf);
    for (port = 0; port < PORT_IDX_NUM * PORT_GROUP_NUM; port++) {
        if (test_bit(port, (unsigned long *)&input_change)) {
            cur_st = (test_bit(port, (unsigned long *)&(obj->input[type].buf)) ? 1 : 0);
            SKU_LOG_DBG("lc_id:%d port:%d st->%d\n", lc_id, port, cur_st);
        }
    }
    return 0;
};
/*only used in intr function*/
static int lc_sff_prs_intr_clear(int lc_id, int port_grp)
{
    struct inv_i2c_client_t *lc_client = NULL;
    int ret = 0;
    struct lc_dev_obj_t *obj = NULL;
    struct lc_config_t *config = NULL;

    if (!grp_valid(port_grp) ||
            !lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }
    config = &(obj->cpld[port_grp]);
    if (!(lc_client = lc_client_find(config))) {
        return -EBADRQC;
    }
    if ((ret = lc_i2c_smbus_read_byte_data(lc_client, LC_CPLD_SFF_PRS_INTR_CLEAR_OFFSET)) < 0) {
        return ret;
    }

    return 0;
}
static int lc_sff_prs_update_byGroup(int lc_id, int port_grp)
{
    int ret = 0;
    if ((ret = lc_sff_prs_intr_clear(lc_id, port_grp)) < 0) {
        return ret;
    }
    return lc_sff_input_update_byGroup(SFF_PRS_TYPE, lc_id, port_grp);
}

static int lc_sff_intr_update_byGroup(int lc_id, int port_grp)
{
    return lc_sff_input_update_byGroup(SFF_INTR_TYPE, lc_id, port_grp);
}

static int lc_sff_oc_update_byGroup(int lc_id, int port_grp)
{
    return lc_sff_input_update_byGroup(SFF_OC_TYPE, lc_id, port_grp);
}
/*1st group*/
static int lc_sff_prs_update_1stGroup(int lc_id)
{
    return lc_sff_prs_update_byGroup(lc_id, 0);
}

static int lc_sff_intr_update_1stGroup(int lc_id)
{
    return lc_sff_intr_update_byGroup(lc_id, 0);
}

static int lc_sff_oc_update_1stGroup(int lc_id)
{
    return lc_sff_oc_update_byGroup(lc_id, 0);
}
/*2nd group*/
static int lc_sff_prs_update_2ndGroup(int lc_id)
{
    return lc_sff_prs_update_byGroup(lc_id, 1);
}

static int lc_sff_intr_update_2ndGroup(int lc_id)
{
    return lc_sff_intr_update_byGroup(lc_id, 1);
}
static int lc_sff_oc_update_2ndGroup(int lc_id)
{
    return lc_sff_oc_update_byGroup(lc_id, 1);
}

static inline void group_idx_get(int port, int *port_grp, int *idx)
{
    *port_grp = (port >> 4);
    *idx = port & 0xf;
}
static int lc_sff_output_bitmap_set(lc_sff_output_type_t type, int lc_id, int port_grp, u16 val)
{
    struct inv_i2c_client_t *lc_client = NULL;
    int ret = 0;
    u8 offset = lc_sff_output_reg[type];
    struct lc_dev_obj_t *obj = NULL;

    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!grp_valid(port_grp)) {
        return -EINVAL;
    }

    if (!(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }

    if (!(lc_client = lc_client_find(&(obj->cpld[port_grp])))) {
        return -EBADRQC;
    }

    if ((ret = lc_i2c_smbus_write_word_data(lc_client, offset, val)) < 0) {
        return ret;
    }

    return 0;
}
static int lc_sff_output_all_set(lc_sff_output_type_t type, int lc_id, unsigned long bitmap)
{
    struct inv_i2c_client_t *lc_client = NULL;
    int ret = 0;
    int port_grp = 0;
    u16 reg[PORT_GROUP_NUM];
    u8 offset = lc_sff_output_reg[type];
    struct lc_dev_obj_t *obj = NULL;

    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }
    if (!(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }

    reg[0] = bitmap & U16_MASK;
    reg[1] = bitmap >> PORT_IDX_NUM;

    /*debug*/
    SKU_LOG_DBG("%s lc_id:%d l:0x%x h:0x%x\n", lc_sff_output_name[type], lc_id, reg[0], reg[1]);
    for (port_grp = 0; port_grp < PORT_GROUP_NUM; port_grp++) {
        if (!(lc_client = lc_client_find(&(obj->cpld[port_grp])))) {
            ret = -EBADRQC;
            break;
        }
        if ((ret = lc_i2c_smbus_write_word_data(lc_client, offset, reg[port_grp])) < 0) {
            break;
        }
    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}

static int lc_sff_output_all_get(lc_sff_output_type_t type, int lc_id, unsigned long *bitmap)
{
    struct inv_i2c_client_t *lc_client = NULL;
    int ret = 0;
    int port_grp = 0;
    unsigned long reg[PORT_GROUP_NUM];
    u8 offset = lc_sff_output_reg[type];
    struct lc_dev_obj_t *obj = NULL;

    if (!p_valid(bitmap)) {
        return -EINVAL;
    }
    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }
    if (!(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }

    /*debug*/
    //SKU_LOG_DBG("%s lc_id:%d port:%d val:%d\n", lc_sff_output_name[type], lc_id, port, val);
    for (port_grp = 0; port_grp < PORT_GROUP_NUM; port_grp++) {
        if (!(lc_client = lc_client_find(&(obj->cpld[port_grp])))) {
            ret = -EBADRQC;
            break;
        }
        if ((ret = lc_i2c_smbus_read_word_data(lc_client, offset)) < 0) {
            break;
        }
        reg[port_grp] = ret;
    }
    if (ret < 0) {
        return ret;
    }
    *bitmap |= reg[0];
    *bitmap |= reg[1] << (PORT_IDX_NUM);
    SKU_LOG_DBG("%s lc_id:%d l:0x%lx h:0x%lx\n", lc_sff_output_name[type], lc_id, reg[0], reg[1]);
    return 0;
}
static int lc_sff_lpmode_set(int lc_id, unsigned long bitmap)
{
    return lc_sff_output_all_set(SFF_LPMODE_TYPE, lc_id, bitmap);
}
static int lc_sff_lpmode_get(int lc_id, unsigned long *bitmap)
{
    return lc_sff_output_all_get(SFF_LPMODE_TYPE, lc_id, bitmap);
}
static int lc_sff_reset_set(int lc_id, unsigned long bitmap)
{
    return lc_sff_output_all_set(SFF_RESET_TYPE, lc_id, bitmap);
}
static int lc_sff_reset_get(int lc_id, unsigned long *bitmap)
{
    return lc_sff_output_all_get(SFF_RESET_TYPE, lc_id, bitmap);
}
static int lc_sff_power_set(int lc_id, unsigned long bitmap)
{
    return lc_sff_output_all_set(SFF_POWER_TYPE, lc_id, bitmap);
}
static int lc_sff_power_get(int lc_id, unsigned long *bitmap)
{
    return lc_sff_output_all_get(SFF_POWER_TYPE, lc_id, bitmap);
}

static int lc_sff_intr_is_asserted(int lc_id, bool *asserted)
{
    bool st = false;
    int lv = 0;
    lv = gpio_get_value(banyan4u_sku.obj[lc_id].sff_intr_gpio);

    if (!lv) {
        st = true;
        SKU_LOG_DBG("lc_id:%d lv:%d asserted\n", lc_id, lv);
    } else {

        //SKU_LOG_DBG("de-asserted\n");
    }
    *asserted = st;
    return 0;
}

/*used to check if mux l1 is alive*/
bool lc_dev_mux_l1_is_alive(int lc_id)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    struct lc_dev_obj_t *obj = NULL;
    bool alive = false;

    if (!p_valid(obj = lc_obj_get(lc_id))) {
        return alive;
    }

    if (!p_valid(lc_client = lc_client_find(&(obj->mux_l1)))) {
        return alive;
    }

    if ((ret = lc_i2c_smbus_read_byte(lc_client)) < 0) {
        return alive;
    }
    //if (0 != ret)
    //{
    alive = true;
    //}
    if (!alive) {
        SKU_LOG_ERR("not alive\n");
    }
    return alive;
}
int lc_dev_mux_l1_reset(int lc_id, u8 lv)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    struct lc_dev_obj_t *obj = NULL;
    u8 reg = 0;

    if (!(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }

    if (!(lc_client = lc_client_find(&(obj->cpld[CPLD1_ID])))) {
        return -EBADRQC;
    }

    if ((ret = lc_i2c_smbus_read_byte_data(lc_client, RST_PCA9548_D_OFFSET)) < 0) {
        return ret;
    }
    reg = ret;

    SKU_LOG_DBG("lc_id:%d get reg:0x%x\n", lc_id, reg);

    if (lv) {
        set_bit(RST_PCA9546_N_BIT, (unsigned long *)&reg);
    } else {
        clear_bit(RST_PCA9546_N_BIT, (unsigned long *)&reg);
    }
    SKU_LOG_DBG("lc_id:%d set reg:0x%x\n", lc_id, reg);

    if ((ret = lc_i2c_smbus_write_byte_data(lc_client, RST_PCA9548_D_OFFSET, reg)) < 0) {
        return ret;
    }

    return 0;
}
static bool mux_l2_id_valid(int mux_l2_id)
{
    if (mux_l2_id < MUX_L2_NUM && mux_l2_id >= 0) {
        return true;
    } else {
        SKU_LOG_ERR("invalid mux_l2_id:%d\n", mux_l2_id);
        return false;
    }
}
int lc_dev_mux_l2_reset(int lc_id, int mux_l2_id, u8 lv)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    struct lc_dev_obj_t *obj = NULL;
    u8 reg = 0;

    if (!mux_l2_id_valid(mux_l2_id)) {

        return -EINVAL;
    }

    if (!(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }

    if (!(lc_client = lc_client_find(&(obj->cpld[CPLD1_ID])))) {
        return -EBADRQC;
    }
    if ((ret = lc_i2c_smbus_read_byte_data(lc_client, RST_PCA9548_D_OFFSET)) < 0) {
        return ret;
    }
    reg = ret;

    SKU_LOG_DBG("lc_id:%d get reg:0x%x\n", lc_id, reg);
    if (lv) {
        set_bit(rst_mux_l2_bit[mux_l2_id], (unsigned long *)&reg);
    } else {
        clear_bit(rst_mux_l2_bit[mux_l2_id], (unsigned long *)&reg);
    }
    SKU_LOG_DBG("lc_id:%d set reg:0x%x\n", lc_id, reg);

    if ((ret = lc_i2c_smbus_write_byte_data(lc_client, RST_PCA9548_D_OFFSET, reg)) < 0) {
        return ret;
    }

    return 0;

}

int lc_dev_mux_reset_set(int lc_id, int lv)
{
    int ret = 0;
    int mux_l2_id = 0;

    for (mux_l2_id = 0; mux_l2_id < MUX_L2_NUM; mux_l2_id++) {
        if ((ret = lc_dev_mux_l2_reset(lc_id, mux_l2_id, lv)) < 0) {
            break;
        }
    }
    if (ret < 0) {
        return ret;
    }
    if ((ret = lc_dev_mux_l1_reset(lc_id, lv)) < 0) {
        return ret;
    }
    return 0;
}

static inline s8 lm75_reg_to_mc(s16 reg, u8 resolution)
{
    //return ((temp >> (16 - resolution)) * 1000) >> (resolution - 8);
    s8 temp = 0;

    temp = reg & 0xff;
#if 0
    long val = 0;
    val = (temp & 0xff) * 1000 + ((temp>>14 & 0x03)*250)*((temp & 0x80)?-1:1);
    return val;
#else
    return temp;
    //return ((new_temp >> (16 - resolution)) * 1000) >> (resolution - 8);

#endif
}
int lc_dev_temp_th_set(int lc_id, int temp)
{
    int ret = 0;
    u8 offset = 0xff;
    s16 reg = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    struct lc_dev_obj_t *obj = NULL;

    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!p_valid(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }
    if (!p_valid(lc_client = lc_client_find(&(obj->tmp_dev[0])))) {
        return -EBADRQC;
    }

    offset = LM75_REG_LOW;
    reg = temp;
    SKU_LOG_ERR("reg:0x%x\n", reg);
    if ((ret = lc_i2c_smbus_write_word_data(lc_client, offset, reg)) < 0) {
        return ret;
    }
    offset = LM75_REG_HIGH;
    reg = temp+3;
    SKU_LOG_ERR("reg:0x%x\n", reg);
    if ((ret = lc_i2c_smbus_write_word_data(lc_client, offset, reg)) < 0) {
        return ret;
    }

    return 0;
}

int lc_dev_temp_get(temp_conf_type_t type, int lc_id, int tmp_id, s8 *val)
{
    int ret = 0;
    u8 offset = 0xff;
    s16 reg = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    struct lc_dev_obj_t *obj = NULL;

    if (!p_valid(val)) {
        return -EINVAL;
    }
    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!p_valid(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }
    if (!p_valid(lc_client = lc_client_find(&(obj->tmp_dev[tmp_id])))) {
        return -EBADRQC;
    }

    switch (type) {
    case TEMP_INPUT:
        offset = LM75_REG_TEMP;
        break;
    case TEMP_TH_L:
        offset = LM75_REG_LOW;
        break;
    case TEMP_TH_H:
        offset = LM75_REG_HIGH;
        break;
    default:
        SKU_LOG_ERR("UNKOWN_TYPE:%d\n", type);
        break;
    }
    if (0xff == offset) {
        return -EINVAL;
    }
    if ((ret = lc_i2c_smbus_read_word_data(lc_client, offset)) < 0) {
        return ret;
    }
    reg = ret;
    //SKU_LOG_ERR("reg:0x%x\n", reg);
    *val = lm75_reg_to_mc(reg, LM75_RESOLUTION);

    return 0;
}

int lc_dev_temp_config_get(int lc_id, int tmp_id, u8 *val)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    struct lc_dev_obj_t *obj = NULL;

    if (!p_valid(val)) {
        return -EINVAL;
    }
    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!p_valid(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }
    if (!p_valid(lc_client = lc_client_find(&(obj->tmp_dev[tmp_id])))) {
        return -EBADRQC;
    }

    if ((ret = lc_i2c_smbus_read_word_data(lc_client, LM75_REG_CONF)) < 0) {
        return ret;
    }
    *val = ret;
    SKU_LOG_ERR("reg:0x%x\n", ret);

    return 0;
}
int lc_dev_temp_get_text(int lc_id, char *buf, int size)
{
    int ret = 0;
    s8 temp;
    s8 temp_th_l;
    s8 temp_th_h;
    int tmp_id = 0;
    int count = 0;
    u8 conf = 0;

    for (tmp_id = 0; tmp_id < TMP_DEV_NUM; tmp_id++) {
        if ((ret = lc_dev_temp_config_get(lc_id, tmp_id, &conf)) < 0) {
            break;
        }

        if ((ret = lc_dev_temp_get(TEMP_INPUT, lc_id, tmp_id, &temp)) < 0) {
            break;
        }
        if ((ret = lc_dev_temp_get(TEMP_TH_L, lc_id, tmp_id, &temp_th_l)) < 0) {
            break;
        }

        if ((ret = lc_dev_temp_get(TEMP_TH_H, lc_id, tmp_id, &temp_th_h)) < 0) {
            break;
        }
        count += scnprintf(buf+count, size-count,
                           "tmp%d: conf:0x%x temp:%d temp_th_l:%d temp_th_h:%d\n",
                           tmp_id, conf, temp, temp_th_l, temp_th_h);
    }
    if (ret < 0) {
        return ret;
    }

    return 0;
}
#if 0
int lc_dev_temp_th_get_text(int lc_id, char *buf, int size)
{
    int ret = 0;
    s8 temp_th[TMP_DEV_NUM];
    int tmp_id = 0;
    int count = 0;

    for (tmp_id = 0; tmp_id < TMP_DEV_NUM; tmp_id++) {

        if ((ret = lc_dev_temp_get(TEMP_TH_H, lc_id, tmp_id, &temp_th[tmp_id])) < 0) {
            break;
        }
        count += scnprintf(buf+count, size-count,
                           "tmp%d: high thershold %ld c\n",
                           tmp_id+1, temp_th[tmp_id]);
    }
    if (ret < 0) {
        return ret;
    }

    for (tmp_id = 0; tmp_id < TMP_DEV_NUM; tmp_id++) {

        if ((ret = lc_dev_temp_get(TEMP_TH_L, lc_id, tmp_id, &temp_th[tmp_id])) < 0) {
            break;
        }
        count += scnprintf(buf+count, size-count,
                           "tmp%d: low thershold %ld c\n",
                           tmp_id+1, temp_th[tmp_id]);
    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}
#endif
/* description: reset lc
 *
 * input:
 * lc_id: lc number
 * lv: reset level

 *return:
        0: success
[note]
    the reset is used to do lc reset via switch cpld reg,

 *  */
int lc_dev_reset_set(int lc_id, u8 lv)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    u8 reg = 0;

    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!(lc_client = lc_client_find(&(banyan4u_sku.switch_cpld)))) {
        return -EBADRQC;
    }

    if ((ret = lc_i2c_smbus_read_byte_data(lc_client, lc_control_reg[lc_id])) < 0) {
        return ret;
    }
    reg = ret;

    SKU_LOG_DBG("lc_id:%d get reg:0x%x\n", lc_id, reg);
    if (lv) {
        reg = inv_set_bit(SWICH_CPLD_LC_CPU_PLTRST_N_BIT, reg);
    } else {
        reg = inv_clear_bit(SWICH_CPLD_LC_CPU_PLTRST_N_BIT, reg);
    }
    SKU_LOG_DBG("lc_id:%d set reg:0x%x\n", lc_id, reg);

    if ((ret = lc_i2c_smbus_write_byte_data(lc_client, lc_control_reg[lc_id], reg)) < 0) {
        return ret;
    }

    return 0;
}

int lc_dev_12v_set(int lc_id, bool on)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    u8 reg = 0;

    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!(lc_client = lc_client_find(&(banyan4u_sku.switch_cpld)))) {
        return -EBADRQC;
    }

    if ((ret = lc_i2c_smbus_read_byte_data(lc_client, lc_control_reg[lc_id])) < 0) {
        return ret;
    }
    reg = ret;

    SKU_LOG_DBG("lc_id:%d get reg:0x%x\n", lc_id, reg);
    if (on) {
        reg = inv_set_bit(SWICH_CPLD_LC_PWR_CONTROL_BIT, reg);
    } else {
        reg = inv_clear_bit(SWICH_CPLD_LC_PWR_CONTROL_BIT, reg);
    }
    SKU_LOG_DBG("lc_id:%d set reg:0x%x\n", lc_id, reg);

    if ((ret = lc_i2c_smbus_write_byte_data(lc_client, lc_control_reg[lc_id], reg)) < 0) {
        return ret;
    }

    return 0;
}
int lc_dev_reset_get(int lc_id, u8 *lv)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    u8 reg = 0;

    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!lv) {
        return -EINVAL;
    }

    if (!(lc_client = lc_client_find(&(banyan4u_sku.switch_cpld)))) {
        return -EBADRQC;
    }

    if ((ret = lc_i2c_smbus_read_byte_data(lc_client, lc_control_reg[lc_id])) < 0) {
        return ret;
    }
    reg = ret;

    SKU_LOG_DBG("lc_id:%d get reg:0x%x\n", lc_id, reg);

    if (test_bit(SWICH_CPLD_LC_CPU_PLTRST_N_BIT, (unsigned long *)&reg)) {
        *lv = 1;
    } else {
        *lv = 0;
    }

    return 0;
}

int lc_dev_ucd_power_set(int lc_id, bool on)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    struct lc_dev_obj_t *obj = NULL;
    u8 reg = 0;

    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }

    if (!(lc_client = lc_client_find(&(obj->cpld[CPLD1_ID])))) {
        return -EBADRQC;
    }
    /*control ucd*/
    if ((ret = lc_i2c_smbus_read_byte_data(lc_client, LC_CPLD_MISC_CONTROL1_OFFSET)) < 0) {
        return ret;
    }
    reg = ret;
    SKU_LOG_DBG("1 lc_id:ucd %d get reg:0x%x\n", lc_id, reg);
    if (on) {
        //reg = inv_set_bit(LC_CPLD_UCD_PMBUS_CNTRL_R_BIT, reg);
        set_bit(LC_CPLD_UCD_PMBUS_CNTRL_R_BIT, (unsigned long *)&reg);
    } else {
        clear_bit(LC_CPLD_UCD_PMBUS_CNTRL_R_BIT, (unsigned long *)&reg);
        //reg = inv_clear_bit(LC_CPLD_UCD_PMBUS_CNTRL_R_BIT, reg);
    }
    if ((ret = lc_i2c_smbus_write_byte_data(lc_client, LC_CPLD_MISC_CONTROL1_OFFSET, reg)) < 0) {
        return ret;
    }
    SKU_LOG_DBG("lc_id:ucd %d get reg:0x%x\n", lc_id, reg);

    return 0;
}
/* description: set lc power
 *
 * input:
 *      on:   true:power on  false: power off
 *return:
        0: success
 *  */
int lc_dev_power_set(int lc_id, bool on)
{
    int ret = 0;

    if (on) {
        if ((ret = lc_dev_12v_set(lc_id, true)) < 0) {
            return ret;
        }

        if ((ret = lc_dev_reset_set(lc_id, 1)) < 0) {
            return ret;
        }

        if ((ret = lc_dev_ucd_power_set(lc_id, true)) < 0) {
            return ret;
        }
    } else {

        if ((ret = lc_dev_ucd_power_set(lc_id, false)) < 0) {
            return ret;
        }

        if ((ret = lc_dev_12v_set(lc_id, false)) < 0) {
            return ret;
        }
    }
    return 0;
}

int lc_dev_phy_reset_set(int lc_id, u8 val)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    struct lc_dev_obj_t *obj = NULL;
    u8 reg = 0;
    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }

    if (!(lc_client = lc_client_find(&(obj->cpld[CPLD1_ID])))) {
        return -EBADRQC;
    }
    if (val) {
        reg = 0xff;
    } else {
        reg = 0x0;
    }

    if ((ret = lc_i2c_smbus_write_byte_data(lc_client, LC_CPLD_PHY_RST_OFFSET, reg)) < 0) {
        return ret;
    }

    return 0;
}

int lc_dev_led_set(int lc_id, lc_led_ctrl_t ctrl)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    struct lc_dev_obj_t *obj = NULL;
    u8 reg = 0;

    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }

    if (!(lc_client = lc_client_find(&(obj->cpld[CPLD1_ID])))) {
        return -EBADRQC;
    }

    reg = ctrl;

    if ((ret = lc_i2c_smbus_write_byte_data(lc_client, LC_ST_LED_OFFSET, reg)) < 0) {
        return ret;
    }
    if (p_valid(lc_led_str[ctrl])) {
        //SKU_LOG_DBG("lc_id:%d %s\n", lc_id, lc_led_str[ctrl]);
    }
    return 0;
}

int lc_dev_led_boot_amber_set(int lc_id, bool on)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    struct lc_dev_obj_t *obj = NULL;
    u8 reg = 0;
    int port_grp = 0;
    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }
    for (port_grp = 0; port_grp < PORT_GROUP_NUM; port_grp++) {
        if (!(lc_client = lc_client_find(&(obj->cpld[port_grp])))) {
            ret = -EBADRQC;
            break;
        }
        if ((ret = lc_i2c_smbus_read_byte_data(lc_client, LC_CPLD_LED_DIAG_OFFSET)) < 0) {
            break;
        }
        reg = ret;
        if (on) {
            set_bit(LC_CPLD_LED_BOOT_AMBER_BIT, (unsigned long *)&reg);
        } else {
            clear_bit(LC_CPLD_LED_BOOT_AMBER_BIT, (unsigned long *)&reg);
        }
        if ((ret = lc_i2c_smbus_write_byte_data(lc_client, LC_CPLD_LED_DIAG_OFFSET, reg)) < 0) {
            break;
        }
    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}

int lc_dev_type_get(int lc_id, lc_type_t *type)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    struct lc_dev_obj_t *obj = NULL;
    u8 reg = 0;
    u8 id = 0;

    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!type) {
        return -EINVAL;
    }

    if (!(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }

    if (!(lc_client = lc_client_find(&(obj->cpld[CPLD1_ID])))) {
        return -EBADRQC;
    }

    if ((ret = lc_i2c_smbus_read_byte_data(lc_client, PCB_VER_OFFSET)) < 0) {
        return ret;
    }

    reg = ret;
    id = bits_get(reg, LC_ID_BIT_S, LC_ID_BIT_NUM);

    if (LC_100G_ID == id) {
        *type = LC_100G_TYPE;
    } else if (LC_400G_ID == id) {
        *type = LC_400G_TYPE;
    } else {
        SKU_LOG_ERR("unknown lc type!\n");
        *type = LC_UNKNOWN_TYPE;
        return -EBADRQC;
    }

    return 0;
}

int lc_dev_type_get_text(int lc_id, char *buf, int size)
{
    int ret = 0;
    lc_type_t type = LC_UNKNOWN_TYPE;

    if ((ret = lc_dev_type_get(lc_id, &type)) < 0) {
        return ret;
    }

    scnprintf(buf, size,
              "%s\n",
              lc_type_name[type]);

    return 0;
}

void lc_dev_phy_ready(unsigned long bitmap, bool *ready)
{
    if (PHY_READY_BITMASK == bitmap) {
        *ready = true;
    } else {
        *ready = false;
    }
}

int lc_dev_power_ready(int lc_id, bool *ready)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    struct lc_dev_obj_t *obj = NULL;
    u8 st1 = 0;
    u8 st2 = 0;
    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }
    if (!ready) {
        return -EINVAL;
    }
    if (!(obj = lc_obj_get(lc_id))) {
        return -EBADRQC;
    }

    if (!(lc_client = lc_client_find(&(obj->cpld[CPLD1_ID])))) {
        return -EBADRQC;
    }
    if ((ret = lc_i2c_smbus_read_byte_data(lc_client, LC_CPLD_DC_DC_ST1_OFFSET)) < 0) {
        return ret;
    }
    st1 = ret;
    if ((ret = lc_i2c_smbus_read_byte_data(lc_client, LC_CPLD_DC_DC_ST1_OFFSET)) < 0) {
        return ret;
    }
    st2 = ret;

    if (0xff == st1 &&
            0xff == st2) {
        *ready = true;
    }

    return 0;
}
static int lc_st_get(lc_st_type_t type, unsigned long *bitmap)
{
    if (!bitmap) {
        return -EINVAL;
    }

    if (!banyan4u_sku.lc_st[type].valid) {
        return -EBADRQC;
    }

    *bitmap = banyan4u_sku.lc_st[type].bitmap;
    //SKU_LOG_DBG("%s 0x%lx\n", lc_st_name[type], *bitmap);
    return 0;
}
#if 0
old
static int lc_st_reg_read(lc_st_type_t type, int lc_id, u8 *st)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;

    if (!st) {
        return -EINVAL;
    }
    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!(lc_client = lc_client_find(&(banyan4u_sku.switch_cpld)))) {
        return -EBADRQC;
    }

    if ((ret = lc_i2c_smbus_read_byte_data(lc_client, lc_st_reg[lc_id])) < 0) {
        return ret;
    }

    if (test_bit(lc_st_bit[type], (unsigned long *)&ret)) {
        *st = true;
    } else {
        *st = false;
    }

    return 0;
}
static int lc_st_update(lc_st_type_t type)
{
    int ret = 0;
    u8 st = 0;
    u8 old_st = 0;
    int lc_id = 0;
    struct ldata_format_t *ldata = NULL;

    ldata = &(banyan4u_sku.lc_st[type]);

    for (lc_id = 0; lc_id < CARD_NUM; lc_id++) {
        if ((ret = lc_st_reg_read(type, lc_id, &st)) < 0) {
            break;
        }
        /*dbg to show transition*/
        old_st = ((test_bit(lc_id, &(ldata->bitmap))) ? 1 : 0);
        if (old_st != st) {
            SKU_LOG_DBG("lc_id:%d %s %d -> %d\n", lc_id, lc_st_name[type], old_st, st);
        }
        if (st) {
            set_bit(lc_id, &(ldata->bitmap));
        } else {
            clear_bit(lc_id, &(ldata->bitmap));
        }
    }
    if (ret < 0) {
        ldata->valid = false;
        ldata->bitmap = 0;
        return ret;
    }
    ldata->valid = true;
    return 0;
}
#endif
static int lc_st_reg_read(int lc_id, u8 *reg)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;

    if (!p_valid(reg)) {
        return -EINVAL;
    }
    if (!lc_id_valid(lc_id)) {
        return -EINVAL;
    }

    if (!(lc_client = lc_client_find(&(banyan4u_sku.switch_cpld)))) {
        return -EBADRQC;
    }

    if ((ret = lc_i2c_smbus_read_byte_data(lc_client, lc_st_reg[lc_id])) < 0) {
        return ret;
    }
    *reg = ret;

    return 0;
}

static void lc_st_data_update(u8 reg, int lc_id, lc_st_type_t type, bool normal)
{
    u8 st = 0;
    u8 old_st = 0;
    struct ldata_format_t *ldata = NULL;

    ldata = &(banyan4u_sku.lc_st[type]);
    if (normal) {
        if (test_bit(lc_st_bit[type], (unsigned long *)&reg)) {
            st = true;
        } else {
            st = false;
        }
        /*dbg to show transition*/
        old_st = ((test_bit(lc_id, &(ldata->bitmap))) ? 1 : 0);
        if (old_st != st) {
            SKU_LOG_DBG("lc_id:%d %s %d -> %d\n", lc_id, lc_st_name[type], old_st, st);
        }
        if (st) {
            set_bit(lc_id, &(ldata->bitmap));
        } else {
            clear_bit(lc_id, &(ldata->bitmap));
        }
        ldata->valid = true;
    } else {

        ldata->valid = false;
        ldata->bitmap = 0;
    }
}

static int lc_st_update(void)
{
    int ret = 0;
    u8 reg = 0;
    int lc_id = 0;
    for (lc_id = 0; lc_id < CARD_NUM; lc_id++) {
        if ((ret = lc_st_reg_read(lc_id, &reg)) < 0) {
            lc_st_data_update(reg, lc_id, LC_ST_PRS_TYPE, false);
            lc_st_data_update(reg, lc_id, LC_ST_OVER_TEMP_TYPE, false);
            lc_st_data_update(reg, lc_id, LC_ST_EJ_R_TYPE, false);
            lc_st_data_update(reg, lc_id, LC_ST_EJ_L_TYPE, false);
            break;
        }
        lc_st_data_update(reg, lc_id, LC_ST_PRS_TYPE, true);
        lc_st_data_update(reg, lc_id, LC_ST_OVER_TEMP_TYPE, true);
        lc_st_data_update(reg, lc_id, LC_ST_EJ_R_TYPE, true);
        lc_st_data_update(reg, lc_id, LC_ST_EJ_L_TYPE, true);
    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}
/* description: get line card presence status
 *
 * input:
 * output:
 *      bitmap: bit0 : lc1 ~ bit3 : lc4
 *      1: present 0: absent
 *
 *return:
        0: success
 *  */
int lc_dev_prs_get(unsigned long *bitmap)
{
    int ret = 0;
    unsigned long tmp = 0;

    if ((ret = lc_st_get(LC_ST_PRS_TYPE, &tmp)) < 0) {
        return ret;
    }

    //SKU_LOG_DBG("lc_prs:%lx\n", ~tmp);
    *bitmap = ~tmp;
    return 0;
}

int lc_dev_ej_r_get(unsigned long *bitmap)
{
    int ret = 0;
    unsigned long ej = 0;

    if ((ret = lc_st_get(LC_ST_EJ_R_TYPE, &ej)) < 0) {
        return ret;
    }

    //SKU_LOG_DBG("lc_prs:%lx\n", ~tmp);
    *bitmap = ~ej;
    return 0;
}
int lc_dev_ej_l_get(unsigned long *bitmap)
{
    int ret = 0;
    unsigned long ej = 0;

    if ((ret = lc_st_get(LC_ST_EJ_L_TYPE, &ej)) < 0) {
        return ret;
    }

    //SKU_LOG_DBG("lc_prs:%lx\n", ~tmp);
    *bitmap = ~ej;
    return 0;
}
/* description: get line card temperature INTR bit
 *
 * input:
 * output:
 *      bitmap: bit0 : lc1 ~ bit3 : lc4
 *      0: interrupt 1: normal
 *
 *return:
        0: success
 *  */
int lc_dev_temp_intr_get(unsigned long *bitmap)
{
    int ret = 0;
    unsigned long tmp = 0;


    if (!p_valid(bitmap)) {
        return -EINVAL;
    }

    if ((ret = lc_st_get(LC_ST_OVER_TEMP_TYPE, &tmp)) < 0) {
        return ret;
    }

    *bitmap = tmp;
    return 0;
}
#if 1
static int lc_dev_over_temp_asserted_byDev(int lc_id, int tmp_id, bool *asserted)
{
    int ret = 0;
    unsigned long intr = 0;
    s8 temp_th_h = 0;
    s8 temp = 0;
    int i = 0;
    if (!p_valid(asserted)) {
        return -EINVAL;
    }

    *asserted = false;
    if ((ret = lc_dev_temp_intr_get(&intr)) < 0) {
        return ret;
    }

    if (!test_bit(lc_id, &intr)) {

        if ((ret = lc_dev_temp_get(TEMP_TH_H, lc_id, tmp_id, &temp_th_h)) < 0) {
            return ret;
        }
        for (i = 0; i < TEMP_CHECK_NUM; i++) {

            if ((ret = lc_dev_temp_get(TEMP_INPUT, lc_id, tmp_id, &temp)) < 0) {
                break;
            }
            if (temp < temp_th_h) {
                break;
            }
        }
        if (i >= TEMP_CHECK_NUM) {

            *asserted = true;
            SKU_LOG_INFO("lc_id:%d tmp_id:%d asserted\n", lc_id, tmp_id);
        }

        if (ret < 0) {
            return ret;
        }

    }

    return 0;
}
#else

static int lc_dev_over_temp_asserted_byDev(int lc_id, int tmp_id, bool *asserted)
{
    int ret = 0;
    unsigned long intr = 0;
    s8 temp_th_h = 0;
    s8 temp = 0;
    int i = 0;
    if (!p_valid(asserted)) {
        return -EINVAL;
    }

    *asserted = false;
    if ((ret = lc_dev_temp_intr_get(&intr)) < 0) {
        return ret;
    }

    if (!test_bit(lc_id, &intr)) {
        *asserted = true;
        SKU_LOG_INFO("lc_id:%d tmp_id:%d asserted\n", lc_id, tmp_id);
    }

    return 0;
}

#endif
int lc_dev_over_temp_asserted(int lc_id, bool *asserted)
{
    bool dev_asserted[TMP_DEV_NUM];
    int ret = 0;
    int tmp_id = 0;

    if ((ret = lc_st_update()) < 0) {
        return ret;
    }

    memset(dev_asserted, 0, sizeof(dev_asserted));
    for (tmp_id = 0; tmp_id < TMP_DEV_NUM; tmp_id++) {
        if ((ret = lc_dev_over_temp_asserted_byDev(lc_id, tmp_id, &dev_asserted[tmp_id])) < 0) {
            break;
        }
    }
    if (ret < 0) {
        return ret;
    }

    if (dev_asserted[0] || dev_asserted[1]) {
        *asserted = true;
    }
    return 0;
}
#if 1
static int lc_dev_over_temp_deasserted_byDev(int lc_id, int tmp_id, bool *deasserted)
{
    int ret = 0;
    unsigned long intr = 0;
    s8 temp_th_l = 0;
    s8 temp = 0;
    int i = 0;

    if (!p_valid(deasserted)) {
        return -EINVAL;
    }

    *deasserted = false;
    if ((ret = lc_dev_temp_intr_get(&intr)) < 0) {
        return ret;
    }

    if (test_bit(lc_id, &intr)) {
        if ((ret = lc_dev_temp_get(TEMP_TH_L, lc_id, tmp_id, &temp_th_l)) < 0) {
            return ret;
        }
        for (i = 0; i < TEMP_CHECK_NUM; i++) {

            if ((ret = lc_dev_temp_get(TEMP_INPUT, lc_id, tmp_id, &temp)) < 0) {
                break;
            }
            if (temp > temp_th_l) {
                break;
            }
        }
        if (i >= TEMP_CHECK_NUM) {

            *deasserted = true;
            SKU_LOG_INFO("lc_id:%d tmp_id:%d deasserted\n", lc_id, tmp_id);
        }
        if (ret < 0) {
            return ret;
        }

    }

    return 0;
}
#else

static int lc_dev_over_temp_deasserted_byDev(int lc_id, int tmp_id, bool *deasserted)
{
    int ret = 0;
    unsigned long intr = 0;
    s8 temp_th_l = 0;
    s8 temp = 0;
    int i = 0;

    if (!p_valid(deasserted)) {
        return -EINVAL;
    }

    *deasserted = false;
    if ((ret = lc_dev_temp_intr_get(&intr)) < 0) {
        return ret;
    }

    if (test_bit(lc_id, &intr)) {
        SKU_LOG_INFO("lc_id:%d tmp_id:%d deasserted\n", lc_id, tmp_id);
        *deasserted = true;
    }

    return 0;
}


#endif
int lc_dev_over_temp_deasserted(int lc_id, bool *deasserted)
{
    bool dev_deasserted[TMP_DEV_NUM];
    int ret = 0;
    int tmp_id = 0;

    if ((ret = lc_st_update()) < 0) {
        return ret;
    }

    memset(dev_deasserted, 0, sizeof(dev_deasserted));
    for (tmp_id = 0; tmp_id < TMP_DEV_NUM; tmp_id++) {
        if ((ret = lc_dev_over_temp_deasserted_byDev(lc_id, tmp_id, &dev_deasserted[tmp_id])) < 0) {
            break;
        }
    }
    if (ret < 0) {
        return ret;
    }

    if (dev_deasserted[0] || dev_deasserted[1]) {
        *deasserted = true;
    }
    return 0;
}
#if 0
<reserved>
static int lc_dig_intr_is_asserted(bool *asserted)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    u8 reg = 0;

    if (!(lc_client = lc_client_find(&(banyan4u_sku.switch_cpld)))) {
        return -EBADRQC;
    }

    if ((ret = lc_i2c_smbus_read_byte_data(lc_client, SWITCH_CPLD_LC_ST_INTR_OFFSET)) < 0) {
        return ret;
    }
    reg = ret;
    if (0 == reg) {
        *asserted = true;
        SKU_LOG_DBG("asserted\n");
    } else {
        *asserted = false;
    }
    //SKU_LOG_DBG("reg:0x%x\n", reg);
    return 0;
}
#endif
static int lc_st_intr_is_asserted(bool *asserted)
{
    bool st = false;
    int lv = 0;
    lv = gpio_get_value(banyan4u_sku.lc_intr_gpio);

    //SKU_LOG_DBG("gpio:%d lv:%d\n", banyan4u_sku.lc_intr_gpio, lv);
    if (!lv) {
        st = true;
        SKU_LOG_DBG("asserted\n");
    }
    *asserted = st;
    return 0;
}

int lc_sff_intr_hdlr_byCard(int lc_id)
{
    struct inv_i2c_client_t *lc_client = NULL;
    int ret = 0;
    u8 flag = 0;
    int bit = 0;
    bool asserted = false;

    if((ret = lc_sff_intr_is_asserted(lc_id, &asserted)) < 0) {
        return ret;
    }

    if (!asserted) {
        return 0;
    }

    if (!p_valid(lc_client = lc_client_find(&(banyan4u_sku.obj[lc_id].cpld[CPLD1_ID])))) {
        return -EBADRQC;
    }
    if ((ret = lc_i2c_smbus_read_byte_data(lc_client, LC_CPLD_SFF_INTR_ST_OFFSET)) < 0) {
        return ret;
    }

    flag = ret;

    for (bit = LC_CPLD_SFF_INTR_CPLD2_PRS_BIT; bit < LC_CPLD_SFF_INTR_NUM; bit++) {

        if (!test_bit(bit, (unsigned long *)&flag)) {
            if(p_valid(sff_intr_funcs[bit])) {
                if ((ret = sff_intr_funcs[bit](lc_id)) < 0) {
                    break;
                }
            }
        }
    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}

static int lc_intr_hdlr(void)
{
    int ret = 0;

#if defined (INTR_MODE)
    bool asserted = false;
    if ((ret = lc_st_intr_is_asserted(&asserted)) < 0) {
        return ret;
    }
    if (asserted) {
        if ((ret = lc_dev_intr_st_update()) < 0) {
            return ret;
        }
        if ((ret = lc_st_update()) < 0) {
            return ret;
        }

        if ((ret = lc_dev_intr_st_clear()) < 0) {
            return ret;
        }
    }
#else

    if ((ret = lc_st_update()) < 0) {
        return ret;
    }

#endif
    return 0;
}
#if 0
<reserved>
static int lc_sff_intr_hdlr(void)
{
    int lc_id = 0;
    int ret = 0;
    bool asserted = false;

    for (lc_id = 0; lc_id < CARD_NUM; lc_id++) {

        if((ret = lc_sff_intr_is_asserted(lc_id, &asserted)) < 0) {
            break;
        }
        if (asserted) {
            if((ret = lc_sff_intr_hdlr_byCard(lc_id)) < 0) {
                break;
            }
        }
    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}
#endif
int lc_io_hdlr(void)
{
    int ret = 0;

    if ((ret = lc_intr_hdlr()) < 0) {
        return ret;
    }
#if 0
    if ((ret = lc_sff_intr_hdlr()) < 0) {
        return ret;
    }
#endif
    return 0;
}
static int intr_gpio_init(struct sku_t *mgr)
{
    int ret = 0;
    int lc_id = 0;

    if ((ret = gpio_is_valid(mgr->lc_intr_gpio)) < 0) {

        SKU_LOG_ERR("gpio:%d is invalid ret:%d\n", mgr->lc_intr_gpio, ret);
        return ret;
    }
    if ((ret = gpio_direction_input(mgr->lc_intr_gpio)) < 0) {

        SKU_LOG_ERR("gpio:%d set dir fail ret:%d\n", mgr->lc_intr_gpio, ret);
        return ret;
    } else {

        SKU_LOG_DBG("gpio:%d set dir ok ret:%d\n", mgr->lc_intr_gpio, ret);
    }
    for (lc_id = 0; lc_id < CARD_NUM; lc_id++) {

        if ((ret = gpio_is_valid(mgr->obj[lc_id].sff_intr_gpio)) < 0) {

            SKU_LOG_ERR("gpio:%d is invalid\n", mgr->obj[lc_id].sff_intr_gpio);
            break;
        }
    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}
#if 0
void intr_gpio_deinit(struct sku_t *mgr)
{
}
#endif
void lc_dev_deinit(void)
{
    lc_dev_intr_enable(false);
    lc_i2c_client_deinit_all(&banyan4u_sku);
    lc_i2c_clients_destroy(&banyan4u_sku);
    SKU_LOG_INFO("ok\n");
}
/*need to run after lc plugged in*/
int lc_dev_lc_cpld_init(int lc_id)
{
    int ret = 0;
    int port_grp = 0;
    int mux_l2_id = 0;

    /*line card sff init*/
    for (port_grp = 0; port_grp < PORT_GROUP_NUM; port_grp++) {

        if ((ret = lc_sff_output_bitmap_set(SFF_RESET_TYPE, lc_id, port_grp, lc_sff_output_def[SFF_RESET_TYPE])) < 0) {
            break;
        }

        if ((ret = lc_sff_output_bitmap_set(SFF_LPMODE_TYPE, lc_id, port_grp, lc_sff_output_def[SFF_LPMODE_TYPE])) < 0) {
            break;
        }

        if ((ret = lc_sff_output_bitmap_set(SFF_POWER_TYPE, lc_id, port_grp, lc_sff_output_def[SFF_POWER_TYPE])) < 0) {
            break;
        }

        if ((ret = lc_sff_input_update_byGroup(SFF_PRS_TYPE, lc_id, port_grp)) < 0) {
            break;
        }
        if ((ret = lc_sff_input_update_byGroup(SFF_INTR_TYPE, lc_id, port_grp)) < 0) {
            break;
        }
        if ((ret = lc_sff_input_update_byGroup(SFF_OC_TYPE, lc_id, port_grp)) < 0) {
            break;
        }
    }

    if (ret < 0) {
        return ret;
    }
    /*lc mux init*/
    for (mux_l2_id = 0; mux_l2_id < MUX_L2_NUM; mux_l2_id++) {
        if ((ret = lc_dev_mux_l2_reset(lc_id, mux_l2_id, 1)) < 0) {
            break;
        }
    }
    if (ret < 0) {
        return ret;
    }
    if ((ret = lc_dev_mux_l1_reset(lc_id, 1)) < 0) {
        return ret;
    }

    return 0;
}

static int lc_dev_intr_enable(bool on)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    u8 reg = 0;

    if (!(lc_client = lc_client_find(&(banyan4u_sku.switch_cpld)))) {
        return -EBADRQC;
    }

    if (on) {
        reg = 1;
    } else {
        reg = 0;
    }

    if ((ret = lc_i2c_smbus_write_byte_data(lc_client, SWITCH_CPLD_INTR_EN_OFFSET, reg)) < 0) {
        return ret;
    }

    return 0;
}

static int lc_dev_intr_st_update(void)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;
    u8 reg = 0;

    if (!(lc_client = lc_client_find(&(banyan4u_sku.switch_cpld)))) {
        return -EBADRQC;
    }

    if ((ret = lc_i2c_smbus_read_byte_data(lc_client, SWITCH_CPLD_INTR_ST_OFFSET)) < 0) {
        return ret;
    }
    reg = ret;

    SKU_LOG_DBG("reg:0x%x\n", reg);
    return 0;
}

static int lc_dev_intr_st_clear(void)
{
    int ret = 0;
    struct inv_i2c_client_t *lc_client = NULL;

    if (!(lc_client = lc_client_find(&(banyan4u_sku.switch_cpld)))) {
        return -EBADRQC;
    }

    if ((ret = lc_i2c_smbus_write_byte_data(lc_client, SWITCH_CPLD_INTR_ST_OFFSET, 0)) < 0) {
        return ret;
    }
    return 0;
}

int lc_dev_init(int platform_id, int io_no_init)
{
    int ret = 0;
    (void)platform_id;
    (void)io_no_init;
    config_load();

    if (lc_i2c_clients_create(&banyan4u_sku) < 0) {
        goto err_exit;
    }
    if (lc_i2c_client_init_all(&banyan4u_sku) < 0) {
        goto kfree_lc_client;
    }

    if (intr_gpio_init(&banyan4u_sku) < 0) {
        goto deinit_lc_client;
    }
    if (lc_dev_intr_enable(true) < 0) {
        goto deinit_lc_client;
    }
    if ((ret = lc_st_update()) < 0) {
        goto deinit_lc_client;
    }
    SKU_LOG_INFO("ok\n");
    return 0;
deinit_lc_client:
    lc_i2c_client_deinit_all(&banyan4u_sku);
kfree_lc_client:
    lc_i2c_clients_destroy(&banyan4u_sku);
err_exit:
    ret = -EBADRQC;
    return ret;
}
