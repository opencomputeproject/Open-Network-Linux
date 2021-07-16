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
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include "inv_def.h"
#include "inv_swps.h"
#include "pltfm_info.h"
#include "module/module_api.h"

#define TRY_NUM (5)
#define MUX_CH_NUM  (8)
#define PAGE_SEL_LOCK_NUM (20)
#define CLEAR_CMD ("clear\n")
#define AUTO_TX_EQ_STR ("auto")
#define MANUAL_TX_EQ_STR ("manual")
#define HELP_CMD ("help\n")

int io_no_init = 0;
module_param(io_no_init, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
#if (RELEASE_TYPE == OFFICIAL_RELEASE)
u32 logLevel = SWPS_ERR_LEV | ERR_ALL_LEV;
#else
//u32 logLevel = SWPS_ERR_LEV | SWPS_INFO_LEV | SWPS_DBG_LEV | ERR_ALL_LEV | MODULE_DBG_LEV;
u32 logLevel = ERR_ALL_LEV | INFO_ALL_LEV | DBG_ALL_LEV;
#endif

bool int_flag_monitor_en = false;
char workBuf[PAGE_SIZE];
struct platform_info_t *pltfmInfo = NULL;
static void swps_polling_task(struct work_struct *work);
static void polling_task_1U(void);
static void polling_task_4U(void);
static DECLARE_DELAYED_WORK(swps_polling, swps_polling_task);
static bool swps_polling_en = true;
static int swps_polling_task_start(void);
static int swps_polling_task_stop(void);
static int sff_kset_create_init(void);
static int sff_kset_deinit(void);
static bool i2c_bus_recovery_is_supported(void);
static void sff_super_fsm_st_set(struct sff_obj_t *sff_obj, sff_super_fsm_st_t st);
static sff_super_fsm_st_t sff_super_fsm_st_get(struct sff_obj_t *sff_obj);
static void sff_super_fsm_st_chg_process(struct sff_obj_t *sff_obj,
        sff_super_fsm_st_t cur_st,
        sff_super_fsm_st_t next_st);
static int sff_all_io_get(sff_io_type_t type, struct sff_mgr_t *sff, unsigned long *bitmap);
static bool sff_io_supported(struct sff_obj_t *sff_obj, sff_io_type_t type);
static int sff_power_get(struct sff_obj_t *sff_obj, u8 *val);
/*<TBD> only need this for gpio i2c*/
static int i2cbus_alive_check_cnt = 0;
#define I2CBUS_ALIVE_CHECK_NUM (1)

static struct swps_kobj_t *swps_kobj_add(char *name,
        struct kobject *mgr,
        struct attribute_group *attr_group);
/*dummy functions*/
int dummy_module_st_get(struct sff_obj_t *sff_obj, u8 *st)
{
    return -ENOSYS;
}
int dummy_intr_get(struct sff_obj_t *sff_obj, u8 *value)
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
int dummy_modsel_set(struct sff_obj_t *sff_obj, u8 value)
{

    return -ENOSYS;

}
int dummy_modsel_get(struct sff_obj_t *sff_obj, u8 *value)
{

    return -ENOSYS;

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
enum {
    LC_EJ_SKIP_FLAG = 0,
    LC_EJ_IS_LOCKED_FLAG = 1,
    LC_EJ_IS_UNLOCKED_FLAG = 2,
};

static const char *sff_type_str[SFF_TYPE_NUM] = {
    [SFF_UNKNOWN_TYPE] = "UNKNOWN",
    [SFP_TYPE] = "SFP",
    [SFP_DD_TYPE] = "SFP_DD",
    [QSFP_TYPE] = "QSFP",
    [QSFP56_TYPE] = "QSFP56",
    [QSFP_DD_TYPE] = "QSFP_DD"
};

static const bool sff_avaliable_io_tbl[SFF_TYPE_NUM][SFF_IO_TYPE_NUM] = {
    [SFP_TYPE] = {
        [SFF_IO_PRS_TYPE] = true,
        [SFF_IO_RST_TYPE] = false,
        [SFF_IO_LPMODE_TYPE] = false,
        [SFF_IO_INTR_TYPE] = false,
        [SFF_IO_MODSEL_TYPE] = false,
        [SFF_IO_TXDISABLE_TYPE] = true,
        [SFF_IO_RXLOS_TYPE] = true,
        [SFF_IO_TXFAULT_TYPE] = true,
        [SFF_IO_TXDISABLE2_TYPE] = false,
        [SFF_IO_RXLOS2_TYPE] = false,
        [SFF_IO_TXFAULT2_TYPE] = false
    },
    [SFP_DD_TYPE] = {
        [SFF_IO_PRS_TYPE] = true,
        [SFF_IO_RST_TYPE] = false,
        [SFF_IO_LPMODE_TYPE] = true,
        [SFF_IO_INTR_TYPE] = false,
        [SFF_IO_MODSEL_TYPE] = false,
        [SFF_IO_TXDISABLE_TYPE] = true,
        [SFF_IO_RXLOS_TYPE] = true,
        [SFF_IO_TXFAULT_TYPE] = true,
        [SFF_IO_TXDISABLE2_TYPE] = true,
        [SFF_IO_RXLOS2_TYPE] = true,
        [SFF_IO_TXFAULT2_TYPE] = true
    },
    [QSFP_TYPE] = {
        [SFF_IO_PRS_TYPE] = true,
        [SFF_IO_RST_TYPE] = true,
        [SFF_IO_LPMODE_TYPE] = true,
        [SFF_IO_INTR_TYPE] = true,
        [SFF_IO_MODSEL_TYPE] = true,
        [SFF_IO_TXDISABLE_TYPE] = false,
        [SFF_IO_RXLOS_TYPE] = false,
        [SFF_IO_TXFAULT_TYPE] = false,
        [SFF_IO_TXDISABLE2_TYPE] = false,
        [SFF_IO_RXLOS2_TYPE] = false,
        [SFF_IO_TXFAULT2_TYPE] = false
    },
    [QSFP56_TYPE] = {
        [SFF_IO_PRS_TYPE] = true,
        [SFF_IO_RST_TYPE] = true,
        [SFF_IO_LPMODE_TYPE] = true,
        [SFF_IO_INTR_TYPE] = true,
        [SFF_IO_MODSEL_TYPE] = true,
        [SFF_IO_TXDISABLE_TYPE] = false,
        [SFF_IO_RXLOS_TYPE] = false,
        [SFF_IO_TXFAULT_TYPE] = false,
        [SFF_IO_TXDISABLE2_TYPE] = false,
        [SFF_IO_RXLOS2_TYPE] = false,
        [SFF_IO_TXFAULT2_TYPE] = false
    },
    [QSFP_DD_TYPE] = {
        [SFF_IO_PRS_TYPE] = true,
        [SFF_IO_RST_TYPE] = true,
        [SFF_IO_LPMODE_TYPE] = true,
        [SFF_IO_INTR_TYPE] = true,
        [SFF_IO_MODSEL_TYPE] = true,
        [SFF_IO_TXDISABLE_TYPE] = false,
        [SFF_IO_RXLOS_TYPE] = false,
        [SFF_IO_TXFAULT_TYPE] = false,
        [SFF_IO_TXDISABLE2_TYPE] = false,
        [SFF_IO_RXLOS2_TYPE] = false,
        [SFF_IO_TXFAULT2_TYPE] = false
    }
};

static struct func_tbl_t *sff_func_tbl_map[SFF_TYPE_NUM] = {
    [SFP_TYPE] = &sfp_func_tbl,
    [QSFP_TYPE] = &qsfp_func_tbl,
    [QSFP_DD_TYPE] = &qsfp_dd_func_tbl,
    [QSFP56_TYPE] = &qsfp_dd_func_tbl,
    [SFP_DD_TYPE] = &sfp_dd_func_tbl,
    [SFF_UNKNOWN_TYPE] = NULL,
};

static  int (*sff_fsm_task_map[SFF_TYPE_NUM])(sff_obj_type *sff_obj) = {

    [SFP_TYPE] = sff_fsm_sfp_task,
    [QSFP_TYPE] = sff_fsm_qsfp_task,
    [QSFP_DD_TYPE] = sff_fsm_qsfp_dd_task,
    [QSFP56_TYPE] = sff_fsm_qsfp_dd_task,
    [SFP_DD_TYPE] = sff_fsm_sfp_dd_task,
    [SFF_UNKNOWN_TYPE] = NULL,
};

/*check if pointer is valid
 *      true: not NULL false: NULL*/
bool inline p_valid(const void *ptr)
{
    if (likely(NULL != ptr)) {
        return true;
    } else {
        return false;
    }
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
void inv_swps_polling_set(bool en)
{
    if (en == swps_polling_en) {
        return;
    }
    if (en) {
        swps_polling_task_start();
    } else {
        swps_polling_task_stop();
    } 
    swps_polling_en = en;
    SWPS_LOG_DBG("en:%d done\n", en);
}
EXPORT_SYMBOL(inv_swps_polling_set);


static inline bool inv_swps_polling_is_enabled(void)
{
    return swps_polling_en;
}

struct fsm_period_t fsm_period_tbl[] = {
    {SFF_FSM_ST_REMOVED, 0, false},
    {SFF_FSM_ST_DETECTED, 3, true},
    {SFF_FSM_ST_INIT, 1, true},
    {SFF_FSM_ST_READY, 5, false},
    {SFF_FSM_ST_IDLE, 20, false},
    {SFF_FSM_ST_SUSPEND, 1, false},
    {SFF_FSM_ST_RESTART, 1, true},
    {SFF_FSM_ST_IDENTIFY, 1, true},
    {SFF_FSM_ST_DATA_READY_CHECK, 1, true},
    {SFF_FSM_ST_TIMEOUT, 30, false},
    {SFF_FSM_ST_ISOLATED, 3, false},
    /*qsfp-dd only {*/
    {SFF_FSM_ST_MGMT_INIT, 3, true},
    {SFF_FSM_ST_MODULE_LOOPBACK_INIT, 3, true},
    {SFF_FSM_ST_MODULE_READY, 3, false},
    {SFF_FSM_ST_MODULE_SW_CONFIG_1, 1, true},
    {SFF_FSM_ST_MODULE_SW_CONFIG_1_WAIT, 50, true}, /*sfp-dd only*/
    {SFF_FSM_ST_MODULE_SW_CONFIG_2, 1, true},
    {SFF_FSM_ST_MODULE_SW_CONFIG_CHECK, 1, true},
    {SFF_FSM_ST_MODULE_SW_CONTROL, 1, true},
    {SFF_FSM_ST_MODULE_READY_CHECK, 1, true},
    /*qsfp-dd only }*/
    {SFF_FSM_ST_UNKNOWN_TYPE, 30, false},
    {SFF_FSM_ST_END, 0xff, false}, /*keep it at the end of table*/
};

const char *sff_fsm_st_str[SFF_FSM_ST_NUM] = {
    [SFF_FSM_ST_REMOVED] = "SFF_FSM_ST_REMOVED",
    [SFF_FSM_ST_DETECTED] = "SFF_FSM_ST_DETECTED",
    [SFF_FSM_ST_INIT] = "SFF_FSM_ST_INIT",
    [SFF_FSM_ST_READY] = "SFF_FSM_ST_READY",
    [SFF_FSM_ST_IDLE] = "SFF_FSM_ST_IDLE",
    [SFF_FSM_ST_SUSPEND] = "SFF_FSM_ST_SUSPEND",
    [SFF_FSM_ST_RESTART] = "SFF_FSM_ST_RESTART",
    [SFF_FSM_ST_ISOLATED] = "SFF_FSM_ST_ISOLATED",
    [SFF_FSM_ST_IDENTIFY] = "SFF_FSM_ST_IDENTIFY",
    [SFF_FSM_ST_DATA_READY_CHECK] = "SFF_FSM_ST_DATA_READY_CHECK",
    [SFF_FSM_ST_TIMEOUT] = "SFF_FSM_ST_TIMEOUT",
    /*qsfp-dd only {*/
    [SFF_FSM_ST_MGMT_INIT] = "SFF_FSM_ST_MGMT_INIT",
    [SFF_FSM_ST_MODULE_LOOPBACK_INIT] = "SFF_FSM_ST_MODULE_LOOPBACK_INIT",
    [SFF_FSM_ST_MODULE_READY] = "SFF_FSM_ST_MODULE_READY",
    [SFF_FSM_ST_MODULE_SW_CONFIG_1] = "SFF_FSM_ST_MODULE_SW_CONFIG_1",
    [SFF_FSM_ST_MODULE_SW_CONFIG_1_WAIT] = "SFF_FSM_ST_MODULE_SW_CONFIG_1_WAIT",
    [SFF_FSM_ST_MODULE_SW_CONFIG_2] = "SFF_FSM_ST_MODULE_SW_CONFIG_2",
    [SFF_FSM_ST_MODULE_SW_CONFIG_CHECK] = "SFF_FSM_ST_MODULE_SW_CONFIG_CHECK",
    [SFF_FSM_ST_MODULE_SW_CONTROL] = "SFF_FSM_ST_MODULE_SW_CONTROL",
    [SFF_FSM_ST_MODULE_READY_CHECK] = "SFF_FSM_ST_MODULE_READY_CHECK",
    /*qsfp-dd only }*/
    [SFF_FSM_ST_UNKNOWN_TYPE] = "SFF_FSM_ST_UNKNOWN_TYPE",
    [SFF_FSM_ST_END] = "SFF_FSM_ST_END", /*keep it at the bottom*/
};
const char *sff_super_fsm_st_str[SFF_SUPER_FSM_ST_NUM] = {

    [SFF_SUPER_FSM_ST_INSERTED] = "SFF_SUPER_FSM_ST_INSERTED",
    [SFF_SUPER_FSM_ST_WAIT_STABLE] = "SFF_SUPER_FSM_ST_WAIT_STABLE",
    [SFF_SUPER_FSM_ST_MODULE_DETECT] ="SFF_SUPER_FSM_ST_MODULE_DETECT",
    [SFF_SUPER_FSM_ST_RUN] = "SFF_SUPER_FSM_ST_RUN",
    [SFF_SUPER_FSM_ST_RESTART] = "SFF_SUPER_FSM_ST_RESTART",
    [SFF_SUPER_FSM_ST_SUSPEND] = "SFF_SUPER_FSM_ST_SUSPEND",
    [SFF_SUPER_FSM_ST_REMOVED] = "SFF_SUPER_FSM_ST_REMOVED",
    [SFF_SUPER_FSM_ST_ISOLATED] = "SFF_SUPER_FSM_ST_ISOLATED",
    [SFF_SUPER_FSM_ST_IDLE] = "SFF_SUPER_FSM_ST_IDLE",
    [SFF_SUPER_FSM_ST_UNSUPPORT] = "SFF_SUPER_FSM_ST_UNSUPPORT",
    [SFF_SUPER_FSM_ST_IO_NOINIT] = "SFF_SUPER_FSM_ST_IO_NOINIT",
    [SFF_SUPER_FSM_ST_TIMEOUT] = "SFF_SUPER_FSM_ST_TIMEOUT"
};

/*struct sff_obj_t;*/

struct kset *swps_kset = NULL;


char *lc_fsm_st_str[LC_FSM_ST_NUM] = {
    [LC_FSM_ST_INSERT] = "LC_FSM_ST_INSERT",
    [LC_FSM_ST_SW_CONFIG] = "LC_FSM_ST_SW_CONFIG",
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

int maxPortNum = 0;
struct swps_mgr_t swps_mgr;
#define CARD_NAME_PREFIX ("card")
#define CARD_NO_BASE (1)
#define SYSFS_COMMON_DIR ("common")
static int sff_kobj_add(struct sff_obj_t *sff_obj);
static int sff_fsm_init(struct sff_obj_t *obj, int type);
static void sff_data_init(struct sff_mgr_t *sff, int port_num);
inline static void sff_data_reset(struct sff_mgr_t *sff);
static void sff_func_init(struct sff_mgr_t *sff);
static int func_tbl_init(struct sff_obj_t *obj, int type);
static void io_no_init_port_done_set(struct sff_obj_t *sff_obj);
static int io_no_init_done_fsm_run(struct sff_obj_t *sff_obj);
static void io_no_init_handler_by_card(struct lc_obj_t *card);
static void io_no_init_handler(struct swps_mgr_t *self);
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
                            SFF_SUPER_FSM_ST_ISOLATED != sff_super_fsm_st_get(sff_obj)) {
                        ch_1st = ch;
                    } else {

                        //SWPS_LOG_DBG("ch:%d test ch_1st:%d ch_2nd:%d\n", ch, ch_1st, ch_2nd);
                        if(SFF_SUPER_FSM_ST_ISOLATED != sff_super_fsm_st_get(sff_obj)) {
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
                    sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_ISOLATED);
                    SWPS_LOG_ERR("transvr in %s is bad\n", sff_obj->name);
                    mux_reset_seq(card);
                    break;
                }

                sff_obj = &(card->sff.obj[ch_2nd]);
                mux_reset_ch_resel(lc_id, ch_2nd);
                if (!i2c_bus_is_alive(card)) {
                    mux_reset_seq(card);
                    sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_ISOLATED);
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
static int lc_prs_scan(struct swps_mgr_t *self)
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
static int lc_prs_scan(struct swps_mgr_t *self)
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
    check_pfunc(lc_func->ej_r_get);
    check_pfunc(lc_func->ej_l_get);

    if ((ret = lc_func->prs_get(&prs)) < 0) {
        return ret;
    }
    if ((ret = lc_func->ej_r_get(&ej_r)) < 0) {
        return ret;
    }
    if ((ret = lc_func->ej_l_get(&ej_l)) < 0) {
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
static int swps_fsm_run_4u(struct swps_mgr_t *self);
static int swps_fsm_run_1u(struct swps_mgr_t *self);
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
        SWPS_LOG_DBG("fail:offset:0x%x try %d/%d! Error Code: %d\n", offset, i, I2C_RETRY_NUM, ret);
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
static int inv_i2c_smbus_read_i2c_block_data(struct i2c_client *client, u8 offset, int len, u8 *buf)
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

static int inv_i2c_smbus_write_i2c_block_data(struct i2c_client *client, u8 offset, int len, const u8 *buf)
{
    int ret = 0;
    int i = 0;
    int cnt = len;
    int block_size = 0;

    while (i < len) {
        block_size = ((cnt > I2C_SMBUS_BLOCK_MAX) ? I2C_SMBUS_BLOCK_MAX : cnt);
        ret = i2c_smbus_write_i2c_block_data(client, offset+i, block_size, buf+i);

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

int i2c_smbus_read_i2c_block_data_retry(struct i2c_client *client, u8 offset, int len, u8 *buf)
{
    int ret = 0;
    int i = 0;

    for (i = 0; i < I2C_RETRY_NUM; i++) {
        ret = inv_i2c_smbus_read_i2c_block_data(client, offset, len, buf);
        if (ret < 0) {
            msleep(I2C_RETRY_DELAY_MS);
            continue;
        }
        break;
    }

    if (i >= I2C_RETRY_NUM) {
        ;
        //SWPS_LOG_ERR("fail:offset:0x%x try %d/%d! Error Code: %d\n", offset, i, I2C_RETRY_NUM, ret);
    }
    return ret;
}
int i2c_smbus_write_i2c_block_data_retry(struct i2c_client *client, u8 offset, int len, const u8 *buf)
{
    int ret = 0;
    int i = 0;

    for (i = 0; i < I2C_RETRY_NUM; i++) {
        ret = inv_i2c_smbus_write_i2c_block_data(client, offset, len, buf);
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
/*fsm functions declaration*/

static void sff_fsm_delay_cnt_reset(struct sff_obj_t *sff_obj, sff_fsm_state_t st);
static void sff_fsm_cnt_run(struct sff_obj_t *sff_obj);
static bool sff_fsm_delay_cnt_is_hit(struct sff_obj_t *sff_obj);
static int sff_super_fsm_run(struct sff_mgr_t *sff);

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
#if 0
time_t get_system_time(void)
{
    time_t current_times;
    time(&current_times);
    return current_times;
}

void page_sel_lock_time_set(struct sff_obj_t *sff_obj)
{
    sff_obj->page_sel_lock_time = get_system_time();
}

void page_sel_lock_time_clear(struct sff_obj_t *sff_obj)
{
    sff_obj->page_sel_lock_time = 0;
}
#endif
static inline unsigned long sff_prs_cache_get(struct sff_mgr_t *sff)
{
    return sff->prs;
}
static inline void sff_prs_cache_update(struct sff_mgr_t *sff, unsigned long bitmap)
{
    sff->prs = bitmap;
}
static int sff_all_prs_get(struct sff_mgr_t *sff, unsigned long *prs)
{
    return sff_all_io_get(SFF_IO_PRS_TYPE, sff, prs);
}

static bool sff_io_supported(struct sff_obj_t *sff_obj, sff_io_type_t type)
{
    return sff_avaliable_io_tbl[sff_obj->type][type];
}

static bool sff_reset_is_supported(struct sff_obj_t *sff_obj)
{
    return sff_io_supported(sff_obj, SFF_IO_RST_TYPE);
}

static bool is_pltfm_4U_type(void)
{
    return ((PLATFORM_4U == pltfmInfo->id) ? true : false);
}
/*the function check if customized mux driver is supported*/
static bool i2c_recovery_is_muxdrv_method(void)
{
     if (pltfmInfo->i2c_recovery_feature.en && 
         MUX_DRV_METHOD == pltfmInfo->i2c_recovery_feature.method) {
        return true;
     } else {
        return false;
     }
}    

static bool sff_power_is_supported(void)
{
    return pltfmInfo->sff_power_supported;
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
        sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_SUSPEND);
        if (cnt > 0) {
            SWPS_LOG_INFO("page_sel_lock occured\n");
        }
    } else {
        sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_RESTART);
    }
    if((ret = sff_reset_set(sff_obj, rst)) < 0) {
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

static void sff_super_fsm_op_process(struct sff_mgr_t *sff,
                               bool (*is_supported)(struct sff_obj_t *sff_obj),
                               unsigned long new_st,
                               unsigned long old_st)
{
    int port = 0;
    int port_num = sff->valid_port_num;
    struct sff_obj_t *sff_obj = NULL;
    unsigned long st_chg = 0;
    
    if (!p_valid(sff) && !p_valid(is_supported)) {
        SWPS_LOG_ERR("null input\n");
        return;
    }
    port_num = sff->valid_port_num;
    st_chg = new_st ^ old_st;
    if (0 != st_chg) {
        SWPS_LOG_DBG("st_chg:0x%lx\n", st_chg);
    }
    for (port = 0; port < port_num; port++) {

        sff_obj = &sff->obj[port];
        if (!is_supported(sff_obj)) {
            continue;
        }
        transvr_type_set(sff_obj, TRANSVR_CLASS_UNKNOWN);

        if (test_bit(port, &st_chg)) {
            if (test_bit(port, &new_st)) {
                sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_RESTART);
            } else {
                sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_SUSPEND);
                SWPS_LOG_DBG("%s go to suspend\n", sff_obj->name);
            }
        }
    }
}

static int sff_all_io_get(sff_io_type_t type, struct sff_mgr_t *sff, unsigned long *bitmap)
{
    int ret = 0;
    int (*get_func)(int lc_id, unsigned long *bitmap) = NULL;
    unsigned long temp = 0; 
    bool is_logic_inverted = false;    
    struct lc_obj_t *card = NULL;
    
    if (!p_valid(sff)) {
        return -EINVAL;
    }
    card = sff_to_lc(sff);    
    switch (type) {
    case SFF_IO_PRS_TYPE:
        get_func = sff->io_drv->prs.get;
        is_logic_inverted = true;
        break;
    case SFF_IO_RST_TYPE:
        get_func = sff->io_drv->reset.get;
        break;
    case SFF_IO_LPMODE_TYPE:
        get_func = sff->io_drv->lpmode.get;
        break;
    case SFF_IO_MODSEL_TYPE:
        get_func = sff->io_drv->modsel.get;
        break;
    case SFF_IO_INTR_TYPE:
        get_func = sff->io_drv->intr.get;
        break;
    case SFF_IO_TXDISABLE_TYPE:
        get_func = sff->io_drv->txdisable.get;
    default:
        break;
    }
    check_pfunc(get_func);
    if ((ret = get_func(card->lc_id, &temp)) < 0) {
        return ret;
    }
    if (is_logic_inverted) {
        temp = ~temp;
    }
    *bitmap = temp;
    return 0;
}

int sff_all_io_set(sff_io_type_t type, struct sff_mgr_t *sff, unsigned long bitmap)
{
    int ret = 0;
    int (*set_func)(int lc_id, unsigned long bitmap) = NULL;
    struct lc_obj_t *card = NULL;

    if (!p_valid(sff)) {
        return -EINVAL;
    }
    card = sff_to_lc(sff);    
    switch (type) {

    case SFF_IO_RST_TYPE:
        set_func = sff->io_drv->reset.set;
        break;
    case SFF_IO_LPMODE_TYPE:
        set_func = sff->io_drv->lpmode.set;
        break;
    case SFF_IO_MODSEL_TYPE:
        set_func = sff->io_drv->modsel.set;
        break;
    case SFF_IO_TXDISABLE_TYPE:
        set_func = sff->io_drv->txdisable.set;    
    default:
        break;
    }
    check_pfunc(set_func);
    if ((ret = set_func(card->lc_id, bitmap)) < 0) {
        return ret;
    }
    return 0;
}
/*(in) bitmap , is in front port order*/
int sff_all_io_set_frontport(sff_io_type_t type, struct sff_mgr_t *sff, unsigned long front_bitmap)
{
    unsigned long phy_bitmap = 0;
    unsigned long cur_bitmap = 0;
    int ret = 0;

    if (!p_valid(sff)) {
        return -EINVAL;
    }
    phy_bitmap = frontPort_to_phyPort(sff, front_bitmap);

    if ((ret = sff_all_io_get(type, sff, &cur_bitmap)) < 0) {
        return ret;
    }
    if ((ret = sff_all_io_set(type, sff, phy_bitmap)) < 0) {
        return ret;
    }
    if (SFF_IO_RST_TYPE == type) {
        sff_super_fsm_op_process(sff, sff_reset_is_supported, phy_bitmap, cur_bitmap);
    }
    return 0;
}
/*output bitmap , is in front port order*/
int sff_all_io_get_frontport(sff_io_type_t type, struct sff_mgr_t *sff, unsigned long *front_bitmap)
{
    unsigned long phy_bitmap = 0;
    int ret = 0;

    if (!p_valid(sff)) {
        return -EINVAL;
    }
    if ((ret = sff_all_io_get(type, sff, &phy_bitmap)) < 0) {
        return ret;
    }
    
    *front_bitmap = phyPort_to_frontPort(sff, phy_bitmap);
    return 0;
}

static int sff_all_power_get(struct sff_mgr_t *sff, unsigned long *bitmap)
{
    int ret = 0;
    unsigned long temp = 0;
    struct lc_obj_t *lc = sff_to_lc(sff);
    struct pltfm_func_t *pltfm_func = lc->swps->pltfm_func;    
    
    check_pfunc(pltfm_func->sff_power.get);
    if ((ret = pltfm_func->sff_power.get(lc->lc_id, &temp)) < 0) {
        return ret;
    }
    *bitmap = temp;
    return 0;
}

static int sff_all_power_set(struct sff_mgr_t *sff, unsigned long bitmap)
{
    int ret = 0;
    struct lc_obj_t *lc = sff_to_lc(sff);
    struct pltfm_func_t *pltfm_func = lc->swps->pltfm_func;    

    check_pfunc(pltfm_func->sff_power.set);
    if ((ret = pltfm_func->sff_power.set(lc->lc_id, bitmap)) < 0) {
        return ret;
    }
    return 0;
}

int sff_all_power_set_frontport(struct sff_mgr_t *sff, unsigned long bitmap)
{
    int ret = 0;
    int port = 0;
    unsigned long phy_bitmap = 0;
    unsigned long cur_power = 0;
    unsigned long power_chg = 0;
    struct sff_obj_t *sff_obj = NULL;
    
    if (!sff_power_is_supported()) {
        return -ENOSYS;
    }
    if ((ret = sff_all_power_get(sff, &cur_power)) < 0) {
        return ret;
    }
    phy_bitmap = frontPort_to_phyPort(sff, bitmap);
    
    if ((ret = sff_all_power_set(sff, phy_bitmap)) < 0) {
        return ret;
    }
    power_chg = cur_power ^ phy_bitmap;
    SWPS_LOG_DBG("power_chg:0x%lx\n", power_chg);
    
    for (port = 0; port < sff->valid_port_num; port++) {

        sff_obj = &sff->obj[port];
        transvr_type_set(sff_obj, TRANSVR_CLASS_UNKNOWN);
        if (test_bit(port, &power_chg)) {
            if (!test_bit(port, &phy_bitmap)) {
                sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_SUSPEND);
            } else {
                sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_RESTART);
            }
        }
    }
    
    return 0;
}
/*common functions*/

void cnt_increment_limit(u32 *data)
{
    if (*data < U32_MAX) {
        (*data)++;
    }
}

int sff_eeprom_read(struct sff_obj_t *sff_obj,
                    u8 addr,
                    u8 offset,
                    u8 *buf,
                    int len)
{
    int ret = 0;
    if (addr != SFF_EEPROM_I2C_ADDR &&
            addr != SFF_DDM_I2C_ADDR) {
        SWPS_LOG_ERR("addr out of range:0x%x\n", addr);
        return -EINVAL;
    }
    check_pfunc(sff_obj->sff->eeprom_drv->read);
    
    if (pltfmInfo->fpga_i2c_supported) {
        if ((ret = sff_obj->sff->eeprom_drv->read(sff_obj->lc_id, sff_obj->port, addr, offset, buf, len)) < 0) {
            sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_ISOLATED);
            return ret;            
        }
        return 0;
    } else {    
        return sff_obj->sff->eeprom_drv->read(sff_obj->lc_id, sff_obj->port, addr, offset, buf, len);
    }
}

int sff_eeprom_write(struct sff_obj_t *sff_obj,
                     u8 addr,
                     u8 offset,
                     const u8 *buf,
                     int len)
{
    if (addr != SFF_EEPROM_I2C_ADDR &&
            addr != SFF_DDM_I2C_ADDR) {
        SWPS_LOG_ERR("addr out of range:0x%x\n", addr);
        return -EINVAL;
    }
    check_pfunc(sff_obj->sff->eeprom_drv->write);
    return sff_obj->sff->eeprom_drv->write(sff_obj->lc_id, sff_obj->port, addr, offset, buf, len);
}

static int sff_page_get(struct sff_obj_t *sff_obj, u8 *page)
{
    int ret = 0;
    u8 addr = SFF_EEPROM_I2C_ADDR;
    u8 offset = SFF_PAGE_SEL_OFFSET;
    u8 reg = 0;

    if ((ret = sff_eeprom_read(sff_obj, addr, offset, &reg, sizeof(reg))) < 0) {
        SWPS_LOG_ERR("fail\n");
        return ret;
    }
    *page = reg;
    return 0;
}

static int sff_page_sel(struct sff_obj_t *sff_obj, u8 page)
{
    int ret = 0;
    u8 addr = SFF_EEPROM_I2C_ADDR;
    u8 offset = SFF_PAGE_SEL_OFFSET;
    bool supported = false;

    if (page >= PAGE_NUM) {
        return -EINVAL;
    }

    check_pfunc(sff_obj->func_tbl->paging_supported);
    if((ret = sff_obj->func_tbl->paging_supported(sff_obj, &supported)) < 0) {
        return ret;
    }

    if (!supported &&
            0 != page) {
        SWPS_LOG_ERR("%s paging not supported,set page:%d fail\n", sff_obj->name, page);
        return -ENOSYS;
    }
    ret = sff_eeprom_write(sff_obj, addr, offset, &page, sizeof(page));

    if (ret < 0) {
        SWPS_LOG_ERR("switch page fail\n");
        return ret;
    }
    return 0;
}

static int sff_paged_eeprom_dump(struct sff_obj_t *sff_obj, u8 *buf, int buf_size)
{
    int ret = 0;
    u8 page = 0;
    int i = 0;
    int j = 0;
    int cnt = 0;
    u8 offset = 0;
    u8 data[EEPROM_SIZE];
    int size = sizeof(data);

    if ((ret = sff_page_get(sff_obj, &page)) < 0) {
        return ret;
    }

    cnt += scnprintf(buf+cnt, buf_size - cnt, "page%d\n", page);
    memset(data, 0, size);

    if (0 == page) {
        offset = 0;
    } else {
        offset = 128;
    }

    if ((ret = sff_paged_eeprom_read(sff_obj, page, offset, data+offset, size-offset)) < 0) {
        MODULE_LOG_ERR("ret:%d read fail\n", ret);
        return ret;
    }
    /*print out offset*/
    cnt += scnprintf(buf+cnt, buf_size-cnt, "    ");
    for (i = 0; i < 16; i++) {
        cnt += scnprintf(buf+cnt, buf_size-cnt, "%01x  ", i);
    }
    cnt += scnprintf(buf+cnt, buf_size-cnt, "\n");

    for (i = offset, j = 0; i < size; i++) {
        j++;

        if (1 == j) {
            cnt += scnprintf(buf+cnt, buf_size-cnt, "%02x: ", i);
        }
        cnt += scnprintf(buf+cnt, buf_size-cnt, "%02x ", data[i]);

        if (16 == j) {
            cnt += scnprintf(buf+cnt, buf_size-cnt, "\n");
            j = 0;
        }
    }
    return cnt;
}

int sff_paged_eeprom_read(struct sff_obj_t *sff_obj,
                          u8 page,
                          u8 offset,
                          u8 *buf,
                          int len)
{
    int ret = 0;
    u8 addr = SFF_EEPROM_I2C_ADDR; //it's fixed

    if (page_sel_is_locked(sff_obj)) {
        MODULE_LOG_DBG("%s page_sel_is_locked\n", sff_obj->name);
        return -EWOULDBLOCK;
    }
    page_sel_lock(sff_obj);
    if (offset > SFF_PAGE_SEL_OFFSET) {
        if ((ret = sff_page_sel(sff_obj, page)) < 0) {
            SWPS_LOG_ERR("%s switch page fail\n", sff_obj->name);
            page_sel_unlock(sff_obj);
            return ret;
        }
    }
    ret = sff_eeprom_read(sff_obj, addr, offset, buf, len);
    page_sel_unlock(sff_obj);
    return ret;
}

int sff_paged_eeprom_write(struct sff_obj_t *sff_obj,
                           u8 page,
                           u8 offset,
                           const u8 *buf,
                           int len)
{
    int ret = 0;
    u8 addr = SFF_EEPROM_I2C_ADDR; //it's fixed

    if (page_sel_is_locked(sff_obj)) {
        MODULE_LOG_DBG("%s page_sel_is_locked\n", sff_obj->name);
        return -EWOULDBLOCK;
    }
    page_sel_lock(sff_obj);
    if (offset > SFF_PAGE_SEL_OFFSET) {
        if ((ret = sff_page_sel(sff_obj, page)) < 0) {
            SWPS_LOG_ERR("%s switch page fail\n", sff_obj->name);
            page_sel_unlock(sff_obj);
            return ret;
        }
    }
    ret = sff_eeprom_write(sff_obj, addr, offset, buf, len);
    page_sel_unlock(sff_obj);
    return ret;
}
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
        SWPS_LOG_DBG("%s\n", obj->kobj.name);
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

    if ((ret = sff_page_sel(sff_obj, page)) < 0) {
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

static ssize_t sff_tx_eq_type_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                              const char *buf, size_t count)
{
    int ret = 0;
    tx_eq_type_t type = TX_EQ_AUTO;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    
    if (match(buf, HELP_CMD)) {
        SWPS_LOG_INFO("echo \"auto\" or \"manual\"\n");
        return count;
    }
    if (p_valid(strnstr(buf, AUTO_TX_EQ_STR, BUF_SIZE))) {
        type = TX_EQ_AUTO;
    } else if (p_valid(strnstr(buf, MANUAL_TX_EQ_STR, BUF_SIZE))) {
        type = TX_EQ_MANUAL;
    } else {
        return -EINVAL;
    }

    check_pfunc(sff_obj->func_tbl->tx_eq_type_set);
    if ((ret = sff_obj->func_tbl->tx_eq_type_set(sff_obj, type)) < 0) {
        return ret;
    }

    return count;
}

static ssize_t lc_prs_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                           char *buf)
{
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
    struct lc_obj_t *card = sff_to_lc(sff);
    unsigned long sys_ready = card->swps->lc_sys_ready;
    return scnprintf(buf, BUF_SIZE, "0x%lx\n", sys_ready);
}

static ssize_t lc_phy_ready_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                 char *buf)
{
    unsigned long phy_ready = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
    struct lc_obj_t *card = sff_to_lc(sff);

    phy_ready = card->phy_ready_bitmap;

    return scnprintf(buf, BUF_SIZE, "0x%lx\n", phy_ready);
}
static ssize_t lc_phy_ready_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  const char *buf, size_t count)
{
    unsigned long phy_ready = 0;
    int ret = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
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
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
    struct lc_obj_t *card = sff_to_lc(sff);
    struct lc_func_t *lc_func = NULL;

    lc_func = card->swps->lc_func;
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
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
    struct lc_obj_t *card = sff_to_lc(sff);
    struct lc_func_t *lc_func = card->swps->lc_func;

    ret = sscanf_to_int(buf, &temp);
    if (ret < 0) {
        return ret;
    }
    check_pfunc(lc_func->temp_th_set);
    ret = lc_func->temp_th_set(card->lc_id, temp);
    if (ret < 0) {
        return ret;
    }

    return count;
}

