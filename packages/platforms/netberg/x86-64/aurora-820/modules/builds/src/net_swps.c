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
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
/*ufile use*/
#include <linux/fs.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include "net_def.h"
#include "net_swps.h"
#include "pltfm_info.h"
#include "sff_eeprom.h"
#include "sfp.h"
#include "qsfp.h"
#include "qsfp_dd.h"

int io_no_init = 0;
module_param(io_no_init, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
u32 logLevel = SWPS_ERR_LEV | SWPS_INFO_LEV;
//u32 logLevel = ERR_ALL_LEV | INFO_ALL_LEV | DBG_ALL_LEV;
bool int_flag_monitor_en = false;
char workBuf[PAGE_SIZE];
struct platform_info_t *pltfmInfo = NULL;
static void swps_polling_task(struct work_struct *work);
static void polling_task_1U(void);
static void polling_task_4U(void);
static DECLARE_DELAYED_WORK(swps_polling, swps_polling_task);
static u8 swps_polling_enabled = 1;
static int swps_polling_task_start(void);
static int swps_polling_task_stop(void);
static int mux_ch_block(int i2c_ch, unsigned long mux_ch);
static int mux_fail_reset(int i2c_ch);
static struct swps_kobj_t *swps_kobj_add(char *name,
        struct kobject *mgr,
        struct attribute_group *attr_group);
/*dummy function only for testing*/

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
/*typedef enum {
    LC_LED_CTRL_OFF = 0,
    LC_LED_CTRL_GREEN_ON,
    LC_LED_CTRL_RED_ON,
    LC_LED_CTRL_AMBER_ON,
    LC_LED_CTRL_GREEN_BLINK,
    LC_LED_CTRL_RED_BLINK,
    LC_LED_CTRL_AMBER_BLINK,
    LC_LED_CTRL_NUM,
} lc_led_ctrl_t;
*/
enum {
    LC_EJ_SKIP_FLAG = 0,
    LC_EJ_IS_LOCKED_FLAG = 1,
    LC_EJ_IS_UNLOCKED_FLAG = 2,
};

struct lc_func_t lc_func_4U = {
    .dev_init = lc_dev_init,
    .dev_deinit = lc_dev_deinit,
    .dev_hdlr = lc_dev_hdlr,
    .polling_task = polling_task_4U,
    .mux_reset_set = lc_dev_mux_reset_set,
    .mux_reset_get = lc_dummy_get,
    .i2c_is_alive = lc_dev_mux_l1_is_alive,
    .cpld_init = lc_dev_lc_cpld_init,
    .power_set = lc_dev_power_set,
    .power_ready = lc_dev_power_ready,
    //.phy_ready = lc_dev_phy_ready,
    .phy_ready = dummy_phy_ready,
    .prs_get = lc_dev_prs_get,
    //.prs_get = dummy_prs_get,
    .reset_set = lc_dev_reset_set,
    .type_get = lc_dev_type_get,
    .type_get_text = lc_dev_type_get_text,
    .led_set = lc_dev_led_set,
    .led_set = lc_dev_led_set,
    .over_temp_asserted = lc_dev_over_temp_asserted,
    .over_temp_deasserted = lc_dev_over_temp_deasserted,
    .temp_get = lc_dev_temp_get_text,
    .phy_reset_set = lc_dev_phy_reset_set,
};

struct lc_func_t lc_func_1U = {
    .dev_init = io_dev_init,
    .dev_deinit = io_dev_deinit,
    .dev_hdlr = io_dev_hdlr,
    .polling_task = polling_task_1U,
    .mux_reset_set = io_dev_mux_reset_set,
    .mux_reset_get = io_dev_mux_reset_get,
    .i2c_is_alive = ioexp_is_channel_ready,
};

/*check if pointer is valid
 *      true: not NULL false: NULL*/
bool inline p_valid(const void *ptr)
{
    return ((NULL != ptr) ? true:false);
}
#if 0
static int string_to_long(const char *buf, long *val)
{
    long ldata = 0;
    int ret = 0;

    if (!p_valid(buf) || !p_valid(val)) {
        return -EINVAL;
    }
    ret = kstrtol(buf, 16, &ldata);

    if (ret < 0) {

        return ret;
    }
    *val = ldata;
    return 0;
}
#endif
static inline void swps_polling_set(u8 enable)
{
    swps_polling_enabled = enable;
}

static inline u8 swps_polling_is_enabled(void)
{
    return swps_polling_enabled;
}

struct fsm_period_t {
    sff_fsm_state_t st;
    int delay_cnt;
};

struct fsm_period_t fsm_period_tbl[] = {
    {SFF_FSM_ST_REMOVED, 0},
    {SFF_FSM_ST_INSERTED, 3},
    {SFF_FSM_ST_DETECTING, 3},
    {SFF_FSM_ST_INIT, 1},
    {SFF_FSM_ST_READY, 5},
    {SFF_FSM_ST_IDLE, 20},
    {SFF_FSM_ST_FAULT, 30},
    {SFF_FSM_ST_SUSPEND, 1},
    {SFF_FSM_ST_RESTART, 1},
    {SFF_FSM_ST_IDENTIFY, 1},
    {SFF_FSM_ST_MONITOR, 30},
    /*qsfp-dd only {*/
    {SFF_FSM_ST_MGMT_INIT, 3},
    {SFF_FSM_ST_MODULE_UNRESET, 1},
    {SFF_FSM_ST_MODULE_HW_INIT, 3},
    {SFF_FSM_ST_MODULE_LOOPBACK_INIT, 3},
    {SFF_FSM_ST_MODULE_READY, 3},
    {SFF_FSM_ST_MODULE_PWR_DOWN, 3},
    {SFF_FSM_ST_MODULE_CMIS_VER_CHECK, 1},
    {SFF_FSM_ST_MODULE_ADVERT_CHECK, 1},
    {SFF_FSM_ST_MODULE_SW_CONFIG_1, 1},
    {SFF_FSM_ST_MODULE_SW_CONFIG_2, 1},
    {SFF_FSM_ST_MODULE_SW_CONFIG_CHECK, 1},
    {SFF_FSM_ST_MODULE_SW_CONTROL, 1},
    {SFF_FSM_ST_MODULE_READY_CHECK, 1},
    /*qsfp-dd only }*/
    {SFF_FSM_ST_UNKNOWN_TYPE, 30},
    {SFF_FSM_ST_END, 0xff}, /*keep it at the end of table*/
};

const char *sff_fsm_st_str[SFF_FSM_ST_NUM] = {
    "SFF_FSM_ST_REMOVED",
    "SFF_FSM_ST_INSERTED",
    "SFF_FSM_ST_DETECTING",
    "SFF_FSM_ST_INIT",
    "SFF_FSM_ST_READY",
    "SFF_FSM_ST_IDLE",
    "SFF_FSM_ST_FAULT",
    "SFF_FSM_ST_SUSPEND",
    "SFF_FSM_ST_RESTART",
    "SFF_FSM_ST_ISOLATED",
    "SFF_FSM_ST_IDENTIFY",
    "SFF_FSM_ST_MONITOR",
    /*qsfp-dd only {*/
    "SFF_FSM_ST_MGMT_INIT",
    "SFF_FSM_ST_MODULE_UNRESET",
    "SFF_FSM_ST_MODULE_HW_INIT",
    "SFF_FSM_ST_MODULE_LOOPBACK_INIT",
    "SFF_FSM_ST_MODULE_READY",
    "SFF_FSM_ST_MODULE_PWR_DOWN",
    "SFF_FSM_ST_MODULE_CMIS_VER_CHECK",
    "SFF_FSM_ST_MODULE_ADVERT_CHECK",
    "SFF_FSM_ST_MODULE_SW_CONFIG_1",
    "SFF_FSM_ST_MODULE_SW_CONFIG_2",
    "SFF_FSM_ST_MODULE_SW_CONFIG_CHECK",
    "SFF_FSM_ST_MODULE_SW_CONTROL",
    "SFF_FSM_ST_MODULE_READY_CHECK",
    /*qsfp-dd only }*/
    "SFF_FSM_ST_UNKNOWN_TYPE",
    "SFF_FSM_ST_END", /*keep it at the bottom*/
};
/*struct sff_obj_t;*/

struct kset *swpsKset = NULL;


char *lc_fsm_st_str[LC_FSM_ST_NUM] = {
    [LC_FSM_ST_INSERT] = "LC_FSM_ST_INSERT",
    [LC_FSM_ST_WAIT_STABLE] = "LC_FSM_ST_STABLE",
    [LC_FSM_ST_POWER_ON] = "LC_FSM_ST_POWER_ON",
    [LC_FSM_ST_POWER_CHECK] = "LC_FSM_ST_POWER_CHECK",
    [LC_FSM_ST_PHY_CHECK] = "LC_FSM_ST_PHY_CHECK",
    [LC_FSM_ST_INIT] = "LC_FSM_ST_INIT",
    [LC_FSM_ST_READY] = "LC_FSM_ST_READY",
    [LC_FSM_ST_REMOVE] = "LC_FSM_ST_REMOVE",
    [LC_FSM_ST_IDLE] = "LC_FSM_ST_IDLE",
    [LC_FSM_ST_THERMAL_TRIP] = "LC_FSM_ST_THERMAL_TRIP",
    [LC_FSM_ST_UNSUPPORTED] = "LC_FSM_ST_UNSUPPORTED",
    [LC_FSM_ST_FAULT] = "LC_FSM_ST_FAULT",

};

enum {
    TYPE_1U = 1,
    TYPE_2U,
    TYPE_3U,
    TYPE_4U,
};
#define LINE_CARD_NUM (4)
//int cardNum = LINE_CARD_NUM;
int maxPortNum = 0;
struct lc_t lcMgr;
char *card_name_str[LINE_CARD_NUM] = {
    "card1",
    "card2",
    "card3",
    "card4",
};

static int sff_kobj_add(struct sff_obj_t *sff_obj);
static void sff_fsm_init(struct sff_obj_t *obj, int type);
static void sff_data_init(struct sff_mgr_t *sff, int port_num);
inline static void sff_data_reset(struct sff_mgr_t *sff);
static void sff_func_init(struct sff_mgr_t *sff);
static void func_tbl_init(struct sff_obj_t *obj, int type);
static void io_no_init_port_done_set(struct sff_obj_t *sff_obj);
static int io_no_init_done_fsm_run(struct sff_obj_t *sff_obj);
static void io_no_init_handler_by_card(struct lc_obj_t *card);
static void io_no_init_handler(struct lc_t *card);
static bool i2c_bus_is_alive(struct lc_obj_t *obj);
static void i2c_bus_recovery(struct lc_obj_t *card);
static void mux_reset_seq(struct lc_obj_t *card);
#define MASK_CREATE(port) ((1L << (port)) - 1L)

#define INVALID_CH (0xff)

static void mux_reset_ch_resel(int lc_id, int port)
{
    sff_eeprom_read_no_retry(lc_id, port);
}

static void mux_reset_ch_resel_byLC(struct lc_obj_t *card)
{
    int port = 0;
    int lc_id = 0;
    struct sff_obj_t *sff_obj = NULL;
    int try_num = 0;
    bool reset_done = false;
    int port_num = card->sff.valid_port_num;
    int ch_1st = 0;
    int ch_2nd = 0;
    int ch = 0;
    int try = 0;
    lc_id = card->lc_id;
    try_num = (card->sff.valid_port_num/MUX_CH_NUM) * 2;

    SWPS_LOG_DBG("mux_ch_resel_start: try:%d try_num:%d\n", try, try_num);
    while (!reset_done && (try++ < try_num)) {
        SWPS_LOG_DBG("mux_ch_resel_done: try:%d try_num:%d\n", try, try_num);
        for (port = 0; port < port_num; port += MUX_CH_NUM) {
            
            ch_1st = INVALID_CH;
            ch_2nd = INVALID_CH;
            for (ch = port; ch < port + MUX_CH_NUM; ch++) {

                sff_obj = &(card->sff.obj[ch]);
                if (ch_1st == INVALID_CH &&
                        SFF_FSM_ST_ISOLATED != sff_fsm_st_get(sff_obj)) {
                    ch_1st = ch;
                } else {

                    //SWPS_LOG_DBG("ch:%d test ch_1st:%d ch_2nd:%d\n", ch, ch_1st, ch_2nd);
                    if(SFF_FSM_ST_ISOLATED != sff_fsm_st_get(sff_obj)) {
                        ch_2nd = ch;
                        break;
                    }
                }
            }
            if (INVALID_CH == ch_1st ||
                    INVALID_CH == ch_2nd) {
                SWPS_LOG_ERR("select ch fail ch_1st:%s ch_2nd:%s \n", card->sff.obj[ch_1st].name,  card->sff.obj[ch_2nd].name);
                break;
            }
            SWPS_LOG_DBG("ch_1st:%s ch_2nd:%s\n", card->sff.obj[ch_1st].name,  card->sff.obj[ch_2nd].name);
            sff_obj = &(card->sff.obj[ch_1st]);
            mux_reset_ch_resel(lc_id, ch_1st);
            if (!i2c_bus_is_alive(card)) {
                sff_fsm_st_set(sff_obj, SFF_FSM_ST_ISOLATED);
                SWPS_LOG_ERR("transvr in %s is bad\n", sff_obj->name);
                mux_reset_seq(card);
                break;
            }

            sff_obj = &(card->sff.obj[ch_2nd]);
            mux_reset_ch_resel(lc_id, ch_2nd);
            if (!i2c_bus_is_alive(card)) {
                mux_reset_seq(card);
                sff_fsm_st_set(sff_obj, SFF_FSM_ST_ISOLATED);
                SWPS_LOG_ERR("transvr in %s is bad\n", sff_obj->name);
                break;
            }
        }
        if (port >= port_num) {
            if (i2c_bus_is_alive(card)) {
                reset_done = true;
                SWPS_LOG_DBG("mux_ch_resel_done: try:%d\n", try);
            }
        }
    }
    if (try >= try_num) {
        SWPS_LOG_ERR("mux_ch_resel_fail\n try:%d\n", try);
    }
}

static bool port_is_range_valid(int lc_id, int port)
{
    int port_num = lcMgr.obj[lc_id].sff.valid_port_num;

    if (port >= port_num && port < 0) {
        return false;
    }
    return true;  
}    

char *port_name_get(int lc_id, int port)
{
    if (!port_is_range_valid(lc_id, port)) {
        return NULL; 
    } 
    return lcMgr.obj[lc_id].sff.obj[port].name;    
}    
/*the line card port number should be obtained from cpld*/
static struct port_info_table_t *lc_port_info_get(struct lc_obj_t *card, lc_type_t type)
{
        struct port_info_table_t *port_info = NULL;

        if (LC_100G_TYPE == type) {
            port_info = pltfmInfo->tbl_1st;
        } else if (LC_400G_TYPE == type) {
            port_info = pltfmInfo->tbl_2nd;
        } else {
            SWPS_LOG_ERR("unknown lc type\n");
        }

        return port_info;
}

static void lc_fsm_st_chg_process(struct lc_obj_t *card,
                                     lc_fsm_st_t cur_st,
                                     lc_fsm_st_t next_st)
{
    if (cur_st != next_st) {
        SWPS_LOG_DBG("%s st change:%s -> %s\n",
                     card->name, lc_fsm_st_str[cur_st],
                     lc_fsm_st_str[next_st]);
    }
}
static void lc_fsm_st_set(struct lc_obj_t *card, lc_fsm_st_t st)
{
    lc_fsm_st_t cur_st = card->st;
    lc_fsm_st_chg_process(card, cur_st, st);
    card->st = st;
}
static lc_fsm_st_t lc_fsm_st_get(struct lc_obj_t *card)
{
    return card->st;
}
/* <TBD> if it's 4U platform , get the info from lc_dev_prs_get()
 * if it's 1u return 1*/
static void lc_insert(struct lc_obj_t *obj)
{
    SWPS_LOG_INFO("%s insert\n", obj->name);
    lc_fsm_st_set(obj, LC_FSM_ST_INSERT);
}
static void lc_remove(struct lc_obj_t *obj)
{
    SWPS_LOG_INFO("%s remove\n", obj->name);
    lc_fsm_st_set(obj, LC_FSM_ST_REMOVE);
}
#if 0
static int lc_prs_scan(struct lc_t *self)
{
    unsigned long prs = 0;
    unsigned long ej_r = 0;
    unsigned long ej_l = 0;
    int lc_num = self->lc_num;
    unsigned long prs_change = 0;
    unsigned long ej_r_chg = 0;
    unsigned long ej_l_chg = 0;
    bool is_prs_chg = false;
    bool is_ej_r_chg = false;
    bool is_ej_l_chg = false;
    int i= 0;
    int ret = 0;
    struct lc_func_t *lc_func = NULL;

    if (!p_valid(self)) {
        SWPS_LOG_ERR("NULL ptr\n");
        return -EINVAL;
    }
    lc_func = self->lc_func;
   
    check_pfunc(lc_func->prs_get);
    if ((ret = lc_func->prs_get(&prs)) < 0) {
        return ret;
    }
    if ((ret = lc_dev_ej_r_get(&ej_r)) < 0) {
        return ret;
    }     
    if ((ret = lc_dev_ej_l_get(&ej_l)) < 0) {
        return ret;
    }     
    
    prs_change = prs ^ (self->lc_prs);
    ej_r_chg = ej_r ^ (self->ej_r);
    ej_l_chg = ej_l ^ (self->ej_l);

    for (i = 0; i < lc_num; i++) {

        if (test_bit(i, &prs_change)) {
            is_prs_chg = true;

            if (test_bit(i, &prs)) {
                lc_insert(&self->obj[i]);
            } else {
                lc_remove(&self->obj[i]);
            }
        }
    }

    self->lc_prs = prs;

    return 0;
}
#else 
static int lc_ej_lock_st_get(int lc_id, unsigned long prs, unsigned long ej_r, unsigned long ej_l)
{
    int ej_st = LC_EJ_SKIP_FLAG;

    if(test_bit(lc_id, &prs) &&
       test_bit(lc_id, &ej_r) && 
       test_bit(lc_id, &ej_l)) {
        
        ej_st = LC_EJ_IS_LOCKED_FLAG;
    } else {
        //if (!test_bit(lc_id, &ej_r) && !test_bit(lc_id, &ej_l)) {
            ej_st = LC_EJ_IS_UNLOCKED_FLAG;
       // }
    }

    return ej_st;
}    
static int lc_prs_scan(struct lc_t *self)
{
    unsigned long prs = 0;
    unsigned long ej_r = 0;
    unsigned long ej_l = 0;
    int lc_num = self->lc_num;
    unsigned long prs_chg = 0;
    unsigned long ej_r_chg = 0;
    unsigned long ej_l_chg = 0;
    bool is_prs_chg = false;
    bool is_ej_r_chg = false;
    bool is_ej_l_chg = false;
    int lc_id = 0;
    int ret = 0;
    int ej_st = LC_EJ_SKIP_FLAG;
    struct lc_func_t *lc_func = NULL;
    struct lc_obj_t *lc_obj = NULL;
    if (!p_valid(self)) {
        SWPS_LOG_ERR("NULL ptr\n");
        return -EINVAL;
    }
    lc_func = self->lc_func;
   
    check_pfunc(lc_func->prs_get);
    if ((ret = lc_func->prs_get(&prs)) < 0) {
        return ret;
    }
    if ((ret = lc_dev_ej_r_get(&ej_r)) < 0) {
        return ret;
    }     
    if ((ret = lc_dev_ej_l_get(&ej_l)) < 0) {
        return ret;
    }     
    
    prs_chg = prs ^ (self->lc_prs);
    ej_r_chg = ej_r ^ (self->ej_r);
    ej_l_chg = ej_l ^ (self->ej_l);

    if (test_bit(lc_id, &prs_chg)) {
        is_prs_chg = true;
    }
    if (test_bit(lc_id, &ej_l_chg)) {
        is_ej_r_chg = true;
    }
    
    if (test_bit(lc_id, &ej_r_chg)) {
        is_ej_r_chg = true;
    }
    for (lc_id = 0; lc_id < lc_num; lc_id++) {
      
        ej_st = lc_ej_lock_st_get(lc_id, prs, ej_r, ej_l); 
        lc_obj = &(self->obj[lc_id]);
        switch (lc_obj->posi_st) {
        case LC_POSI_INIT_ST:
            if (LC_EJ_IS_LOCKED_FLAG == ej_st) {
                 lc_obj->posi_st = LC_POSI_LOCK_CHECK_ST;
                 lc_obj->prs_locked_cnt = 0;
            } else {

                lc_obj->posi_st = LC_POSI_MON_ST;                    
            }            
            break;
        
        case LC_POSI_MON_ST:
            
            if (is_prs_chg || is_ej_r_chg || is_ej_l_chg)  {
            
                if (LC_EJ_IS_LOCKED_FLAG == ej_st) {
                     lc_obj->posi_st = LC_POSI_LOCK_CHECK_ST;
                     lc_obj->prs_locked_cnt = 0;
                } else {

                    lc_obj->posi_st = LC_POSI_RELEASED_ST;                    
                }
            } 
           break; 
        
        case LC_POSI_RELEASED_ST:
                
            if (LC_EJ_IS_LOCKED_FLAG == ej_st) {
                lc_obj->posi_st = LC_POSI_LOCK_CHECK_ST;                    
                lc_obj->prs_locked_cnt = 0;
            }
                 
            break;
        case LC_POSI_LOCK_CHECK_ST:
                
            if (LC_EJ_IS_LOCKED_FLAG == ej_st) {
                lc_obj->prs_locked_cnt++;
            } else if (LC_EJ_IS_UNLOCKED_FLAG == ej_st) {
                lc_obj->posi_st = LC_POSI_RELEASED_ST;                    
                lc_remove(lc_obj);
            }
            
            if (lc_obj->prs_locked_cnt >= LC_PRS_LOCKED_NUM) {
                lc_obj->prs_locked_cnt = 0;
                lc_insert(lc_obj);
                lc_obj->posi_st = LC_POSI_LOCKED_ST;                    
            }
                 
            break;
        case LC_POSI_LOCKED_ST:
            
            if (LC_EJ_IS_UNLOCKED_FLAG == ej_st) {
                lc_obj->posi_st = LC_POSI_RELEASED_ST;                    
                lc_remove(lc_obj);
            }
            
            break;
        default:
            break;
        }
    }
    self->lc_prs = prs;
    self->ej_r = ej_r;
    self->ej_l = ej_l;
    return 0;
}



#endif
static int lc_fsm_run_4U(struct lc_t *card);
static int lc_fsm_run_1U(struct lc_t *card);
/*struct sff_mgr_t Sff;*/

#define to_swps_kobj(x) container_of(x, struct swps_kobj_t, kobj)

void sff_fsm_st_chg_process(struct sff_obj_t *sff_obj, sff_fsm_state_t cur_st, sff_fsm_state_t next_st);

static inline bool transvr_is_detected(struct sff_obj_t *sff_obj)
{
    return (sff_obj->fsm.st == SFF_FSM_ST_READY ? (true) : (false));

}
inline void transvr_type_set(struct sff_obj_t *sff_obj, int type)
{
    sff_obj->transvr_type=type;
}
inline int transvr_type_get(struct sff_obj_t *sff_obj)
{
    return sff_obj->transvr_type;
}

static struct monitor_para_t monitor_para_table[] = {
    /*  refer to sff 8636- ABLE 6-8  CHANNEL MONITORING VALUES (PAGE 00H BYTES 34-81)*/

    {LN_MONITOR_RX_PWR_TYPE, 10000, "mW"},
    {LN_MONITOR_TX_PWR_TYPE, 10000, "mW"},
    {LN_MONITOR_TX_BIAS_TYPE, 500, "mA"},

};
struct monitor_para_t *monitor_para_find(int type)
{
    int idx = 0;
    int size = ARRAY_SIZE(monitor_para_table);
    for (idx = 0; idx < size; idx++) {

        if (type == monitor_para_table[idx].type)
            break;
    }
    if (idx >= size)
        return NULL;

    return &monitor_para_table[idx];
}
/*utility*/
static int sscanf_to_int(const char *buf, int *value);
static bool match(const char *str1, const char *str2);

int i2c_smbus_write_byte_data_retry(struct i2c_client *client, u8 offset, u8 data)
{
    int ret = 0;
    int i = 0;

    for (i = 0; i < I2C_RETRY_NUM; i++) {
        ret = i2c_smbus_write_byte_data(client, offset, data);
        if (ret < 0) {
            msleep(I2C_RETRY_DELAY_MS);
            continue;
        }
        break;
    }

    if (i >= I2C_RETRY_NUM) {
        SWPS_LOG_ERR("fail:offset:0x%x try %d/%d! Error Code: %d\n", offset, i, I2C_RETRY_NUM, ret);
    }

    return ret;
}
int i2c_smbus_read_byte_data_retry(struct i2c_client *client, u8 offset)
{
    int ret = 0;
    int i = 0;

    for (i = 0; i < I2C_RETRY_NUM; i++) {

        ret = i2c_smbus_read_byte_data(client, offset);
        if (ret < 0) {
            msleep(I2C_RETRY_DELAY_MS);
            continue;
        }
        break;
    }

    if (i >= I2C_RETRY_NUM) {
        printk("%s fail:offset:0x%x try %d/%d! Error Code: %d\n", __func__, offset, i, I2C_RETRY_NUM, ret);
    }

    return ret;
}

int i2c_smbus_read_word_data_retry(struct i2c_client *client, u8 offset)
{
    int ret = 0;
    int i = 0;

    for (i = 0; i < I2C_RETRY_NUM; i++) {
        ret = i2c_smbus_read_word_data(client, offset);
        if (ret < 0) {
            msleep(I2C_RETRY_DELAY_MS);
            continue;
        }
        break;
    }

    if (i >= I2C_RETRY_NUM) {
        SWPS_LOG_ERR("%s fail:offset:0x%x try %d/%d! Error Code: %d\n", __func__, offset, i, I2C_RETRY_NUM, ret);
    }

    return ret;
}

int i2c_smbus_write_word_data_retry(struct i2c_client *client, u8 offset, u16 buf)
{
    int i;
    int ret = 0;

    for(i=0; i< I2C_RETRY_NUM; i++) {
        ret = i2c_smbus_write_word_data(client, offset, buf);
        if (ret < 0) {
            msleep(I2C_RETRY_DELAY_MS);
            continue;
        }
        break;
    }

    if (i >= I2C_RETRY_NUM) {
        SWPS_LOG_ERR("%s fail:offset:0x%x try %d/%d! Error Code: %d\n", __func__, offset, i, I2C_RETRY_NUM, ret);
    }
    return ret;
}
/*the linux i2c_smbus_read_i2c_block_data has block data size limitation : I2C_SMBUS_BLOCK_MAX
 * so we need to break the bigger data down*/
static int net_i2c_smbus_read_i2c_block_data(struct i2c_client *client, u8 offset, int len, u8 *buf)
{
    int ret = 0;
    int i = 0;
    int cnt = len;
    int block_size = 0;
    
    while (i < len) {
        block_size = ((cnt > I2C_SMBUS_BLOCK_MAX) ? I2C_SMBUS_BLOCK_MAX : cnt);
        ret = i2c_smbus_read_i2c_block_data(client, offset+i, block_size, buf+i);
        
        if (ret < 0) {
            break;
        }
        
        i += block_size;
        cnt = len - i;
    }
    
    if (ret < 0) {
        return ret;
    }
    return 0;
}
#if 1
int i2c_smbus_read_i2c_block_data_retry(struct i2c_client *client, u8 offset, int len, u8 *buf)
{
    int ret = 0;
    int i = 0;

    for (i = 0; i < I2C_RETRY_NUM; i++) {
        ret = net_i2c_smbus_read_i2c_block_data(client, offset, len, buf);
        if (ret < 0) {
            msleep(I2C_RETRY_DELAY_MS);
            continue;
        }
        break;
    }

    if (i >= I2C_RETRY_NUM) {
        SWPS_LOG_ERR("fail:offset:0x%x try %d/%d! Error Code: %d\n", offset, i, I2C_RETRY_NUM, ret);
    }
    return ret;
}
#else 

int i2c_smbus_read_i2c_block_data_retry(struct i2c_client *client, u8 offset, int len, u8 *buf)
{
    int ret = 0;
    int i = 0;

    for (i = 0; i < len; i++) {
        ret = i2c_smbus_read_byte_data_retry(client, offset + i);
        
        if (ret < 0) {
            break;
        }
        buf[i] = ret;
    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}


#endif
/*fsm functions declaration*/

static void sff_fsm_delay_cnt_reset(struct sff_obj_t *sff_obj, sff_fsm_state_t st);
static void sff_fsm_cnt_run(struct sff_obj_t *sff_obj);
static bool sff_fsm_delay_cnt_is_hit(struct sff_obj_t *sff_obj);
static int sff_fsm_run(struct sff_mgr_t *sff);

bool page_sel_is_locked(struct sff_obj_t *sff_obj)
{
    return sff_obj->page_sel_lock;
}    

void page_sel_lock(struct sff_obj_t *sff_obj)
{
    sff_obj->page_sel_lock = true;
}    

void page_sel_unlock(struct sff_obj_t *sff_obj)
{
    sff_obj->page_sel_lock = false;
}    
static inline unsigned long prs_bitmap_get(struct sff_mgr_t *sff)
{
    return sff->prs;
}
static inline void prs_bitmap_update(struct sff_mgr_t *sff, unsigned long bitmap)
{
    sff->prs = bitmap;
}
static int sff_prs_bitmap_get(struct sff_mgr_t *sff, unsigned long *prs)
{
    int ret = 0;
    unsigned long bitmap = 0;
    struct lc_obj_t *lc = sff_to_lc(sff);
    if (!p_valid(prs)) {
        return -EINVAL;
    }
    if((ret = sff->io_drv->prs_all_get(lc->lc_id, &bitmap)) < 0) {
        return ret;
    }
    /*use positive logic*/
    bitmap = ~bitmap;
    *prs = bitmap;
    return 0;
}

static int sff_prs_bitmap_get_external(struct sff_mgr_t *sff, unsigned long *prs)
{
    int port_num = sff->valid_port_num;
    unsigned long bitmap = 0;
    unsigned long front_port_prs = 0;
    int ret = 0;
    int port = 0;
    if ((ret = sff_prs_bitmap_get(sff, &bitmap)) < 0) {
        return ret;
    }

    for (port = 0; port < port_num; port++) {
        if (test_bit(port, &bitmap)) {
            set_bit(sff->obj[port].front_port, &front_port_prs);
        }
    }

    *prs = front_port_prs;
    return 0;
}
int net_sff_prs_get(int lc_id, unsigned long *prs)
{
    if (!p_valid(prs)) {
        return -EBADRQC;
    }
    return sff_prs_bitmap_get_external(&(lcMgr.obj[lc_id].sff), prs);
}
EXPORT_SYMBOL(net_sff_prs_get);

static bool sff_reset_is_supported(struct sff_obj_t *sff_obj)
{
   return ((SFP_TYPE != sff_obj->type) ? true : false); 
}    

static bool sff_lpmode_is_supported(struct sff_obj_t *sff_obj)
{
   return ((SFP_TYPE != sff_obj->type) ? true : false); 
}
/*<TBD> find better way  to check if sff_power is supported*/
static bool sff_power_is_supported(struct sff_obj_t *sff_obj)
{
   return false;
}    
/*reset set operation comfines sff io reset set and sff fsm transition event*/
int sff_reset_set_oper(struct sff_obj_t *sff_obj, u8 rst)
{
    int ret = 0;
    int cnt = 0;
    u8 lock = 0;    
    transvr_type_set(sff_obj, TRANSVR_CLASS_UNKNOWN);

    if (!rst) {
        
        lock = (page_sel_is_locked(sff_obj) ? 1 : 0);
        
        while(cnt <= PAGE_SEL_LOCK_NUM && lock) {       
            lock = (page_sel_is_locked(sff_obj) ? 1 : 0);
            cnt++;
            msleep(50);    
        }
        
        if (cnt > PAGE_SEL_LOCK_NUM) {
            SWPS_LOG_ERR("lock time out\n");
            return -ETIMEDOUT;
        }
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_SUSPEND);
        if (cnt > 0) {
            SWPS_LOG_INFO("page_sel_lock occured\n");
        }
    } else {
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_RESTART);
    }
    if((ret = sff_obj->func_tbl->reset_set(sff_obj, rst)) < 0) {
        return ret;
    }
    return 0;
}
static unsigned long frontPort_to_phyPort(struct sff_mgr_t *sff, 
                                          unsigned long front_bitmap)
{
    int port = 0;
    int front_port = 0;
    int front_port_num = 0;
    unsigned long phy_bitmap = 0;
    
    front_port_num = sff->valid_port_num;
    for (front_port = 0; front_port < front_port_num; front_port++) {
        port = sff->frontPort_to_port[front_port];
        
        if (test_bit(front_port, &front_bitmap)) {
            set_bit(port, &phy_bitmap);
        } else {
            clear_bit(port, &phy_bitmap);
        }
    }
    return phy_bitmap;
}
static unsigned long phyPort_to_frontPort(struct sff_mgr_t *sff, 
                                unsigned long phy_bitmap)
{
    int port = 0;
    int port_num = 0;
    unsigned long front_bitmap = 0;
    struct sff_obj_t *sff_obj = NULL;
    
    port_num = sff->valid_port_num;
    for (port = 0; port < port_num; port++) {

        sff_obj = &sff->obj[port];

        if (test_bit(port, &phy_bitmap)) {
            set_bit(sff_obj->front_port, &front_bitmap);
        } else {
            clear_bit(sff_obj->front_port, &front_bitmap);
        }
    }

    return front_bitmap;
}

