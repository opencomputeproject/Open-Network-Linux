#ifndef __SWPS_H
#define __SWPS_H

#include <linux/i2c.h>
#include "sff_spec.h"
#include "lc_dev.h"
#include "io_dev.h"

#define READBACK_CHECK
#define DEBUG_LOG (1)

#define SWPS_KSET ("swps")
#define TRY_NUM (5)
#define SFF_EEPROM_I2C_ADDR (0xA0 >> 1)
#define SFF_DDM_I2C_ADDR (0xA2 >> 1)
#define WORD_SIZE (2)
#define SFP_LANE_NUM (1)
#define QSFP_LANE_NUM (4)
#define QSFP_DD_LANE_NUM (8)
#define VENDOR_INFO_BUF_SIZE (32)
#define BUF_SIZE (PAGE_SIZE)
#define MUX_CH_NUM  (8)

#define SWPS_POLLING_PERIOD    (msecs_to_jiffies(100))  /* msec */

#define DYNAMIC_SFF_KOBJ

#define PAGE_NUM  (256)
#define EEPROM_SIZE (256)
//#define QSFP_INT_FLAG_SUPPORT
#define EEPROM_HALF_SIZE (128)
#define QSFP_INT_FLAG_SUPPORT
#define PAGE_SEL_LOCK_NUM (20)
#define CLEAR_CMD ("clear\n")

#define bit_mask(bit) (1 << (bit))
/*bit:start bit from rightmost , num: the num of bits what to be extracted*/
#define bits_get(reg, bit, num) ((reg >> bit) & ((1 << num)-1))
#define sff_to_lc(x) container_of(x, struct lc_obj_t, sff)