static ssize_t lc_type_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                            char *buf)
{
    int ret = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
    struct lc_obj_t *card = sff_to_lc(sff);
    struct lc_func_t *lc_func = card->swps->lc_func;

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
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
    struct lc_obj_t *card = sff_to_lc(sff);
    struct lc_func_t *lc_func = card->swps->lc_func;
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
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
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
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    return sff_paged_eeprom_dump(sff_obj, buf, BUF_SIZE);
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
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    check_pfunc(sff_obj->func_tbl->active_ctrl_set_get);
    return sff_obj->func_tbl->active_ctrl_set_get(sff_obj, buf, BUF_SIZE);
}

static ssize_t sff_intr_flag_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  char *buf)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    check_pfunc(sff_obj->func_tbl->intr_flag_show);
    return sff_obj->func_tbl->intr_flag_show(sff_obj, buf, BUF_SIZE);
}
static ssize_t module_st_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                              char *buf)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    check_pfunc(sff_obj->func_tbl->module_st_get);
    return sff_obj->func_tbl->module_st_get(sff_obj, buf, BUF_SIZE);
}
static ssize_t data_path_st_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                 char *buf)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    check_pfunc(sff_obj->func_tbl->data_path_st_get);
    return sff_obj->func_tbl->data_path_st_get(sff_obj, buf, BUF_SIZE);
}
static ssize_t module_type_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                char *buf)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    check_pfunc(sff_obj->func_tbl->module_type_get);
    return sff_obj->func_tbl->module_type_get(sff_obj, buf, BUF_SIZE);
}