static void sff_fsm_op_process(struct sff_mgr_t *sff, 
                                bool (*is_supported)(struct sff_obj_t *sff_obj), 
                                unsigned long phy_bitmap)
{
    int port = 0;
    int port_num = sff->valid_port_num;
    struct sff_obj_t *sff_obj = NULL;
    if (!p_valid(sff) && !p_valid(is_supported)) {
        SWPS_LOG_ERR("null input\n");
        return;
    }
    port_num = sff->valid_port_num; 
    for (port = 0; port < port_num; port++) {
                   
        sff_obj = &sff->obj[port];
        if (!is_supported(sff_obj)) {
            continue;
        }
        transvr_type_set(sff_obj, TRANSVR_CLASS_UNKNOWN);

        if (!test_bit(port, &phy_bitmap)) {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_SUSPEND);
        } else {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_RESTART);
        }
    } 
}    

/*(in) bitmap , is in front port order*/
int net_sff_output_set(sff_io_output_type_t type, int lc_id, unsigned long bitmap)
{
    unsigned long phy_bitmap = 0;
    struct sff_mgr_t *sff = NULL;
    int ret = 0;
    int (*set_func)(int lc_id, unsigned long bitmap) = NULL;
    
    sff = &lcMgr.obj[lc_id].sff;
    phy_bitmap = frontPort_to_phyPort(sff, bitmap);
    
    switch (type) {

    case SFF_IO_RST_TYPE:
        set_func = sff->io_drv->reset_all_set;
        sff_fsm_op_process(sff, sff_reset_is_supported, phy_bitmap); 
        break;
    case SFF_IO_PWR_TYPE:
        set_func = sff->io_drv->power_all_set;
        sff_fsm_op_process(sff, sff_power_is_supported, phy_bitmap); 
        break;
    case SFF_IO_LPMODE_TYPE:
        set_func = sff->io_drv->lpmode_all_set;
        break;
    default:
        break;
    }
    check_pfunc(set_func); 
    if ((ret = set_func(lc_id, phy_bitmap)) < 0) {
        return ret;
    }
    return 0;
}    
/*output bitmap , is in front port order*/
int net_sff_output_get(sff_io_output_type_t type, int lc_id, unsigned long *bitmap)
{
    unsigned long phy_bitmap = 0;
    struct sff_mgr_t *sff = NULL;
    int ret = 0;
    int (*get_func)(int lc_id, unsigned long *bitmap) = NULL;
    
    sff = &lcMgr.obj[lc_id].sff;
    
    switch (type) {

    case SFF_IO_RST_TYPE:
        get_func = sff->io_drv->reset_all_get;
        break;
    case SFF_IO_PWR_TYPE:
        get_func = sff->io_drv->power_all_get;
        break;
    case SFF_IO_LPMODE_TYPE:
        get_func = sff->io_drv->lpmode_all_get;
        break;
    default:
        break;
    }
    check_pfunc(get_func); 
    if ((ret = get_func(lc_id, &phy_bitmap)) < 0) {
        return ret;
    }
    *bitmap = phyPort_to_frontPort(sff, phy_bitmap); 
    return 0;
}    
int net_sff_reset_set(int lc_id, unsigned long bitmap)
{
    return net_sff_output_set(SFF_IO_RST_TYPE, lc_id, bitmap);
}
EXPORT_SYMBOL(net_sff_reset_set);