#if 0
#if (DEBUG_LOG == 1)
#define SWPS_LOG_DEBUG(fmt, args...) \
    printk (KERN_INFO "[SWPS]%s " fmt "\r\n",__FUNCTION__,  ##args)
#else
#define SWPS_LOG_DEBUG(fmt, args...)
#endif
#define SWPS_LOG_INFO(fmt, args...) printk (KERN_INFO "[SWPS]%s:"fmt,__FUNCTION__,  ##args)
#define SWPS_LOG_ERR(fmt, args...)  printk (KERN_ERR "[SWPS]%s: " fmt "\r\n",__FUNCTION__,  ##args)
#endif
#define check_pfunc(p) \
    do { \
        if (p == NULL) { \
        printk( KERN_ERR "%s, %s = NULL.\n", __FUNCTION__, #p); \
        return -ENOSYS; \
        } \
    }while(0)
enum LOG_LEVEL {

    SWPS_ERR_LEV = 0x1,
    MODULE_ERR_LEV = 0x2,
    DEV_ERR_LEV = 0x04,
    EEPROM_ERR_LEV = 0x08,
    ERR_ALL_LEV = 0xff,

    SWPS_INFO_LEV = 0x100,
    MODULE_INFO_LEV = 0x200,
    DEV_INFO_LEV = 0x400,
    EEPROM_INFO_LEV = 0x800,
    INFO_ALL_LEV = 0xff00,

    SWPS_DBG_LEV = 0x10000,
    MODULE_DBG_LEV = 0x20000,
    DEV_DBG_LEV = 0x40000,
    EEPROM_DBG_LEV = 0x80000,
    DBG_ALL_LEV = 0xff0000,

};

#define SWPS_LOG_ERR(fmt, args...) \
    do { \
        if (logLevel & SWPS_ERR_LEV) \
        { \
            printk (KERN_ERR "[SWPS]%s: "fmt,__FUNCTION__,  ##args); \
        } \
    } while (0)

#define SWPS_LOG_INFO(fmt, args...) \
    do { \
        if (logLevel & SWPS_INFO_LEV) \
        { \
            printk (KERN_INFO "[SWPS]%s: "fmt,__FUNCTION__,  ##args); \
        } \
    } while (0)

#define SWPS_LOG_DBG(fmt, args...) \
    do { \
        if (logLevel & SWPS_DBG_LEV) \
        { \
            printk (KERN_INFO "[SWPS]%s: "fmt,__FUNCTION__,  ##args); \
        } \
    } while (0)

#define MODULE_LOG_ERR(fmt, args...) \
    do { \
        if (logLevel & MODULE_ERR_LEV) \
        { \
            printk (KERN_ERR "[MODULE]%s: "fmt,__FUNCTION__,  ##args); \
        } \
    } while (0)

#define MODULE_LOG_INFO(fmt, args...) \
    do { \
        if (logLevel & MODULE_INFO_LEV) \
        { \
            printk (KERN_INFO "[MODULE]%s: "fmt,__FUNCTION__,  ##args); \
        } \
    } while (0)

#define MODULE_LOG_DBG(fmt, args...) \
    do { \
        if (logLevel & MODULE_DBG_LEV) \
        { \
            printk (KERN_INFO "[MODULE]%s: "fmt,__FUNCTION__,  ##args); \
        } \
    } while (0)


/*public function*/
#define I2C_RETRY_NUM    (3)
#define I2C_RETRY_DELAY_MS (10)

#define LC_INSERT_WAIT_STABLE_NUM (10)
#define LC_OVER_TEMP_NUM (10)
#define LC_PRS_LOCKED_NUM (5)

typedef enum {
    SFP_TYPE = 0,
    QSFP_TYPE,
    QSFP_DD_TYPE,
    SFF_TYPE_NUM
} sff_type;
#if 0
typedef enum {
    TX_ALL_CH_DISABLE_OFF = 0x00,
    TX_CH1_DISABLE_ON = 1 << 0,
    TX_CH2_DISABLE_ON = 1 << 1,
    TX_CH3_DISABLE_ON = 1 << 2,
    TX_CH4_DISABLE_ON = 1 << 3,
    TX_CH5_DISABLE_ON = 1 << 4,
    TX_CH6_DISABLE_ON = 1 << 5,
    TX_CH7_DISABLE_ON = 1 << 6,
    TX_CH8_DISABLE_ON = 1 << 7,
    TX_1CH_DISABLE_ON = TX_CH1_DISABLE_ON, /*sfp*/
    TX_4CH_DISABLE_ON = 0x0f, /*qsfp*/
    TX_8CH_DISABLE_ON = 0xff, /*qsfp-dd*/

} tx_disable_t;
#endif
/*so far it's for sfp only*/
typedef enum {
    SOFT_RX_RATE_RS0 = 1 << 0,
    SOFT_TX_RATE_RS1 = 1 << 1,
} rate_control_t;
typedef enum {
    SFF_IO_RST_TYPE,
    SFF_IO_PWR_TYPE,
    SFF_IO_LPMODE_TYPE,
    SFF_IO_OUTPUT_TYPE_NUM    
} sff_io_output_type_t;
typedef enum {
    TX_EQ_TYPE,
    RX_EM_TYPE,
    RX_AM_TYPE,
} sysfs_attrbute_index_ln_control_t;

typedef enum {
    LN_MONITOR_RX_PWR_TYPE,
    LN_MONITOR_TX_PWR_TYPE,
    LN_MONITOR_TX_BIAS_TYPE,
} sysfs_attrbute_index_ln_monitor_t;

typedef enum {
    LN_STATUS_RX_LOS_TYPE,
    LN_STATUS_TX_LOS_TYPE,
    LN_STATUS_TX_FAULT_TYPE,
    LN_STATUS_NUM,
} sysfs_attrbute_index_ln_status_t;

typedef enum {
    VENDOR_NAME_TYPE,
    VENDOR_PN_TYPE,
    VENDOR_SN_TYPE,
    VENDOR_REV_TYPE,
} sysfs_attrbute_index_vendor_info_t;

typedef enum {
    SFF_FSM_ST_REMOVED = 0,
    SFF_FSM_ST_INSERTED,
    SFF_FSM_ST_DETECTING,
    SFF_FSM_ST_INIT,
    SFF_FSM_ST_READY,
    SFF_FSM_ST_IDLE,
    SFF_FSM_ST_FAULT,
    SFF_FSM_ST_SUSPEND,
    SFF_FSM_ST_RESTART,
    SFF_FSM_ST_ISOLATED,
    SFF_FSM_ST_IDENTIFY,
    SFF_FSM_ST_MONITOR,
    /*qsfp-dd only {*/
    SFF_FSM_ST_MGMT_INIT,
    SFF_FSM_ST_MODULE_UNRESET,
    SFF_FSM_ST_MODULE_HW_INIT,
    SFF_FSM_ST_MODULE_LOOPBACK_INIT,
    SFF_FSM_ST_MODULE_READY,
    SFF_FSM_ST_MODULE_PWR_DOWN,
    SFF_FSM_ST_MODULE_CMIS_VER_CHECK,
    SFF_FSM_ST_MODULE_ADVERT_CHECK,
    SFF_FSM_ST_MODULE_SW_CONFIG_1,
    SFF_FSM_ST_MODULE_SW_CONFIG_2,
    SFF_FSM_ST_MODULE_SW_CONFIG_CHECK,
    SFF_FSM_ST_MODULE_SW_CONTROL,
    SFF_FSM_ST_MODULE_READY_CHECK,
    /*qsfp-dd only }*/
    SFF_FSM_ST_UNKNOWN_TYPE,
    SFF_FSM_ST_END,
    SFF_FSM_ST_NUM,
} sff_fsm_state_t;
#define QSFP_DD_INT_LN_FLAG_NUM (19)
#define QSFP_DD_INT_MODULE_FLAG_NUM (6)

struct intr_flag_t {
    u8 reg;
    u32 cnt;
    bool chg;
};

struct swps_kobj_t {
    struct kobject kobj;
    struct sff_obj_t *sff_obj;
};

typedef struct sff_obj_t sff_obj_type;

struct sff_fsm_t {
    sff_fsm_state_t st;
    int (*task)(sff_obj_type *sff_obj);
    int cnt; /*used to count how many fsm loop's been running*/
    int delay_cnt; /*the target count for each state, will be reset during each state transition*/
    struct fsm_period_t *period_tbl;
};

struct qsfp_dd_fsm_func_t {
    int (*advert_check)(struct sff_obj_t *sff_obj, bool *pass);
    int (*sw_config_1)(struct sff_obj_t *sff_obj);
    int (*sw_config_2)(struct sff_obj_t *sff_obj, bool *pass);
    int (*sw_config_check)(struct sff_obj_t *sff_obj, bool *pass);
    int (*sw_control)(struct sff_obj_t *sff_obj);
    int (*module_ready_check)(struct sff_obj_t *sff_obj, bool *ready);
};

struct qsfp_dd_priv_data {
    int lane_num;
    u8 module_type;
    union qsfp_dd_app_advert_fields fields[APSEL_NUM];
    bool paging_supported;
    bool rev4_quick_en;
    u8 eeprom_cache[PAGE_NUM][EEPROM_SIZE];
    struct intr_flag_t intr_module_flag[QSFP_DD_INT_MODULE_FLAG_NUM];
    struct intr_flag_t intr_ln_flag[QSFP_DD_INT_LN_FLAG_NUM];
    u8 lane_st[LN_STATUS_NUM];
    struct qsfp_dd_fsm_func_t *fsm_func;
    int apsel;
};

struct qsfp_priv_data {
    u8 lane_st[LN_STATUS_NUM];
    u8 eeprom_cache[PAGE_NUM][EEPROM_SIZE];
    bool paging_supported;
};

union priv_data_t {
    struct qsfp_dd_priv_data qsfp_dd;
    struct qsfp_priv_data qsfp;
};

struct sff_obj_t {
    int lc_id; /*duplicate lc_id from lc_obj_t lc_id for fast access*/
    char *lc_name; /*duplicate lc_id from lc_obj_t lc_id for fast access*/
    int port;  /*phy port(ga) corresponding to hw connection , eeprom, cpld reg mapping..*/
    int front_port; /*the front port you see from front panel, index from 0 , refer to port_info_map*/
    char *name; /*front port name index from 1 , port1...., refer to port_info_map*/
    sff_type type;
    int transvr_type;
    struct swps_kobj_t *kobj;
    union priv_data_t priv_data;
    struct sff_fsm_t fsm;
    struct sff_mgr_t *mgr;
    struct func_tbl_t *func_tbl;
    bool page_sel_lock;
};
struct mux_ch_t {
    int i2c_ch;
    unsigned long mux_ch;
    bool is_fail;
    unsigned long block_ch;
};
#define MUX_MAX_NUM (32) /*<TBD>*/
struct sff_mgr_t {
    int valid_port_num;
    unsigned long prs;
    struct sff_obj_t *obj;
    int *frontPort_to_port;
    struct swps_kobj_t *mgr_kobj;
    struct swps_kobj_t *common_kobj;
    int (*prs_scan)(struct sff_mgr_t *sff);
    unsigned long io_no_init_port_done;
    struct sff_io_driver_t *io_drv;
    struct sff_eeprom_driver_t *eeprom_drv;
};
#if 0
typedef enum {
    LC_LED_CTRL_OFF = 0,
    LC_LED_CTRL_GREEN_ON,
    LC_LED_CTRL_RED_ON,
    LC_LED_CTRL_AMBER_ON,
    LC_LED_CTRL_GREEN_BLINK,
    LC_LED_CTRL_RED_BLINK,
    LC_LED_CTRL_AMBER_BLINK,
    LC_LED_CTRL_NUM,
} lc_led_ctrl_t;

typedef enum {
    LC_UNKNOWN_TYPE = 0,
    LC_100G_TYPE,
    LC_400G_TYPE,
    LC_TYPE_NUM,
} lc_type_t;
#endif

typedef enum {

    LC_FSM_ST_INSERT,
    LC_FSM_ST_WAIT_STABLE,
    LC_FSM_ST_POWER_ON,
    LC_FSM_ST_POWER_CHECK,
    LC_FSM_ST_PHY_CHECK,
    LC_FSM_ST_INIT,
    LC_FSM_ST_READY,
    LC_FSM_ST_REMOVE,
    LC_FSM_ST_IDLE,
    LC_FSM_ST_THERMAL_TRIP,
    LC_FSM_ST_UNSUPPORTED,
    LC_FSM_ST_FAULT,
    LC_FSM_ST_NUM,

} lc_fsm_st_t;
typedef struct lc_t lc_mgr_type;

typedef enum {
    LC_POSI_INIT_ST,
    LC_POSI_MON_ST,
    LC_POSI_RELEASED_ST,
    LC_POSI_LOCK_CHECK_ST,
    LC_POSI_LOCKED_ST,
}lc_posi_st_t; 

struct lc_obj_t {
    struct sff_mgr_t sff;
    struct swps_kobj_t *card_kobj;
    char *name;
    int lc_id;
    unsigned long prs;
    lc_fsm_st_t st;
    lc_mgr_type *mgr;
    bool power_ready;
    bool is_phy_ready;
    unsigned long phy_ready_bitmap;
    int temp;
    lc_type_t type;
    u32 wait_stable_cnt;
    u32 over_temp_cnt;
    bool ej_released;
    bool prs_locked;
    u32 prs_locked_cnt;
    lc_posi_st_t posi_st;    
    struct mux_ch_t mux_l1;
};

struct sff_io_driver_t {
    int (*prs_all_get)(int lc_id, unsigned long *bitmap);
    int (*intr_all_get)(int lc_id, unsigned long *bitmap);
    int (*rx_los_all_get)(int lc_id, unsigned long *bitmap);
    int (*tx_fault_all_get)(int lc_id, unsigned long *bitmap);
    int (*reset_set)(int lc_id, int port, u8 reset);
    int (*reset_get)(int lc_id, int port, u8 *reset);
    int (*reset_all_set)(int lc_id, unsigned long bitmap);
    int (*reset_all_get)(int lc_id, unsigned long *bitmap);
    int (*power_set)(int lc_id, int port, u8 reset);
    int (*power_get)(int lc_id, int port, u8 *reset);
    int (*power_all_set)(int lc_id, unsigned long bitmap);
    int (*power_all_get)(int lc_id, unsigned long *bitmap);
    int (*lpmode_set)(int lc_id, int port, u8 value);
    int (*lpmode_get)(int lc_id, int port, u8 *value);
    int (*lpmode_all_set)(int lc_id, unsigned long bitmap);
    int (*lpmode_all_get)(int lc_id, unsigned long *bitmap);
    int (*tx_disable_set)(int lc_id, int port, u8 value);
    int (*tx_disable_get)(int lc_id, int port, u8 *value);
    int (*mode_sel_set)(int lc_id, int port, u8 value);
    int (*mode_sel_get)(int lc_id, int port, u8 *value);
    int (*oc_all_get)(int lc_id, unsigned long *bitmap);
};

struct sff_eeprom_driver_t {
    int (*eeprom_read)(int lc_id, int port,
                       u8 slave_addr,
                       u8 offset,
                       u8 *buf,
                       size_t len);
    int (*eeprom_write)(int lc_id, int port,
                        u8 slave_addr,
                        u8 offset,
                        const u8 *buf,
                        size_t len);

};
struct lc_func_t {
    int (*dev_init)(int platform_id, int io_no_init);
    void (*dev_deinit)(void);
    void (*polling_task)(void);
    int (*dev_hdlr)(void);
    bool (*i2c_is_alive)(int lc_id);
    int (*cpld_init)(int lc_id);
    int (*power_set)(int lc_id, bool on);
    int (*power_ready)(int lc_id, bool *ready);
    void (*phy_ready)(unsigned long bitmap, bool *ready);
    int (*prs_get)(unsigned long *bitmap);
    int (*reset_set)(int lc_id, u8 lv);
    int (*reset_get)(int lc_id, u8 *lv);
    int (*type_get)(int lc_id, lc_type_t *type);
    int (*type_get_text)(int lc_id, char *buf, int size);
    int (*over_temp_asserted)(int lc_id, bool *asserted);
    int (*over_temp_deasserted)(int lc_id, bool *deasserted);
    int (*temp_get)(int lc_id, char *buf, int size);
    int (*temp_th_get)(int lc_id, char *buf, int size);
    int (*led_set)(int lc_id, lc_led_ctrl_t ctrl);
    int (*mux_reset_set)(int lc_id, int rst);
    int (*mux_reset_get)(int lc_id, int *rst);
    int (*phy_reset_set)(int lc_id, u8 val);

};
struct lc_t {
    struct lc_obj_t *obj;
    struct swps_kobj_t *common_kobj;
    int lc_num;
    unsigned long lc_sys_ready;
    unsigned long lc_prs;
    unsigned long ej_r;
    unsigned long ej_l;
    unsigned long io_no_init_all_done;
    struct lc_func_t *lc_func;
};
/* func table for each sff object*/
struct func_tbl_t {
    /* private function , for driver internal use {*/
    int (*eeprom_read)(struct sff_obj_t *sff_obj, u8 addr, u8 offset, u8 *buf, int len);
    int (*eeprom_write)(struct sff_obj_t *sff_obj, u8 addr, u8 offset, const u8 *buf, int len);
    /* }*/
    /*sff low speed signals for both driver internal use and export to sysfs interface*/
    int (*prs_get)(struct sff_obj_t *sff_obj, u8 *value);
    int (*lpmode_set)(struct sff_obj_t *sff_obj, u8 value);
    int (*lpmode_get)(struct sff_obj_t *sff_obj, u8 *value);
    int (*reset_set)(struct sff_obj_t *sff_obj, u8 value);
    int (*reset_get)(struct sff_obj_t *sff_obj, u8 *value);
    int (*power_set)(struct sff_obj_t *sff_obj, u8 value);
    int (*power_get)(struct sff_obj_t *sff_obj, u8 *value);
    int (*mode_sel_set)(struct sff_obj_t *sff_obj, u8 value);
    int (*mode_sel_get)(struct sff_obj_t *sff_obj, u8 *value);
    int (*intL_get)(struct sff_obj_t *sff_obj, u8 *value);
    int (*tx_disable_set)(struct sff_obj_t *sff_obj, u8 value);
    int (*tx_disable_get)(struct sff_obj_t *sff_obj, u8 *value);
    int (*temperature_get)(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
    int (*voltage_get)(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
    int (*lane_control_set)(struct sff_obj_t *sff_obj, int type, u32 value);
    int (*lane_control_get)(struct sff_obj_t *sff_obj, int type, u32 *value);
    int (*lane_monitor_get)(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
    int (*vendor_info_get)(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
    int (*lane_status_get)(struct sff_obj_t *sff_obj, int type, u8 *value);
    int (*module_st_get)(struct sff_obj_t *sff_obj, u8 *st);
    int (*id_get)(struct sff_obj_t *sff_obj, u8 *id);
    bool (*is_id_matched)(struct sff_obj_t *sff_obj);
    int (*eeprom_dump)(struct sff_obj_t *sff_obj, u8 *buf);
    int (*page_sel)(struct sff_obj_t *sff_obj, int page);
    int (*page_get)(struct sff_obj_t *sff_obj, u8 *page);
    int (*intr_flag_show)(struct sff_obj_t *sff_obj, char *buf, int size);
    void (*intr_flag_clear)(struct sff_obj_t *sff_obj);
};

struct port_info_map_t {
    int front_port; /*the front port you see from front panel*/
    sff_type type;
    char *name;  /*front port name index from 1 , port1....*/
};

struct port_info_table_t {
    struct port_info_map_t *map;
    int size;
};

/*it is for compose monitor data into u16 format from u8*/
union monitor_data_t {
    u8 byte[2];
    u16 word;
    s16 signed_word;
};

struct monitor_para_t {
    int type;
    u16 divider;
    char *unit;
};

typedef enum {
    TRANSVR_CLASS_UNKNOWN=0,
    /* Transceiver class for Optical 10G */
    TRANSVR_CLASS_OPTICAL_10G_S_AOC =(27011),
    TRANSVR_CLASS_OPTICAL_10G_S_SR  =(27012),
    TRANSVR_CLASS_OPTICAL_10G_S_LR  =(27013),
    TRANSVR_CLASS_OPTICAL_10G_S_ER  =(27014),
    TRANSVR_CLASS_OPTICAL_10G_Q_AOC =(27015),
    TRANSVR_CLASS_OPTICAL_10G_Q_SR  =(27016),
    TRANSVR_CLASS_OPTICAL_10G_Q_LR  =(27017),
    TRANSVR_CLASS_OPTICAL_10G_Q_ER  =(27018),
    /* Transceiver class for Optical 25G */
    TRANSVR_CLASS_OPTICAL_25G_AOC   =(27021),
    TRANSVR_CLASS_OPTICAL_25G_SR    =(27022),
    TRANSVR_CLASS_OPTICAL_25G_LR    =(27023),
    TRANSVR_CLASS_OPTICAL_25G_ER    =(27024),
    /* Transceiver class for Optical 40G */
    TRANSVR_CLASS_OPTICAL_40G_AOC   =(27041),
    TRANSVR_CLASS_OPTICAL_40G_SR4   =(27042),
    TRANSVR_CLASS_OPTICAL_40G_LR4   =(27043),
    TRANSVR_CLASS_OPTICAL_40G_ER4   =(27044),
    /* Transceiver class for Optical 100G */
    TRANSVR_CLASS_OPTICAL_100G_AOC  =(27101),
    TRANSVR_CLASS_OPTICAL_100G_SR4  =(27102),
    TRANSVR_CLASS_OPTICAL_100G_LR4  =(27103),
    TRANSVR_CLASS_OPTICAL_100G_ER4  =(27104),
    TRANSVR_CLASS_OPTICAL_100G_PSM4 =(27105),
    TRANSVR_CLASS_OPTICAL_100G_CWDM4 =(27106),
    /* Transceiver class for Copper */
    TRANSVR_CLASS_COPPER_L1_1G      =(28001),
    TRANSVR_CLASS_COPPER_L1_10G     =(28011),
    TRANSVR_CLASS_COPPER_L4_10G     =(28012),
    TRANSVR_CLASS_COPPER_L1_25G     =(28021),
    TRANSVR_CLASS_COPPER_L4_40G     =(28041),
    TRANSVR_CLASS_COPPER_L4_100G    =(28101),

} transvr_type_t;

typedef enum {
    I2C_CRUSH_INIT_ST,
    I2C_CRUSH_IO_I2C_CHECK_ST,
    I2C_CRUSH_BAD_TRANSVR_DETECT_ST,
    I2C_CRUSH_I2C_RECHECK_ST,
    I2C_CRUSH_END_ST,
} i2c_crush_hande_st;

void transvr_type_set(struct sff_obj_t *sff_obj, int type);
int transvr_type_get(struct sff_obj_t *sff_obj);
//u8 masked_bits_get(u8 val, int bit_s, int bit_e);
sff_fsm_state_t sff_fsm_st_get(struct sff_obj_t *sff_obj);
void sff_fsm_st_set(struct sff_obj_t *sff_obj, sff_fsm_state_t st);
void sff_fsm_state_change_process(struct sff_obj_t *sff_obj, sff_fsm_state_t cur_st, sff_fsm_state_t next_st);
struct monitor_para_t *monitor_para_find(int type);
int sff_fsm_kobj_change_event(struct sff_obj_t *sff_obj);

int i2c_smbus_write_byte_data_retry(struct i2c_client *client, u8 offset, u8 data);
int i2c_smbus_read_byte_data_retry(struct i2c_client *client, u8 offset);
int i2c_smbus_write_word_data_retry(struct i2c_client *client, u8 offset, u16 buf);
int i2c_smbus_read_word_data_retry(struct i2c_client *client, u8 offset);
int i2c_smbus_read_i2c_block_data_retry(struct i2c_client *client, u8 offset, int len, u8 *buf);
bool p_valid(const void *ptr);
int sff_eeprom_read(struct sff_obj_t *sff_obj, u8 addr, u8 offset, u8 *buf, int len);
int sff_eeprom_write(struct sff_obj_t *sff_obj,u8 addr, u8 offset, const u8 *buf, int len);
int sff_prs_get(struct sff_obj_t *sff_obj, u8 *prs);
int sff_power_set(struct sff_obj_t *sff_obj, u8 value);
int sff_power_get(struct sff_obj_t *sff_obj, u8 *value);
int dummy_reset_set(struct sff_obj_t *sff_obj, u8 value);
int dummy_reset_get(struct sff_obj_t *sff_obj, u8 *value);
int dummy_lpmode_set(struct sff_obj_t *sff_obj, u8 value);
int dummy_lpmode_get(struct sff_obj_t *sff_obj, u8 *value);
int dummy_mode_sel_set(struct sff_obj_t *sff_obj, u8 value);
int dummy_mode_sel_get(struct sff_obj_t *sff_obj, u8 *value);
int dummy_intL_get(struct sff_obj_t *sff_obj, u8 *value);
int dummy_module_st_get(struct sff_obj_t *sff_obj, u8 *st);
int dummy_eeprom_dump(struct sff_obj_t *sff_obj, u8 *buf);
int dummy_page_sel(struct sff_obj_t *sff_obj, int page);
int dummy_page_sel(struct sff_obj_t *sff_obj, int page);
int dummy_page_get(struct sff_obj_t *sff_obj, u8 *page);
int dummy_lane_control_set(struct sff_obj_t *sff_obj, int type, u32 value);
int dummy_lane_control_get(struct sff_obj_t *sff_obj, int type, u32 *value);
int dummy_lane_status_get(struct sff_obj_t *sff_obj, int type, u8 *st);
char *port_name_get(int lc_id, int port);
bool page_sel_is_locked(struct sff_obj_t *sff_obj);
void page_sel_lock(struct sff_obj_t *sff_obj);
void page_sel_unlock(struct sff_obj_t *sff_obj);

#endif /* __SWPS_H */