static ssize_t sff_type_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                             char *buf)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    return scnprintf(buf, BUF_SIZE, "%s\n", sff_type_str[sff_obj->type]);

}
static ssize_t sff_port_num_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                 char *buf)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    return scnprintf(buf, BUF_SIZE, "%d\n", sff_obj->sff->valid_port_num);
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
    return scnprintf(buf, BUF_SIZE, "0x%x\n", logLevel);
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

    long ldata = 0;
    int ret = 0;
    /*use kernel api instead*/
    if (!buf || !value) {
        return -EINVAL;
    }
    ret = kstrtol(buf, 0, &ldata);
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
    check_pfunc(sff_obj->func_tbl->lane_control_set);
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

    check_pfunc(sff_obj->func_tbl->lane_control_get);
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
    int ret = 0;
    u8 ch_status = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    check_pfunc(sff_obj->func_tbl->lane_status_get);
    if ((ret = sff_obj->func_tbl->lane_status_get(sff_obj, st_type, &ch_status)) < 0) {
        return ret;
    }

    return scnprintf(buf, BUF_SIZE, "0x%x\n", ch_status);
}
static ssize_t sff_rxlos_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                              char *buf)
{
    return lane_status_show(LN_STATUS_RXLOS_TYPE, kobj, buf);
}
static ssize_t sff_tx_los_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                               char *buf)
{
    return lane_status_show(LN_STATUS_TXLOS_TYPE, kobj, buf);
}
static ssize_t sff_txfault_show(struct swps_kobj_t *kobj, struct swps_attribute *attr,
                                char *buf)
{
    return lane_status_show(LN_STATUS_TXFAULT_TYPE, kobj, buf);
}
static ssize_t lane_monitor_show(int moni_type, struct swps_kobj_t *swps_kobj, char *buf)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    check_pfunc(sff_obj->func_tbl->lane_monitor_get);
    return sff_obj->func_tbl->lane_monitor_get(sff_obj, moni_type, buf, BUF_SIZE);
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
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    check_pfunc(sff_obj->func_tbl->vendor_info_get);
    return sff_obj->func_tbl->vendor_info_get(sff_obj, info_type, buf, BUF_SIZE);
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
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    check_pfunc(sff_obj->func_tbl->temperature_get);
    return sff_obj->func_tbl->temperature_get(sff_obj, buf, BUF_SIZE);
}
static ssize_t voltage_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                            char *buf)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    check_pfunc(sff_obj->func_tbl->voltage_get);
    return sff_obj->func_tbl->voltage_get(sff_obj, buf, BUF_SIZE);
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

static ssize_t super_fsm_st_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                           char *buf)
{
    int st = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    st = sff_super_fsm_st_get(sff_obj);
    return scnprintf(buf, BUF_SIZE, "%s\n", sff_super_fsm_st_str[st]);
}
static ssize_t lc_fsm_st_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                              char *buf)
{
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
    struct lc_obj_t *card = sff_to_lc(sff);
    return scnprintf(buf, BUF_SIZE, "%s\n", lc_fsm_st_str[card->st]);
}

static ssize_t sff_all_io_store(sff_io_type_t type, struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   const char *buf, size_t count)
{
    int ret = 0;
    unsigned long bitmap = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
    int i = 0;

    for (i = 0; i < count; i++) {
        if (i < sff->valid_port_num) {
            if (buf[i] == 'x' || buf[i] == 'X') {
                continue;
            }
            if (buf[i] != '0' && buf[i] != '1') {
                SWPS_LOG_ERR("%d:set val = %c is not support.\n", i, buf[i]);
                return -EINVAL;
            }

            if ('1' == buf[i]) {
                set_bit(i, &bitmap);
            } else {
                clear_bit(i, &bitmap);
            }
        }
    }
    SWPS_LOG_DBG("set val = 0x%lx\n", bitmap);
    if ((ret = sff_all_io_set_frontport(type, sff, bitmap)) < 0) {
        return ret;
    }
    return count;
}

static ssize_t sff_all_io_show(sff_io_type_t type, struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  char *buf)
{
    int ret = 0;
    unsigned long front_port_io = 0;
    int port = 0;
    int front_port = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
    int front_port_num = sff->valid_port_num;

    if ((ret = sff_all_io_get_frontport(type, sff, &front_port_io)) < 0) {
        return ret;
    }

    for (front_port = 0; front_port < front_port_num; front_port++) {
        port = sff->frontPort_to_port[front_port];

        if (sff_io_supported(&sff->obj[port], type)) {
            buf[front_port] = (test_bit(front_port, &front_port_io) ? '1' : '0');
        } else {
            buf[front_port] = 'X';
        }
    }

    buf[front_port] = '\n';
    return (ssize_t)strlen(buf);
}