/*bitmap , is in front port order*/
int net_sff_reset_get(int lc_id, unsigned long *bitmap)
{
    return net_sff_output_get(SFF_IO_RST_TYPE, lc_id, bitmap);
}
EXPORT_SYMBOL(net_sff_reset_get);

int sff_lpmode_set_oper(struct sff_obj_t *sff_obj, u8 val)
{
    int ret = 0;

    if((ret = sff_obj->func_tbl->lpmode_set(sff_obj, val)) < 0) {
        return ret;
    }
    
    return 0;
}
int net_sff_lpmode_set(int lc_id, unsigned long bitmap)
{
    return net_sff_output_set(SFF_IO_LPMODE_TYPE, lc_id, bitmap);
}
EXPORT_SYMBOL(net_sff_lpmode_set);

/*bitmap , is in front port order*/
int net_sff_lpmode_get(int lc_id, unsigned long *bitmap)
{
    return net_sff_output_get(SFF_IO_LPMODE_TYPE, lc_id, bitmap);
}
EXPORT_SYMBOL(net_sff_lpmode_get);

int net_sff_power_set(int lc_id, unsigned long bitmap)
{
    return net_sff_output_set(SFF_IO_PWR_TYPE, lc_id, bitmap);
}
EXPORT_SYMBOL(net_sff_power_set);

int net_sff_power_get(int lc_id, unsigned long *bitmap)
{
    return net_sff_output_get(SFF_IO_PWR_TYPE, lc_id, bitmap);
}
EXPORT_SYMBOL(net_sff_power_get);

int dummy_page_get(struct sff_obj_t *sff_obj, u8 *page)
{
    return -ENOSYS;
}
int dummy_page_sel(struct sff_obj_t *sff_obj, int page)
{
    return -ENOSYS;
}
int dummy_eeprom_dump(struct sff_obj_t *sff_obj, u8 *buf)
{
    return -ENOSYS;
}

int sff_eeprom_read(struct sff_obj_t *sff_obj,
                           u8 addr,
                           u8 offset,
                           u8 *buf,
                           int len)
{
    check_pfunc(sff_obj->mgr->eeprom_drv->eeprom_read); 
    return sff_obj->mgr->eeprom_drv->eeprom_read(sff_obj->lc_id, sff_obj->port, addr, offset, buf, len);

}

int net_sff_eeprom_read(const unsigned int lc_id,
                           const unsigned port, 
                           const unsigned int addr,
                           const unsigned int offset,
                           const unsigned int count,
                           u8 *buf)
{
    int phy_port = 0;
    int front_port = 0;
    struct sff_obj_t *sff_obj = NULL;
    struct sff_mgr_t *sff = NULL;

    sff = &lcMgr.obj[lc_id].sff;
    front_port = port;
    phy_port = sff->frontPort_to_port[front_port];
    sff_obj = &sff->obj[phy_port]; 
    return sff_eeprom_read(sff_obj, addr, offset, buf, count);
}
EXPORT_SYMBOL(net_sff_eeprom_read);

int sff_eeprom_write(struct sff_obj_t *sff_obj,
                            u8 addr,
                            u8 offset,
                            const u8 *buf,
                            int len)
{
    check_pfunc(sff_obj->mgr->eeprom_drv->eeprom_write); 
    return sff_obj->mgr->eeprom_drv->eeprom_write(sff_obj->lc_id, sff_obj->port, addr, offset, buf, len);
}
int net_sff_eeprom_write(const unsigned int lc_id,
                           const unsigned port, 
                           const unsigned int addr,
                           const unsigned int offset,
                           const unsigned int count,
                           const u8 *buf)
{
    int phy_port = 0;
    int front_port = 0;
    struct sff_obj_t *sff_obj = NULL;
    struct sff_mgr_t *sff = NULL;

    sff = &lcMgr.obj[lc_id].sff;
    front_port = port;
    phy_port = sff->frontPort_to_port[front_port];
    sff_obj = &sff->obj[phy_port]; 
    return sff_eeprom_write(sff_obj, addr, offset, buf, count);
}
EXPORT_SYMBOL(net_sff_eeprom_write);

static long
sscanf_to_long(const char *buf, long *value)
{
    long result  = 0;
    char hex_tag[] = "0x";
    char *ptr = NULL;
    ptr = strstr(buf, hex_tag);
    if (ptr) {
        if (strcspn(buf, hex_tag) == 0) { /*first index*/
            if (sscanf(buf,"%lx",&result) < 0) {
                goto exit_err;
            }
        } else {
            goto exit_err;
        }
    } else {
        if (sscanf(buf,"%ld",&result) < 0) {
            goto exit_err;
        }
    }
    *value = result;
    return 0;

exit_err:
    return -EBADRQC;
}

bool match(const char *str1, const char *str2)
{
    bool is_match = false;
    if(strcmp(str1, str2) == 0) {
        is_match = true;
    }
    return is_match;
}

static int sff_type_get(struct sff_obj_t *sff_obj)
{
    int type = SFP_TYPE;
    type = sff_obj->type;
    return type;

}
/*sysfs attr*/
/* a custom attribute that works just for a struct sff_kobj. */
struct swps_attribute {
    struct attribute attr;
    ssize_t (*show)(struct swps_kobj_t *sff, struct swps_attribute *attr, char *buf);
    ssize_t (*store)(struct swps_kobj_t *sff, struct swps_attribute *attr, const char *buf, size_t count);
};
#define to_swps_attr(x) container_of(x, struct swps_attribute, attr)

static ssize_t swps_attr_show(struct kobject *kobj, struct attribute *attr,
                              char *buf)
{
    struct swps_attribute *attribute;
    struct swps_kobj_t *obj;

    attribute = to_swps_attr(attr);
    obj = to_swps_kobj(kobj);

    if (!p_valid(attribute->show)) {
        return -EIO;
    }

    return attribute->show(obj, attribute, buf);

}

static ssize_t swps_attr_store(struct kobject *kobj, struct attribute *attr,
                               const char *buf, size_t count)
{
    struct swps_attribute *attribute;
    struct swps_kobj_t *obj;

    attribute = to_swps_attr(attr);
    obj = to_swps_kobj(kobj);

    if (!p_valid(attribute->store))
        return -ENOSYS;

    return attribute->store(obj, attribute, buf, count);

}
static struct sysfs_ops swps_sys_ops = {
    .show   = swps_attr_show,
    .store  = swps_attr_store,
};

void swps_kobj_release(struct kobject *kobj)
{
    struct swps_kobj_t *obj;
    obj = to_swps_kobj(kobj);

    if (p_valid(obj)) {
        SWPS_LOG_INFO("%s\n", obj->kobj.name);
        kfree(obj);
    }
}

static struct kobj_type swpsKtype = {
    .release = swps_kobj_release,
    .sysfs_ops = &swps_sys_ops,
    .default_attrs = NULL,
};


static ssize_t sff_page_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                              const char *buf, size_t count)
{
    int ret = 0;
    int page = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    ret = sscanf_to_int(buf, &page);
    if(ret < 0) {
        return ret;
    }

    ret = sff_obj->func_tbl->page_sel(sff_obj, page);

    if (ret < 0) {
        return ret;
    }

    return count;
}
static ssize_t sff_page_sel_lock_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                              const char *buf, size_t count)
{
    int ret = 0;
    int lock = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    ret = sscanf_to_int(buf, &lock);
    if(ret < 0) {
        return ret;
    }
    if (lock != 1 && lock != 0) {
        return -EINVAL;
    }
    if (1 == lock) {
        page_sel_lock(sff_obj);
    } else {
        page_sel_unlock(sff_obj);
    }
    return count;
}
static ssize_t lc_prs_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                           char *buf)
{
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);
    unsigned long sys_ready = card->mgr->lc_sys_ready; 
    return scnprintf(buf, BUF_SIZE, "0x%lx\n", sys_ready);
}

static ssize_t lc_phy_ready_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                 char *buf)
{
    unsigned long phy_ready = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);
    
    phy_ready = card->phy_ready_bitmap;

    return scnprintf(buf, BUF_SIZE, "0x%lx\n", phy_ready);
}
static ssize_t lc_phy_ready_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  const char *buf, size_t count)
{
    unsigned long phy_ready = 0;
    int ret = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *lc = sff_to_lc(sff);

    ret = sscanf_to_long(buf, &phy_ready);
    //ret = string_to_long(buf, &phy_ready);
    if(ret < 0) {
        return ret;
    }
    if (phy_ready != lc->phy_ready_bitmap) {
        SWPS_LOG_DBG("%s phy_ready:0x%lx -> 0x%lx\n", lc->name, lc->phy_ready_bitmap, phy_ready);
    }
 
    lc->phy_ready_bitmap = phy_ready;
    return count;
}
static ssize_t lc_temp_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                            char *buf)
{
    int ret = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);
    struct lc_func_t *lc_func = NULL;

    lc_func = card->mgr->lc_func;
    check_pfunc(lc_func->temp_get);
    if ((ret = lc_func->temp_get(card->lc_id, buf, BUF_SIZE)) < 0) {
        return ret;
    }
    
    return scnprintf(buf, BUF_SIZE, "%s", buf);
}
/*it's for testing*/
static ssize_t lc_temp_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                             const char *buf, size_t count)
{
    int temp = 0;
    int ret = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);

    ret = sscanf_to_int(buf, &temp);
    if (ret < 0) {
        return ret;
    }

    ret = lc_dev_temp_th_set(card->lc_id, temp);
    if (ret < 0) {
        return ret;
    }

    return count;
}

static ssize_t lc_type_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                            char *buf)
{
    int ret = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);
    struct lc_func_t *lc_func = card->mgr->lc_func;
    
    check_pfunc(lc_func->power_ready);
    if((ret = lc_func->type_get_text(card->lc_id, buf, BUF_SIZE)) < 0) {
        return ret;
    }
    
    return scnprintf(buf, BUF_SIZE, "%s", buf);
}

static ssize_t lc_power_ready_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   char *buf)
{
    int ret = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);
    struct lc_func_t *lc_func = card->mgr->lc_func;
    bool ready = false;

    check_pfunc(lc_func->power_ready);
    if ((ret = lc_func->power_ready(card->lc_id, &ready)) < 0) {
        return ret;
    }
    
    return scnprintf(buf, BUF_SIZE, "%d\n", ready);
}
#if 0 /*reserved*/
static ssize_t lc_power_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                               const char *buf, size_t count)
{
    int ret = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);
    struct lc_func_t *lc_func = card->mgr->lc_func;
    bool on = false;
    int  power = 0;

    ret = sscanf_to_int(buf, &power);

    if(ret < 0) {
        return ret;
    }
    on = (power ? true : false);
    check_pfunc(lc_func->power_set);
    if ((ret = lc_func->power_set(card->lc_id, on)) < 0) {
        return ret;
    }

    return count;
}
#endif
static ssize_t sff_eeprom_dump_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                    char *buf)
{
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sff_obj->func_tbl->eeprom_dump(sff_obj, buf);

    if(ret < 0) {
        return ret;
    }

    return scnprintf(buf, BUF_SIZE, "%s", buf);
}
static ssize_t id_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                       char *buf)
{
    unsigned  char id = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sff_obj->func_tbl->id_get(sff_obj, &id);

    if(ret < 0) {
        return ret;
    }

    return scnprintf(buf, BUF_SIZE, "0x%x\n", id);
}
/*qsfp_dd only*/

static ssize_t active_ctrl_set_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                    char *buf)
{
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    if ((ret = qsfp_dd_active_ctrl_set_get(sff_obj, buf, BUF_SIZE)) < 0) {
        return ret;
    }
    return scnprintf(buf, BUF_SIZE, "%s", buf);
}

static ssize_t sff_intr_flag_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                      char *buf)
{
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    
    check_pfunc(sff_obj->func_tbl->intr_flag_show);
    if ((ret = sff_obj->func_tbl->intr_flag_show(sff_obj, buf, BUF_SIZE)) < 0) {
        return ret;
    }
    return scnprintf(buf, BUF_SIZE, "%s", buf);
}
static ssize_t module_st_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                              char *buf)
{
    u8 val = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sff_obj->func_tbl->module_st_get(sff_obj, &val);

    if(ret < 0) {
        return ret;
    }
    return scnprintf(buf, BUF_SIZE, "0x%x\n", val);
}
static ssize_t type_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                         char *buf)
{
    int type = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    type = sff_type_get(sff_obj);
    if (SFP_TYPE == type) {
        return scnprintf(buf, BUF_SIZE, "%s\n", "SFP_TYPE");
    } else if (QSFP_TYPE == type) {
        return scnprintf(buf, BUF_SIZE, "%s\n", "QSFP_TYPE");
    } else {

        return scnprintf(buf, BUF_SIZE, "%s\n", "QSFP_DD_TYPE");

    }

}
static ssize_t sff_port_num_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                 char *buf)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    return scnprintf(buf, BUF_SIZE, "%d\n", sff_obj->mgr->valid_port_num);
}

static ssize_t swps_version_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                 char *buf)
{
    return scnprintf(buf, BUF_SIZE, "%s\n", SWPS_VERSION);
}

static ssize_t pltfm_name_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                               char *buf)
{
    return scnprintf(buf, BUF_SIZE, "%s\n", pltfmInfo->name);
}
static ssize_t io_no_init_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                               char *buf)
{
    return scnprintf(buf, BUF_SIZE, "%d\n", io_no_init);
}

static ssize_t log_level_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                              char *buf)
{
    return scnprintf(buf, BUF_SIZE, "%x\n", logLevel);
}

static ssize_t log_level_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                               const char *buf, size_t count)
{
    int lv = 0;
    int ret = 0;

    ret = sscanf_to_int(buf, &lv);
    if (ret < 0) {
        return ret;
    }
    logLevel = lv;

    return count;
}
static int
sscanf_to_int(const char *buf, int *value)
{
#if 1
    int   result  = 0;
    char hex_tag[] = "0x";
    char *ptr = NULL;
    ptr = strstr(buf, hex_tag);
    if (ptr) {
        if (strcspn(buf, hex_tag) == 0) { /*first index*/
            if (sscanf(buf,"%x",&result) < 0) {
                goto exit_err;
            }
        } else {
            goto exit_err;
        }
    } else {
        if (sscanf(buf,"%d",&result) < 0) {
            goto exit_err;
        }
    }
    *value = result;
    return 0;

exit_err:
    return -EBADRQC;
#else

    int ldata = 0;
    int ret = 0;
    /*use kernel api instead*/
    if (!buf || !value) {
        return -EINVAL;
    }
    ret = kstrtol(buf, 16, &ldata);
    if (ret < 0) {

        return ret;
    }
    *value = (int)ldata;
    return 0;

#endif
}
static ssize_t lane_control_store(int ctrl_type, struct swps_kobj_t *swps_kobj,
                                  const char *buf, size_t count)
{
    int ch_ctrl = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    ret = sscanf_to_int(buf, &ch_ctrl);
    SWPS_LOG_DBG("ch_ctrl:0x%x ret:%d\n", ch_ctrl, ret);

    if(ret < 0) {
        return ret;
    }
    ret = sff_obj->func_tbl->lane_control_set(sff_obj, ctrl_type, ch_ctrl);

    if(ret < 0) {
        return ret;
    }
    return count;
}
static ssize_t lane_control_show(int ctrl_type, struct swps_kobj_t *swps_kobj, char *buf)
{
    u32 ch_ctrl = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sff_obj->func_tbl->lane_control_get(sff_obj, ctrl_type, &ch_ctrl);

    if(ret < 0) {
        return ret;
    }
    return scnprintf(buf, BUF_SIZE, "0x%x\n", ch_ctrl);
}
static ssize_t sff_tx_eq_store(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                               const char *buf, size_t count)
{
    return lane_control_store(TX_EQ_TYPE, kobj, buf, count);

}
static ssize_t sff_rx_em_store(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                               const char *buf, size_t count)
{
    return lane_control_store(RX_EM_TYPE, kobj, buf, count);
}

