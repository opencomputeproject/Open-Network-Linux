#ifndef __SWPS_H
#define __SWPS_H

#include <linux/i2c.h>
#include "module/sff_common.h"

#define INV_BAIDU_SOLUTION (0)
#define SWPS_KSET ("swps")
#define SWPS_POLLING_PERIOD    (msecs_to_jiffies(100))  /* msec */
#define SFF_FSM_DEFAULT_TIMEOUT (200) /*unit swps polling period*/

#define SFF_SUPER_FSM_NORMAL_STABLE_NUM (3) /*unit:swps poling period*/
#define SFF_SUPER_FSM_SFPDD_SW_STABLE_NUM (30) /*unit:swps poling period*/
#define SFF_SUPER_FSM_SFPDD_HW_STABLE_NUM (80) /*unit:swps poling period*/
#define SFF_SUPER_FSM_TIMEOUT_NUM (30) /*unit:swps poling period*/

#define bit_mask(bit) (1 << (bit))
/*bit:start bit from rightmost , num: the num of bits what to be extracted*/
#define bits_get(reg, bit, num) ((reg >> bit) & ((1 << num)-1))
#define sff_to_lc(x) container_of(x, struct lc_obj_t, sff)

#define check_pfunc(p) \
    do { \
        if (p == NULL) { \
        printk( KERN_ERR "%s, %s = NULL.\n", __FUNCTION__, #p); \
        return -ENOSYS; \
        } \
    }while(0)
#define check_p(p) \
    do { \
        if (p == NULL) { \
        printk( KERN_ERR "NULL ptr\n"); \
        return -EINVAL; \
        } \
    }while(0)
enum LOG_LEVEL {

    SWPS_ERR_LEV = 0x1,
    MODULE_ERR_LEV = 0x2,
    SKU_ERR_LEV = 0x04,
    EEPROM_ERR_LEV = 0x08,
    ERR_ALL_LEV = 0xff,

    SWPS_INFO_LEV = 0x100,
    MODULE_INFO_LEV = 0x200,
    SKU_INFO_LEV = 0x400,
    EEPROM_INFO_LEV = 0x800,
    INFO_ALL_LEV = 0xff00,

    SWPS_DBG_LEV = 0x10000,
    MODULE_DBG_LEV = 0x20000,
    SKU_DBG_LEV = 0x40000,
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


#define WORD_SIZE (2)
#define BUF_SIZE (PAGE_SIZE)
#define I2C_RETRY_NUM    (3)
#define I2C_RETRY_DELAY_MS (10)

#define LC_OVER_TEMP_NUM (10)
#define LC_PRS_LOCKED_NUM (5)

typedef enum {
    SFF_IO_PRS_TYPE,
    SFF_IO_RST_TYPE,
    SFF_IO_LPMODE_TYPE,
    SFF_IO_INTR_TYPE,
    SFF_IO_MODSEL_TYPE,
    SFF_IO_TXDISABLE_TYPE,
    SFF_IO_RXLOS_TYPE,
    SFF_IO_TXFAULT_TYPE,
    SFF_IO_TXDISABLE2_TYPE,
    SFF_IO_RXLOS2_TYPE,
    SFF_IO_TXFAULT2_TYPE,
    SFF_IO_TYPE_NUM
} sff_io_type_t;

typedef enum {
    SFF_FSM_ST_REMOVED = 0,
    SFF_FSM_ST_DETECTED,
    SFF_FSM_ST_INIT,
    SFF_FSM_ST_READY,
    SFF_FSM_ST_IDLE,
    SFF_FSM_ST_SUSPEND,
    SFF_FSM_ST_RESTART,
    SFF_FSM_ST_ISOLATED,
    SFF_FSM_ST_IDENTIFY,
    SFF_FSM_ST_DATA_READY_CHECK,
    SFF_FSM_ST_TIMEOUT,
    /*qsfp-dd only {*/
    SFF_FSM_ST_MGMT_INIT,
    SFF_FSM_ST_MODULE_LOOPBACK_INIT,
    SFF_FSM_ST_MODULE_READY,
    SFF_FSM_ST_MODULE_SW_CONFIG_1,
    SFF_FSM_ST_MODULE_SW_CONFIG_1_WAIT,
    SFF_FSM_ST_MODULE_SW_CONFIG_2,
    SFF_FSM_ST_MODULE_SW_CONFIG_CHECK,
    SFF_FSM_ST_MODULE_SW_CONTROL,
    SFF_FSM_ST_MODULE_READY_CHECK,
    /*qsfp-dd only }*/
    SFF_FSM_ST_UNKNOWN_TYPE,
    SFF_FSM_ST_END,
    SFF_FSM_ST_NUM,
} sff_fsm_state_t;

typedef enum {
    SFF_SUPER_FSM_ST_INSERTED,
    SFF_SUPER_FSM_ST_WAIT_STABLE,
    SFF_SUPER_FSM_ST_MODULE_DETECT,
    SFF_SUPER_FSM_ST_RUN,
    SFF_SUPER_FSM_ST_RESTART,
    SFF_SUPER_FSM_ST_SUSPEND,
    SFF_SUPER_FSM_ST_REMOVED,
    SFF_SUPER_FSM_ST_ISOLATED,
    SFF_SUPER_FSM_ST_IDLE,
    SFF_SUPER_FSM_ST_UNSUPPORT,
    SFF_SUPER_FSM_ST_IO_NOINIT,
    SFF_SUPER_FSM_ST_TIMEOUT,
    SFF_SUPER_FSM_ST_NUM
} sff_super_fsm_st_t;

struct swps_kobj_t {
    struct kobject kobj;
    struct sff_obj_t *sff_obj;
};

typedef struct sff_obj_t sff_obj_type;

struct timeout_t {
    int cnt;
    int num;
    bool en;
};

struct fsm_period_t {
    sff_fsm_state_t st;
    int delay_cnt;
    bool timeout_required;
};

struct sff_fsm_t {
    sff_fsm_state_t st;
    int (*task)(sff_obj_type *sff_obj);
    int cnt; /*used to count how many fsm loop's been running*/
    int delay_cnt; /*the target count for each state, will be reset during each state transition*/
    struct fsm_period_t *period_tbl;
    struct timeout_t timeout;
};
struct sff_super_fsm_t {
    sff_super_fsm_st_t st;
    int stable_cnt;
    int stable_num;
    int timeout_cnt;
};

struct u8_format_t {
    u8 val;
    bool valid;
};

struct sff_obj_t {
    int lc_id; /*duplicate lc_id from lc_obj_t lc_id for fast access*/
    char *lc_name; /*duplicate lc_name from lc_obj_t name for fast access*/
    int port;  /*phy port(ga) corresponding to hw connection , eeprom, cpld reg mapping..*/
    int front_port; /*the front port you see from front panel, index from 0 , refer to port_info_map*/
    char *name; /*front port name index from 1 , port1...., refer to port_info_map*/
    sff_type type;
    sff_type def_type;
    int transvr_type;
    struct swps_kobj_t *kobj;
    void *priv_data;
    struct sff_fsm_t fsm;
    struct sff_mgr_t *sff;
    struct func_tbl_t *func_tbl;
    bool page_sel_lock;
    time_t page_sel_lock_time;
    struct sff_super_fsm_t super_fsm;
    bool init_op_required;
};

struct mux_ch_t {
    unsigned long mux_ch;
    unsigned long block_ch;
};
#define MUX_MAX_NUM (32) /*<TBD>*/
struct sff_mgr_t {
    int valid_port_num;
    unsigned long prs;
    struct sff_obj_t *obj;
    int *frontPort_to_port;
    struct swps_kobj_t *mgr_kobj;
    int (*prs_scan)(struct sff_mgr_t *sff);
    unsigned long io_no_init_port_done;
    struct sff_io_driver_t *io_drv;
    struct sff_eeprom_driver_t *eeprom_drv;
};

typedef enum {

    LC_FSM_ST_INSERT,
    LC_FSM_ST_SW_CONFIG,
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
typedef struct swps_mgr_t swps_mgr_type;

typedef enum {
    LC_POSI_INIT_ST,
    LC_POSI_MON_ST,
    LC_POSI_RELEASED_ST,
    LC_POSI_LOCK_CHECK_ST,
    LC_POSI_LOCKED_ST,
} lc_posi_st_t;

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

#define NAME_STR_LEN (10)
struct lc_obj_t {
    struct sff_mgr_t sff;
    struct swps_kobj_t *card_kobj;
    char name[NAME_STR_LEN];
    int lc_id;
    unsigned long prs;
    lc_fsm_st_t st;
    swps_mgr_type *swps;
    bool power_ready;
    bool is_phy_ready;
    unsigned long phy_ready_bitmap;
    int temp;
    lc_type_t type;
    u32 over_temp_cnt;
    bool ej_released;
    bool prs_locked;
    u32 prs_locked_cnt;
    lc_posi_st_t posi_st;
    struct mux_ch_t mux_l1;
};

struct sff_io_func_t {
    int (*set)(int lc_id, unsigned long bitmap);
    int (*get)(int lc_id, unsigned long *bitmap);
};

struct sff_io_driver_t {

    struct sff_io_func_t prs;
    struct sff_io_func_t intr;
    struct sff_io_func_t rxlos;
    struct sff_io_func_t rxlos2;
    struct sff_io_func_t txfault;
    struct sff_io_func_t txfault2;
    struct sff_io_func_t reset;
    struct sff_io_func_t lpmode;
    struct sff_io_func_t modsel;
    struct sff_io_func_t txdisable;
    struct sff_io_func_t txdisable2;
};

struct sff_eeprom_driver_t {
    int (*read)(int lc_id, int port,
                u8 slave_addr,
                u8 offset,
                u8 *buf,
                size_t len);
    int (*write)(int lc_id, int port,
                 u8 slave_addr,
                 u8 offset,
                 const u8 *buf,
                 size_t len);

};
/*the structure contains
 * platform replated function ex:
 * sff io access ,i2c client
 * mux gio reset
 * i2c hang recovery ..
 * cpld interrupt mode ...*/
struct pltfm_func_t {
    int (*init)(int platform_id, int io_no_init);
    void (*deinit)(void);
    int (*io_hdlr)(void);
    bool (*i2c_is_alive)(int lc_id);
    int (*mux_reset_set)(int lc_id, int rst);
    int (*mux_reset_get)(int lc_id, int *rst);
    int (*sff_get_ready_action)(int lc_id, int port);
    int (*sff_detected_action)(int lc_id, int port);
    struct sff_io_func_t sff_oc;
    struct sff_io_func_t sff_power;
    int (*mux_failed_ch_get)(int lc_id, unsigned long *ch);
    int (*mux_blocked_ch_set)(int lc_id, unsigned long ch);
    int (*mux_fail_set)(int lc_id, bool is_fail);
    int (*mux_fail_get)(int lc_id, bool *is_fail);
    int (*mux_ch_to_port)(int lc_id, int ch);
    int (*mux_port_to_ch)(int lc_id, int port);
};
/*the functions only contain 
 * 4U plugable linecard type platfrom*/

struct lc_func_t {
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
    int (*phy_reset_set)(int lc_id, u8 val);
    int (*led_boot_amber_set)(int lc_id, bool on);
    int (*temp_th_set)(int lc_id, int temp);
    int (*intr_hdlr)(int lc_id);
    int (*ej_r_get)(unsigned long *bitmap);
    int (*ej_l_get)(unsigned long *bitmap);
};

struct swps_mgr_t {
    struct lc_obj_t *obj;
    struct swps_kobj_t *common_kobj;
    int lc_num;
    unsigned long lc_sys_ready;
    unsigned long lc_prs;
    unsigned long ej_r;
    unsigned long ej_l;
    unsigned long io_no_init_all_done;
    struct lc_func_t *lc_func;
    struct pltfm_func_t *pltfm_func;
    void (*polling_task)(void);
};
/* func table for each sff object*/
struct func_tbl_t {
    int (*txdisable_set)(struct sff_obj_t *sff_obj, u8 value);
    int (*txdisable_get)(struct sff_obj_t *sff_obj, u8 *value);
    int (*temperature_get)(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
    int (*voltage_get)(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
    int (*lane_control_set)(struct sff_obj_t *sff_obj, int type, u32 value);
    int (*lane_control_get)(struct sff_obj_t *sff_obj, int type, u32 *value);
    int (*lane_monitor_get)(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
    int (*vendor_info_get)(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
    int (*lane_status_get)(struct sff_obj_t *sff_obj, int type, u8 *value);
    int (*module_st_get)(struct sff_obj_t *sff_obj, char *buf, int buf_size);
    int (*data_path_st_get)(struct sff_obj_t *sff_obj, char *buf, int buf_size);
    int (*module_type_get)(struct sff_obj_t *sff_obj, char *buf, int buf_size);
    int (*id_get)(struct sff_obj_t *sff_obj, u8 *id);
    bool (*is_id_matched)(struct sff_obj_t *sff_obj);
    int (*paging_supported)(struct sff_obj_t *sff_obj, bool *supported);
    int (*intr_flag_show)(struct sff_obj_t *sff_obj, char *buf, int size);
    void (*intr_flag_clear)(struct sff_obj_t *sff_obj);
    int (*apsel_apply)(struct sff_obj_t *sff_obj, int apsel);
    int (*apsel_get)(struct sff_obj_t *sff_obj);
    int (*active_ctrl_set_get)(struct sff_obj_t *sff_obj, char *buf, int size);
    void (*rev4_quick_set)(struct sff_obj_t *sff_obj, bool en);
    bool (*rev4_quick_get)(struct sff_obj_t *sff_obj);
    int (*rev_get)(struct sff_obj_t *sff_obj, char *buf, int size);
    int (*tx_eq_type_set)(struct sff_obj_t *sff_obj, tx_eq_type_t type);
    tx_eq_type_t (*tx_eq_type_get)(struct sff_obj_t *sff_obj);
    /*these two functions are called by sff_super_fsm*/
    int (*init_op)(struct sff_obj_t *sff_obj);
    int (*remove_op)(struct sff_obj_t *sff_obj);
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
    NATIVE_METHOD,
    MUX_DRV_METHOD
} i2c_recovery_method_t;

struct i2c_recovery_feature_t {
    bool en;
    i2c_recovery_method_t method;
};

typedef enum {
    I2C_CRUSH_INIT_ST,
    I2C_CRUSH_IO_I2C_CHECK_ST,
    I2C_CRUSH_BAD_TRANSVR_DETECT_ST,
    I2C_CRUSH_I2C_RECHECK_ST,
    I2C_CRUSH_END_ST,
} i2c_crush_hande_st;

struct int_vector_t {
    int *tbl;
    int size;
};

struct ldata_format_t {

    unsigned long bitmap;
    unsigned long valid;
};

void transvr_type_set(struct sff_obj_t *sff_obj, int type);
int transvr_type_get(struct sff_obj_t *sff_obj);
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
int i2c_smbus_write_i2c_block_data_retry(struct i2c_client *client, u8 offset, int len, const u8 *buf);
bool p_valid(const void *ptr);
int sff_eeprom_write(struct sff_obj_t *sff_obj,u8 addr, u8 offset, const u8 *buf, int len);
int sff_paged_eeprom_write(struct sff_obj_t *sff_obj,u8 addr, u8 offset, const u8 *buf, int len);
int sff_eeprom_read(struct sff_obj_t *sff_obj, u8 addr, u8 offset, u8 *buf, int len);
int sff_paged_eeprom_read(struct sff_obj_t *sff_obj, u8 addr, u8 offset, u8 *buf, int len);
bool page_sel_is_locked(struct sff_obj_t *sff_obj);
void page_sel_lock(struct sff_obj_t *sff_obj);
void page_sel_unlock(struct sff_obj_t *sff_obj);
void page_sel_lock_time_set(struct sff_obj_t *sff_obj);
void page_sel_lock_time_clear(struct sff_obj_t *sff_obj);
time_t get_system_time(void);
void cnt_increment_limit(u32 *data);
/*sff io common functions*/
int sff_prs_get(struct sff_obj_t *sff_obj, u8 *prs);
int sff_lpmode_set(struct sff_obj_t *sff_obj, u8 value);
int sff_lpmode_get(struct sff_obj_t *sff_obj, u8 *value);
int sff_modsel_set(struct sff_obj_t *sff_obj, u8 value);
int sff_modsel_get(struct sff_obj_t *sff_obj, u8 *value);
int sff_reset_set(struct sff_obj_t *sff_obj, u8 value);
int sff_reset_get(struct sff_obj_t *sff_obj, u8 *value);
int sff_intr_get(struct sff_obj_t *sff_obj, u8 *value);
int sff_txdisable_set(struct sff_obj_t *sff_obj, u8 value);
int sff_txdisable_get(struct sff_obj_t *sff_obj, u8 *value);
int sff_rxlos_get(struct sff_obj_t *sff_obj, u8 *value);
int sff_txfault_get(struct sff_obj_t *sff_obj, u8 *value);
/*dummy functions*/
int dummy_reset_set(struct sff_obj_t *sff_obj, u8 value);
int dummy_reset_get(struct sff_obj_t *sff_obj, u8 *value);
int dummy_lpmode_set(struct sff_obj_t *sff_obj, u8 value);
int dummy_lpmode_get(struct sff_obj_t *sff_obj, u8 *value);
int dummy_modsel_set(struct sff_obj_t *sff_obj, u8 value);
int dummy_modsel_get(struct sff_obj_t *sff_obj, u8 *value);
int dummy_intr_get(struct sff_obj_t *sff_obj, u8 *value);
int dummy_module_st_get(struct sff_obj_t *sff_obj, u8 *st);
int dummy_eeprom_dump(struct sff_obj_t *sff_obj, u8 *buf);
int dummy_page_sel(struct sff_obj_t *sff_obj, int page);
int dummy_page_sel(struct sff_obj_t *sff_obj, int page);
int dummy_page_get(struct sff_obj_t *sff_obj, u8 *page);
int dummy_lane_control_set(struct sff_obj_t *sff_obj, int type, u32 value);
int dummy_lane_control_get(struct sff_obj_t *sff_obj, int type, u32 *value);
int dummy_lane_status_get(struct sff_obj_t *sff_obj, int type, u8 *st);
extern u32 logLevel;
extern bool int_flag_monitor_en;
#endif /* __SWPS_H */