static ssize_t sff_all_isolated_transvr_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  char *buf)
{
    int port = 0;
    int front_port = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
    int front_port_num = sff->valid_port_num;

    for (front_port = 0; front_port < front_port_num; front_port++) {
        port = sff->frontPort_to_port[front_port];
        buf[front_port] = ((SFF_SUPER_FSM_ST_ISOLATED == sff->obj[port].super_fsm.st) ? '1' : '0');
    }

    buf[front_port] = '\n';
    return (ssize_t)strlen(buf);
}

static ssize_t sff_all_reset_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   const char *buf, size_t count)
{
    return sff_all_io_store(SFF_IO_RST_TYPE, swps_kobj, attr, buf, count);
}
static ssize_t sff_all_reset_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  char *buf)
{
    return sff_all_io_show(SFF_IO_RST_TYPE, swps_kobj, attr, buf);
}
static ssize_t sff_all_lpmode_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                    const char *buf, size_t count)
{
    return sff_all_io_store(SFF_IO_LPMODE_TYPE, swps_kobj, attr, buf, count);
}
static ssize_t sff_all_lpmode_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   char *buf)
{
    return sff_all_io_show(SFF_IO_LPMODE_TYPE, swps_kobj, attr, buf);
}

static ssize_t sff_all_modsel_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                    const char *buf, size_t count)
{
    return sff_all_io_store(SFF_IO_MODSEL_TYPE, swps_kobj, attr, buf, count);
}
static ssize_t sff_all_modsel_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   char *buf)
{
    return sff_all_io_show(SFF_IO_MODSEL_TYPE, swps_kobj, attr, buf);
}

static ssize_t sff_all_intL_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   char *buf)
{
    return sff_all_io_show(SFF_IO_INTR_TYPE, swps_kobj, attr, buf);
}
static ssize_t sff_all_present_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                char *buf)
{
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
    int front_port_num = sff->valid_port_num;
    int front_port = 0;
    sff_all_io_show(SFF_IO_PRS_TYPE, swps_kobj, attr, buf);
    for (front_port = 0; front_port < front_port_num; front_port++) {
        if (buf[front_port] == '0') {
            buf[front_port] = '1';
        }
        else {
            buf[front_port] = '0';
        }
    }
    return (ssize_t)strlen(buf);
}
static ssize_t sff_all_power_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   const char *buf, size_t count)
{
    int ret = 0;
    unsigned long power = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
    int i = 0;

    if (!sff_power_is_supported()) {
        return -ENOSYS;
    }
    
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
    if ((ret = sff_all_power_set_frontport(sff, power)) < 0) {
        return ret;
    }
    return count;
}
static ssize_t sff_all_power_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  char *buf)
{
    int ret = 0;
    unsigned long front_port_power = 0;
    unsigned long phy_port_power = 0;
    int port = 0;
    int front_port = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
    int front_port_num = sff->valid_port_num;

    if (sff_power_is_supported()) {

        if ((ret = sff_all_power_get(sff, &phy_port_power)) < 0) {
            return ret;
        }
        front_port_power = phyPort_to_frontPort(sff, phy_port_power);

    } else {
        /*[note] if sff_power is not supported means power is always on*/
        for (front_port = 0; front_port < sff->valid_port_num; front_port++) {
            set_bit(front_port, &front_port_power); 
        }
    }

    for (front_port = 0; front_port < front_port_num; front_port++) {
        port = sff->frontPort_to_port[front_port];
        buf[front_port] = (test_bit(front_port, &front_port_power) ? '1' : '0');
    }

    buf[front_port] = '\n';
    return (ssize_t)strlen(buf);
}
static ssize_t sff_all_txdisable_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  char *buf)
{
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
       int front_port_num = sff->valid_port_num;
    int front_port = 0;
    u8 txdisable = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = NULL;
    /*[node] For baidu spec, not present port is shown 0, QSFP shown 0~F */
    for(front_port = 0; front_port < front_port_num; front_port++) {
        sff_obj = &(sff->obj[front_port]);
        if (sff_obj->kobj == NULL) {
            buf[front_port] = '0';
        }
        else {
            ret = sff_obj->func_tbl->txdisable_get(sff_obj, &txdisable);
            if (ret < 0) {
                buf[front_port] = '0';
            }
            else {
                if (INV_BAIDU_SOLUTION) {
                    buf[front_port] = ( txdisable == 0 ? '0' : ( sff_obj->type == SFP_TYPE ? '1' : 'f') );
                }
                else {
                    buf[front_port] = ( txdisable == 0 ? '0' : '1' );
                }
            }
        }
    }

       buf[front_port] = '\n';
    return (ssize_t)strlen(buf);
}
static ssize_t sff_all_txdisable_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   const char *buf, size_t count)
{
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
       int front_port_num = sff->valid_port_num;
    int front_port = 0;
    int txdisable = 0;
    int len = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = NULL;

    // To comfirm if the input value is valid
    len = strlen(buf);
    if (len < front_port_num) {
        SWPS_LOG_DBG( "The length of input value (%s) is %d but the valid length is all port count (%d)\n", buf, len, front_port_num);
        return -EINVAL;
    }
    for(front_port = 0; front_port < front_port_num; front_port++) {
        if (INV_BAIDU_SOLUTION) {
            if (buf[front_port] != '0' && buf[front_port] != '1' && buf[front_port] != 'f' && buf[front_port] != 'F') {
                SWPS_LOG_ERR("port %d has the illegal value (%c) which is not support.\n", front_port, buf[front_port]);
                return -EINVAL;
            }
        }
        else {
            if (buf[front_port] != '0' && buf[front_port] != '1') {
                SWPS_LOG_ERR("port %d has the illegal value (%c) which is not support.\n", front_port, buf[front_port]);
                return -EINVAL;
            }
        }
    }
    /*[node] For baidu spec, not present port is shown 0, QSFP shown 0~F */
    for(front_port = 0; front_port < front_port_num; front_port++) {
        if (INV_BAIDU_SOLUTION) {
            if (buf[front_port] == 'f' || buf[front_port] == 'F') {
                txdisable = 1;
            }
        }
        else {
            ret = sscanf_to_int(&buf[front_port], &txdisable);
        }
        if (ret < 0) {
            SWPS_LOG_ERR("Invoke sscanf_to_int failed due to val = %c and port is %d\n", buf[front_port], front_port);
            return -EINVAL;
        }
        else {
            sff_obj = &(sff->obj[front_port]);
            if (sff_obj->kobj == NULL) {
                continue;
            }
            ret = sff_obj->func_tbl->txdisable_set(sff_obj, txdisable);
            if(ret < 0) {
                SWPS_LOG_ERR("Set tx_disable value of port %d failed (val = %c)\n", front_port, buf[front_port]);
                continue;
            }
        }
    }

    return count;
}
static ssize_t sff_reset_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                               const char *buf, size_t count)
{
    int ret = 0;
    int rst = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    if((ret = sscanf_to_int(buf, &rst)) < 0) {
        return ret;
    }
    /*[node] For baidu spec, not present port is shown 1  */
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

    if ((ret = sff_reset_get(sff_obj, &reset)) < 0) {
        return ret;
    }
    return scnprintf(buf, BUF_SIZE, "%d\n", reset);
}
#if 0 /*reserved*/
static ssize_t sff_power_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                               const char *buf, size_t count)
{
    int ret = 0;
    int set_power = 0;
    int cur_power = 0;
    unsigned long bitmap = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    struct sff_mgr_t *sff = sff_obj->sff;

    if((ret = sscanf_to_int(buf, &set_power)) < 0) {
        return ret;
    }
    
    if((ret = sff_all_power_get(sff, &bitmap)) < 0) {
        return ret;
    }
    cur_power = (test_bit(sff_obj->port, &bitmap) ? 1 : 0);
    
    if (set_power) {
        set_bit(sff_obj->port, &bitmap);
    } else {
        clear_bit(sff_obj->port, &bitmap);
    }

    if((ret = sff_all_power_set(sff, bitmap)) < 0) {
        return ret;
    }
    if (cur_power != set_power) {
        transvr_type_set(sff_obj, TRANSVR_CLASS_UNKNOWN);
        
        if (set_power) {
            sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_RESTART);
        } else {
            sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_SUSPEND);
        }
    }
    return count;
}
#endif
#if 0 /*reserved*/
static ssize_t sff_power_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                              char *buf)
{
    u8 power = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    if ((ret = sff_power_get(sff_obj, &power)) < 0) {
        return ret;
    }
    return scnprintf(buf, BUF_SIZE, "%d\n", power);
}
#endif
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

    check_pfunc(sff_obj->func_tbl->rev4_quick_set);
    sff_obj->func_tbl->rev4_quick_set(sff_obj, en);
    return count;
}
static ssize_t rev4_quick_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                               char *buf)
{
    bool en = false;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    check_pfunc(sff_obj->func_tbl->rev4_quick_get);
    en = sff_obj->func_tbl->rev4_quick_get(sff_obj);
    return scnprintf(buf, BUF_SIZE, "%d\n", en);
}
static ssize_t gpio_mux_reset_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   char *buf)
{
    int val = 0;
    int ret = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
    struct lc_obj_t *card = sff_to_lc(sff);
    struct pltfm_func_t *pltfm_func = card->swps->pltfm_func;

    check_pfunc(pltfm_func->mux_reset_get);
    if ((ret = pltfm_func->mux_reset_get(card->lc_id, &val)) < 0) {
        return ret;
    }

    return scnprintf(buf, BUF_SIZE, "%d\n", val);
}
static ssize_t gpio_mux_reset_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                    const char *buf, size_t count)
{
    int ret = 0;
    int val = 0;
    struct sff_mgr_t *sff = swps_kobj->sff_obj->sff;
    struct lc_obj_t *card = sff_to_lc(sff);
    struct pltfm_func_t *pltfm_func = card->swps->pltfm_func;
    
    ret = sscanf_to_int(buf, &val);
    if(ret < 0) {
        return ret;
    }

    check_pfunc(pltfm_func->mux_reset_set);
    if((ret = pltfm_func->mux_reset_set(card->lc_id, val)) < 0) {
        return ret;
    }
    
    if (!i2c_recovery_is_muxdrv_method()) {
        if (val) {
            mux_reset_ch_resel_byLC(card);
            SWPS_LOG_DBG("mux_reset_ch_resel done\n");
        }
    }
    return count;
}
static ssize_t sff_lpmode_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                const char *buf, size_t count)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    int ret = 0;
    int lpmode = 0;

    if((ret = sscanf_to_int(buf, &lpmode)) < 0) {
        return ret;
    }
    if((ret = sff_lpmode_set(sff_obj, lpmode)) < 0) {
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

    if ((ret = sff_lpmode_get(sff_obj, &lpmode)) < 0) {
        return ret;
    }
    return scnprintf(buf, BUF_SIZE, "%d\n", lpmode);
}
static ssize_t sff_intL_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                             char *buf)
{
    u8 intr = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    if ((ret = sff_intr_get(sff_obj, &intr)) < 0) {
        return ret;
    }

    return scnprintf(buf, BUF_SIZE, "%d\n", intr);
}
static ssize_t sff_modsel_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                const char *buf, size_t count)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    int ret = 0;
    int modsel = 0;

    if((ret = sscanf_to_int(buf, &modsel)) < 0) {
        return ret;
    }
    if((ret = sff_modsel_set(sff_obj, modsel)) < 0) {
        return ret;
    }

    return count;
}
static ssize_t sff_modsel_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                               char *buf)
{
    u8 modsel = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    if ((ret = sff_modsel_get(sff_obj, &modsel)) < 0) {
        return ret;
    }
    return scnprintf(buf, BUF_SIZE, "%d\n", modsel);
}
static ssize_t sff_txdisable_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                   const char *buf, size_t count)
{
    int ret = 0;
    int txdisable = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    ret = sscanf_to_int(buf, &txdisable);

    if(ret < 0) {
        return ret;
    }

    ret = sff_obj->func_tbl->txdisable_set(sff_obj, txdisable);

    if(ret < 0) {
        return ret;
    }
    return count;
}
static ssize_t sff_txdisable_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  char *buf)
{
    u8 txdisable = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    ret = sff_obj->func_tbl->txdisable_get(sff_obj, &txdisable);

    if(ret < 0) {
        return ret;
    }


    return scnprintf(buf, BUF_SIZE, "0x%x\n", txdisable);
}