static ssize_t sff_rx_am_store(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                               const char *buf, size_t count)
{
    return lane_control_store(RX_AM_TYPE, kobj, buf, count);
}

static ssize_t sff_tx_eq_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                              char *buf)
{
    return lane_control_show(TX_EQ_TYPE, kobj, buf);
}
static ssize_t sff_rx_em_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                              char *buf)
{
    return lane_control_show(RX_EM_TYPE, kobj, buf);
}
static ssize_t sff_rx_am_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                              char *buf)
{
    return lane_control_show(RX_AM_TYPE, kobj, buf);
}
static ssize_t lane_status_show(int st_type, struct swps_kobj_t *swps_kobj, char *buf)
{
    u8 ch_status = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sff_obj->func_tbl->lane_status_get(sff_obj, st_type, &ch_status);

    if(ret < 0) {
        return ret;
    }
    return scnprintf(buf, BUF_SIZE, "0x%x\n", ch_status);

}
static ssize_t sff_rx_los_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                               char *buf)
{
    return lane_status_show(LN_STATUS_RX_LOS_TYPE, kobj, buf);
}
static ssize_t sff_tx_los_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                               char *buf)
{
    return lane_status_show(LN_STATUS_TX_LOS_TYPE, kobj, buf);
}
static ssize_t sff_tx_fault_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                                 char *buf)
{
    return lane_status_show(LN_STATUS_TX_FAULT_TYPE, kobj, buf);
}
static ssize_t lane_monitor_show(int moni_type, struct swps_kobj_t *swps_kobj, char *buf)
{
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    ret = sff_obj->func_tbl->lane_monitor_get(sff_obj, moni_type, buf, BUF_SIZE);
    if(ret < 0) {
        return ret;
    }
    return scnprintf(buf, BUF_SIZE, "%s", buf);
}
static ssize_t sff_tx_power_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                                 char *buf)
{
    return lane_monitor_show(LN_MONITOR_TX_PWR_TYPE, kobj, buf);
}
static ssize_t sff_rx_power_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                                 char *buf)
{
    return lane_monitor_show(LN_MONITOR_RX_PWR_TYPE, kobj, buf);
}
static ssize_t sff_tx_bias_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                                char *buf)
{
    return lane_monitor_show(LN_MONITOR_TX_BIAS_TYPE, kobj, buf);
}

static ssize_t vendor_info_show(int info_type, struct swps_kobj_t *swps_kobj, char *buf)
{
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sff_obj->func_tbl->vendor_info_get(sff_obj, info_type, buf, BUF_SIZE);

    if(ret < 0) {
        return ret;
    }
    return scnprintf(buf, BUF_SIZE, "%s", buf);
}
static ssize_t vendor_name_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                                char *buf)
{
    return vendor_info_show(VENDOR_NAME_TYPE, kobj, buf);
}
static ssize_t vendor_pn_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                              char *buf)
{
    return vendor_info_show(VENDOR_PN_TYPE, kobj, buf);
}
static ssize_t vendor_sn_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                              char *buf)
{
    return vendor_info_show(VENDOR_SN_TYPE, kobj, buf);
}
static ssize_t vendor_rev_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                               char *buf)
{
    return vendor_info_show(VENDOR_REV_TYPE, kobj, buf);
}
static ssize_t temperature_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                char *buf)
{
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sff_obj->func_tbl->temperature_get(sff_obj, buf, BUF_SIZE);
    if (ret < 0) {
        return ret;
    }

    return scnprintf(buf, BUF_SIZE, "%s", buf);
}
static ssize_t voltage_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                            char *buf)
{
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sff_obj->func_tbl->voltage_get(sff_obj, buf, BUF_SIZE);
    if (ret < 0) {
        return ret;
    }

    return scnprintf(buf, BUF_SIZE, "%s", buf);
}

static ssize_t transvr_type_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                 char *buf)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    return scnprintf(buf, BUF_SIZE, "%d\n", transvr_type_get(sff_obj));
}

static ssize_t fsm_st_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                           char *buf)
{
    int st = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    st = sff_fsm_st_get(sff_obj);
    return scnprintf(buf, BUF_SIZE, "%s\n", sff_fsm_st_str[st]);
}
static ssize_t lc_fsm_st_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                           char *buf)
{
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);
    return scnprintf(buf, BUF_SIZE, "%s\n", lc_fsm_st_str[card->st]);
}
static ssize_t sff_reset_all_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   const char *buf, size_t count)
{
    int ret = 0;
    unsigned long rst = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);
    int i = 0;

    for (i = 0; i < count; i++) {
        if (i < sff->valid_port_num) {
            if (buf[i] != '0' && buf[i] != '1') {
                SWPS_LOG_ERR("%d:set val = %c is not support.\n", i, buf[i]);
                return -EINVAL;
            }
            
            if ('1' == buf[i]) {
                set_bit(i, &rst);
            } else {
                clear_bit(i, &rst);
            }
        }
    }
    SWPS_LOG_DBG("set val = 0x%lx\n", rst);
    if ((ret = net_sff_reset_set(card->lc_id, rst)) < 0) {
        return ret;
    }
    return count;
}
static ssize_t sff_reset_all_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  char *buf)
{
    int ret = 0;
    unsigned long front_port_rst = 0;
    int port = 0;
    int front_port = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);
    int front_port_num = sff->valid_port_num;

    if ((ret = net_sff_reset_get(card->lc_id, &front_port_rst)) < 0) {
        return ret;
    }

    for (front_port = 0; front_port < front_port_num; front_port++) {
        port = sff->frontPort_to_port[front_port];
        
        if (sff_reset_is_supported(&sff->obj[port])) {
            buf[front_port] = (test_bit(front_port, &front_port_rst) ? '1' : '0');
        } else {
            buf[front_port] = 'X';
        }
    }

    buf[front_port] = '\n';
    return (ssize_t)strlen(buf);
}
static ssize_t sff_lpmode_all_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   const char *buf, size_t count)
{
    int ret = 0;
    unsigned long lpmode = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);
    int i = 0;

    for (i = 0; i < count; i++) {
        if (i < sff->valid_port_num) {
            if (buf[i] != '0' && buf[i] != '1') {
                SWPS_LOG_ERR("%d:set val = %c is not support.\n", i, buf[i]);
                return -EINVAL;
            }
            if ('1' == buf[i]) {
                set_bit(i, &lpmode);
            } else {
                clear_bit(i, &lpmode);
            }
        }
    }
    if ((ret = net_sff_lpmode_set(card->lc_id, lpmode)) < 0) {
        return ret;
    }
    return count;
}
static ssize_t sff_lpmode_all_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  char *buf)
{
    int ret = 0;
    unsigned long front_port_lpmode = 0;
    int port = 0;
    int front_port = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);
    int front_port_num = sff->valid_port_num;

    if ((ret = net_sff_lpmode_get(card->lc_id, &front_port_lpmode)) < 0) {
        return ret;
    }

    for (front_port = 0; front_port < front_port_num; front_port++) {
        port = sff->frontPort_to_port[front_port];

        if (sff_lpmode_is_supported(&sff->obj[port])) {
            buf[front_port] = (test_bit(front_port, &front_port_lpmode) ? '1' : '0');
        } else {
            buf[front_port] = 'X';
        }
    }

    buf[front_port] = '\n';
    return (ssize_t)strlen(buf);
}
static ssize_t sff_power_all_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   const char *buf, size_t count)
{
    int ret = 0;
    unsigned long power = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);
    int i = 0;

    for (i = 0; i < count; i++) {
        if (i < sff->valid_port_num) {
            if (buf[i] != '0' && buf[i] != '1') {
                SWPS_LOG_ERR("%d:set val = %c is not support.\n", i, buf[i]);
                return -EINVAL;
            }
            if ('1' == buf[i]) {
                set_bit(i, &power);
            } else {
                clear_bit(i, &power);
            }
        }
    }
    if ((ret = net_sff_power_set(card->lc_id, power)) < 0) {
        return ret;
    }
    return count;
}
static ssize_t sff_power_all_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  char *buf)
{
    int ret = 0;
    unsigned long front_port_power = 0;
    int port = 0;
    int front_port = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);
    int front_port_num = sff->valid_port_num;

    if ((ret = net_sff_power_get(card->lc_id, &front_port_power)) < 0) {
        return ret;
    }

    for (front_port = 0; front_port < front_port_num; front_port++) {
        port = sff->frontPort_to_port[front_port];

        if (sff_power_is_supported(&sff->obj[port])) {
            buf[front_port] = (test_bit(front_port, &front_port_power) ? '1' : '0');
        } else {
            buf[front_port] = 'X';
        }
    }

    buf[front_port] = '\n';
    return (ssize_t)strlen(buf);
}
static ssize_t sff_reset_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                               const char *buf, size_t count)
{
    int ret = 0;
    int rst = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sscanf_to_int(buf, &rst);

    if(ret < 0) {
        return ret;
    }

    if ((ret = sff_reset_set_oper(sff_obj, rst)) < 0) {
        return ret;
    }
    return count;
}
static ssize_t sff_reset_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                              char *buf)
{
    u8 reset = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sff_obj->func_tbl->reset_get(sff_obj, &reset);

    if (ret < 0) {
        return ret;
    }
    return scnprintf(buf, BUF_SIZE, "%d\n", reset);
}

static ssize_t sff_power_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                               const char *buf, size_t count)
{
    int ret = 0;
    int power = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sscanf_to_int(buf, &power);

    if(ret < 0) {
        return ret;
    }

    ret = sff_obj->func_tbl->power_set(sff_obj, power);

    if(ret < 0) {
        return ret;
    }

    transvr_type_set(sff_obj, TRANSVR_CLASS_UNKNOWN);

    if (power) {
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_RESTART);
    } else {
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_SUSPEND);
    }

    return count;
}

static ssize_t sff_power_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                              char *buf)
{
    u8 power = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sff_obj->func_tbl->power_get(sff_obj, &power);

    if (ret < 0) {
        return ret;
    }
    return scnprintf(buf, BUF_SIZE, "%d\n", power);
}


static ssize_t rev4_quick_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                               const char *buf, size_t count)
{
    int ret = 0;
    int val = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    bool en = false;
    ret = sscanf_to_int(buf, &val);

    if(ret < 0) {
        return ret;
    }
    en = ((val) ? true : false);
    qsfp_dd_rev4_quick_set(sff_obj, en);
    return count;
}
static ssize_t rev4_quick_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                              char *buf)
{
    bool en = false;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    en = qsfp_dd_rev4_quick_get(sff_obj);
    return scnprintf(buf, BUF_SIZE, "%d\n", en);
}
static ssize_t gpio_mux_reset_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   char *buf)
{
    int val = 0;
    int ret = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);
    struct lc_func_t *lc_func = card->mgr->lc_func;

    check_pfunc(lc_func->mux_reset_get);
    if ((ret = lc_func->mux_reset_get(card->lc_id, &val)) < 0) {
        return ret;
    }

    return scnprintf(buf, BUF_SIZE, "%d\n", val);
}
static ssize_t gpio_mux_reset_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                    const char *buf, size_t count)
{
    int ret = 0;
    int val = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    struct lc_obj_t *card = sff_to_lc(sff);
    struct lc_func_t *lc_func = card->mgr->lc_func;

    ret = sscanf_to_int(buf, &val);
    if(ret < 0) {
        return ret;
    }
    
    check_pfunc(lc_func->mux_reset_set);
    if((ret = lc_func->mux_reset_set(card->lc_id, val)) < 0) {
        return ret;
    }
    if (val) {
        mux_reset_ch_resel_byLC(card);
        SWPS_LOG_DBG("mux_reset_ch_resel done\n");
    }
    return count;
}
static ssize_t sff_lpmode_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                const char *buf, size_t count)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    int ret = 0;
    int lpmode = 0;

    ret = sscanf_to_int(buf, &lpmode);

    if(ret < 0) {
        return ret;
    }
    ret = sff_obj->func_tbl->lpmode_set(sff_obj, lpmode);

    if(ret < 0) {
        return ret;
    }
    return count;
}
static ssize_t sff_lpmode_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                               char *buf)
{
    u8 lpmode = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sff_obj->func_tbl->lpmode_get(sff_obj, &lpmode);
    if (ret < 0) {
        return ret;
    }
    return scnprintf(buf, BUF_SIZE, "%d\n", lpmode);
}
static ssize_t sff_intL_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                             char *buf)
{
    u8 intL = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sff_obj->func_tbl->intL_get(sff_obj, &intL);
    if (ret < 0) {
        return ret;
    }

    return scnprintf(buf, BUF_SIZE, "%d\n", intL);
}
static ssize_t sff_mode_sel_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  const char *buf, size_t count)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    int err_code = -1;
    int mode_sel = 0;
    int type = 0;
    type = sff_type_get(sff_obj);

    if (SFP_TYPE == type) {

        return err_code;
    }
    err_code = sscanf_to_int(buf, &mode_sel);

    if(err_code < 0) {
        return err_code;
    }
#if 0
    err_code = mode_sel_set(sff_obj, mode_sel);

    if(err_code < 0) {
        return err_code;
    }
#endif   
    return count;
}
static ssize_t sff_mode_sel_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                 char *buf)
{
    u8 mode_sel = 0;
    int err_code = -1;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    int type = sff_type_get(sff_obj);

    if (SFP_TYPE != type) {
#if 0
        if (mode_sel_get(sff_obj, &mode_sel) < 0) {
            return scnprintf(buf, BUF_SIZE, "%d\n", err_code);
        }
#endif        
    } else {

        return scnprintf(buf, BUF_SIZE, "%d\n", err_code);
    }

    return scnprintf(buf, BUF_SIZE, "%d\n", mode_sel);
}
static ssize_t sff_tx_disable_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                    const char *buf, size_t count)
{
    int ret = 0;
    int tx_disable = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    ret = sscanf_to_int(buf, &tx_disable);

    if(ret < 0) {
        return ret;
    }

    ret = sff_obj->func_tbl->tx_disable_set(sff_obj, tx_disable);

    if(ret < 0) {
        return ret;
    }
    return count;
}
static ssize_t sff_tx_disable_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   char *buf)
{
    u8 tx_disable = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sff_obj->func_tbl->tx_disable_get(sff_obj, &tx_disable);

    if(ret < 0) {
        return ret;
    }


    return scnprintf(buf, BUF_SIZE, "%d\n", tx_disable);
}

static ssize_t sff_prs_all_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                char *buf)
{
    int ret = 0;
    unsigned long front_port_prs = 0;
    int port = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->mgr;
    int port_num = sff->valid_port_num;
    
    if ((ret = sff_prs_bitmap_get_external(sff, &front_port_prs)) < 0) {
        return ret;
    }

    for (port = 0; port < port_num; port++) {

        buf[port] = (test_bit(port, &front_port_prs) ? '1' : '0');
    }

    buf[port] = '\n';
    return (ssize_t)strlen(buf);
}
static ssize_t swps_polling_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  const char *buf, size_t count)
{
    int enable = 0;
    int err_code=0;
    err_code = sscanf_to_int(buf, &enable);
    if(err_code < 0)
        return err_code;

    if(enable == swps_polling_is_enabled()) {
        return count;
    }

    if(enable) {
        swps_polling_task_start();
    } else {
        swps_polling_task_stop();
    }
    swps_polling_set(enable);

    return count;
}

static ssize_t sff_apsel_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  const char *buf, size_t count)
{
    int ret = 0;
    int apsel = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    ret = sscanf_to_int(buf, &apsel);
    
    if(ret < 0) {
        return ret;
    }
    if ((ret = qsfp_dd_apsel_apply(sff_obj, apsel)) < 0) {
        return ret;
    }

    return count;
}
static ssize_t sff_apsel_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   char *buf)
{
    int apsel = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    apsel = qsfp_dd_apsel_get(sff_obj);

    return scnprintf(buf, BUF_SIZE, "%d\n", apsel);
}

static ssize_t sff_intr_flag_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  const char *buf, size_t count)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    
    if (!match(buf, CLEAR_CMD)) {
        return -EINVAL; 
    } 
    check_pfunc(sff_obj->func_tbl->intr_flag_clear);
    sff_obj->func_tbl->intr_flag_clear(sff_obj);

    return count;
}

static ssize_t swps_polling_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                 char *buf)
{

    return scnprintf(buf, BUF_SIZE, "%d\n", swps_polling_is_enabled());
}

static ssize_t sff_int_flag_monitor_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
        const char *buf, size_t count)
{
    int enable = 0;
    int err_code=0;
    err_code = sscanf_to_int(buf, &enable);
    if(err_code < 0)
        return err_code;

    int_flag_monitor_en = ((enable == 1) ? true : false);

    return count;
}
static ssize_t sff_int_flag_monitor_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
        char *buf)
{
    int enable = 0;

    enable = ((int_flag_monitor_en == true) ? 1 : 0);
    return scnprintf(buf, BUF_SIZE, "%d\n", enable);
}

static ssize_t sff_page_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                             char *buf)
{
    int ret = 0;
    u8 page = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    ret = sff_obj->func_tbl->page_get(sff_obj, &page);
    if (ret < 0) {
        return ret;
    }
    return scnprintf(buf, BUF_SIZE, "%d\n", page);
}

static ssize_t sff_page_sel_lock_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                             char *buf)
{
    u8 lock = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    
    lock = (page_sel_is_locked(sff_obj) ? 1 : 0);
    return scnprintf(buf, BUF_SIZE, "%d\n", lock);
}

static struct swps_attribute lc_fsm_st_attr =
    __ATTR(fsm_st, S_IRUGO, lc_fsm_st_show, NULL);

static struct swps_attribute sff_port_num_attr =
    __ATTR(port_num, S_IRUGO, sff_port_num_show, NULL);

static struct swps_attribute sff_type_attr =
    __ATTR(type, S_IRUGO, type_show, NULL);

static struct swps_attribute sff_id_attr =
    __ATTR(id, S_IRUGO, id_show, NULL);

static struct swps_attribute sff_transvr_type_attr =
    __ATTR(transvr_type, S_IRUGO, transvr_type_show, NULL);

static struct swps_attribute sff_fsm_st_attr =
    __ATTR(fsm_st, S_IRUGO, fsm_st_show, NULL);

static struct swps_attribute sff_prs_all_attr =
    __ATTR(prs, S_IRUGO, sff_prs_all_show, NULL);

static struct swps_attribute swps_polling_attr =
    __ATTR(swps_polling, S_IWUSR|S_IRUGO, swps_polling_show, swps_polling_store);

static struct swps_attribute sff_int_flag_monitor_attr =
    __ATTR(sff_int_flag_monitor, S_IWUSR|S_IRUGO, sff_int_flag_monitor_show, sff_int_flag_monitor_store);

static struct swps_attribute sff_reset_attr =
    __ATTR(reset, S_IWUSR|S_IRUGO, sff_reset_show, sff_reset_store);
static struct swps_attribute sff_reset_all_attr =
    __ATTR(reset, S_IWUSR|S_IRUGO, sff_reset_all_show, sff_reset_all_store);

static struct swps_attribute sff_lpmode_all_attr =
    __ATTR(lpmode, S_IWUSR|S_IRUGO, sff_lpmode_all_show, sff_lpmode_all_store);

static struct swps_attribute sff_power_all_attr =
    __ATTR(power, S_IWUSR|S_IRUGO, sff_power_all_show, sff_power_all_store);

static struct swps_attribute sff_power_attr =
    __ATTR(power, S_IWUSR|S_IRUGO, sff_power_show, sff_power_store);

static struct swps_attribute sff_intL_attr =
    __ATTR(intL, S_IRUGO, sff_intL_show, NULL);

static struct swps_attribute sff_lpmode_attr =
    __ATTR(lpmode, S_IWUSR|S_IRUGO, sff_lpmode_show, sff_lpmode_store);

static struct swps_attribute sff_mode_sel_attr =
    __ATTR(mode_sel, S_IWUSR|S_IRUGO, sff_mode_sel_show, sff_mode_sel_store);

static struct swps_attribute sff_tx_disable_attr =
    __ATTR(tx_disable, S_IWUSR|S_IRUGO, sff_tx_disable_show, sff_tx_disable_store);

static struct swps_attribute sff_rx_los_attr =
    __ATTR(rx_los, S_IRUGO, sff_rx_los_show, NULL);

static struct swps_attribute sff_tx_los_attr =
    __ATTR(tx_los, S_IRUGO, sff_tx_los_show, NULL);

static struct swps_attribute sff_tx_fault_attr =
    __ATTR(tx_fault, S_IRUGO, sff_tx_fault_show, NULL);

static struct swps_attribute sff_tx_eq_attr =
    __ATTR(tx_eq, S_IWUSR|S_IRUGO, sff_tx_eq_show, sff_tx_eq_store);

static struct swps_attribute sff_rx_em_attr =
    __ATTR(rx_em, S_IWUSR|S_IRUGO, sff_rx_em_show, sff_rx_em_store);

static struct swps_attribute sff_rx_am_attr =
    __ATTR(rx_am, S_IWUSR|S_IRUGO, sff_rx_am_show, sff_rx_am_store);

static struct swps_attribute sff_tx_power_attr =
    __ATTR(tx_power, S_IRUGO, sff_tx_power_show, NULL);

static struct swps_attribute sff_rx_power_attr =
    __ATTR(rx_power, S_IRUGO, sff_rx_power_show, NULL);

static struct swps_attribute sff_tx_bias_attr =
    __ATTR(tx_bias, S_IRUGO, sff_tx_bias_show, NULL);

static struct swps_attribute sff_vendor_name_attr =
    __ATTR(vendor_name, S_IRUGO, vendor_name_show, NULL);

static struct swps_attribute sff_vendor_part_number_attr =
    __ATTR(vendor_pn, S_IRUGO, vendor_pn_show, NULL);

static struct swps_attribute sff_vendor_serial_number_attr =
    __ATTR(vendor_sn, S_IRUGO, vendor_sn_show, NULL);

static struct swps_attribute sff_vendor_rev_attr =
    __ATTR(vendor_rev, S_IRUGO, vendor_rev_show, NULL);

static struct swps_attribute sff_temperature_attr =
    __ATTR(temperature, S_IRUGO, temperature_show, NULL);

static struct swps_attribute sff_voltage_attr =
    __ATTR(voltage, S_IRUGO, voltage_show, NULL);

static struct swps_attribute sff_eeprom_dump_attr =
    __ATTR(eeprom_dump, S_IRUGO, sff_eeprom_dump_show, NULL);
static struct swps_attribute sff_page_attr =
    __ATTR(page, S_IWUSR|S_IRUGO, sff_page_show, sff_page_store);
static struct swps_attribute sff_page_sel_lock_attr =
    __ATTR(page_sel_lock, S_IWUSR|S_IRUGO, sff_page_sel_lock_show, sff_page_sel_lock_store);
static struct swps_attribute sff_module_st_attr =
    __ATTR(module_st, S_IRUGO, module_st_show, NULL);
static struct swps_attribute sff_active_ctrl_set_attr =
    __ATTR(active_ctrl_set, S_IRUGO, active_ctrl_set_show, NULL);

static struct swps_attribute sff_apsel_attr =
    __ATTR(apsel, S_IWUSR|S_IRUGO, sff_apsel_show, sff_apsel_store);

static struct swps_attribute sff_intr_flag_attr =
    __ATTR(intr_flag, S_IWUSR|S_IRUGO, sff_intr_flag_show, sff_intr_flag_store);

static struct swps_attribute mux_reset_attr =
    __ATTR(mux_reset, S_IWUSR|S_IRUGO, gpio_mux_reset_show, gpio_mux_reset_store);
static struct swps_attribute swps_version_attr =
    __ATTR(swps_version, S_IRUGO, swps_version_show, NULL);
static struct swps_attribute lc_prs_attr =
    __ATTR(card_prs, S_IRUGO, lc_prs_show, NULL);
static struct swps_attribute lc_temp_attr =
    __ATTR(temp, S_IWUSR|S_IRUGO, lc_temp_show, lc_temp_store);
static struct swps_attribute lc_power_ready_attr =
    __ATTR(power, S_IRUGO, lc_power_ready_show, NULL);
static struct swps_attribute lc_phy_ready_attr =
    __ATTR(phy_ready, S_IWUSR|S_IRUGO, lc_phy_ready_show, lc_phy_ready_store);
static struct swps_attribute lc_type_attr =
    __ATTR(type, S_IRUGO, lc_type_show, NULL);
static struct swps_attribute rev4_quick_attr =
    __ATTR(rev4_quick, S_IWUSR|S_IRUGO, rev4_quick_show, rev4_quick_store);
static struct swps_attribute log_level_attr =
    __ATTR(log_level, S_IWUSR|S_IRUGO, log_level_show, log_level_store);
static struct swps_attribute pltfm_name_attr =
    __ATTR(platform, S_IRUGO, pltfm_name_show, NULL);
static struct swps_attribute io_no_init_attr =
    __ATTR(io_no_init, S_IRUGO, io_no_init_show, NULL);

static struct attribute *sfp_attributes[] = {

    /*io pin attribute*/
    &sff_tx_disable_attr.attr,
    /*eeprom attribute*/
    &sff_type_attr.attr,
    &sff_id_attr.attr,
    &sff_tx_eq_attr.attr,
    &sff_rx_em_attr.attr,
    &sff_rx_am_attr.attr,
    &sff_rx_los_attr.attr,
    &sff_tx_los_attr.attr,
    &sff_tx_fault_attr.attr,
    &sff_tx_power_attr.attr,
    &sff_rx_power_attr.attr,
    &sff_tx_bias_attr.attr,
    &sff_temperature_attr.attr,
    &sff_voltage_attr.attr,
    &sff_vendor_name_attr.attr,
    &sff_vendor_part_number_attr.attr,
    &sff_vendor_serial_number_attr.attr,
    &sff_vendor_rev_attr.attr,
    /*transceiver identified info attribute*/
    &sff_transvr_type_attr.attr,
    &sff_fsm_st_attr.attr,
    //&sff_eeprom_dump_attr.attr,
    NULL

};

static struct attribute_group sfp_group = {
    .attrs = sfp_attributes,
};

static struct attribute *qsfp_attributes[] = {
    /*io pin attribute*/
    &sff_reset_attr.attr,
    &sff_power_attr.attr,
    &sff_lpmode_attr.attr,
    &sff_mode_sel_attr.attr,
    &sff_intL_attr.attr,
    /*eeprom attribute*/
    &sff_type_attr.attr,
    &sff_id_attr.attr,
    &sff_tx_eq_attr.attr,
    &sff_rx_em_attr.attr,
    &sff_rx_am_attr.attr,
    &sff_rx_los_attr.attr,
    &sff_tx_los_attr.attr,
    &sff_tx_fault_attr.attr,
    &sff_tx_power_attr.attr,
    &sff_rx_power_attr.attr,
    &sff_tx_bias_attr.attr,
    &sff_temperature_attr.attr,
    &sff_voltage_attr.attr,
    &sff_tx_disable_attr.attr,
    &sff_vendor_name_attr.attr,
    &sff_vendor_part_number_attr.attr,
    &sff_vendor_serial_number_attr.attr,
    &sff_vendor_rev_attr.attr,
    /*transceiver identified info attribute*/
    &sff_transvr_type_attr.attr,
    &sff_fsm_st_attr.attr,
    &sff_eeprom_dump_attr.attr,
    &sff_page_attr.attr,
    NULL
};
static struct attribute_group qsfp_group = {
    .attrs = qsfp_attributes,
};
static struct attribute *qsfp_dd_attributes[] = {
    /*io pin attribute*/
    &sff_reset_attr.attr,
    &sff_power_attr.attr,
    &sff_lpmode_attr.attr,
    &sff_mode_sel_attr.attr,
    &sff_intL_attr.attr,
    /*eeprom attribute*/
    &sff_type_attr.attr,
    &sff_id_attr.attr,
    &sff_rx_los_attr.attr,
    &sff_tx_los_attr.attr,
    &sff_tx_fault_attr.attr,
    &sff_tx_power_attr.attr,
    &sff_rx_power_attr.attr,
    &sff_tx_bias_attr.attr,
    &sff_temperature_attr.attr,
    &sff_voltage_attr.attr,
    &sff_tx_disable_attr.attr,
    &sff_vendor_name_attr.attr,
    &sff_vendor_part_number_attr.attr,
    &sff_vendor_serial_number_attr.attr,
    &sff_vendor_rev_attr.attr,
    /*transceiver identified info attribute*/
    //&sff_transvr_type_attr.attr,
    &sff_fsm_st_attr.attr,
    &sff_eeprom_dump_attr.attr,
    &sff_page_attr.attr,
    &sff_page_sel_lock_attr.attr,
    &sff_module_st_attr.attr,
    &rev4_quick_attr.attr,
    &sff_active_ctrl_set_attr.attr,
    &sff_apsel_attr.attr,
    &sff_intr_flag_attr.attr,
    NULL
};
static struct attribute_group qsfp_dd_group = {
    .attrs = qsfp_dd_attributes,
};
/*attribute of all sff modules , mainly reprsed as bitmap form*/
static struct attribute *sff_common_attributes[] = {
    &sff_prs_all_attr.attr,
    &mux_reset_attr.attr,
    &sff_int_flag_monitor_attr.attr,
    &sff_port_num_attr.attr,
    &sff_reset_all_attr.attr,
    &sff_lpmode_all_attr.attr,
    &sff_power_all_attr.attr,
    NULL
};
static struct attribute_group sff_common_group = {
    .attrs = sff_common_attributes,
};
static struct attribute *lc_common_attributes[] = {
    &swps_version_attr.attr,
    &swps_polling_attr.attr,
    &lc_prs_attr.attr,
    &log_level_attr.attr,
    &pltfm_name_attr.attr,
    &io_no_init_attr.attr,
    NULL
};
static struct attribute_group lc_common_group = {
    .attrs = lc_common_attributes,
};

static struct attribute *lc_attributes[] = {
    &lc_temp_attr.attr,
    &lc_power_ready_attr.attr,
    &lc_phy_ready_attr.attr,
    &lc_type_attr.attr,
    &lc_fsm_st_attr.attr,
    NULL
};
static struct attribute_group lc_group = {
    .attrs = lc_attributes,
};

static void sff_frontPort_remap_destroy(struct sff_mgr_t *sff)
{
    if (p_valid(sff->frontPort_to_port)) {
        kfree(sff->frontPort_to_port);
    }
}
static void lc_sff_frontPort_remap_destroy(struct lc_t *card)
{
    int i = 0;
    int lc_num = card->lc_num;

    for (i = 0; i < lc_num; i++) {
        sff_frontPort_remap_destroy(&card->obj[i].sff);
    }
}

static void sff_objs_destroy(struct sff_mgr_t *sff)
{
    if (p_valid(sff->obj)) {
        kfree(sff->obj);
    }
}
static void lc_sff_objs_destroy(struct lc_t *card)
{
    int i = 0;
    int lc_num = card->lc_num;

    for (i = 0; i < lc_num; i++) {

        sff_objs_destroy(&card->obj[i].sff);
    }
}
static void lc_objs_destroy(struct lc_t *card)
{
    if (p_valid(card->obj)) {
        kfree(card->obj);
    }
}

static int swps_polling_task_start(void)
{
    /*<TBD> check what's the right way to reset the fsm state*/
    schedule_delayed_work(&swps_polling, SWPS_POLLING_PERIOD);
    return 0;

}
static int swps_polling_task_stop(void)
{

    cancel_delayed_work_sync(&swps_polling);
    return 0;

}

static int sff_objs_init(struct sff_mgr_t *sff, struct port_info_table_t *tbl)
{
    struct port_info_map_t *map = NULL;
    struct sff_obj_t *sff_obj = NULL;
    int port_num = 0;
    int port = 0;
    map = tbl->map;
    port_num = tbl->size;

    if (!p_valid(tbl)) {
        return -EBADRQC;
    }

    if (!p_valid(map)) {
        return -EBADRQC;
    }

    sff_obj = sff->obj;

    for (port = 0; port < port_num; port++) {

        sff_obj[port].port = port;
        sff_obj[port].front_port = map[port].front_port;
        /*front port to port remap init*/
        sff->frontPort_to_port[sff_obj[port].front_port] = port;
        sff_obj[port].type = map[port].type;
        sff_obj[port].name = map[port].name;
        sff_fsm_init(&sff_obj[port], map[port].type);
        func_tbl_init(&sff_obj[port], map[port].type);
    }
    return 0;
}
/*size of sff_object is the max port numbers on line card
 *ex: if 400 line card contain 8 ports
         100 line card contain 32 ports
 *       we will define size of sff obj = 32
 *       however we'll only init 8 ports if it's 400g line card
 * */
static int sff_objs_create(struct sff_mgr_t *sff, int size)
{

    struct sff_obj_t *sff_obj = NULL;
    int port = 0;
    struct lc_obj_t *card = sff_to_lc(sff);
    
    sff_obj = kzalloc(sizeof(struct sff_obj_t) * size, GFP_KERNEL);
    if(!p_valid(sff_obj)) {
        return -ENOMEM;
    }
    for (port = 0; port < size; port++) {
    /*link sff objs to their container sff*/
        sff_obj[port].mgr = sff;
    /*assign lc_id to corresponding sff_obj for fast access*/    
        sff_obj[port].lc_id = card->lc_id; 
        sff_obj[port].lc_name = card->name; 
    }
    sff->obj = sff_obj;
    
    return 0;
}
static int lc_sff_objs_create(struct lc_t *card, int max_port_num)
{
    int lc_num = card->lc_num;
    int i = 0;
    int ret = 0;

    for (i = 0; i < lc_num; i++) {
        ret = sff_objs_create(&(card->obj[i].sff), max_port_num);
        if (ret < 0) {
            break;
        }
    }

    if (ret < 0) {
        lc_sff_objs_destroy(card);
    }

    return ret;
}