static ssize_t swps_polling_store(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                                  const char *buf, size_t count)
{
    int val = 0;
    bool en = false;
    int ret = 0;
    
    if((ret = sscanf_to_int(buf, &val)) < 0) {
        return ret;
    }

    en = ((val == 1) ? true : false);
    inv_swps_polling_set(en);

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
    check_pfunc(sff_obj->func_tbl->apsel_apply);
    if ((ret = sff_obj->func_tbl->apsel_apply(sff_obj, apsel)) < 0) {
        return ret;
    }

    return count;
}
static ssize_t sff_apsel_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                              char *buf)
{
    int apsel = 0;
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;

    check_pfunc(sff_obj->func_tbl->apsel_get);
    apsel = sff_obj->func_tbl->apsel_get(sff_obj);

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

    return scnprintf(buf, BUF_SIZE, "%d\n", inv_swps_polling_is_enabled());
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

    if ((ret = sff_page_get(sff_obj, &page)) < 0) {
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

static ssize_t sff_tx_eq_type_show(struct swps_kobj_t *swps_kobj, struct swps_attribute *attr,
                             char *buf)
{
    struct sff_obj_t *sff_obj = swps_kobj->sff_obj;
    tx_eq_type_t type = TX_EQ_AUTO;
    char *str = NULL;

    check_pfunc(sff_obj->func_tbl->tx_eq_type_get);
    type = sff_obj->func_tbl->tx_eq_type_get(sff_obj);
    if (type == TX_EQ_MANUAL) {
        str = MANUAL_TX_EQ_STR;
    } else {
        str = AUTO_TX_EQ_STR;
    }
    return scnprintf(buf, BUF_SIZE, "%s\n", str);
}

static struct swps_attribute lc_fsm_st_attr =
    __ATTR(fsm_st, S_IRUGO, lc_fsm_st_show, NULL);

static struct swps_attribute sff_port_num_attr =
    __ATTR(port_num, S_IRUGO, sff_port_num_show, NULL);

static struct swps_attribute sff_type_attr =
    __ATTR(type, S_IRUGO, sff_type_show, NULL);

static struct swps_attribute sff_id_attr =
    __ATTR(id, S_IRUGO, id_show, NULL);

static struct swps_attribute sff_transvr_type_attr =
    __ATTR(transvr_type, S_IRUGO, transvr_type_show, NULL);

static struct swps_attribute sff_fsm_st_attr =
    __ATTR(fsm_st, S_IRUGO, fsm_st_show, NULL);

static struct swps_attribute sff_super_fsm_st_attr =
    __ATTR(super_fsm_st, S_IRUGO, super_fsm_st_show, NULL);

static struct swps_attribute sff_all_present_attr =
    __ATTR(present, S_IRUGO, sff_all_present_show, NULL);

static struct swps_attribute sff_all_isolated_port_attr =
    __ATTR(isolated_port, S_IRUGO, sff_all_isolated_transvr_show, NULL);

static struct swps_attribute swps_polling_attr =
    __ATTR(swps_polling, S_IWUSR|S_IRUGO, swps_polling_show, swps_polling_store);

static struct swps_attribute sff_int_flag_monitor_attr =
    __ATTR(sff_int_flag_monitor, S_IWUSR|S_IRUGO, sff_int_flag_monitor_show, sff_int_flag_monitor_store);

static struct swps_attribute sff_reset_attr =
    __ATTR(reset, S_IWUSR|S_IRUGO, sff_reset_show, sff_reset_store);
static struct swps_attribute sff_all_reset_attr =
    __ATTR(reset, S_IWUSR|S_IRUGO, sff_all_reset_show, sff_all_reset_store);

static struct swps_attribute sff_all_lpmode_attr =
    __ATTR(lpmode, S_IWUSR|S_IRUGO, sff_all_lpmode_show, sff_all_lpmode_store);

static struct swps_attribute sff_all_modsel_attr =
    __ATTR(modsel, S_IWUSR|S_IRUGO, sff_all_modsel_show, sff_all_modsel_store);

static struct swps_attribute sff_all_intL_attr =
    __ATTR(intL, S_IRUGO, sff_all_intL_show, NULL);

static struct swps_attribute sff_all_power_attr =
    __ATTR(power, S_IWUSR|S_IRUGO, sff_all_power_show, sff_all_power_store);

static struct swps_attribute sff_all_txdisable_attr =
    __ATTR(tx_disable, S_IWUSR|S_IRUGO, sff_all_txdisable_show, sff_all_txdisable_store);
#if 0 /*reserved*/
static struct swps_attribute sff_power_attr =
    __ATTR(power, S_IWUSR|S_IRUGO, sff_power_show, sff_power_store);
#endif
static struct swps_attribute sff_intL_attr =
    __ATTR(intL, S_IRUGO, sff_intL_show, NULL);

static struct swps_attribute sff_lpmode_attr =
    __ATTR(lpmode, S_IWUSR|S_IRUGO, sff_lpmode_show, sff_lpmode_store);

static struct swps_attribute sff_modsel_attr =
    __ATTR(modsel, S_IWUSR|S_IRUGO, sff_modsel_show, sff_modsel_store);

static struct swps_attribute sff_txdisable_attr =
    __ATTR(tx_disable, S_IWUSR|S_IRUGO, sff_txdisable_show, sff_txdisable_store);

static struct swps_attribute sff_rxlos_attr =
    __ATTR(rx_los, S_IRUGO, sff_rxlos_show, NULL);

static struct swps_attribute sff_tx_los_attr =
    __ATTR(tx_los, S_IRUGO, sff_tx_los_show, NULL);

static struct swps_attribute sff_txfault_attr =
    __ATTR(tx_fault, S_IRUGO, sff_txfault_show, NULL);

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
static struct swps_attribute sff_data_path_st_attr =
    __ATTR(data_path_st, S_IRUGO, data_path_st_show, NULL);
static struct swps_attribute sff_module_type_attr =
    __ATTR(module_type, S_IRUGO, module_type_show, NULL);
static struct swps_attribute sff_active_ctrl_set_attr =
    __ATTR(active_ctrl_set, S_IRUGO, active_ctrl_set_show, NULL);

static struct swps_attribute sff_apsel_attr =
    __ATTR(apsel, S_IWUSR|S_IRUGO, sff_apsel_show, sff_apsel_store);

static struct swps_attribute sff_intr_flag_attr =
    __ATTR(intr_flag, S_IWUSR|S_IRUGO, sff_intr_flag_show, sff_intr_flag_store);

static struct swps_attribute sff_tx_eq_type_attr =
    __ATTR(tx_eq_type, S_IWUSR|S_IRUGO, sff_tx_eq_type_show, sff_tx_eq_type_store);

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
    &sff_txdisable_attr.attr,
    /*eeprom attribute*/
    &sff_type_attr.attr,
    &sff_id_attr.attr,
    &sff_tx_eq_attr.attr,
    &sff_rx_em_attr.attr,
    &sff_rx_am_attr.attr,
    &sff_rxlos_attr.attr,
    &sff_tx_los_attr.attr,
    &sff_txfault_attr.attr,
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
    &sff_super_fsm_st_attr.attr,
    //&sff_eeprom_dump_attr.attr,
    NULL

};

static struct attribute_group sfp_group = {
    .attrs = sfp_attributes,
};

static struct attribute *qsfp_attributes[] = {
    /*io pin attribute*/
    &sff_reset_attr.attr,
    //&sff_power_attr.attr,
    &sff_lpmode_attr.attr,
    &sff_modsel_attr.attr,
    &sff_intL_attr.attr,
    /*eeprom attribute*/
    &sff_type_attr.attr,
    &sff_id_attr.attr,
    &sff_tx_eq_attr.attr,
    &sff_rx_em_attr.attr,
    &sff_rx_am_attr.attr,
    &sff_rxlos_attr.attr,
    &sff_tx_los_attr.attr,
    &sff_txfault_attr.attr,
    &sff_tx_power_attr.attr,
    &sff_rx_power_attr.attr,
    &sff_tx_bias_attr.attr,
    &sff_temperature_attr.attr,
    &sff_voltage_attr.attr,
    &sff_txdisable_attr.attr,
    &sff_vendor_name_attr.attr,
    &sff_vendor_part_number_attr.attr,
    &sff_vendor_serial_number_attr.attr,
    &sff_vendor_rev_attr.attr,
    /*transceiver identified info attribute*/
    &sff_transvr_type_attr.attr,
    &sff_fsm_st_attr.attr,
    &sff_super_fsm_st_attr.attr,
    &sff_eeprom_dump_attr.attr,
    &sff_page_attr.attr,
    &sff_page_sel_lock_attr.attr,
    &sff_intr_flag_attr.attr,
    NULL
};
static struct attribute_group qsfp_group = {
    .attrs = qsfp_attributes,
};
static struct attribute *qsfp_dd_attributes[] = {
    /*io pin attribute*/
    &sff_reset_attr.attr,
    //&sff_power_attr.attr,
    &sff_lpmode_attr.attr,
    &sff_modsel_attr.attr,
    &sff_intL_attr.attr,
    /*eeprom attribute*/
    &sff_type_attr.attr,
    &sff_id_attr.attr,
    &sff_rxlos_attr.attr,
    &sff_tx_los_attr.attr,
    &sff_txfault_attr.attr,
    &sff_tx_power_attr.attr,
    &sff_rx_power_attr.attr,
    &sff_tx_bias_attr.attr,
    &sff_temperature_attr.attr,
    &sff_voltage_attr.attr,
    &sff_txdisable_attr.attr,
    &sff_vendor_name_attr.attr,
    &sff_vendor_part_number_attr.attr,
    &sff_vendor_serial_number_attr.attr,
    &sff_vendor_rev_attr.attr,
    /*transceiver identified info attribute*/
    //&sff_transvr_type_attr.attr,
    &sff_fsm_st_attr.attr,
    &sff_super_fsm_st_attr.attr,
    &sff_eeprom_dump_attr.attr,
    &sff_page_attr.attr,
    &sff_page_sel_lock_attr.attr,
    &sff_module_st_attr.attr,
    &sff_data_path_st_attr.attr,
    &sff_module_type_attr.attr,
    &rev4_quick_attr.attr,
    &sff_active_ctrl_set_attr.attr,
    &sff_apsel_attr.attr,
    &sff_intr_flag_attr.attr,
    &sff_tx_eq_attr.attr,
    &sff_tx_eq_type_attr.attr,
    NULL
};
static struct attribute_group qsfp_dd_group = {
    .attrs = qsfp_dd_attributes,
};
static struct attribute *sfp_dd_attributes[] = {
    /*io pin attribute*/
    &sff_reset_attr.attr,
    //&sff_power_attr.attr,
    &sff_lpmode_attr.attr,
    &sff_modsel_attr.attr,
    &sff_intL_attr.attr,
    /*eeprom attribute*/
    &sff_type_attr.attr,
    &sff_id_attr.attr,
    &sff_rxlos_attr.attr,
    &sff_tx_los_attr.attr,
    &sff_txfault_attr.attr,
    &sff_tx_power_attr.attr,
    &sff_rx_power_attr.attr,
    &sff_tx_bias_attr.attr,
    &sff_temperature_attr.attr,
    &sff_voltage_attr.attr,
    &sff_txdisable_attr.attr,
    &sff_vendor_name_attr.attr,
    &sff_vendor_part_number_attr.attr,
    &sff_vendor_serial_number_attr.attr,
    &sff_vendor_rev_attr.attr,
    /*transceiver identified info attribute*/
    //&sff_transvr_type_attr.attr,
    &sff_fsm_st_attr.attr,
    &sff_super_fsm_st_attr.attr,
    &sff_eeprom_dump_attr.attr,
    &sff_page_attr.attr,
    &sff_page_sel_lock_attr.attr,
    &sff_module_st_attr.attr,
    &sff_data_path_st_attr.attr,
    &sff_module_type_attr.attr,
    &rev4_quick_attr.attr,
    &sff_active_ctrl_set_attr.attr,
    &sff_apsel_attr.attr,
    &sff_intr_flag_attr.attr,
    NULL
};
static struct attribute_group sfp_dd_group = {
    .attrs = sfp_dd_attributes,
};

static struct attribute_group qsfp56_group = {
    .attrs = qsfp_dd_attributes,
};
/*attribute of all sff modules , some represents as bitmap form*/
static struct attribute *sff_common_attributes[] = {
    &sff_all_present_attr.attr,
    &mux_reset_attr.attr,
    &sff_int_flag_monitor_attr.attr,
    &sff_port_num_attr.attr,
    &sff_all_reset_attr.attr,
    &sff_all_lpmode_attr.attr,
    &sff_all_modsel_attr.attr,
    &sff_all_intL_attr.attr,
    &sff_all_power_attr.attr,
    &sff_all_txdisable_attr.attr,
    &sff_all_isolated_port_attr.attr,
    NULL
};

static struct attribute_group sff_common_group = {
    .attrs = sff_common_attributes,
};
static struct attribute *swps_common_attributes[] = {
    &swps_version_attr.attr,
    &swps_polling_attr.attr,
    &lc_prs_attr.attr,
    &log_level_attr.attr,
    &pltfm_name_attr.attr,
    &io_no_init_attr.attr,
    NULL
};
static struct attribute_group swps_common_group = {
    .attrs = swps_common_attributes,
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
static struct attribute_group *sff_attr_group_map[SFF_TYPE_NUM] = {
    [SFP_TYPE] = &sfp_group,
    [QSFP_TYPE] = &qsfp_group,
    [QSFP_DD_TYPE] = &qsfp_dd_group,
    [QSFP56_TYPE] = &qsfp56_group,
    [SFP_DD_TYPE] = &sfp_dd_group,
    [SFF_UNKNOWN_TYPE] = NULL,
};

static void sff_frontPort_remap_destroy(struct sff_mgr_t *sff)
{
    if (p_valid(sff->frontPort_to_port)) {
        kfree(sff->frontPort_to_port);
    }
}
static void lc_sff_frontPort_remap_destroy(struct swps_mgr_t *self)
{
    int i = 0;
    int lc_num = self->lc_num;

    for (i = 0; i < lc_num; i++) {
        sff_frontPort_remap_destroy(&self->obj[i].sff);
    }
}

static void sff_objs_destroy(struct sff_mgr_t *sff)
{
    int port = 0;
    int port_num = sff->valid_port_num;
    struct sff_obj_t *sff_obj = NULL;
        
    for (port = 0; port < port_num; port++) {
        sff_obj = &sff->obj[port];
        if (p_valid(sff_obj->priv_data)) {
            kfree(sff_obj->priv_data);
            SWPS_LOG_DBG("%s free priv_data\n", sff_obj->name);
        }
    }
    if (p_valid(sff->obj)) {
        kfree(sff->obj);
    }
}
static void lc_sff_objs_destroy(struct swps_mgr_t *self)
{
    int i = 0;
    int lc_num = self->lc_num;

    for (i = 0; i < lc_num; i++) {

        sff_objs_destroy(&self->obj[i].sff);
    }
}
static void lc_objs_destroy(struct swps_mgr_t *self)
{
    if (p_valid(self->obj)) {
        kfree(self->obj);
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
    int ret = 0;
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
        sff_obj[port].name = map[port].name;
        sff_obj[port].def_type = map[port].type;
        sff_obj[port].type = sff_obj[port].def_type;
        sff_obj[port].init_op_required = false;
        sff_obj[port].priv_data = NULL;
        if ((ret = sff_fsm_init(&sff_obj[port], sff_obj[port].def_type)) < 0) {
            break;
        }
        if ((ret = func_tbl_init(&sff_obj[port], sff_obj[port].def_type)) < 0) {
            break;
        }
    }
    if (ret < 0) {
        return ret;
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
        sff_obj[port].sff = sff;
        /*assign lc_id to corresponding sff_obj for fast access*/
        sff_obj[port].lc_id = card->lc_id;
        sff_obj[port].lc_name = card->name;
    }
    sff->obj = sff_obj;
    return 0;
}
static int lc_sff_objs_create(struct swps_mgr_t *self, int max_port_num)
{
    int lc_num = self->lc_num;
    int i = 0;
    int ret = 0;

    for (i = 0; i < lc_num; i++) {
        ret = sff_objs_create(&(self->obj[i].sff), max_port_num);
        if (ret < 0) {
            break;
        }
    }

    if (ret < 0) {
        lc_sff_objs_destroy(self);
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
static int lc_sff_frontPort_remap_create(struct swps_mgr_t *self, int max_port_num)
{
    int lc_num = self->lc_num;
    int i = 0;
    int ret = 0;

    for (i = 0; i < lc_num; i++) {
        ret = sff_frontPort_remap_create(&(self->obj[i].sff), max_port_num);
        if (ret < 0) {
            break;
        }
    }

    if (ret < 0) {
        lc_sff_frontPort_remap_destroy(self);
    }

    return ret;
}
static int lc_objs_create(struct swps_mgr_t *self)
{

    struct lc_obj_t *obj = NULL;
    int lc_num = self->lc_num;
    obj = kzalloc(sizeof(struct lc_obj_t) * lc_num, GFP_KERNEL);
    if(!p_valid(obj)) {
        return -ENOMEM;
    }

    self->obj = obj;
    return 0;

}

static int lc_objs_init(struct swps_mgr_t *self)
{
    int lc_num = self->lc_num;
    int i = 0;
    struct lc_obj_t *obj = NULL;
    for (i = 0; i < lc_num; i++) {
        obj = &(self->obj[i]);
        obj->swps = self;
        obj->lc_id = i;
        obj->prs = 0;
        obj->over_temp_cnt = 0;
        obj->st = LC_FSM_ST_IDLE;
        obj->power_ready = false;
        obj->is_phy_ready = true;
        obj->phy_ready_bitmap = 0xff;
        obj->ej_released = false;
        obj->prs_locked = false;
        obj->prs_locked_cnt = 0;
        obj->posi_st = LC_POSI_INIT_ST;
        
        memset(obj->name, 0, sizeof(obj->name));
        scnprintf(obj->name, sizeof(obj->name), "%s%d", CARD_NAME_PREFIX, i+CARD_NO_BASE);
    }
    return 0;
}
static int swps_common_kobj_create(struct swps_mgr_t *self)
{
    if (!p_valid(self->common_kobj)) {
        self->common_kobj = swps_kobj_add(SYSFS_COMMON_DIR, &swps_kset->kobj, &swps_common_group);
    }
    if(!p_valid(self->common_kobj)) {
        return -EBADRQC;
    }
    /*this link will let card kobj able to link to sff_obj*/
    self->common_kobj->sff_obj = self->obj->sff.obj;
    return 0;

}
static void polling_task_1U(void)
{
    if (!p_valid(swps_mgr.pltfm_func->io_hdlr)) {
        return;
    }
    swps_mgr.pltfm_func->io_hdlr();
    swps_fsm_run_1u(&swps_mgr);

    if (i2c_bus_recovery_is_supported()) {
        if (++i2cbus_alive_check_cnt >= I2CBUS_ALIVE_CHECK_NUM) {
            i2cbus_alive_check_cnt = 0;
            if (!i2c_bus_is_alive(&swps_mgr.obj[0])) {
                i2c_bus_recovery(&swps_mgr.obj[0]);
            }
        }
    }
    io_no_init_handler(&swps_mgr);
}
static void polling_task_4U(void)
{
    if (!p_valid(swps_mgr.pltfm_func->io_hdlr)) {
        return;
    }
    swps_mgr.pltfm_func->io_hdlr();
    lc_prs_scan(&swps_mgr);
    swps_fsm_run_4u(&swps_mgr);
    io_no_init_handler(&swps_mgr);
}
static int lc_func_load(struct swps_mgr_t *obj)
{
    obj->lc_func = pltfmInfo->lc_func;
    if (!p_valid(obj->lc_func)) {
        return -ENOSYS;
    }
    return 0;
}

static int pltfm_func_load(struct swps_mgr_t *obj)
{
    obj->pltfm_func = pltfmInfo->pltfm_func;
    if (!p_valid(obj->pltfm_func)) {
        return -ENOSYS;
    }
    return 0;
}

static int polling_task_load(struct swps_mgr_t *obj)
{
    if (is_pltfm_4U_type()) {
        obj->polling_task = polling_task_4U;
    } else {
        obj->polling_task = polling_task_1U;
    }
    if (!p_valid(obj->polling_task)) {
        return -ENOSYS;
    }
    
    return 0;
}    
static int lc_create_init(struct swps_mgr_t *self, int lc_num, int card_max_port_num)
{
    int ret = 0;
    /*init priv parameter*/
    self->lc_num = lc_num;
    self->lc_prs = 0;
    self->ej_r = 0;
    self->ej_l = 0;
    self->lc_sys_ready = 0;

    if ((ret = lc_objs_create(self)) < 0) {
       
        SWPS_LOG_ERR("lc_objs_create fail ret:%d\n", ret);
        goto exit_err;
    }

    if (lc_objs_init(self) < 0) {
        SWPS_LOG_ERR("lc_objs_init fail\n");
        goto exit_free_lc_objs;
    }
    
    if ((ret = lc_sff_objs_create(self, card_max_port_num)) < 0) {
        SWPS_LOG_ERR("lc_sff_objs_create fail ret:%d\n", ret);
        goto exit_free_lc_objs;
    }
    
    if ((ret = lc_sff_frontPort_remap_create(self, card_max_port_num)) < 0) {
        SWPS_LOG_ERR("lc_sff_frontPort_remap_create fail ret:%d\n", ret);
        goto exit_free_sff_objs;
    }
    
    if ((ret = sff_kset_create_init()) < 0) {
        SWPS_LOG_ERR("sff_kset_create_init fail ret:%d\n", ret);
        goto exit_free_frontPort_remap;
    }
    
    if ((ret = swps_common_kobj_create(self)) < 0) {
        SWPS_LOG_ERR("swps_common_kobj_create fail ret:%d\n", ret);
        goto exit_free_kset;
    }

    return 0;
exit_free_kset:
    sff_kset_deinit();
exit_free_frontPort_remap:
    lc_sff_frontPort_remap_destroy(self);
exit_free_sff_objs:
    lc_sff_objs_destroy(self);
exit_free_lc_objs:
    lc_objs_destroy(self);
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
    obj->kobj.kset = swps_kset;

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
    scnprintf(tmp_str_1, sizeof(tmp_str_1), "%s=%s", "IF_TYPE", "SR4");
    uevent_envp[0] = tmp_str_1;
    uevent_envp[1] = NULL;

    kobject_uevent_env(&sff_kobj->kobj, KOBJ_CHANGE, uevent_envp);
    return 0;
}
static int sff_kobj_add(struct sff_obj_t *sff_obj)
{
    struct attribute_group *attr_group = NULL;
    struct lc_obj_t *card = NULL;
    struct swps_kobj_t *mgr_kobj = NULL;

    if (p_valid(sff_obj->kobj)) {
        /*kobj already exists skip creation*/
        SWPS_LOG_DBG("%s kobj already exists skip creation\n", sff_obj->name);
        return 0;
    }
    attr_group = sff_attr_group_map[sff_obj->type];
    
    if (!p_valid(attr_group)) {
        return -ENOSYS;
    }
    card = sff_to_lc(sff_obj->sff);
    mgr_kobj = sff_obj->sff->mgr_kobj;
    
    if (!p_valid(mgr_kobj)) {
        return -EBADRQC;
    }
    
    sff_obj->kobj = swps_kobj_add(sff_obj->name, &(mgr_kobj->kobj), attr_group);
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
    swps_kset = kset_create_and_add(SWPS_KSET, NULL, NULL);

    if(!p_valid(swps_kset)) {
        SWPS_LOG_ERR("kset creat fail\n");
        return -EBADRQC;
    }
    return 0;
}
static int sff_kset_deinit(void)
{
    if(p_valid(swps_kset)) {
        kset_unregister(swps_kset);
    }
    return 0;
}

static int lc_kobj_init_create(struct lc_obj_t *card)
{
    struct attribute_group *attr_group = NULL;

    if (is_pltfm_4U_type()) {
        attr_group = &lc_group;
    }
    if (!p_valid(card->card_kobj)) {
        card->card_kobj = swps_kobj_add(card->name, &swps_kset->kobj, attr_group);
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
static void lc_sff_kobjs_destroy(struct swps_mgr_t *self)
{
    int i;
    int lc_num = self->lc_num;
    struct sff_mgr_t *sff = NULL;
    for (i = 0; i < lc_num; i++) {
        sff = &(self->obj[i].sff);
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
static void lc_kobj_destroy(struct swps_mgr_t *self)
{

    int i;
    int lc_num = self->lc_num;
    struct lc_obj_t *obj = NULL;
    for (i = 0; i < lc_num; i++) {
        obj = &(self->obj[i]);
        _lc_sff_mgr_kobj_destroy(obj);
        _lc_kobj_destroy(obj);
    }
}
static void swps_common_kobj_destroy(struct swps_mgr_t *self)
{
    if(p_valid(self->common_kobj)) {
        swps_kobj_del(&(self->common_kobj));
    }
}
static void transvr_insert(struct sff_obj_t *sff_obj)
{
    SWPS_LOG_INFO("into %s %s\n", sff_obj->lc_name, sff_obj->name);
    sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_INSERTED);
    sff_obj->init_op_required = true;
    transvr_type_set(sff_obj, TRANSVR_CLASS_UNKNOWN);
}
static void transvr_remove(struct sff_obj_t *sff_obj)
{
    struct mux_ch_t *mux = NULL;
    struct lc_obj_t *lc = sff_to_lc(sff_obj->sff);
    struct pltfm_func_t *pltfm_func = lc->swps->pltfm_func;
    int ch = 0;
    SWPS_LOG_INFO("from %s %s\n", sff_obj->lc_name, sff_obj->name);
    sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_REMOVED);
    transvr_type_set(sff_obj, TRANSVR_CLASS_UNKNOWN);
    sff_kobj_del(sff_obj);
    
    if (i2c_recovery_is_muxdrv_method()) {
        mux = &(lc->mux_l1);
        if (0 != mux->block_ch) {
            pltfm_func->mux_fail_set(lc->lc_id, false);
            ch = pltfm_func->mux_port_to_ch(lc->lc_id, sff_obj->port);
            if (ch < 0) {
                return;
            }
            clear_bit(ch, &(mux->block_ch));
            pltfm_func->mux_blocked_ch_set(sff_obj->lc_id, mux->block_ch);
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


    if((ret = sff_all_prs_get(sff, &bitmap)) < 0) {
        return ret;
    }
    sff_prs_cache_update(sff, bitmap); //update current prs_bitmap

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
    if((ret = sff_all_prs_get(sff, &bitmap)) < 0) {
        SWPS_LOG_ERR("fail\n");
        return ret;
    }
    /*check which bits are updated*/
    //bitmap = ~bitmap;  /*reverse it to be human readable format*/
    prs_change = bitmap ^ sff_prs_cache_get(sff);
    sff_prs_cache_update(sff, bitmap); //update current prs_bitmap

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
    if (p_valid(swps_mgr.polling_task)) {
        swps_mgr.polling_task();
    }
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
static void sff_super_fsm_st_set(struct sff_obj_t *sff_obj, sff_super_fsm_st_t st)
{
    sff_super_fsm_st_t old_st = sff_obj->super_fsm.st;
    sff_super_fsm_st_chg_process(sff_obj, old_st, st);
    sff_obj->super_fsm.st = st;

}
static sff_super_fsm_st_t sff_super_fsm_st_get(struct sff_obj_t *sff_obj)
{
    return sff_obj->super_fsm.st;
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

int sff_io_set(sff_io_type_t type, struct sff_obj_t *sff_obj, u8 val)
{
    unsigned long bitmap = 0;
    int ret = 0;

    if (!sff_io_supported(sff_obj, type)) {
        return -ENOSYS;
    }
    
    if ((ret = sff_all_io_get(type, sff_obj->sff, &bitmap)) < 0) {
        return ret;
    }
    
    if (val) {
        set_bit(sff_obj->port, &bitmap);
    } else {
        clear_bit(sff_obj->port, &bitmap);
    }
    
    if ((ret = sff_all_io_set(type, sff_obj->sff, bitmap)) < 0) {
        return ret;
    }
    return 0;
}

int sff_io_get(sff_io_type_t type, struct sff_obj_t *sff_obj, u8 *val)
{
    u8 ret = 0;
    unsigned long bitmap = 0;
    
    if (!p_valid(val)) {
        return -EINVAL;
    }
    
    if (!sff_io_supported(sff_obj, type)) {
        return -ENOSYS;
    }
    if ((ret = sff_all_io_get(type, sff_obj->sff, &bitmap)) < 0) {
        return ret;
    }
    if (test_bit(sff_obj->port, &bitmap)) {
        *val = 1;
    } else {
        *val = 0;
    }
    return 0;
}    
/*sff common function*/
int sff_prs_get(struct sff_obj_t *sff_obj, u8 *prs)
{
    return sff_io_get(SFF_IO_PRS_TYPE, sff_obj, prs);
}
/*[note] if sff_power_is NOT supported  the *value will be 1 
 * because the power(vcc) will be always on*/
static int sff_power_get(struct sff_obj_t *sff_obj, u8 *val)
{
    int ret = 0;
    struct sff_mgr_t *sff = sff_obj->sff;
    unsigned long bitmap = 0;

    if (!sff_power_is_supported()) {
        *val = 1;
        return 0;
    }
    if ((ret = sff_all_power_get(sff, &bitmap)) < 0) {
        return ret;
    }
    if (test_bit(sff_obj->port, &bitmap)) {
        *val = 1;
    } else {
        *val = 0;
    }
    return 0;
}

int sff_lpmode_set(struct sff_obj_t *sff_obj, u8 value)
{
    return sff_io_set(SFF_IO_LPMODE_TYPE, sff_obj, value);
}

int sff_lpmode_get(struct sff_obj_t *sff_obj, u8 *value)
{
    return sff_io_get(SFF_IO_LPMODE_TYPE, sff_obj, value);
}

int sff_modsel_set(struct sff_obj_t *sff_obj, u8 value)
{
    return sff_io_set(SFF_IO_MODSEL_TYPE, sff_obj, value);
}

int sff_modsel_get(struct sff_obj_t *sff_obj, u8 *value)
{
   return sff_io_get(SFF_IO_MODSEL_TYPE, sff_obj, value);
}

int sff_reset_get(struct sff_obj_t *sff_obj, u8 *value)
{
    return sff_io_get(SFF_IO_RST_TYPE, sff_obj, value);
}

int sff_reset_set(struct sff_obj_t *sff_obj, u8 value)
{
    return sff_io_set(SFF_IO_RST_TYPE, sff_obj, value);
}

int sff_intr_get(struct sff_obj_t *sff_obj, u8 *value)
{
    return sff_io_get(SFF_IO_INTR_TYPE, sff_obj, value);
}

int sff_txdisable_set(struct sff_obj_t *sff_obj, u8 value)
{
    return sff_io_set(SFF_IO_TXDISABLE_TYPE, sff_obj, value);
}

int sff_txdisable_get(struct sff_obj_t *sff_obj, u8 *value)
{
    return sff_io_get(SFF_IO_TXDISABLE_TYPE, sff_obj, value);
}

int sff_rxlos_get(struct sff_obj_t *sff_obj, u8 *value)
{
    return sff_io_get(SFF_IO_RXLOS_TYPE, sff_obj, value);
}

int sff_txfault_get(struct sff_obj_t *sff_obj, u8 *value)
{
    return sff_io_get(SFF_IO_TXFAULT_TYPE, sff_obj, value);
}
/*the function handles sff_fsm_st transistion ,
and all the states listed here only be triggered by events
sff_fsm_task will do nothing in those states except for following
-
SFF_FSM_ST_DETECTED

*/
static void sff_super_fsm_st_chg_process(struct sff_obj_t *sff_obj,
        sff_super_fsm_st_t cur_st,
        sff_super_fsm_st_t next_st)
{
    if (cur_st != next_st) {

        if (p_valid(sff_obj->lc_name) && p_valid(sff_obj->name)) {
            SWPS_LOG_DBG("%s %s st change:%s -> %s\n",
                         sff_obj->lc_name, sff_obj->name, sff_super_fsm_st_str[cur_st],
                         sff_super_fsm_st_str[next_st]);
        }
        switch (next_st) {

        case SFF_SUPER_FSM_ST_ISOLATED:
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_ISOLATED);
            break;
        case SFF_SUPER_FSM_ST_RUN:

            if (cur_st == SFF_SUPER_FSM_ST_MODULE_DETECT) {
                sff_fsm_st_set(sff_obj, SFF_FSM_ST_DETECTED);
            } else {
                SWPS_LOG_ERR("unexpected! %s %s st change:%s -> %s\n",
                             sff_obj->lc_name, sff_obj->name, sff_super_fsm_st_str[cur_st],
                             sff_super_fsm_st_str[next_st]);
                sff_fsm_st_set(sff_obj, SFF_FSM_ST_DETECTED);
            }
            break;

        case SFF_SUPER_FSM_ST_REMOVED:
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_REMOVED);
            break;
        case SFF_SUPER_FSM_ST_IDLE:
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_IDLE);
            break;

        case SFF_SUPER_FSM_ST_SUSPEND:
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_SUSPEND);
            break;
        case SFF_SUPER_FSM_ST_RESTART:
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_RESTART);
            break;

        default:
            break;
        }
    }
}

static inline void sff_fsm_timeout_set(struct sff_obj_t *sff_obj, int timeout)
{
    sff_obj->fsm.timeout.en = true;
    sff_obj->fsm.timeout.num = timeout;
    sff_obj->fsm.timeout.cnt = 0;
}    

static inline void sff_fsm_timeout_unset(struct sff_obj_t *sff_obj)
{
    sff_obj->fsm.timeout.en = false;
    sff_obj->fsm.timeout.cnt = 0;
    //SWPS_LOG_DBG("%s \n", sff_obj->name);
}    

static inline bool sff_fsm_timeout_required(struct sff_obj_t *sff_obj, sff_fsm_state_t next_st)
{
    bool found = false;
    int idx = 0;
    struct sff_fsm_t *fsm = &(sff_obj->fsm);
    struct fsm_period_t *table = fsm->period_tbl;
    bool required = false;
    
    for (idx = 0; table[idx].st != SFF_FSM_ST_END; idx++) {
        if (next_st == table[idx].st) {
            found = true;
            break;
        }
    }

    if (found) {
        required = table[idx].timeout_required;
    } 
    return required;
}    

static inline bool sff_fsm_timeout_enabled(struct sff_obj_t *sff_obj)
{
    return sff_obj->fsm.timeout.en;
}    

static inline bool sff_fsm_is_timeout(struct sff_obj_t *sff_obj)
{
    bool is_timeout = false;
    
    if (++(sff_obj->fsm.timeout.cnt) >= sff_obj->fsm.timeout.num) {
        sff_fsm_timeout_unset(sff_obj);
        is_timeout = true;
        SWPS_LOG_DBG("%s timeout occurs\n", sff_obj->name);
    }
    return is_timeout;
}    

void sff_fsm_st_chg_process(struct sff_obj_t *sff_obj,
                            sff_fsm_state_t cur_st,
                            sff_fsm_state_t next_st)
{
    struct lc_obj_t *lc = sff_to_lc(sff_obj->sff);
    struct pltfm_func_t *pltfm_func = lc->swps->pltfm_func;
    int ret = 0;
    
    if (cur_st != next_st) {

        sff_fsm_delay_cnt_reset(sff_obj, next_st);
        if (p_valid(sff_obj->lc_name) && p_valid(sff_obj->name)) {
            SWPS_LOG_DBG("%s %s st change:%s -> %s\n",
                         sff_obj->lc_name, sff_obj->name, sff_fsm_st_str[cur_st],
                         sff_fsm_st_str[next_st]);
        }
        if (sff_fsm_timeout_required(sff_obj, next_st)) {
            sff_fsm_timeout_set(sff_obj, SFF_FSM_DEFAULT_TIMEOUT);
        } else {
            sff_fsm_timeout_unset(sff_obj);
        }
        switch (next_st) {

        case SFF_FSM_ST_READY:
            if (p_valid(pltfm_func->sff_get_ready_action)) {
                ret = pltfm_func->sff_get_ready_action(sff_obj->lc_id, sff_obj->port);
            }            
            SWPS_LOG_INFO("%s %s st change to %s\n",
                         sff_obj->lc_name, sff_obj->name,
                         sff_fsm_st_str[next_st]);
            break; 

        case SFF_FSM_ST_DETECTED:
            if (p_valid(pltfm_func->sff_detected_action)) {
                ret = pltfm_func->sff_detected_action(sff_obj->lc_id, sff_obj->port);
            }            
            break;    
        case SFF_FSM_ST_TIMEOUT:
            SWPS_LOG_INFO("%s %s st change to %s\n",
                         sff_obj->lc_name, sff_obj->name,
                         sff_fsm_st_str[next_st]);
            break;
        default:
            break;
        }
        if (ret < 0) {
            if (p_valid(sff_obj->lc_name) && p_valid(sff_obj->name)) {
                SWPS_LOG_ERR("%s %s st change:%s -> %s action fail\n",
                             sff_obj->lc_name, sff_obj->name, sff_fsm_st_str[cur_st],
                             sff_fsm_st_str[next_st]);
            }
        }
    }
}
/*io_no_init is special functions when remove and re insert modules don't do sff io control to avoid cause port flap*/
static void io_no_init_port_done_set(struct sff_obj_t *sff_obj)
{
    unsigned long *bitmap = &(sff_obj->sff->io_no_init_port_done);
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
        sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_IO_NOINIT);
        sff_kobj_add(sff_obj);
    } else {
        sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_REMOVED);
        sff_kobj_del(sff_obj);
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
    card_done = &(card->swps->io_no_init_all_done);
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
static void io_no_init_handler(struct swps_mgr_t *self)
{
    int i = 0;
    int lc_num = self->lc_num;
    unsigned long all_done = self->io_no_init_all_done;
    unsigned long lc_ready = 0;
    lc_fsm_st_t st = LC_FSM_ST_IDLE;

    if (io_no_init) {
        for (i = 0; i < lc_num; i++) {
            st = lc_fsm_st_get(&self->obj[i]);
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
static sff_type detected_module_type_get(u8 id)
{
    sff_type type = SFF_UNKNOWN_TYPE;

    switch (id) {

    case SFF_8024_ID_SFP:
    case SFF_8024_ID_SFP2:
        type = SFP_TYPE;
        break;
    case SFF_8024_ID_QSFP:
    case SFF_8024_ID_QSFP_PLUS:
    case SFF_8024_ID_QSFP28:
        type = QSFP_TYPE;

        break;
    case SFF_8024_ID_QSFP_DD:
        type = QSFP_DD_TYPE;
        break;
    case SFF_8024_ID_SFP_DD:
        type = SFP_DD_TYPE;
        break;
    case SFF_8024_ID_QSFP56:
        type = QSFP56_TYPE;
        break;
    default:
        break;
    }
    return type;
}
static int sff_id_get(struct sff_obj_t *sff_obj, u8 *id)
{
    return sff_eeprom_read(sff_obj, SFF_EEPROM_I2C_ADDR, SFF_8024_ID_OFFSET, id, 1);
}
/* <reserved>
static int sff_status_get(struct sff_obj_t *sff_obj, u8 *status)
{
    return sff_eeprom_read(sff_obj, SFF_EEPROM_I2C_ADDR, SFF_STATUS_OFFSET, status, 1);
}
*/
static bool module_type_is_valid(sff_type detected_type)
{
    bool valid = false;
    
    switch (detected_type) {
    case SFP_DD_TYPE: 
    case SFP_TYPE:
    case QSFP_DD_TYPE:
    case QSFP_TYPE:
    case QSFP56_TYPE:
        valid = true;
        break;

    default:
        break;
    }
    return valid;
}
static int (*fsm_task_find(sff_type type))(struct sff_obj_t *)
{
    int (*task)(sff_obj_type *sff_obj) = NULL;

    task = sff_fsm_task_map[type];
    return task;
}

static int _sff_super_fsm_run(struct sff_obj_t *sff_obj)
{
    sff_super_fsm_st_t st = SFF_SUPER_FSM_ST_IDLE;
    int ret = 0;
    u8 lv = 0;
    u8 power = 0;
    u8 lpmode = 0;
    u8 rst = 0;
    u8 id = 0;
    sff_type detected_type = SFF_UNKNOWN_TYPE;

    st = sff_super_fsm_st_get(sff_obj);
    switch (st) {

    case SFF_SUPER_FSM_ST_IDLE:
    case SFF_SUPER_FSM_ST_UNSUPPORT:
    case SFF_SUPER_FSM_ST_ISOLATED:
        break;
    case SFF_SUPER_FSM_ST_REMOVED:

        check_pfunc(sff_obj->func_tbl->remove_op);
        if ((ret = sff_obj->func_tbl->remove_op(sff_obj)) < 0) {
            break;
        }
        sff_obj->type = sff_obj->def_type;
        sff_obj->init_op_required = false;
        sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_IDLE);
        break;
    case SFF_SUPER_FSM_ST_INSERTED:

        if ((ret = sff_power_get(sff_obj, &power)) < 0) {
            return ret;
        }

        if (!power) {
            SWPS_LOG_INFO("%s power is not on:%d\n",sff_obj->name, power);
            sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_SUSPEND);
            break;
        }
        if (sff_reset_is_supported(sff_obj)) {
            
            if ((ret = sff_reset_get(sff_obj, &rst)) < 0) {
                return ret;
            }
            if (!rst) {
                SWPS_LOG_INFO("%s rst is asserted:%d\n",sff_obj->name, rst);
                sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_SUSPEND);
                break;
            }
        }
        sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_WAIT_STABLE);
        sff_obj->super_fsm.stable_cnt = 0;
        if (SFP_DD_TYPE == sff_obj->type) {
            if ((ret = sff_lpmode_get(sff_obj, &lpmode)) < 0) {
                return ret;
            }
            if (1 == lpmode) {
                sff_obj->super_fsm.stable_num = SFF_SUPER_FSM_SFPDD_SW_STABLE_NUM;
            } else {
                sff_obj->super_fsm.stable_num = SFF_SUPER_FSM_SFPDD_HW_STABLE_NUM;
            }
        } else {
            sff_obj->super_fsm.stable_num = SFF_SUPER_FSM_NORMAL_STABLE_NUM;
        }
        break;
    case SFF_SUPER_FSM_ST_WAIT_STABLE:
        if (++(sff_obj->super_fsm.stable_cnt) >= sff_obj->super_fsm.stable_num) {
            sff_obj->super_fsm.stable_cnt = 0;
            sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_MODULE_DETECT);
            sff_obj->super_fsm.timeout_cnt = 0;
        }
        break;
    case SFF_SUPER_FSM_ST_MODULE_DETECT:
        if (++(sff_obj->super_fsm.timeout_cnt) >= SFF_SUPER_FSM_TIMEOUT_NUM) {
            sff_obj->super_fsm.timeout_cnt = 0;
            sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_TIMEOUT);
            break;
        }
        if ((ret = sff_id_get(sff_obj, &id)) < 0) {
            break;
        }
        if (0 == id) {
            break;
        } 

        detected_type = detected_module_type_get(id);
        if (module_type_is_valid(detected_type)) {
            sff_obj->type = detected_type;
            SWPS_LOG_DBG("%s detected_module_type ok\n", sff_obj->name);
        } else {
            sff_obj->type = sff_obj->def_type;
            sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_UNSUPPORT);
            SWPS_LOG_DBG("%s unsupported_module_type ok\n", sff_obj->name);
            break;
        }
        /*load func_tbl*/
        if ((ret = func_tbl_init(sff_obj, sff_obj->type)) < 0) {
            break;
        }
        if (sff_obj->init_op_required) {
            check_pfunc(sff_obj->func_tbl->init_op);
            if ((ret = sff_obj->func_tbl->init_op(sff_obj)) < 0) {
                break;
            }
            sff_obj->init_op_required = false;
        }
        /*add kobj and load corresponding sys attr group after type is detected*/
        sff_kobj_add(sff_obj);
        sff_obj->fsm.task = fsm_task_find(sff_obj->type);
        if (!p_valid(sff_obj->fsm.task)) {
            sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_UNSUPPORT);
            SWPS_LOG_DBG("%s unsupported_module_type ok\n", sff_obj->name);
            break;
        }
        sff_obj->page_sel_lock_time = 0;
        sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_RUN);
        break;

    case SFF_SUPER_FSM_ST_RUN:
        // SWPS_LOG_DBG("%s's page lock is %d (lock time is %ld)\n", sff_obj->name, page_sel_is_locked(sff_obj), sff_obj->page_sel_lock_time);
        if (sff_fsm_delay_cnt_is_hit(sff_obj)) {

            sff_obj->fsm.task(sff_obj);
        }
        sff_fsm_cnt_run(sff_obj);
        if (sff_fsm_timeout_enabled(sff_obj)) {
            if (sff_fsm_is_timeout(sff_obj)) {
                sff_fsm_st_set(sff_obj, SFF_FSM_ST_TIMEOUT);
            }
        }
        break;

    case SFF_SUPER_FSM_ST_SUSPEND:
        break;

    case SFF_SUPER_FSM_ST_RESTART:
        if ((ret = sff_prs_get(sff_obj, &lv)) < 0) {
            break;
        }
        if (lv) {
            sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_INSERTED);
        } else {
            sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_IDLE);
        }
        break;
    case SFF_SUPER_FSM_ST_IO_NOINIT:
        sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_RUN);
        break;
    case SFF_SUPER_FSM_ST_TIMEOUT:
        break;
    default:
        SWPS_LOG_ERR("unknown fsm st:%d\n", st);
        break;

    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}