static int sff_frontPort_remap_create(struct sff_mgr_t *sff, int size)
{

    int *map = NULL;

    map = kzalloc(sizeof(int) * size, GFP_KERNEL);
    if(!p_valid(map)) {
        return -ENOMEM;
    }

    sff->frontPort_to_port = map;
    return 0;


}
static int lc_sff_frontPort_remap_create(struct lc_t *card, int max_port_num)
{
    int lc_num = card->lc_num;
    int i = 0;
    int ret = 0;

    for (i = 0; i < lc_num; i++) {
        ret = sff_frontPort_remap_create(&(card->obj[i].sff), max_port_num);
        if (ret < 0) {
            break;
        }
    }

    if (ret < 0) {
        lc_sff_frontPort_remap_destroy(card);
    }

    return ret;
}
static int lc_objs_create(struct lc_t *card)
{

    struct lc_obj_t *obj = NULL;
    int lc_num = card->lc_num;
    obj = kzalloc(sizeof(struct lc_obj_t) * lc_num, GFP_KERNEL);
    if(!p_valid(obj)) {
        return -ENOMEM;
    }

    card->obj = obj;
    return 0;

}
static int lc_objs_init(struct lc_t *card)
{
    int lc_num = card->lc_num;
    int i = 0;
    struct lc_obj_t *obj = NULL;
    for (i = 0; i < lc_num; i++) {
        obj = &(card->obj[i]);
        obj->mgr = card;
        obj->lc_id = i;
        obj->name = card_name_str[i];
        obj->prs = 0;
        obj->wait_stable_cnt = 0;
        obj->over_temp_cnt = 0;
        obj->st = LC_FSM_ST_IDLE;
        obj->power_ready = false;
        obj->is_phy_ready = true;
        obj->phy_ready_bitmap = 0xff;
        obj->ej_released = false;
        obj->prs_locked = false;
        obj->prs_locked_cnt = 0;
        obj->posi_st = LC_POSI_INIT_ST;
        obj->mux_l1.i2c_ch = lc_dev_mux_l1_i2c_ch_get(obj->lc_id);
        if (obj->mux_l1.i2c_ch < 0) {
            SWPS_LOG_ERR("lc_id:%d invalid i2c_ch:%d\n", obj->lc_id, obj->mux_l1.i2c_ch);
            return -EBADRQC;
        }
    }
    return 0;
}
static int lc_kobj_common_create(struct lc_t *card)
{
    if (!p_valid(card->common_kobj)) {
        card->common_kobj = swps_kobj_add("common", &swpsKset->kobj, &lc_common_group);
    }
    if(!p_valid(card->common_kobj)) {
        return -EBADRQC;
    }
    /*this link will let card kobj able to link to sff_obj*/
    card->common_kobj->sff_obj = card->obj->sff.obj;
    return 0;

}
static void polling_task_1U(void)
{
    lcMgr.lc_func->dev_hdlr();
    lc_fsm_run_1U(&lcMgr);
    if (!i2c_bus_is_alive(&lcMgr.obj[0])) {
        i2c_bus_recovery(&lcMgr.obj[0]);
    }
    io_no_init_handler(&lcMgr);
}
static void polling_task_4U(void)
{
    /*4U real functions
     *self->lc_func->dev_handler(self);
     *lc_prs_scan(self);
     * lc_fsm_run_4U(&lcMgr);
     *  io_no_init_handler(&lcMgr);*/
    lcMgr.lc_func->dev_hdlr();
   // io_dev_hdlr();
    lc_prs_scan(&lcMgr);
    lc_fsm_run_4U(&lcMgr);
    io_no_init_handler(&lcMgr);
}
static int lc_func_load(struct lc_t *obj)
{
    obj->lc_func = &lc_func_1U;

    if (!p_valid(obj->lc_func)) {
        return -ENOSYS;
    }
    return 0;
}
static int lc_create_init(struct lc_t *card, int lc_num, int card_max_port_num)
{
    /*init priv parameter*/
    card->lc_num = lc_num;
    card->lc_prs = 0;
    card->ej_r = 0;
    card->ej_l = 0;
    card->lc_sys_ready = 0;
    if (lc_objs_create(card) < 0) {
        goto exit_err;
    }

    if (lc_objs_init(card) < 0) {
        goto exit_err;
    }

    if (lc_sff_objs_create(card, card_max_port_num) < 0) {
        goto exit_free_lc_objs;
    }

    if (lc_sff_frontPort_remap_create(card, card_max_port_num) < 0) {
        goto exit_free_sff_objs;
    }

    if (lc_kobj_common_create(card) < 0) {
        goto exit_free_frontPort_remap;
        SWPS_LOG_ERR("lc_kobj_common_create fail\n");
    }

    return 0;

exit_free_frontPort_remap:
    lc_sff_frontPort_remap_destroy(card);
exit_free_sff_objs:
    lc_sff_objs_destroy(card);
exit_free_lc_objs:
    lc_objs_destroy(card);
exit_err:
    return -EBADRQC;
}
void swps_kobj_del(struct swps_kobj_t **obj)
{
    if (p_valid(*obj)) {
        kobject_put(&((*obj)->kobj));
        *obj = NULL;
    }
}
/*the function is for adding kobject of port dynamically*/
static struct swps_kobj_t *swps_kobj_add(char *name,
        struct kobject *mgr,
        struct attribute_group *attr_group)
{
    int ret = 0;
    struct swps_kobj_t *obj = NULL;
    obj = kzalloc(sizeof(struct swps_kobj_t), GFP_KERNEL);
    if (!p_valid(obj)) {
        SWPS_LOG_ERR("sff_kobject_create %s kzalloc error", name);
        return NULL;
    }
    obj->kobj.kset = swpsKset;

    ret = kobject_init_and_add(&obj->kobj, &swpsKtype, mgr, name);
    if (ret < 0) {

        kobject_put(&obj->kobj);
        SWPS_LOG_ERR("kobject creat fail1\n");
        return NULL;
    }
    if (NULL != attr_group) {

        SWPS_LOG_DBG("sysfs_create_group: %s\n", name);
        if ((ret = sysfs_create_group(&obj->kobj, attr_group)) != 0) {
            sysfs_remove_group(&obj->kobj, attr_group);
            swps_kobj_del(&obj);
            SWPS_LOG_ERR("create sff:%s attrs error.ret:%d\n", name, ret);
            return NULL;
        }
    }

    kobject_uevent(&obj->kobj, KOBJ_ADD);
    return obj;
}

int sff_fsm_kobj_change_event(struct sff_obj_t *sff_obj)
{

    struct swps_kobj_t *sff_kobj = sff_obj->kobj;
    char *uevent_envp[4];
    char tmp_str_1[32];
    snprintf(tmp_str_1, sizeof(tmp_str_1), "%s=%s", "IF_TYPE", "SR4");
    uevent_envp[0] = tmp_str_1;
    uevent_envp[1] = NULL;

    kobject_uevent_env(&sff_kobj->kobj, KOBJ_CHANGE, uevent_envp);
    return 0;
}
static int sff_kobj_add(struct sff_obj_t *sff_obj)
{
    struct attribute_group *attr_group = &sfp_group;
    struct lc_obj_t *card = NULL;
    struct swps_kobj_t *mgr_kobj = NULL;

    SWPS_LOG_DBG("%s start\n", sff_obj->name);

    if (SFP_TYPE == sff_obj->type) {
        attr_group = &sfp_group;
    } else if (QSFP_TYPE == sff_obj->type) {
        attr_group = &qsfp_group;
    } else {
        attr_group = &qsfp_dd_group;
    }
    card = sff_to_lc(sff_obj->mgr);
    mgr_kobj = sff_obj->mgr->mgr_kobj;
    if (!p_valid(sff_obj->kobj)) {
        sff_obj->kobj = swps_kobj_add(sff_obj->name, &(mgr_kobj->kobj), attr_group);
    }
    if(!p_valid(sff_obj->kobj)) {
        return -EBADRQC;
    }

    sff_obj->kobj->sff_obj = sff_obj;
    return 0;
}

static int sff_kobj_del(struct sff_obj_t *sff_obj)
{
    struct swps_kobj_t **sff_kobj = &(sff_obj->kobj);

    if(p_valid(*sff_kobj)) {
        swps_kobj_del(sff_kobj);
    }
    return 0;
}
static int sff_kset_create_init(void)
{
    swpsKset = kset_create_and_add(SWPS_KSET, NULL, NULL);

    if(!p_valid(swpsKset)) {
        SWPS_LOG_ERR("kset creat fail\n");
        return -EBADRQC;
    }
    return 0;
}
static int sff_kset_deinit(void)
{
    if(p_valid(swpsKset)) {
        kset_unregister(swpsKset);
    }
    return 0;
}

static int lc_kobj_init_create(struct lc_obj_t *card)
{
    int lc_num = card->mgr->lc_num;
    struct attribute_group *attr_group = NULL;

    if (TYPE_4U == lc_num) {
        attr_group = &lc_group;
    }
    if (!p_valid(card->card_kobj)) {
        card->card_kobj = swps_kobj_add(card->name, &swpsKset->kobj, attr_group);
    }
    if(!p_valid(card->card_kobj)) {
        return -EBADRQC;
    }

    /*this link will let card kobj able to link to sff_obj*/
    card->card_kobj->sff_obj = card->sff.obj;
    return 0;
}
static int lc_sff_mgr_kobj_create(struct lc_obj_t *card)
{
#if !defined (DYNAMIC_SFF_KOBJ)
    struct sff_obj_t *sff_obj = NULL;
    int port = 0;
    int port_num = card->sff.valid_port_num;
    int ret = 0;
#endif    
    if(!p_valid(card->card_kobj)) {
        return -EBADRQC;
    }
    
    if(!p_valid(card->sff.mgr_kobj)) {
        card->sff.mgr_kobj = swps_kobj_add("sff", &(card->card_kobj->kobj), &sff_common_group);
    }
    if(!p_valid(card->sff.mgr_kobj)) {
        return -EBADRQC;
    }
    /*this link will let card kobj able to link to sff_obj*/
    card->sff.mgr_kobj->sff_obj = card->sff.obj;
#if !defined (DYNAMIC_SFF_KOBJ)
    for (port = 0; port < port_num; port++) {
        sff_obj = &(card->sff.obj[port]);
        if ((ret =sff_kobj_add(sff_obj)) < 0) {
            return ret;
        }
    }
#endif
    return 0;
}

/*@note: kobj num is identical to valid port num*/
static void sff_kobjs_destroy(struct sff_mgr_t *sff)
{
    int port;
    int port_num = sff->valid_port_num;
    struct sff_obj_t *sff_obj = NULL;
    for (port = 0; port < port_num; port++) {
        sff_obj = &(sff->obj[port]);
        sff_kobj_del(sff_obj);
    }
}
static void lc_sff_kobjs_destroy(struct lc_t *card)
{
    int i;
    int lc_num = card->lc_num;
    struct sff_mgr_t *sff = NULL;
    for (i = 0; i < lc_num; i++) {
        sff = &(card->obj[i].sff);
        sff_kobjs_destroy(sff);
    }

}
static void _lc_kobj_destroy(struct lc_obj_t *obj)
{
    if (p_valid(obj->card_kobj)) {
        swps_kobj_del(&(obj->card_kobj));
    }
}
static void _lc_sff_mgr_kobj_destroy(struct lc_obj_t *obj)
{
    if(p_valid(obj->sff.mgr_kobj)) {
        swps_kobj_del(&(obj->sff.mgr_kobj));
    }
}
static void lc_kobj_destroy(struct lc_t *card)
{

    int i;
    int lc_num = card->lc_num;
    struct lc_obj_t *obj = NULL;
    for (i = 0; i < lc_num; i++) {
        obj = &(card->obj[i]);
        _lc_sff_mgr_kobj_destroy(obj);
        _lc_kobj_destroy(obj);
    }
}
static void lc_common_kobj_destroy(struct lc_t *card)
{
    if(p_valid(card->common_kobj)) {
        swps_kobj_del(&(card->common_kobj));
    }
}
static void transvr_insert(struct sff_obj_t *sff_obj)
{
    SWPS_LOG_INFO("into %s\n", sff_obj->name);
    sff_fsm_st_set(sff_obj, SFF_FSM_ST_INSERTED);
    transvr_type_set(sff_obj, TRANSVR_CLASS_UNKNOWN);
#if defined (DYNAMIC_SFF_KOBJ)
    sff_kobj_add(sff_obj);
#endif
}
static void transvr_remove(struct sff_obj_t *sff_obj)
{
    struct mux_ch_t *mux = NULL;
    struct lc_obj_t *lc = sff_to_lc(sff_obj->mgr);

    mux = &(lc->mux_l1);
    SWPS_LOG_INFO("from %s\n", sff_obj->name);
    sff_fsm_st_set(sff_obj, SFF_FSM_ST_REMOVED);
    transvr_type_set(sff_obj, TRANSVR_CLASS_UNKNOWN);
#if defined (DYNAMIC_SFF_KOBJ)
    sff_kobj_del(sff_obj);
#endif
    
    if (0 != mux->block_ch) {
        mux->is_fail = false;
        mux->block_ch &= ~(1L << (sff_obj->port));
        if(mux_ch_block(mux->i2c_ch, mux->block_ch) < 0) {
            
            SWPS_LOG_INFO(" %s unblock mux ch fail\n", sff_obj->name);
        } else {
            mux_fail_reset(mux->i2c_ch);
            SWPS_LOG_INFO(" %s unblock mux ch ok block_ch:%lx\n", sff_obj->name, mux->block_ch);
        }
    }
}

static int io_no_init_scan(struct sff_mgr_t *sff)
{
    int ret = 0;
    int port = 0;
    int port_num = sff->valid_port_num;
    struct sff_obj_t *sff_obj = NULL;
    unsigned long bitmap=0;


    if((ret = sff_prs_bitmap_get(sff, &bitmap)) < 0) {
        return ret;
    }
    prs_bitmap_update(sff, bitmap); //update current prs_bitmap

    for (port = 0; port < port_num; port++) {

        sff_obj = &sff->obj[port];
        ret = io_no_init_done_fsm_run(sff_obj);
        if (ret < 0) {
            break;
        }
    }

    return ret;
}
static int sff_prs_scan(struct sff_mgr_t *sff)
{
    int ret = 0;
    unsigned long bitmap=0;
    unsigned long prs_change = 0;
    int port = 0;
    struct sff_obj_t *sff_obj = NULL;
    int port_num = sff->valid_port_num;
    /*<TBD> probably can't get all prs pins status at once
     in case  one bad transciver module is plugged in, it will lead to i2c bus hang up
     we need to figure out the way to detect this situation and skip it and send out the warning */
    if((ret = sff_prs_bitmap_get(sff, &bitmap)) < 0) {
        SWPS_LOG_ERR("fail\n");
        return ret;
    }
    /*check which bits are updated*/
    //bitmap = ~bitmap;  /*reverse it to be human readable format*/
    prs_change = bitmap ^ prs_bitmap_get(sff);
    prs_bitmap_update(sff, bitmap); //update current prs_bitmap

    for (port = 0; port < port_num; port++) {
        sff_obj = &(sff->obj[port]);
        if(test_bit(port, &prs_change)) {
            if(test_bit(port, &bitmap)) {
                transvr_insert(sff_obj);
            } else {
                transvr_remove(sff_obj);
            }
        }
    }
    return 0;
}
static void swps_polling_task(struct work_struct *work)
{
    lcMgr.lc_func->polling_task();
    schedule_delayed_work(&swps_polling, SWPS_POLLING_PERIOD);
}


/*fsm functions*/

inline sff_fsm_state_t sff_fsm_st_get(struct sff_obj_t *sff_obj)
{
    return sff_obj->fsm.st;
}
inline void sff_fsm_st_set(struct sff_obj_t *sff_obj, sff_fsm_state_t st)
{
    sff_fsm_state_t old_st = sff_obj->fsm.st;
    sff_fsm_st_chg_process(sff_obj, old_st, st);
    sff_obj->fsm.st = st;
}
/*set target count to new fsm state*/
static void sff_fsm_delay_cnt_reset(struct sff_obj_t *sff_obj, sff_fsm_state_t st)
{
    int found = 0;
    int idx = 0;
    struct sff_fsm_t *fsm = &(sff_obj->fsm);
    struct fsm_period_t *table = fsm->period_tbl;
    for (idx = 0; table[idx].st != SFF_FSM_ST_END; idx++) {
        if (st == table[idx].st) {
            found = 1;
            break;
        }
    }
    fsm->cnt = 0;

    if (found) {
        fsm->delay_cnt = table[idx].delay_cnt;
    } else {
        fsm->delay_cnt = 0;
    }
}
static void sff_fsm_cnt_run(struct sff_obj_t *sff_obj)
{

    (sff_obj->fsm.cnt)++;
}
static bool sff_fsm_delay_cnt_is_hit(struct sff_obj_t *sff_obj)
{
    bool is_hit = false;
    struct sff_fsm_t *fsm = &(sff_obj->fsm);


    if(fsm->cnt >= fsm->delay_cnt) {
        fsm->cnt = 0;
        is_hit = true;
    }
    return is_hit;
}