static int sff_super_fsm_run(struct sff_mgr_t *sff)
{
    int port = 0;
    int ret = 0;
    struct sff_obj_t *sff_obj = NULL;
    int port_num = sff->valid_port_num;

    for(port = 0; port < port_num; port++) {
        sff_obj = &(sff->obj[port]);
        if ((ret = _sff_super_fsm_run(sff_obj)) < 0) {
            break;
        }
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
    check_pfunc(card->swps->lc_func->phy_ready);
    card->swps->lc_func->phy_ready(bitmap, is_ready);

    return 0;
}

static int lc_fsm_run_1u(struct lc_obj_t *card)
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
        if((ret = sff_objs_init(&(card->sff), port_info)) < 0) {
            SWPS_LOG_ERR("sff_objs_init fail\n");
            break;;
        }

        ret = lc_sff_mgr_kobj_create(card);
        if (ret < 0) {
            break;
        }

        lc_fsm_st_set(card, LC_FSM_ST_READY);
        io_no_init = 0;
        break;

    case LC_FSM_ST_READY:

        if ((ret = sff_prs_scan(&card->sff)) < 0) {
            break;
        }
        if ((ret = sff_super_fsm_run(&card->sff)) < 0) {
            break;
        }

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
    unsigned long *sys_ready = &(card->swps->lc_sys_ready);
    if (ready) {
        set_bit(card->lc_id, sys_ready);
    } else {
        clear_bit(card->lc_id, sys_ready);
    }
}
static int lc_fsm_run_4u(struct lc_obj_t *card)
{
    lc_fsm_st_t st = LC_FSM_ST_IDLE;
    int ret = 0;
    bool is_power_ready = false;
    bool is_phy_ready = false;
    bool asserted = false;
    bool deasserted = false;
    struct port_info_table_t *port_info = NULL;
    struct lc_func_t *lc_func = NULL;
    struct pltfm_func_t *pltfm_func = NULL;
    struct mux_ch_t *mux = NULL;
    lc_type_t type = LC_UNKNOWN_TYPE;

    if (!p_valid(card)) {
        SWPS_LOG_ERR("NULL ptr\n");
        return -EINVAL;
    }
    lc_func = card->swps->lc_func;
    pltfm_func = card->swps->pltfm_func;
    st = lc_fsm_st_get(card);
    
    switch (st) {

    case LC_FSM_ST_IDLE:
        break;
    case LC_FSM_ST_REMOVE:

        if (i2c_recovery_is_muxdrv_method()) {
            mux = &(card->mux_l1);
            if (0 != mux->block_ch) {
                mux->block_ch = 0;
                pltfm_func->mux_blocked_ch_set(card->lc_id, mux->block_ch);
                pltfm_func->mux_fail_set(card->lc_id, false);
            }
        }
        
        sff_kobjs_destroy(&card->sff);
        _lc_sff_mgr_kobj_destroy(card);
        _lc_kobj_destroy(card);
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

        lc_fsm_st_set(card, LC_FSM_ST_SW_CONFIG);
        check_pfunc(lc_func->reset_set);
        if ((ret = lc_func->reset_set(card->lc_id, 0)) < 0) {
            break;
        }
        break;
    case LC_FSM_ST_SW_CONFIG:
        check_pfunc(lc_func->type_get);
        if ((ret = lc_func->type_get(card->lc_id, &type)) < 0) {
            card->type = LC_UNKNOWN_TYPE;
            break;
        }
        card->type = type;
        if (is_pltfm_4U_type()) { /*<TBD> this may be only for specific customer*/
            if (LC_100G_TYPE != type) {
                SWPS_LOG_ERR("platform %s only support 100G card\n", pltfmInfo->name);
                lc_fsm_st_set(card, LC_FSM_ST_UNSUPPORTED);
                break;
            }
        }
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
        if((ret = sff_objs_init(&(card->sff), port_info)) < 0) {
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
            check_pfunc(lc_func->led_boot_amber_set);
            if ((ret = lc_func->led_boot_amber_set(card->lc_id, false)) < 0) {
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
        
        check_pfunc(lc_func->intr_hdlr);
        if ((ret = lc_func->intr_hdlr(card->lc_id)) < 0) {
            break;
        }

        if ((ret = card->sff.prs_scan(&card->sff)) < 0) {
            break;
        }

        if ((ret = sff_super_fsm_run(&card->sff)) < 0) {
            break;
        }
        
        if (i2c_bus_recovery_is_supported()) {
            if (!i2c_bus_is_alive(card)) {
                i2c_bus_recovery(card);
            }
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

static int swps_fsm_run_1u(struct swps_mgr_t *self)
{
    int ret = 0;
    ret = lc_fsm_run_1u(&self->obj[0]);
    if (ret < 0) {
        return ret;
    }
    return 0;
}
/*i2c bus check and recovery functons*/
static bool i2c_bus_is_alive(struct lc_obj_t *card)
{
    struct pltfm_func_t *pltfm_func = card->swps->pltfm_func;
    if (NULL == pltfm_func->i2c_is_alive) {
        SWPS_LOG_ERR("no function\n");
        return false;
    }
    return pltfm_func->i2c_is_alive(card->lc_id);
}
static void mux_reset_seq(struct lc_obj_t *card)
{
    struct pltfm_func_t *pltfm_func = card->swps->pltfm_func;
    if (NULL == pltfm_func->mux_reset_set) {
        SWPS_LOG_ERR("no function\n");
        return;
    }

    pltfm_func->mux_reset_set(card->lc_id, 0);
    msleep(1);
    pltfm_func->mux_reset_set(card->lc_id, 1);
}
/*old method keep it temporary*/
static void bad_transvr_detect_native(struct lc_obj_t *card)
{
    int port = 0;
    struct sff_obj_t *sff_obj = NULL;
    int port_num = card->sff.valid_port_num;
    u8 id = 0;
    int ret = 0;
    mux_reset_seq(card);
    mux_reset_ch_resel_byLC(card);
    SWPS_LOG_DBG("%s start\n", card->name);
    for (port = 0; port < port_num; port++) {

        sff_obj = &(card->sff.obj[port]);
        if (SFF_SUPER_FSM_ST_ISOLATED == sff_super_fsm_st_get(sff_obj)) {
            continue;
        }
        if (SFF_SUPER_FSM_ST_IDLE != sff_super_fsm_st_get(sff_obj)) {
            id = 0 ;
            if ((ret = sff_id_get(sff_obj, &id)) >= 0) {
                if (id != 0) {
                    SWPS_LOG_DBG("%s id matched:0x%x\n", sff_obj->name, id);
                    continue;
                }
            } 
            
            if (!i2c_bus_is_alive(card)) {
                SWPS_LOG_DBG("transvr in %s isolated\n", sff_obj->name);
                sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_ISOLATED);
                mux_reset_seq(card);
                mux_reset_ch_resel_byLC(card);
            } else {

                SWPS_LOG_DBG("%s i2c bus is alive\n", sff_obj->name);
            }
        }

    }

    SWPS_LOG_DBG("bad transvr report == \n");
    for (port = 0; port < port_num; port++) {

        sff_obj = &(card->sff.obj[port]);
        if (SFF_SUPER_FSM_ST_ISOLATED == sff_super_fsm_st_get(sff_obj)) {
            SWPS_LOG_DBG("transvr in %s isolated\n", sff_obj->name);
        }
    }
    SWPS_LOG_DBG("bad transvr report == \n");
}

static int num_to_power_two(unsigned long num)
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
/*<note> this function need to go with customized inv pca9548 mux driver*/
static void bad_transvr_detect_muxdrv(struct lc_obj_t *card)
{
    unsigned long failed_ch = 0;
    int port = 0;
    int ch = 0;
    bool is_fail = false;
    struct mux_ch_t *mux = NULL;
    struct sff_obj_t *sff_obj = NULL;
    unsigned long block_all = 0;
    struct pltfm_func_t *pltfm_func = card->swps->pltfm_func;    
    int lc_id = card->lc_id;
    mux = &card->mux_l1;

    /*step 1 check if mux failed*/
    pltfm_func->mux_fail_get(lc_id, &is_fail);
    if (is_fail) {
        /*step 2 block all mux ch*/
        block_all = (1L << (card->sff.valid_port_num)) - 1;
        pltfm_func->mux_blocked_ch_set(lc_id, block_all); 
        SWPS_LOG_ERR("block all ch first: 0x%lx\n", block_all);
        /*get current failed ch*/
        pltfm_func->mux_failed_ch_get(lc_id, &failed_ch);
        /*print out info */
        SWPS_LOG_ERR("mux fail_ch:0x%lx \n", failed_ch);
    }
    /*step 3 reset mux*/
    mux_reset_seq(card);
    /*step 4 block failed mux ch*/
    if (is_fail && failed_ch != 0) {
        ch = num_to_power_two(failed_ch);
        if (ch >= card->sff.valid_port_num) {
            SWPS_LOG_ERR("failed_ch:%d out of range\n", ch);
            pltfm_func->mux_blocked_ch_set(lc_id, mux->block_ch);
            return;
        }
        
        set_bit(ch, &(mux->block_ch));
        pltfm_func->mux_blocked_ch_set(lc_id, mux->block_ch); 
        /*following is for set sff_fsm_st of the port to isolated state*/
        port = pltfm_func->mux_ch_to_port(lc_id, ch);
        if (port < 0 || port >= card->sff.valid_port_num) {
            SWPS_LOG_ERR("port out of range: %d\n", port);
            return;
        }
        if (!p_valid(sff_obj = &(card->sff.obj[port]))) {
            SWPS_LOG_ERR("NULL ptr port_id:%d\n", port);
            return;
        }
        SWPS_LOG_ERR("block_ch:0x%lx %s\n", mux->block_ch, sff_obj->name);
        sff_super_fsm_st_set(sff_obj, SFF_SUPER_FSM_ST_ISOLATED);
    }
}

static bool i2c_bus_recovery_is_supported(void)
{
    return pltfmInfo->i2c_recovery_feature.en;
}
static void i2c_bus_recovery(struct lc_obj_t *card)
{
    if (i2c_recovery_is_muxdrv_method()) {
        bad_transvr_detect_muxdrv(card);
    } else {
        bad_transvr_detect_native(card);
    }
    SWPS_LOG_ERR("mux reset done\n");
    if (i2c_bus_is_alive(card)) {
        SWPS_LOG_ERR("i2c bus recovery done\n");
    } else {
        SWPS_LOG_ERR("i2c bus recovery fail\n");
    }

}
static int swps_fsm_run_4u(struct swps_mgr_t *self)
{
    int i = 0;
    int lc_num = self->lc_num;
    int ret = 0;

    for (i = 0; i < lc_num; i++) {
        ret = lc_fsm_run_4u(&self->obj[i]);

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

static int func_tbl_init(struct sff_obj_t *obj, int type)
{
    obj->func_tbl = sff_func_tbl_map[type];

    if (!p_valid(obj->func_tbl)) {
        SWPS_LOG_ERR("load func fail!\n");
        return -ENOSYS;
    }
    return 0;
}
static int sff_fsm_init(struct sff_obj_t *obj, int type)
{
    struct sff_fsm_t *fsm = NULL;
    struct sff_super_fsm_t *super_fsm = NULL;

    fsm = &(obj->fsm);
    super_fsm = &(obj->super_fsm);

    fsm->task = fsm_task_find(type);

    if (!p_valid(fsm->task)) {
        SWPS_LOG_ERR("%s unknown task!\n", obj->name);
        return -ENOSYS;
    }
    fsm->period_tbl = fsm_period_tbl;
    fsm->st = SFF_FSM_ST_IDLE;
    fsm->cnt = 0;
    fsm->delay_cnt = 0;
    super_fsm->st = SFF_SUPER_FSM_ST_IDLE;
    super_fsm->stable_cnt = 0;
    return 0;
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
static int drv_load(struct swps_mgr_t *self)
{
    int lc_num = self->lc_num;
    int lc_id = 0;

    if (!p_valid(pltfmInfo->io_drv)) {
        return -ENOSYS;
    }

    if (!p_valid(pltfmInfo->eeprom_drv)) {
        return -ENOSYS;
    }

    for (lc_id = 0; lc_id < lc_num; lc_id++) {
        self->obj[lc_id].sff.io_drv = pltfmInfo->io_drv;
        self->obj[lc_id].sff.eeprom_drv = pltfmInfo->eeprom_drv;
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
        SWPS_LOG_ERR("sff_eeprom_init fail\n");
        goto exit_err;
    }
    sff_eeprom_port_num_set(maxPortNum);

    if (pltfm_func_load(&swps_mgr) < 0) {
        SWPS_LOG_ERR("pltfm_func_load fail\n");
        goto exit_err;
    }
    if (is_pltfm_4U_type()) {
        if (lc_func_load(&swps_mgr) < 0) {
            SWPS_LOG_ERR("lc_func_load fail\n");
            goto exit_err;
        }
    }
    
    if (polling_task_load(&swps_mgr) < 0) {
        SWPS_LOG_ERR("polling_task_load fail\n");
        goto exit_err;
    }
    
    if (swps_mgr.pltfm_func->init(pltfmInfo->id, io_no_init) < 0) {
        SWPS_LOG_ERR("pltfm_init fail\n");
        goto exit_err;
    }

    if (lc_create_init(&swps_mgr, pltfmInfo->lc_num, maxPortNum) < 0) {
        SWPS_LOG_ERR("lc_create_init fail\n");
        goto exit_err;
    }
    
    if (drv_load(&swps_mgr) < 0) {
        SWPS_LOG_ERR("drv load fail\n");
        goto exit_err;
    }

    if(inv_swps_polling_is_enabled()) {
        swps_polling_task_start();
    }
    SWPS_LOG_INFO("ok! platform:%s VER:%s\n", pltfmInfo->name, SWPS_VERSION);
    return 0;

exit_err:
    return -EBADRQC;
}

static void __exit swps_exit(void)
{
    struct mux_ch_t *mux = NULL;
    int lc_id = 0;

    if(inv_swps_polling_is_enabled()) {
        swps_polling_task_stop();
    }

    sff_eeprom_deinit();
    swps_mgr.pltfm_func->deinit();

    lc_sff_kobjs_destroy(&swps_mgr);
    lc_kobj_destroy(&swps_mgr);
    swps_common_kobj_destroy(&swps_mgr);
    sff_kset_deinit();
    if (i2c_recovery_is_muxdrv_method()) {
        for (lc_id = 0; lc_id < swps_mgr.lc_num; lc_id++) {
            mux = &(swps_mgr.obj[lc_id].mux_l1);
            if (0 != mux->block_ch) {
                swps_mgr.pltfm_func->mux_blocked_ch_set(lc_id, 0);
                swps_mgr.pltfm_func->mux_fail_set(lc_id, false);
            }
        }
    }
    lc_sff_frontPort_remap_destroy(&swps_mgr);
    lc_sff_objs_destroy(&swps_mgr);
    lc_objs_destroy(&swps_mgr);

    SWPS_LOG_INFO("ok! platform:%s VER:%s\n", pltfmInfo->name, SWPS_VERSION);
}

int inv_swps_eeprom_init(int lc_id)
{
    int ret = 0;
    (void)(lc_id);
    if ((ret = sff_eeprom_init(pltfmInfo->id)) < 0) {
        SWPS_LOG_ERR("sff_eeprom_init fail\n");
        
    }
    SWPS_LOG_DBG("done\n");
    return 0;
}    
EXPORT_SYMBOL(inv_swps_eeprom_init);

void inv_swps_eeprom_deinit(int lc_id)
{
    (void)(lc_id);
    sff_eeprom_deinit();
    SWPS_LOG_DBG("done\n");
}    
EXPORT_SYMBOL(inv_swps_eeprom_deinit);

module_init(swps_init);
module_exit(swps_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alang <huang.alang@inventec.com>");