int dummy_lane_control_set(struct sff_obj_t *sff_obj, int type, u32 value)
{
    return -ENOSYS;
}
int dummy_lane_control_get(struct sff_obj_t *sff_obj, int type, u32 *value)
{
    return -ENOSYS;
}

int dummy_lane_status_get(struct sff_obj_t *sff_obj, int type, u8 *st)
{
    return -ENOSYS;
}

int sff_prs_get(struct sff_obj_t *sff_obj, u8 *prs)
{
    int ret = 0;
    unsigned long bitmap = 0;
    struct sff_mgr_t *sff = sff_obj->mgr;
    if (!p_valid(prs)) {
        return -EINVAL;
    }
    if ((ret = sff_prs_bitmap_get(sff, &bitmap)) < 0) {
        return ret;
    }
    if (test_bit(sff_obj->port, &bitmap)) {
        *prs = 1;
    } else {
        *prs = 0;
    }
    return 0;
}
int sff_power_set(struct sff_obj_t *sff_obj, u8 value)
{
    int ret = 0;
    
    check_pfunc(sff_obj->mgr->io_drv->power_set);
    if ((ret = sff_obj->mgr->io_drv->power_set(sff_obj->lc_id, sff_obj->port, value)) < 0) {
        return ret;
    }

    return 0;
}
int sff_power_get(struct sff_obj_t *sff_obj, u8 *value)
{
    int ret = 0;

    check_pfunc(sff_obj->mgr->io_drv->power_get);
    if ((ret = sff_obj->mgr->io_drv->power_get(sff_obj->lc_id, sff_obj->port, value)) < 0) {
        return ret;
    }
    return 0;
}

int dummy_intL_get(struct sff_obj_t *sff_obj, u8 *value)
{
    return -ENOSYS;

}
int dummy_reset_set(struct sff_obj_t *sff_obj, u8 value)
{
    return -ENOSYS;
}
int dummy_reset_get(struct sff_obj_t *sff_obj, u8 *value)
{

    return -ENOSYS;

}
int dummy_lpmode_set(struct sff_obj_t *sff_obj, u8 value)
{
    return -ENOSYS;
}
int dummy_lpmode_get(struct sff_obj_t *sff_obj, u8 *value)
{

    return -ENOSYS;

}
int dummy_mode_sel_set(struct sff_obj_t *sff_obj, u8 value)
{

    return -ENOSYS;

}
int dummy_mode_sel_get(struct sff_obj_t *sff_obj, u8 *value)
{

    return -ENOSYS;

}
void sff_fsm_st_chg_process(struct sff_obj_t *sff_obj,
                                  sff_fsm_state_t cur_st,
                                  sff_fsm_state_t next_st)
{
    if (cur_st != next_st) {

        sff_fsm_delay_cnt_reset(sff_obj, next_st);
        //SWPS_LOG_DBG("port:%d st change:%d -> %d\n",
        //            port, st,sff_fsm_st_get(sff_obj));
        if (p_valid(sff_obj->lc_name) && p_valid(sff_obj->name)) {
            SWPS_LOG_DBG("%s %s st change:%s -> %s\n",
                        sff_obj->lc_name, sff_obj->name, sff_fsm_st_str[cur_st],
                        sff_fsm_st_str[next_st]);
        }
    }
}
/*io_no_init is special functions when remove and re insert modules don't do sff io control to avoid cause port flap*/
static void io_no_init_port_done_set(struct sff_obj_t *sff_obj)
{
    unsigned long *bitmap = &(sff_obj->mgr->io_no_init_port_done);
    set_bit(sff_obj->port, bitmap);

}
static int io_no_init_done_fsm_run(struct sff_obj_t *sff_obj)
{
    u8 prs = 0;
    int ret = 0;
    int old_st = 0;
    if ((ret = sff_prs_get(sff_obj, &prs)) < 0) {
        return ret;
    }

    old_st = sff_fsm_st_get(sff_obj);
    if (prs) {
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_READY);
#if defined (DYNAMIC_SFF_KOBJ)
        sff_kobj_add(sff_obj);
#endif
    } else {
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_REMOVED);
#if defined (DYNAMIC_SFF_KOBJ)
        sff_kobj_del(sff_obj);
#endif
    }

    io_no_init_port_done_set(sff_obj);

    return ret;
}
static void io_no_init_handler_by_card(struct lc_obj_t *card)
{
    unsigned long mask = 0;
    struct sff_mgr_t *sff = &(card->sff);
    unsigned long *card_done = NULL;
    unsigned long port_done = 0;
    if (0 == io_no_init) {
        return;
    }
    card_done = &(card->mgr->io_no_init_all_done);
    if (test_bit(card->lc_id, card_done)) {
        return;
    }
    mask = MASK_CREATE(sff->valid_port_num);
    port_done = sff->io_no_init_port_done;
    SWPS_LOG_DBG("done_bitmap:0x%lx mask:0x%lx\n", port_done, mask);
    if (mask == port_done) {
        SWPS_LOG_DBG("card:%d io_no_init done\n", card->lc_id);
        set_bit(card->lc_id, card_done);
        /*switch back to normal present scan function*/
        sff->prs_scan = sff_prs_scan;
    }
}
static void io_no_init_handler(struct lc_t *card)
{
    int i = 0;
    int lc_num = card->lc_num;
    unsigned long all_done = card->io_no_init_all_done;
    unsigned long lc_ready = 0;
    lc_fsm_st_t st = LC_FSM_ST_IDLE;

    if (io_no_init) {
        for (i = 0; i < lc_num; i++) {
            st = lc_fsm_st_get(&card->obj[i]);
            if (LC_FSM_ST_READY == st) {
                set_bit(i, &lc_ready);
            }
        }

        if (all_done != 0 &&
                lc_ready == all_done) {
            io_no_init = 0;
            SWPS_LOG_DBG("io_no_init all done\n");
        }
    }

}
static int sff_fsm_run(struct sff_mgr_t *sff)
{
    int port = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = NULL;
    int port_num = sff->valid_port_num;

    for(port = 0; port < port_num; port++) {
        sff_obj = &(sff->obj[port]);
        if (sff_fsm_delay_cnt_is_hit(sff_obj)) {
            ret = sff_obj->fsm.task(sff_obj);
            if (ret < 0) {
                break;
            }
        }
        sff_fsm_cnt_run(sff_obj);
    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}

static int lc_phy_ready(struct lc_obj_t *card, bool *is_ready)
{
    unsigned long bitmap = 0;
    if (!p_valid(is_ready)) {
        return -EINVAL;
    }
    bitmap = card->phy_ready_bitmap;
    check_pfunc(card->mgr->lc_func->phy_ready);
    card->mgr->lc_func->phy_ready(bitmap, is_ready);

    return 0;
}

static int _lc_fsm_run_1U(struct lc_obj_t *card)
{
    lc_fsm_st_t st = LC_FSM_ST_IDLE;
    int ret = 0;
    struct port_info_table_t *port_info = NULL;

    if (!p_valid(card)) {
        SWPS_LOG_ERR("NULL ptr\n");
        return -EINVAL;
    }
    st = lc_fsm_st_get(card);
    switch (st) {

    case LC_FSM_ST_IDLE:
        lc_fsm_st_set(card, LC_FSM_ST_INIT);
        break;

    case LC_FSM_ST_INIT:

        ret = lc_kobj_init_create(card);
        if (ret < 0) {
            break;
        }
        port_info = pltfmInfo->tbl_1st;
        if (!p_valid(port_info)) {
            ret = -EBADRQC;
            break;
        }
        sff_data_init(&card->sff, port_info->size);
        sff_func_init(&card->sff);
        if(sff_objs_init(&(card->sff), port_info) < 0) {
            SWPS_LOG_ERR("sff_objs_init fail\n");
            break;;
        }

        ret = lc_sff_mgr_kobj_create(card);
        if (ret < 0) {
            break;
        }

        lc_fsm_st_set(card, LC_FSM_ST_READY);

        break;

    case LC_FSM_ST_READY:

        if ((ret = card->sff.prs_scan(&card->sff)) < 0) {
            break;
        }
        if ((ret = sff_fsm_run(&card->sff)) < 0) {
            break;
        }
        io_no_init_handler_by_card(card);

        break;

    default:
        break;
    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}
static bool is_skip_io_ctrl(void)
{
    return (io_no_init ? true : false);
}

static void lc_sys_ready_set(struct lc_obj_t *card, bool ready)
{
    unsigned long *sys_ready = &(card->mgr->lc_sys_ready);
    if (ready) {
        set_bit(card->lc_id, sys_ready);
    } else {
        clear_bit(card->lc_id, sys_ready);
    }
}    
static int _lc_fsm_run_4U(struct lc_obj_t *card)
{
    lc_fsm_st_t st = LC_FSM_ST_IDLE;
    int ret = 0;
    bool is_power_ready = false;
    bool is_phy_ready = false;
    bool asserted = false;
    bool deasserted = false;
    struct port_info_table_t *port_info = NULL;
    struct lc_func_t *lc_func = NULL;
    struct mux_ch_t *mux = NULL;
    lc_type_t type = LC_UNKNOWN_TYPE;

    if (!p_valid(card)) {
        SWPS_LOG_ERR("NULL ptr\n");
        return -EINVAL;
    }
    lc_func = card->mgr->lc_func;
    st = lc_fsm_st_get(card);
    switch (st) {

    case LC_FSM_ST_IDLE:
        break;
    case LC_FSM_ST_REMOVE:
        
        sff_kobjs_destroy(&card->sff);
        _lc_sff_mgr_kobj_destroy(card);
        _lc_kobj_destroy(card);
        mux = &(card->mux_l1);
        if (0 != mux->block_ch) {
            if ((ret = mux_ch_block(mux->i2c_ch, 0)) < 0) {
                break;
            }
            mux_fail_reset(mux->i2c_ch);
        }
        check_pfunc(lc_func->power_set);
        if ((ret = lc_func->power_set(card->lc_id, false)) < 0) {
            SWPS_LOG_ERR("%s turn off power fail\n", card->name);
        }
        check_pfunc(lc_func->led_set);
        if ((ret = lc_func->led_set(card->lc_id, LC_LED_CTRL_OFF)) < 0) {
            break;
        }

        lc_sys_ready_set(card, false); 
        lc_fsm_st_set(card, LC_FSM_ST_IDLE);
        break;

    case LC_FSM_ST_INSERT:

        lc_fsm_st_set(card, LC_FSM_ST_WAIT_STABLE);
        card->wait_stable_cnt = 0;
        check_pfunc(lc_func->reset_set);
        if ((ret = lc_func->reset_set(card->lc_id, 0)) < 0) {
            break;
        }
        break;
    case LC_FSM_ST_WAIT_STABLE:    
        card->wait_stable_cnt++;
        if (card->wait_stable_cnt >= LC_INSERT_WAIT_STABLE_NUM) {
            card->wait_stable_cnt = 0;
        } else {
            break;
        }
        check_pfunc(lc_func->type_get);
        if ((ret = lc_func->type_get(card->lc_id, &type)) < 0) {
            card->type = LC_UNKNOWN_TYPE; 
            break;
        }
        card->type = type;
        
	/*<TBD> do we need to check power status before setting the power*/
        ret = lc_kobj_init_create(card);
        if (ret < 0) {
            break;
        }
        /*init sw structure*/
        port_info = lc_port_info_get(card, card->type);
        if (!p_valid(port_info)) {
            ret = -EBADRQC;
            lc_fsm_st_set(card, LC_FSM_ST_FAULT);
            break;
        }
        sff_data_init(&card->sff, port_info->size);
        sff_func_init(&card->sff);
        if(sff_objs_init(&(card->sff), port_info) < 0) {
            SWPS_LOG_ERR("sff_objs_init fail\n");
            break;;
        }
       
        if (is_skip_io_ctrl()) {
            lc_fsm_st_set(card, LC_FSM_ST_INIT);
            break;
        }
        lc_fsm_st_set(card, LC_FSM_ST_POWER_ON);

        break;
    case LC_FSM_ST_POWER_ON:
        
        check_pfunc(lc_func->power_set);
        if ((ret = lc_func->power_set(card->lc_id, true)) < 0) {
            break;
        }

        lc_fsm_st_set(card, LC_FSM_ST_POWER_CHECK);

        break;

    case LC_FSM_ST_POWER_CHECK:
        
        check_pfunc(lc_func->power_ready);
        if ((ret = lc_func->power_ready(card->lc_id, &is_power_ready)) < 0) {
            break;
        }
        
        if (is_power_ready) {
            check_pfunc(lc_func->phy_reset_set);
            if ((ret = lc_func->phy_reset_set(card->lc_id, 1)) < 0) {
                break;
            }
            lc_fsm_st_set(card, LC_FSM_ST_INIT);
        }
        break;
    
    case LC_FSM_ST_INIT:
        /*the follow status is used to notify phy driver the lc sys(power and phy reset) is ready*/
        lc_sys_ready_set(card, true); 
        
        check_pfunc(lc_func->cpld_init);
        if ((ret = lc_func->cpld_init(card->lc_id)) < 0) {
            break;
        }
        ret = lc_sff_mgr_kobj_create(card);
        if (ret < 0) {
            break;
        }

        lc_fsm_st_set(card, LC_FSM_ST_PHY_CHECK);

        break;

    case LC_FSM_ST_PHY_CHECK:

        if ((ret = lc_phy_ready(card, &is_phy_ready)) < 0) {
            break;
        }
        if (is_phy_ready) {
            check_pfunc(lc_func->led_set);
            if ((ret = lc_func->led_set(card->lc_id, LC_LED_CTRL_GREEN_ON)) < 0) {
                
                SWPS_LOG_INFO("%s phy ready\n", card->name);
                break;
            }
            if ((ret = lc_dev_led_boot_amber_set(card->lc_id, false)) < 0) {
                break;
            }
        }
        lc_fsm_st_set(card, LC_FSM_ST_READY);
        SWPS_LOG_INFO("%s becomes ready\n", card->name);
        break;
   
    case LC_FSM_ST_READY:

        ret = lc_phy_ready(card, &is_phy_ready);
        if (ret < 0) {
            break;
        }
        
        if (!is_phy_ready) {
            
            check_pfunc(lc_func->led_set);
            if ((ret = lc_func->led_set(card->lc_id, LC_LED_CTRL_RED_ON)) < 0) {
                break;
            }
            //lc_fsm_st_set(card, LC_FSM_ST_FAULT);
        } else {
            
            check_pfunc(lc_func->led_set);
            if ((ret = lc_func->led_set(card->lc_id, LC_LED_CTRL_GREEN_ON)) < 0) {
                break;
            }

        }
        
        if ((ret = lc_sff_intr_hdlr_byCard(card->lc_id)) < 0) {
            break;
        }
        
        if ((ret = card->sff.prs_scan(&card->sff)) < 0) {
            break;
        }
        
        if ((ret = sff_fsm_run(&card->sff)) < 0) {
            break;
        }
        if (!i2c_bus_is_alive(card)) {
            i2c_bus_recovery(card);
        }
        if (card->over_temp_cnt++ >= LC_OVER_TEMP_NUM) {
            card->over_temp_cnt = 0;

            check_pfunc(lc_func->over_temp_asserted);
            if ((ret = lc_func->over_temp_asserted(card->lc_id, &asserted)) < 0) {
                break;
            }
            if (asserted) {
                SWPS_LOG_ERR("%s thermal trip!\n", card->name);
                check_pfunc(lc_func->power_set);
                if ((ret = lc_func->power_set(card->lc_id, false)) < 0) {
                    break;
                }
                check_pfunc(lc_func->led_set);
                if ((ret = lc_func->led_set(card->lc_id, LC_LED_CTRL_OFF)) < 0) {
                    break;
                }
                lc_sys_ready_set(card, false); 

                sff_kobjs_destroy(&card->sff);
                sff_data_reset(&card->sff);
                _lc_sff_mgr_kobj_destroy(card);
                lc_fsm_st_set(card, LC_FSM_ST_THERMAL_TRIP);
                break;
            }
        } 
        if ((ret = lc_sff_intr_hdlr_byCard(card->lc_id)) < 0) {
            break;
        }
        
        if ((ret = card->sff.prs_scan(&card->sff)) < 0) {
            break;
        }
        
        io_no_init_handler_by_card(card);

        break;
    case LC_FSM_ST_THERMAL_TRIP:
        /*if over-temp warning is lifted, restart power on sequence*/
        /*temp check*/
        if (card->over_temp_cnt++ >= 10) {
            card->over_temp_cnt = 0;
            check_pfunc(lc_func->over_temp_deasserted);
            if ((ret = lc_func->over_temp_deasserted(card->lc_id, &deasserted)) < 0) {
                break;
            }
            if (deasserted) {
                lc_fsm_st_set(card, LC_FSM_ST_POWER_ON);
                check_pfunc(lc_func->reset_set);
                if ((ret = lc_func->reset_set(card->lc_id, 0)) < 0) {
                    break;
                }
            }
        }
        break;
    case LC_FSM_ST_UNSUPPORTED:
        break;
    case LC_FSM_ST_FAULT:
        //lc_fsm_st_set(card, LC_FSM_ST_PHY_CHECK);
        break;
    default:
        break;
    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}

static int lc_fsm_run_1U(struct lc_t *card)
{
    int ret = 0;
    ret = _lc_fsm_run_1U(&card->obj[0]);
    if (ret < 0) {
        return ret;
    }
    return 0;
}
/*i2c bus check and recovery functons*/
static bool i2c_bus_is_alive(struct lc_obj_t *card)
{
    struct lc_func_t *lc_func = card->mgr->lc_func;
    if (NULL == lc_func->i2c_is_alive) {
        SWPS_LOG_ERR("no function\n");
        return false;
    }
    return lc_func->i2c_is_alive(card->lc_id);
}
static void mux_reset_seq(struct lc_obj_t *card)
{
    struct lc_func_t *lc_func = card->mgr->lc_func;
    if (NULL == lc_func->mux_reset_set) {
        SWPS_LOG_ERR("no function\n");
        return;
    }

    lc_func->mux_reset_set(card->lc_id, 0);
    msleep(1);
    lc_func->mux_reset_set(card->lc_id, 1);
}
#if 0 /*old method keep it temporary*/
static void bad_transvr_detect(struct lc_obj_t *card)
{
    int port = 0;
    struct sff_obj_t *sff_obj = NULL;
    int port_num = card->sff.valid_port_num;

    SWPS_LOG_DBG("%s start\n", card->name);
    for (port = 0; port < port_num; port++) {

        sff_obj = &(card->sff.obj[port]);
        if (SFF_FSM_ST_ISOLATED == sff_fsm_st_get(sff_obj)) {
            continue;
        }
        if (SFF_FSM_ST_IDLE != sff_fsm_st_get(sff_obj)) {

            if (sff_obj->func_tbl->is_id_matched(sff_obj)) {
                SWPS_LOG_DBG("%s id matched\n", sff_obj->name);
                continue;
            }
            if (!i2c_bus_is_alive(card)) {
                SWPS_LOG_ERR("transvr in %s isolated\n", sff_obj->name);
                sff_fsm_st_set(sff_obj, SFF_FSM_ST_ISOLATED);
                mux_reset_seq(card);
                mux_reset_ch_resel_byLC(card);
            } else {

                SWPS_LOG_ERR("%s i2c bus is alive\n", sff_obj->name);
            }
        }

    }
    
    SWPS_LOG_ERR("bad transvr report == \n");
    for (port = 0; port < port_num; port++) {

        sff_obj = &(card->sff.obj[port]);
        if (SFF_FSM_ST_ISOLATED == sff_fsm_st_get(sff_obj)) {
            SWPS_LOG_ERR("transvr in %s isolated\n", sff_obj->name);
        }
    }    
    SWPS_LOG_ERR("bad transvr report == \n");
}
#endif
#define MAX_ACC_SIZE (1023)

int read_ufile(const char *path_t,
                       char *buf,
                                      size_t len)
{
    int err = 0;
    struct file *fp;
    mm_segment_t fs;
    loff_t pos;

    if (len >= MAX_ACC_SIZE) {
    len = MAX_ACC_SIZE - 1;
    }
    fp = filp_open(path_t, O_RDONLY, S_IRUGO);
    if (IS_ERR(fp)) {
    SWPS_LOG_ERR("open path%s fail\n", path_t);
    return -ENODEV;
    }
    fs = get_fs();
    set_fs(KERNEL_DS);
    pos = 0;
#if ( LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0) )
        err = vfs_read(fp, buf, len, &pos);
#else
        err = kernel_read(fp, buf, len, &pos);
#endif
    if (err < 0) {
    SWPS_LOG_ERR("%s: vfs_read failed.\n", __func__);
    }
    filp_close(fp, NULL);
    set_fs(fs);
    return err;
}

static int write_ufile(const char *path_t,
                               char *buf,
                                                      size_t len)
{
    int err = 0;
    struct file *fp;
    mm_segment_t fs;
    loff_t pos;

    if (len >= MAX_ACC_SIZE) {
    len = MAX_ACC_SIZE - 1;
    }
    fp = filp_open(path_t, O_WRONLY, S_IWUSR | S_IRUGO);

    if (IS_ERR(fp)) {
    SWPS_LOG_ERR("open path%s fail\n", path_t);
    return -ENODEV;
    }
    fs = get_fs();
    set_fs(KERNEL_DS);
    pos = 0;
#if ( LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0) )
        err = vfs_write(fp, buf, len, &pos);
#else
        err = kernel_write(fp, buf, len, &pos);
#endif
    if (err < 0) {
    SWPS_LOG_ERR("%s: vfs_write failed.\n", __func__);
    }
    filp_close(fp, NULL);
    set_fs(fs);
    return err;
}

int mux_is_fail(int i2c_ch, bool *is_fail)
{
    char path_t[100];
    int ret = 0;
    char reg[5];
    int st = 0;

    memset(path_t, 0, sizeof(path_t));
    memset(reg, 0, sizeof(reg));

    snprintf(path_t, sizeof(path_t), "/sys/bus/i2c/devices/i2c-%d/%d-0071/mux_fail",
    i2c_ch, i2c_ch);

    if ((ret = read_ufile(path_t, reg, sizeof(reg))) < 0) {
        
        return ret;
    }

    ret = sscanf_to_int(reg, &st);
    if(ret < 0) {
        return ret;
    }
    *is_fail = (st ? true:false);
    return 0;
}

int mux_last_ch_get(int i2c_ch, unsigned long *mux_ch)
{
    char path_t[100];
    int ret = 0;
    char reg[70];
    unsigned long st = 0;

    memset(path_t, 0, sizeof(path_t));
    memset(reg, 0, sizeof(reg));

    snprintf(path_t, sizeof(path_t), "/sys/bus/i2c/devices/i2c-%d/%d-0071/current_channel",
    i2c_ch, i2c_ch);

    if ((ret = read_ufile(path_t, reg, sizeof(reg))) < 0) {
        return ret;
    }

    ret = sscanf_to_long(reg, &st);
    if(ret < 0) {
        return ret;
    }
    *mux_ch = st;
    return 0;

}

static int mux_fail_reset(int i2c_ch)
{
    char path_t[100];
    int ret = 0;
    char block[5];
    int mux_is_fail = 0;

    memset(path_t, 0, sizeof(path_t));
    memset(block, 0, sizeof(block));

    snprintf(path_t, sizeof(path_t), "/sys/bus/i2c/devices/i2c-%d/%d-0071/mux_fail",
    i2c_ch, i2c_ch);

    snprintf(block, sizeof(block), "%d",
    mux_is_fail);

    ret = write_ufile(path_t, block, sizeof(block));
    if (ret < 0) {
        SWPS_LOG_ERR("mux_fail reset fail i2c_ch:%d\n", i2c_ch);
    }
    return ret;
}

static int mux_ch_block(int i2c_ch, unsigned long mux_ch)
{
    char path_t[100];
    int ret = 0;
    char block[70];

    memset(path_t, 0, sizeof(path_t));
    memset(block, 0, sizeof(block));

    snprintf(path_t, sizeof(path_t), "/sys/bus/i2c/devices/i2c-%d/%d-0071/block_channel",
    i2c_ch, i2c_ch);

    snprintf(block, sizeof(block), "%ld",
    mux_ch);

    ret = write_ufile(path_t, block, sizeof(block));
    if (ret < 0) {
        SWPS_LOG_ERR("mux_ch_block fail i2c_ch:%d mux_ch:%ld\n", i2c_ch, mux_ch);
    }
    return ret;
}

static int num_to_power_two(int num)
{
    int i = 0;
    int size = 8 * sizeof(num);
    for (i = 0; i < size; i++) {
        
        if (test_bit(i, (unsigned long *)&num)) {
            break;
        }
    }
    return i;
}    
static void failed_mux_detect(struct lc_obj_t *card)
{
    unsigned long mux_ch = 0;
    int port = 0;
    bool is_fail = false;
    struct mux_ch_t *mux = NULL;
    struct sff_obj_t *sff_obj = NULL;
    unsigned long block_all = 0;
    mux = &card->mux_l1;
             
    /*step 1 check if mux failed*/
    if (mux_is_fail(mux->i2c_ch, &is_fail) < 0) {
        
        SWPS_LOG_ERR("mux_ch check fail mux_ch:0x%lx \n", mux_ch);
        return;
    }
    if (is_fail) {
        /*step 2 block all mux ch*/
        block_all = (1L << (card->sff.valid_port_num)) - 1;
        if (mux_ch_block(mux->i2c_ch, block_all) < 0) {
            return;
        }
        SWPS_LOG_ERR("block_ch:0x%lx\n", block_all);
        mux->is_fail = true;
        if (mux_last_ch_get(mux->i2c_ch, &mux_ch) < 0) {
            return;
        } 
        mux->mux_ch = mux_ch;
        /*print out info */
        SWPS_LOG_ERR("mux_ch check OK mux_ch:0x%lx \n", mux_ch);
    }
    /*step 3 reset mux*/
    mux_reset_seq(card);
   /*step 4 block failed mux ch*/ 
    if (mux->is_fail && mux->mux_ch != 0) {
        mux->block_ch |= mux->mux_ch;
        if (mux->block_ch > block_all) {
            SWPS_LOG_ERR("block_ch:0x%lx out of range\n", mux->block_ch);
            return;
        } 
        if (mux_ch_block(mux->i2c_ch, mux->block_ch) < 0) {
            return;
        }
        /*following is for set sff_fsm_st to isolated*/
        port = num_to_power_two(mux->mux_ch);
        if (port < 0 && port > card->sff.valid_port_num) {
            SWPS_LOG_ERR("port out of range: %d\n", port);
            return;
        }
        sff_obj = &(card->sff.obj[port]);
        SWPS_LOG_ERR("block_ch:0x%lx %s\n", mux->block_ch, sff_obj->name);
        if (!p_valid(sff_obj)) {
            SWPS_LOG_ERR("NULL ptr port_id:%d\n", port);
            return;
        }
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_ISOLATED);
    } 
}
static bool i2c_bus_recovery_is_supported(void)
{
    bool is_supported = false;
    SWPS_LOG_DBG("i2c bus recovery is not supported\n");
    return is_supported;
}    
static void i2c_bus_recovery(struct lc_obj_t *card)
{
    if (!i2c_bus_recovery_is_supported()) {
        return;
    }
    failed_mux_detect(card);
    SWPS_LOG_ERR("mux reset done\n");
    if (i2c_bus_is_alive(card)) {
        SWPS_LOG_ERR("i2c bus recovery done\n");
    } else {
        SWPS_LOG_ERR("i2c bus recovery fail\n");
    }

}
static int lc_fsm_run_4U(struct lc_t *card)
{
    int i = 0;
    int lc_num = card->lc_num;
    int ret = 0;

    for (i = 0; i < lc_num; i++) {
        ret = _lc_fsm_run_4U(&card->obj[i]);

        if (ret < 0) {
            break;
        }
    }


    if (ret < 0) {
        SWPS_LOG_ERR("something wrong\n");
        return ret;
    }
    return 0;
}
/*extract the bits out of the val, bits need to be contious, bit start to end*/
u8 masked_bits_get(u8 val, int bit_s, int bit_e)
{
    u8 mask = 0;
    int bit = 0;

    for (bit = bit_s; bit <= bit_e; bit++) {
        mask |= (1 << bit);
    }
    val = val & (mask);
    val = val >> bit_s;

    return val;
}

int dummy_module_st_get(struct sff_obj_t *sff_obj, u8 *st)
{
    return -ENOSYS;
}

static void func_tbl_init(struct sff_obj_t *obj, int type)
{
    if(SFP_TYPE == type) {
        obj->func_tbl = sfp_func_load();
    } else if (QSFP_TYPE == type) {
        obj->func_tbl = qsfp_func_load();

    } else if (QSFP_DD_TYPE == type) {
        obj->func_tbl = qsfp_dd_func_load();
    } else {
        obj->func_tbl = qsfp_func_load();
        SWPS_LOG_ERR("load func fail!\n");
    }

}
static void sff_fsm_init(struct sff_obj_t *obj, int type)
{
    struct sff_fsm_t *fsm = NULL;

    fsm = &(obj->fsm);
    if(SFP_TYPE == type) {
        fsm->task = sff_fsm_sfp_task;

    } else if (QSFP_TYPE == type) {

        fsm->task = sff_fsm_qsfp_task;

    } else if (QSFP_DD_TYPE == type) {

        fsm->task = sff_fsm_qsfp_dd_task;

    }
    fsm->period_tbl = fsm_period_tbl;
    fsm->st = SFF_FSM_ST_IDLE;
    fsm->cnt = 0;
    fsm->delay_cnt = 0;
}

static struct platform_info_t *_platform_info_load(int platform_id)
{
    int i = 0;
    for (i = 0; platform_info_tbl[i].id != PLATFORM_END; i++) {
        if (platform_info_tbl[i].id == platform_id) {

            return &platform_info_tbl[i];
        }

    }

    return NULL;
}
static int port_info_table_load(void)
{
    int platform_id = PLATFORM_ID;
    struct platform_info_t *pltfm_info = NULL;

    pltfm_info = _platform_info_load(platform_id);
    if (!p_valid(pltfm_info)) {
        return -EBADRQC;
    }

    pltfmInfo = pltfm_info;

    if (NULL == pltfmInfo->tbl_1st) {
        return -EBADRQC;
    }

    if (NULL == pltfmInfo->name) {
        return -EBADRQC;
    }

    if (NULL != pltfmInfo->tbl_2nd) {
        maxPortNum = ((pltfmInfo->tbl_1st->size > pltfmInfo->tbl_2nd->size) ?
                      (pltfmInfo->tbl_1st->size) : (pltfmInfo->tbl_2nd->size));
    } else {
        maxPortNum = pltfmInfo->tbl_1st->size;
    }
    return 0;
}
inline static void sff_data_reset(struct sff_mgr_t *sff)
{
    sff->prs = 0;   
}
static void sff_data_init(struct sff_mgr_t *sff, int port_num)
{
    sff->valid_port_num = port_num;
    sff_data_reset(sff);
}
static void sff_func_init(struct sff_mgr_t *sff)
{
    if (io_no_init) {
        sff->prs_scan = io_no_init_scan;
    } else {
        sff->prs_scan = sff_prs_scan;
    }
}
static int drv_load(struct lc_t *self)
{
    int lc_num = self->lc_num;
    int lc_id = 0;
    struct sff_io_driver_t *io_drv = NULL;
    struct sff_eeprom_driver_t *eeprom_drv = NULL;
    io_drv = sff_io_drv_get_iodev();
       
    if (!p_valid(io_drv)) {
        return -ENOSYS;
    }

    eeprom_drv = sff_eeprom_drv_get();
    if (!p_valid(eeprom_drv)) {
        return -ENOSYS;
    }
    
    for (lc_id = 0; lc_id < lc_num; lc_id++) {
        self->obj[lc_id].sff.io_drv = io_drv;
        self->obj[lc_id].sff.eeprom_drv = eeprom_drv;
    }  
    return 0;
}
static int __init swps_init(void)
{
    if (port_info_table_load() < 0) {

        SWPS_LOG_ERR("port_info_table_load fail\n");
        goto exit_err;
    }
    if (sff_eeprom_init(pltfmInfo->id) < 0) {
        goto exit_err;
        SWPS_LOG_ERR("sff_eeprom_init fail\n");
    }
    sff_eeprom_port_num_set(maxPortNum);

    if (sff_kset_create_init() < 0) {
        SWPS_LOG_ERR("sff_kset_create_init fail\n");
        goto exit_err;
    }
    
    if (lc_func_load(&lcMgr) < 0) {
        SWPS_LOG_ERR("lc_func_load fail\n");
        goto exit_err;
    }
    
    if (lc_create_init(&lcMgr, pltfmInfo->lc_num, maxPortNum) < 0) {
        SWPS_LOG_ERR("lc_create_init fail\n");
        goto exit_err;
    }
    
    if (lcMgr.lc_func->dev_init(pltfmInfo->id, io_no_init) < 0) {
        goto exit_err;
        SWPS_LOG_ERR("dev_init fail\n");
    }
    
    if (drv_load(&lcMgr) < 0) {
        goto exit_err;
        SWPS_LOG_ERR("drv load fail\n");
    }
    
    if(swps_polling_is_enabled()) {
        swps_polling_task_start();
    }
    SWPS_LOG_INFO("swps:%s  init ok\n", pltfmInfo->name);
    return 0;

exit_err:
    return -EBADRQC;
}

static void __exit swps_exit(void)
{
    struct mux_ch_t *mux = NULL;
    int lc_id = 0;
    
    if(swps_polling_is_enabled()) {
        swps_polling_task_stop();
    }
     
    sff_eeprom_deinit();
    lcMgr.lc_func->dev_deinit();

    lc_sff_kobjs_destroy(&lcMgr);
    lc_kobj_destroy(&lcMgr);
    lc_common_kobj_destroy(&lcMgr);
    sff_kset_deinit();

    for (lc_id = 0; lc_id < lcMgr.lc_num; lc_id++) { 
        mux = &(lcMgr.obj[lc_id].mux_l1);
        if (0 != mux->block_ch) {
            if (mux_ch_block(mux->i2c_ch, 0) < 0) {
                SWPS_LOG_ERR("block ch fail:i2c_ch:%d\n", mux->i2c_ch);
            }
            mux_fail_reset(mux->i2c_ch);
        }
    }
    lc_sff_frontPort_remap_destroy(&lcMgr);
    lc_sff_objs_destroy(&lcMgr);
    lc_objs_destroy(&lcMgr);

    SWPS_LOG_INFO("swps:%s  deinit ok\n", pltfmInfo->name);
}


module_init(swps_init);
module_exit(swps_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Netberg Ltd <support@netbergtw.com>");
