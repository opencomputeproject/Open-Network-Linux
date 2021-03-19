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
#include "net_swps.h"
#include "qsfp_dd.h"

/*page 0x00 register*/
#define QSFP_DD_ID_OFFSET   (0)
#define QSFP_DD_ST_INDICATOR2_OFFSET (2)
#define FLAT_MEM_BIT (7)
#define QSFP_DD_MODULE_GLOBAL_CONTROL_OFFSET  (26)
#define QSFP_DD_MODULE_GLOBAL_CONTROL_LOW_PWR_BIT (6)
#define QSFP_DD_CMIS_REV_OFFSET (1)
#define QSFP_DD_BANK_SEL_OFFSET (126)
#define QSFP_DD_MODULE_TYPE_OFFSET  (85)

/*page 0x10 register*/
#define QSFP_DD_DATA_PATH_DEINIT_OFFSET (128)
#define QSFP_DD_PAGE_SEL_OFFSET (127)
#define QSFP_DD_DATA_PATH_PWR_UP_OFFSET    (128)
#define QSFP_DD_TX_DISABLE_OFFSET   (130)
#define QSFP_DD_CMIS_REV4_VAL (0x40)
#define QSFP_DD_CMIS_REV3_VAL (0x30)
#define QSFP_DD_VCC_OFFSET (16)
#define QSFP_DD_TEMP_OFFSET (14)
#define QSFP_DD_STAGE_CTRL_SET_ZERO_APPL_SEL_CTRL_OFFSET (145)
#define QSFP_DD_STAGE_CTRL_SET_ZERO_DATA_PATH_INIT_OFFSET (143)    
/*page 0x11*/
#define QSFP_DD_DATA_PATH_ST_OFFSET (128)
#define QSFP_DD_DATA_PATH_ST_NUM (4)
#define QSFP_DD_CONFIG_ERR_CODE_OFFSET (202)
#define QSFP_DD_ACTIVE_CTRL_SET_OFFSET (206)
#define QSFP_DD_LN_MONITOR_TX_PWR_OFFSET (154)
#define QSFP_DD_LN_MONITOR_RX_PWR_OFFSET (186)
#define QSFP_DD_LN_MONITOR_TX_BIAS_OFFSET (170)
#define QSFP_DD_VENDOR_NAME_OFFSET (129)
#define QSFP_DD_VENDOR_PN_OFFSET (148)
#define QSFP_DD_VENDOR_SN_OFFSET (166)
#define QSFP_DD_VENDOR_REV_OFFSET (164)

#define QSFP_DD_VENDOR_NAME_LEN (16)
#define QSFP_DD_VENDOR_PN_LEN (16)
#define QSFP_DD_VENDOR_SN_LEN (16)
#define QSFP_DD_VENDOR_REV_LEN (2)

#define QSFP_DD_MODULE_ST_INT_OFFSET (0x03)
#define QSFP_DD_MODULE_ST_BIT_MIN (1)
#define QSFP_DD_MODULE_ST_BIT_NUM (3)
#define QSFP_DD_INTR_LN_FLAG_DATA_PATH_CHG_OFFSET (134)
#define QSFP_DD_INTR_LN_FLAG_START_OFFSET (QSFP_DD_INTR_LN_FLAG_DATA_PATH_CHG_OFFSET)

extern u32 logLevel;
extern bool int_flag_monitor_en;
/*qsfp-dd*/
const u8 app_advert_fields_offset[APSEL_NUM][APP_ADVERT_FIELD_NUM] = {
    { 86, 87, 88, 89, 176},
    { 90, 91, 92, 93, 177},
};
u8 app_advert_fields_page[APP_ADVERT_FIELD_NUM] = {
    QSFP_PAGE0,
    QSFP_PAGE0,
    QSFP_PAGE0,
    QSFP_PAGE0,
    QSFP_PAGE_01h,
};

const char *intr_module_flag_str[QSFP_DD_INT_MODULE_FLAG_NUM] = {
    "FW_FAULT",
    "L_VCC_TEMP_WARN",
    "L_AUX_ALARM_WARN",
    "VENDOR_DEFINED_ERR",
    "RESERVERD",
    "CUSTOM",
};

enum {
    DATA_PATH_CHG_ID,
    L_TX_FAULT_ID,
    L_TX_LOS_ID,
    L_TX_CDR_LOL_ID,
    L_TX_APAP_EQ_INPUT_FAULT_ID,
    L_TX_POWER_HIGH_ALARM_ID,
    L_TX_POWER_LOW_ALARM_ID,
    L_TX_POWER_HIGH_WARN_ID,
    L_TX_POWER_LOW_WARN_ID,
    L_TX_BIAS_HIGH_ALARM_ID,
    L_TX_BIAS_LOW_ALARM_ID,
    L_TX_BIAS_HIGH_WARN_ID,
    L_TX_BIAS_LOW_WARN_ID,
    L_RX_LOS_ID,
    L_RX_CDR_LOL_ID,
    L_RX_POWER_HIGH_ALARM_ID,
    L_RX_POWER_LOW_ALARM_ID,
    L_RX_POWER_HIGH_WARN_ID,
    L_RX_POWER_LOW_WARN_ID,
};

const char *intr_ln_flag_str[QSFP_DD_INT_LN_FLAG_NUM] = {

    "DATA_PATH_CHG",
    "L_TX_FAULT",
    "L_TX_LOS",
    "L_TX_CDR_LOL",
    "L_TX_APAP_EQ_INPUT_FAULT",
    "L_TX_POWER_HIGH_ALARM",
    "L_TX_POWER_LOW_ALARM",
    "L_TX_POWER_HIGH_WARN",
    "L_TX_POWER_LOW_WARN",
    "L_TX_BIAS_HIGH_ALARM",
    "L_TX_BIAS_LOW_ALARM",
    "L_TX_BIAS_HIGH_WARN",
    "L_TX_BIAS_LOW_WARN",
    "L_RX_LOS",
    "L_RX_CDR_LOL",
    "L_RX_POWER_HIGH_ALARM",
    "L_RX_POWER_LOW_ALARM",
    "L_RX_POWER_HIGH_WARN",
    "L_RX_POWER_LOW_WARN",
};
static int app_advertising_field_get(struct sff_obj_t *sff_obj);
#if 0
static int qsfp_dd_active_control_set_indicator(struct sff_obj_t *sff_obj);
#endif
static int qsfp_dd_data_path_deinit(struct sff_obj_t *sff_obj, u8 val);
static int data_path_power_up(struct sff_obj_t *sff_obj, bool up);
static int qsfp_dd_is_data_path_activated(struct sff_obj_t *sff_obj, bool *activated);
static int qsfp_dd_lowPwr_set(struct sff_obj_t *sff_obj);
static int stage_control_set0(struct sff_obj_t *sff_obj);
static int config_error_code_check(struct sff_obj_t *sff_obj, bool *is_pass);
static int advert_check(struct sff_obj_t *sff_obj, bool *pass);
static int module_ready_check_rev3(struct sff_obj_t *sff_obj, bool *ready);
static int module_ready_check_rev4(struct sff_obj_t *sff_obj, bool *ready);

static int sw_config_1_rev3(struct sff_obj_t *sff_obj);
static int sw_config_2_rev3(struct sff_obj_t *sff_obj, bool *pass);
static int sw_config_check_rev3(struct sff_obj_t *sff_obj, bool *pass);
static int sw_control_rev3(struct sff_obj_t *sff_obj);

static int sw_config_1_rev4(struct sff_obj_t *sff_obj);
static int sw_config_2_rev4(struct sff_obj_t *sff_obj, bool *pass);
static int sw_config_check_rev4(struct sff_obj_t *sff_obj, bool *pass);
static int sw_control_rev4(struct sff_obj_t *sff_obj);

static int sw_config_1_rev4_full(struct sff_obj_t *sff_obj);
static int sw_config_2_rev4_full(struct sff_obj_t *sff_obj, bool *pass);
static int sw_config_check_rev4_full(struct sff_obj_t *sff_obj, bool *pass);
static int sw_control_rev4_full(struct sff_obj_t *sff_obj);

inline static struct qsfp_dd_priv_data *qsfp_dd_priv_get(struct sff_obj_t *sff_obj)
{
    return &(sff_obj->priv_data.qsfp_dd);
}    
struct qsfp_dd_fsm_func_t qsfp_dd_FsmFuncRev3 = {

    .advert_check = advert_check,
    .sw_config_1 = sw_config_1_rev3,
    .sw_config_2 = sw_config_2_rev3,
    .sw_config_check = sw_config_check_rev3,
    .sw_control = sw_control_rev3,
    .module_ready_check = module_ready_check_rev3
};
/*quick sw init*/
struct qsfp_dd_fsm_func_t qsfp_dd_FsmFuncRev4 = {

    .advert_check = advert_check,
    .sw_config_1 = sw_config_1_rev4,
    .sw_config_2 = sw_config_2_rev4,
    .sw_config_check = sw_config_check_rev4,
    .sw_control = sw_control_rev4,
    .module_ready_check = module_ready_check_rev4
};

/*full sw init*/
struct qsfp_dd_fsm_func_t qsfp_dd_FsmFuncRev4_full = {

    .advert_check = advert_check,
    .sw_config_1 = sw_config_1_rev4_full,
    .sw_config_2 = sw_config_2_rev4_full,
    .sw_config_check = sw_config_check_rev4_full,
    .sw_control = sw_control_rev4_full,
    .module_ready_check = module_ready_check_rev4
};

static int qsfp_dd_paging_supported(struct sff_obj_t *sff_obj, bool *supported);
static int _page_switch(struct sff_obj_t *sff_obj, u8 page);
static int qsfp_dd_lpmode_set(struct sff_obj_t *sff_obj, u8 value);
static int qsfp_dd_lpmode_get(struct sff_obj_t *sff_obj, u8 *value);
static int qsfp_dd_intL_get(struct sff_obj_t *sff_obj, u8 *value);
static int qsfp_dd_reset_set(struct sff_obj_t *sff_obj, u8 value);
static int qsfp_dd_reset_get(struct sff_obj_t *sff_obj, u8 *value);
static int qsfp_dd_mode_sel_set(struct sff_obj_t *sff_obj, u8 value);
static int qsfp_dd_mode_sel_get(struct sff_obj_t *sff_obj, u8 *value);
int qsfp_dd_lane_monitor_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
int qsfp_dd_tx_disable_set(struct sff_obj_t *sff_obj, u8 val);
int qsfp_dd_tx_disable_get(struct sff_obj_t *sff_obj, u8 *val);
int qsfp_dd_id_get(struct sff_obj_t *sff_obj, u8 *val);
bool qsfp_dd_is_id_matched(struct sff_obj_t *sff_obj);
int qsfp_dd_vendor_info_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
int qsfp_dd_module_st_get(struct sff_obj_t *sff_obj, u8 *st);
int qsfp_dd_temperature_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
int qsfp_dd_voltage_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
int qsfp_dd_page_sel(struct sff_obj_t *sff_obj, int page);
int qsfp_dd_page_get(struct sff_obj_t *sff_obj, u8 *page);
int qsfp_dd_eeprom_dump(struct sff_obj_t *sff_obj, u8 *buf);
int qsfp_dd_ln_st_get(struct sff_obj_t *sff_obj, int type, u8 *st);
static void qsfp_dd_intr_flag_clear(struct sff_obj_t *sff_obj);
static int qsfp_dd_intr_flag_show(struct sff_obj_t *sff_obj, char *buf, int size);

struct func_tbl_t qsfp_dd_func_tbl = {
    .eeprom_read = sff_eeprom_read,
    .eeprom_write = sff_eeprom_write,
    .prs_get = sff_prs_get,
    .lpmode_set = qsfp_dd_lpmode_set,
    .lpmode_get = qsfp_dd_lpmode_get,
    .reset_set = qsfp_dd_reset_set,
    .reset_get = qsfp_dd_reset_get,
    .power_set = sff_power_set,
    .power_get = sff_power_get,
    .mode_sel_set = qsfp_dd_mode_sel_set,
    .mode_sel_get = qsfp_dd_mode_sel_get,
    .intL_get = qsfp_dd_intL_get,
    .tx_disable_set = qsfp_dd_tx_disable_set,
    .tx_disable_get = qsfp_dd_tx_disable_get,
    .temperature_get = qsfp_dd_temperature_get,
    .voltage_get = qsfp_dd_voltage_get,
    .lane_control_set = dummy_lane_control_set,
    .lane_control_get = dummy_lane_control_get,
    .lane_monitor_get = qsfp_dd_lane_monitor_get,
    .vendor_info_get = qsfp_dd_vendor_info_get,
    .lane_status_get =  qsfp_dd_ln_st_get,
    .module_st_get = qsfp_dd_module_st_get,
    .id_get = qsfp_dd_id_get,
    .is_id_matched = qsfp_dd_is_id_matched,
    .eeprom_dump = qsfp_dd_eeprom_dump,
    .page_sel = qsfp_dd_page_sel,
    .page_get = qsfp_dd_page_get,
    .intr_flag_show = qsfp_dd_intr_flag_show,
    .intr_flag_clear = qsfp_dd_intr_flag_clear,
};
struct func_tbl_t *qsfp_dd_func_load(void)
{
    return &qsfp_dd_func_tbl;
}    
static int qsfp_dd_lpmode_set(struct sff_obj_t *sff_obj, u8 value)
{
    int ret = 0;

    if ((ret = sff_obj->mgr->io_drv->lpmode_set(sff_obj->lc_id, sff_obj->port, value)) < 0) {
        return ret;
    }
    return 0;
}
static int qsfp_dd_lpmode_get(struct sff_obj_t *sff_obj, u8 *value)
{
    int ret = 0;

    if ((ret = sff_obj->mgr->io_drv->lpmode_get(sff_obj->lc_id, sff_obj->port, value)) < 0) {
        return ret;
    }
    return 0;
}
static int qsfp_dd_mode_sel_set(struct sff_obj_t *sff_obj, u8 value)
{
    int ret = 0;

    if ((ret = sff_obj->mgr->io_drv->mode_sel_set(sff_obj->lc_id, sff_obj->port, value)) < 0) {
        return ret;
    }
    return 0;

}
static int qsfp_dd_mode_sel_get(struct sff_obj_t *sff_obj, u8 *value)
{
    int ret = 0;

    if ((ret = sff_obj->mgr->io_drv->mode_sel_get(sff_obj->lc_id, sff_obj->port, value)) < 0) {
        return ret;
    }
    return 0;
}
static int qsfp_dd_reset_set(struct sff_obj_t *sff_obj, u8 value)
{
    int ret = 0;

    if ((ret = sff_obj->mgr->io_drv->reset_set(sff_obj->lc_id, sff_obj->port, value)) < 0) {
        return ret;
    }
    return 0;
}
static int qsfp_dd_reset_get(struct sff_obj_t *sff_obj, u8 *value)
{
    int ret = 0;

    if ((ret = sff_obj->mgr->io_drv->reset_get(sff_obj->lc_id, sff_obj->port, value)) < 0) {
        return ret;
    }
    return 0;
}
static int qsfp_dd_intL_get(struct sff_obj_t *sff_obj, u8 *value)
{
    int ret = 0;
    unsigned long bitmap = 0;

    if (!p_valid(value)) {
        return -EINVAL;
    }

    if ((ret = sff_obj->mgr->io_drv->intr_all_get(sff_obj->lc_id, &bitmap)) < 0) {
        return ret;
    }

    if (test_bit(sff_obj->port, &bitmap)) {
        *value = 1;
    } else {
        *value = 0;
    }
    return 0;
}

static int qsfp_dd_eeprom_read(struct sff_obj_t *sff_obj,
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
    if (offset > QSFP_DD_PAGE_SEL_OFFSET) {
        ret = _page_switch(sff_obj, page);
        if (ret < 0) {
            MODULE_LOG_ERR("%s switch page fail\n", sff_obj->name);
            page_sel_unlock(sff_obj);
            return ret;
        }
    }
    ret = sff_eeprom_read(sff_obj, addr, offset, buf, len);
    page_sel_unlock(sff_obj);
    return ret;
}
static int qsfp_dd_eeprom_write(struct sff_obj_t *sff_obj,
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
    if (offset > QSFP_DD_PAGE_SEL_OFFSET) {
        ret = _page_switch(sff_obj, page);
        if (ret < 0) {
            MODULE_LOG_ERR("%s switch page fail\n", sff_obj->name);
            page_sel_unlock(sff_obj);
            return ret;
        }
    }
    ret = sff_eeprom_write(sff_obj, addr, offset, buf, len);
    page_sel_unlock(sff_obj);
    return ret;
}
int qsfp_dd_eeprom_dump(struct sff_obj_t *sff_obj, u8 *buf)
{
    int ret = 0;
    u8 page = 0;
    int i = 0;
    int j = 0;
    int cnt = 0;
    u8 *data = NULL;
    u8 offset = 0;
    int last = EEPROM_SIZE;
    if ((ret = qsfp_dd_page_get(sff_obj, &page)) < 0) {

        return ret;
    }
    if (page < QSFP_PAGE0 ||  page >= PAGE_NUM) {

        return -EBFONT;
    }
    cnt += snprintf(buf+cnt, BUF_SIZE - cnt, "page%d\n", page);
    data = sff_obj->priv_data.qsfp_dd.eeprom_cache[page];
    memset(data, 0, sizeof(EEPROM_SIZE));
    if (QSFP_PAGE0 == page) {

        offset = 0;
        ret = qsfp_dd_eeprom_read(sff_obj, page, 0, data, EEPROM_SIZE);
        /*ret = qsfp_eeprom_read(sff_obj, page, 128, data+128, 64);
         *         ret = qsfp_eeprom_read(sff_obj, page, 192, data+192, 64);
         *                 */

    } else {

        offset = 128;
        ret = qsfp_dd_eeprom_read(sff_obj, page, offset, data+offset, EEPROM_HALF_SIZE);

    }

    if (ret < 0) {
        MODULE_LOG_ERR("ret:%d read fail\n", ret);
        return ret;
    }
#if 0
    for (i = 0; i < 256; i++) {
        MODULE_LOG_DBG("reg[%d]:%x\n", i, data[i]);
    }
#endif
    /*print out offset*/
#if 1
    cnt += snprintf(buf+cnt, BUF_SIZE - cnt, "    ");
    for (i = 0; i < 16; i++) {
        cnt += snprintf(buf+cnt, BUF_SIZE - cnt, "%01x  ", i);
    }
    cnt += snprintf(buf+cnt, BUF_SIZE - cnt, "\n");
#endif
    for (i = offset, j=0; i < last; i++) {

        j++;

        if (1 == j) {

            cnt += snprintf(buf+cnt, BUF_SIZE - cnt, "%02x: ", i);

        }

        cnt += snprintf(buf+cnt, BUF_SIZE - cnt, "%02x ", data[i]);

        if (16 == j) {

            cnt += snprintf(buf+cnt, BUF_SIZE - cnt, "\n");
            j = 0;
        }

    }
    return 0;
}

static int qsfp_dd_paging_supported(struct sff_obj_t *sff_obj, bool *supported)
{
    int ret = 0;
    u8 reg = 0;
    u8 addr = SFF_EEPROM_I2C_ADDR; //it's fixed
    if(!supported) {
        return -EINVAL;
    }
    if((ret = sff_eeprom_read(sff_obj, addr, QSFP_DD_ST_INDICATOR2_OFFSET, &reg, sizeof(reg))) < 0) {
        return ret;
    }
      
    if (test_bit(FLAT_MEM_BIT, (unsigned long *)&reg)) {
        *supported = false;
        MODULE_LOG_DBG("%s paging is not supported!\n", sff_obj->name);
    } else {
        *supported = true;
    }

    return 0;
}

int qsfp_dd_page_get(struct sff_obj_t *sff_obj, u8 *page)
{
    int ret = 0;
    u8 addr = SFF_EEPROM_I2C_ADDR;
    u8 offset_page_sel = QSFP_DD_PAGE_SEL_OFFSET;
    u8 reg = 0;

    if((ret = sff_eeprom_read(sff_obj, addr, offset_page_sel, &reg, sizeof(reg))) < 0) {
        return ret;
    }
    *page = reg;
    return 0;
}
static int _page_switch(struct sff_obj_t *sff_obj, u8 page)
{
    int ret = 0;
    u8 addr = SFF_EEPROM_I2C_ADDR;
    u8 offset_page_sel = QSFP_DD_PAGE_SEL_OFFSET;
    u8 cur_page = 0;
    bool supported = false;
#if 1
    if ((ret = qsfp_dd_paging_supported(sff_obj, &supported)) < 0) {
        return ret;
    }
#else
    /*use cache instead*/
    supported = sff_obj->priv_data.qsfp_dd.paging_supported;
#endif
    if (!supported &&
            QSFP_PAGE0 != page) {
        MODULE_LOG_ERR("%s paging not supported page:%d supported:%d\n", sff_obj->name, page, supported);
        return -ENOSYS;
    }
    ret = qsfp_dd_page_get(sff_obj, &cur_page);
    if (ret < 0) {
        return ret;
    }
    if (cur_page == page) {
        return 0;
    }
    ret = sff_eeprom_write(sff_obj, addr, offset_page_sel, &page, sizeof(page));

    if (ret < 0) {
        MODULE_LOG_ERR("switch page fail\n");
        return ret;
    }
    return 0;
}
int qsfp_dd_page_sel(struct sff_obj_t *sff_obj, int page)
{
    if (page < QSFP_PAGE0 ||  page >= PAGE_NUM) {

        return -EINVAL;
    }

    return _page_switch(sff_obj, page);
}
static int advert_check(struct sff_obj_t *sff_obj, bool *pass)
{
    int ret = 0;
    if ((ret = app_advertising_field_get(sff_obj)) < 0) {
        /*print error code*/
        return ret;
    }
#if 0
    if ((ret = qsfp_dd_active_control_set_indicator(sff_obj)) < 0) {
        return ret;
    }
#endif
    /*<TBD> do we need to check advertise?*/
    *pass = true;
    return 0;
}
static int sw_config_1_rev3(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    /* host select application*/
    /*stage control set 0*/
    if ((ret = stage_control_set0(sff_obj)) < 0) {
        /*print error code*/
        return ret;
    }
    return 0;
}
static int sw_config_2_rev3(struct sff_obj_t *sff_obj, bool *pass)
{
    *pass = true;
    return 0;
}
static int sw_config_check_rev3(struct sff_obj_t *sff_obj, bool *pass)
{
    int ret = 0;
    bool is_pass = false;
    if ((ret = config_error_code_check(sff_obj, &is_pass)) < 0) {

        return ret;
    }
    *pass = is_pass;
    return 0;
}
static int sw_control_rev3(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    /*set tx_disable true to avoid link flapping*/
    if ((ret = qsfp_dd_tx_disable_set(sff_obj, 0xff)) < 0) {
        return ret;
    }
    /*Host sets the
     *         * DataPathPwrUp bits for
     *                 * all host lanes in the
     *                         * data path(s) that it
     *                                 * wants to enable (See
     *                                         * Table 53, Page 10h
     *                                                 * Register 128)*/

    if ((ret = data_path_power_up(sff_obj, true)) < 0) {
        return ret;
    }

    return 0;
}
static int module_ready_check_rev3(struct sff_obj_t *sff_obj, bool *ready)
{
    u8 module_st = 0;
    int ret = 0;
    bool tmp_ready = false;
    bool activated = false;
    if ((ret = qsfp_dd_module_st_get(sff_obj, &module_st)) < 0) {
        return ret;
    }
    if ((ret = qsfp_dd_is_data_path_activated(sff_obj, &activated)) < 0) {
        return ret;
    }
    /*check module state register if it transit to module ready*/
    if (MODULE_READY_ST_ENCODE == module_st &&
            true == activated ) {
        /*set tx_disable false, */
        if ((ret = qsfp_dd_tx_disable_set(sff_obj, 0x0)) < 0) {
            return ret;
        }
        tmp_ready = true;
    }

    *ready = tmp_ready;
    return 0;
}
static int module_ready_check_rev4(struct sff_obj_t *sff_obj, bool *ready)
{
    u8 module_st = 0;
    int ret = 0;
    bool tmp_ready = false;
    bool activated = false;

    if ((ret = qsfp_dd_module_st_get(sff_obj, &module_st)) < 0) {
        return ret;
    }
    if ((ret = qsfp_dd_is_data_path_activated(sff_obj, &activated)) < 0) {
        return ret;
    }
    /*check module state register if it transit to module ready*/
    if (MODULE_READY_ST_ENCODE == module_st &&
            true == activated ) {
        tmp_ready = true;
    }

    *ready = tmp_ready;
    return 0;
}
static int sw_config_1_rev4(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    if ((ret = qsfp_dd_lowPwr_set(sff_obj)) < 0) {
        return ret;
    }
    return 0;
}
static int sw_config_2_rev4(struct sff_obj_t *sff_obj, bool *pass)
{
    *pass = true;
    return 0;
}

static int sw_config_check_rev4(struct sff_obj_t *sff_obj, bool *pass)
{
    *pass = true;
    return 0;
}

static int sw_control_rev4(struct sff_obj_t *sff_obj)
{
    return 0;
}

static int sw_config_1_rev4_full(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    if ((ret = qsfp_dd_data_path_deinit(sff_obj, 0xff)) < 0) {
        return ret;
    }
    if ((ret = qsfp_dd_lowPwr_set(sff_obj)) < 0) {
        return ret;
    }
    return 0;
}

static int sw_config_2_rev4_full(struct sff_obj_t *sff_obj, bool *pass)
{
    u8 module_st = 0;
    int ret = 0;

    if ((ret = qsfp_dd_module_st_get(sff_obj, &module_st)) < 0) {
        return ret;
    }
    /*check module state register if it transit to module ready*/
    if (MODULE_READY_ST_ENCODE != module_st) {
        *pass = false;
        return 0;
    }

    /* host select application*/
    /*stage control set 0*/
    if ((ret = stage_control_set0(sff_obj)) < 0) {
        return ret;
    }

    *pass = true;
    return 0;
}

static int sw_config_check_rev4_full(struct sff_obj_t *sff_obj, bool *pass)
{
    int ret = 0;
    bool is_pass = false;
    if ((ret = config_error_code_check(sff_obj, &is_pass)) < 0) {

        return ret;
    }
    *pass = is_pass;
    return 0;
}

static int sw_control_rev4_full(struct sff_obj_t *sff_obj)
{
    int ret = 0;

    if ((ret = qsfp_dd_data_path_deinit(sff_obj, 0x0)) < 0) {
        return ret;
    }

    return 0;
}

static int cmis_ver_get(struct sff_obj_t *sff_obj, u8 *ver)
{
    int ret = 0;
    u8 buf = 0;

    if ((ret = qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE0, QSFP_DD_CMIS_REV_OFFSET, &buf, 1)) < 0) {
        return ret;
    }
    MODULE_LOG_DBG("%s cmis ver:0x%x\n", sff_obj->name, buf);
    *ver = buf;
    return 0;
}
static int fsm_func_get(struct sff_obj_t *sff_obj, struct qsfp_dd_fsm_func_t **ppfunc)
{
    int ret = 0;
    u8 ver = 0;
    struct qsfp_dd_fsm_func_t *func = &qsfp_dd_FsmFuncRev3;
    bool rev4_quick = qsfp_dd_rev4_quick_get(sff_obj);
    if ((ret = cmis_ver_get(sff_obj, &ver)) < 0) {
        return ret;
    }

    if (ver == QSFP_DD_CMIS_REV3_VAL) {
        func = &qsfp_dd_FsmFuncRev3;
    } else if (ver == QSFP_DD_CMIS_REV4_VAL) {
        if (rev4_quick) {
            func = &qsfp_dd_FsmFuncRev4;
            MODULE_LOG_DBG("%s rev4 quick is loaded\n", sff_obj->name);
        } else {   
            func = &qsfp_dd_FsmFuncRev4_full;
            MODULE_LOG_DBG("%s rev4 full is loaded\n", sff_obj->name);
        }
    } else {

        MODULE_LOG_ERR("%s rev:%x not supported, using rev 3.0 as default\n", sff_obj->name, ver);
    }
    *ppfunc = func;
    return 0;
}
/*TBD lan number can be changed*/
static int valid_lane_num_get(struct sff_obj_t *sff_obj)
{

    int num = 8;
    return num;

}
static bool is_bank_num_valid(struct sff_obj_t *sff_obj)
{
    u8 bank_no = 0xff;
    bool valid = false;
    /*check if it's bank0? */
    if (qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE0, QSFP_DD_BANK_SEL_OFFSET, &bank_no, sizeof(bank_no)) < 0) {

        MODULE_LOG_ERR("%s qsfp_dd_eeprom_read fail", sff_obj->name);
        return valid;
    }
    if (0x00 == bank_no) {

        valid = true;
    } else {
        MODULE_LOG_ERR("%s bank no is not 0", sff_obj->name);
    }

    return valid;
}
/*CMIS REV 4.0*/
static int qsfp_dd_lowPwr_set(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    u8 buf = 0;
    u8 page = QSFP_PAGE0;
    u8 offset = QSFP_DD_MODULE_GLOBAL_CONTROL_OFFSET;

    if ((ret = qsfp_dd_eeprom_read(sff_obj, page, offset, &buf, sizeof(buf))) < 0) {
        return ret;
    }

    clear_bit(QSFP_DD_MODULE_GLOBAL_CONTROL_LOW_PWR_BIT, (unsigned long *)&buf);

    if( (ret = qsfp_dd_eeprom_write(sff_obj, page, offset, &buf, sizeof(buf))) < 0) {
        return ret;
    }

    return 0;
}
/*Table 8-44  Page 10h Overview offset 128*/
static int qsfp_dd_data_path_deinit(struct sff_obj_t *sff_obj, u8 val)
{
    int ret = 0;
    u8 buf = val;
    u8 page = QSFP_PAGE_10h;
    u8 offset = QSFP_DD_DATA_PATH_DEINIT_OFFSET;

    if( (ret = qsfp_dd_eeprom_write(sff_obj, page, offset, &buf, sizeof(buf))) < 0) {
        return ret;
    }

    return 0;
}
#if 0
/*Table 23- Module Global and Squelch Mode Controls (Lower Page, active modules only)
 *  * byte 26 bit 4 */
static int qsfp_dd_force_low_pwr(struct sff_obj_t *sff_obj, int en)
{
    u8 buf = 0;
    int ret = 0;
    u8 page = QSFP_PAGE0;
    u8 offset = 26;
    if ((ret = qsfp_dd_eeprom_read(sff_obj, page, offset, &buf, 1)) < 0) {

        MODULE_LOG_ERR("port:%d qsfp_dd_eeprom_read fail", port);
        return ret;
    }
    buf = ret;
    if (en) {
        set_bit(4, (unsigned long *)&buf);
    } else {

        clear_bit(4, (unsigned long *)&buf);
    }

    if( (ret = qsfp_dd_eeprom_write(sff_obj, page, offset, &buf, 1)) < 0) {
        MODULE_LOG_ERR("write fail\n");
        return ret;
    }
    return 0;
}
#endif
#if 0
/*(Page 11h, active modules only)
 *  * app selected: offset 206-213 (Table 73)
 *   * tx control offset 214-221 (Table 74)
 *    * rx control offset 222-239 (Table 75)*/
static int qsfp_dd_active_control_set_indicator(struct sff_obj_t *sff_obj)
{
    u8 data[34];
    int ret = 0;
    int count = 0;
    int size = sizeof(data);
    if (!is_bank_num_valid(sff_obj)) {
        return -1;
    }
    if ((ret = qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE_11h, 206, data, size)) < 0) {
        return ret;
    }

    for (count = 0; count < size; count++) {

    }

    return 0;
}
/*Table 46- Implemented Signal Integrity Controls (Page 01h)
 *  *
 *   * */
int qsfp_dd_channel_control_get(struct sff_obj_t *sff_obj, int type, u32 *value)
{
    int ret = 0;
    u8 data[2];
    /*TBD replace port with sff_obj->name*/
    if (!is_bank_num_valid(sff_obj)) {
        return -1;
    }
    if ((ret = qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE_01h, 161, data, 2)) < 0) {

        return -1;
    }

    /*offset:161*/
    if (data[0] | TX_CDR_BIT) {

        MODULE_LOG_DBG("%s Tx CDR implemented", sff_obj->name);
    }
    if (data[0] | TX_CDR_BYPASS_BIT) {

        MODULE_LOG_DBG("%s Tx CDR Bypass control implemented ", sff_obj->name);
    }
    if (data[0] | TX_INPUT_FIX_MANUAL_BIT) {

        MODULE_LOG_DBG("%s Tx Input Eq fixed manual", sff_obj->name);
    }
    if (data[0] | ADAPTIVE_TX_INPUT_FIX_MANUAL_BIT) {

        MODULE_LOG_DBG("port:%d Adaptive Tx Input Eq", port);
    }
    if (data[0] | TX_INPUT_FREEZE_BIT) {

        MODULE_LOG_DBG("port:%d Tx Input Eq Freeze", port);
    }
    /*offset:162*/
    if (data[1] | RX_CDR_BIT) {

        MODULE_LOG_DBG("port:%d Rx CDR implemented", port);
    }
    if (data[1] | RX_CDR_BYPASS_BIT) {

        MODULE_LOG_DBG("port:%d Rx CDR Bypass control implemented ", port);
    }
    if (data[1] | RX_OUTPUT_AMP_CONTROL_BIT) {

        MODULE_LOG_DBG("port:%d Rx output amp", port);
    }
    if (data[1] | RX_OUTPUT_EQ_CONTROL_BIT) {

        MODULE_LOG_DBG("port:%d Rx output eq", port);
    }
    if (data[1] | STAGE_SET1_BIT) {

        MODULE_LOG_DBG("port:%d stage_set1", port);
    }

    return 0;
}
#endif
int qsfp_dd_temperature_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size)
{
    int ret = 0;
    u8 page = QSFP_PAGE0;
    u8 offset = QSFP_DD_TEMP_OFFSET;
    u8 reg[WORD_SIZE];
    s16 temp = 0;
    s16 data_h = 0;
    s16 data_l = 0;
    s16 divider = 256;
    s16 decimal = 0;
    char *unit = "c";

    if((ret = qsfp_dd_eeprom_read(sff_obj, page, offset, reg, sizeof(reg))) < 0) {
        return ret;
    }
    data_h = reg[0];
    data_l = reg[1];
    temp = (data_h << 8) | data_l;
    decimal = ((temp/divider)*divider)-temp;
    decimal = abs(decimal);
    decimal = decimal*1000/divider;

    scnprintf(buf, buf_size,
              "%d.%d %s\n",
              temp/divider,
              decimal, unit);

    return 0;
}

/*Table 22- Module Monitors (Lower Page, active modules only)
 *  * offset 16,17*/
int qsfp_dd_voltage_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size)
{
    int ret = 0;
    u8 page = QSFP_PAGE0;
    u8 offset = QSFP_DD_VCC_OFFSET;
    u8 reg[WORD_SIZE];
    u16 vol = 0;
    u16 data_h = 0;
    u16 data_l = 0;
    u16 divider = 10000;
    char *unit = "v";

    if((ret = qsfp_dd_eeprom_read(sff_obj, page, offset, reg, sizeof(reg))) < 0) {
        return ret;
    }
    data_h = reg[0];
    data_l = reg[1];
    vol = (data_h << 8) | data_l;
    scnprintf(buf, buf_size,
              "%d.%d %s\n",
              vol/divider,
              vol%divider, unit);

    return 0;
}

/*1.7.7.3  Lane-Specific Monitors */
/*Table 70- Lane-Specific Monitors (Page 11h, active modules only) */
int qsfp_dd_lane_monitor_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size)
{
    int ret = 0;
    u8 page = QSFP_PAGE_11h;
    u8 offset = 0;
    int len = 0;
    int count = 0;
    int idx = 0;
    u8 ch_monitor[QSFP_DD_LANE_NUM*WORD_SIZE]; /* 2(u16 data) *4 (channel)*/
    u16 data_h = 0;
    u16 data_l = 0;
    u16 ch_data = 0;
    u16 divider = 10000;
    char *unit = NULL;
    struct monitor_para_t *para = NULL;

    len = sizeof(ch_monitor);

    switch(type) {
    case LN_MONITOR_TX_PWR_TYPE:
        offset = QSFP_DD_LN_MONITOR_TX_PWR_OFFSET;
        break;
    case LN_MONITOR_RX_PWR_TYPE:
        offset = QSFP_DD_LN_MONITOR_RX_PWR_OFFSET;
        break;
    case LN_MONITOR_TX_BIAS_TYPE:
        offset = QSFP_DD_LN_MONITOR_TX_BIAS_OFFSET;
        break;
    default:
        break;
    }
    if (0 == offset) {
        return -EINVAL;
    }
    if((ret = qsfp_dd_eeprom_read(sff_obj, page, offset, ch_monitor, len)) < 0) {
        return ret;
    }
    para = monitor_para_find(type);
    if (!para) {
        return -EBADRQC;
    }
    divider = para->divider;
    unit = para->unit;
    for (idx = 0; idx < len; idx += WORD_SIZE) {
        /*big edian*/
        data_h = ch_monitor[idx];
        data_l = ch_monitor[idx+1];
        ch_data = (data_h << 8) | data_l;

        count += scnprintf(buf+count, buf_size-count,
                           "ch%d: %d.%d %s\n",
                           (idx >> 1) + 1, ch_data/divider,
                           ch_data%divider, unit);
    }

    return 0;
}
/*rev3 1.7.3.2*/
int qsfp_dd_vendor_info_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size)
{
    int ret = 0;
    u8 page = QSFP_PAGE0;
    u8 offset = 0;
    int len = 0;
    u8 byte_data[VENDOR_INFO_BUF_SIZE];

    switch(type) {
    case VENDOR_NAME_TYPE:
        offset = QSFP_DD_VENDOR_NAME_OFFSET;
        len = QSFP_DD_VENDOR_NAME_LEN;

        break;
    case VENDOR_PN_TYPE:
        offset = QSFP_DD_VENDOR_PN_OFFSET;
        len = QSFP_DD_VENDOR_PN_LEN;

        break;
    case VENDOR_SN_TYPE:
        offset = QSFP_DD_VENDOR_SN_OFFSET;
        len = QSFP_DD_VENDOR_SN_LEN;

        break;
    case VENDOR_REV_TYPE:
        offset = QSFP_DD_VENDOR_REV_OFFSET;
        len = QSFP_DD_VENDOR_REV_LEN;

        break;
    default:
        break;
    }
    if (0 == offset) {
        return -EINVAL;
    }
    /*use a big enough buf to handle all the vendor info*/
    if (len >= VENDOR_INFO_BUF_SIZE) {
        return -EBADRQC;
    }

    if((ret = qsfp_dd_eeprom_read(sff_obj, page, offset, byte_data, len)) < 0) {
        return ret;
    }
    /*add terminal of string*/
    byte_data[len] = '\0';
    scnprintf(buf, buf_size, "%s\n", byte_data);

    return 0;
}
/*Table 18- Identifier and Status Summary (Lower Page) byte3 , 3-1 Module state*/
int qsfp_dd_module_st_get(struct sff_obj_t *sff_obj, u8 *st)
{
    u8 data = 0;
    int ret = 0;
    if (!st) {
        return -EINVAL;
    }
    if ((ret = qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE0, QSFP_DD_MODULE_ST_INT_OFFSET, &data, sizeof(data))) < 0) {

        MODULE_LOG_ERR("%s qsfp_dd_eeprom_read fail", sff_obj->name);
        return ret;
    }

    *st = bits_get(data, QSFP_DD_MODULE_ST_BIT_MIN, QSFP_DD_MODULE_ST_BIT_NUM);
    return 0;
}
/*value : the status of 8 lane, one lane: 4 bits*/
/*table 65 page 11h offset 128~131*/
static int qsfp_dd_data_path_st_get(struct sff_obj_t *sff_obj, u8 st[], int size)
{
    int ret = 0;
    u8 reg[QSFP_DD_DATA_PATH_ST_NUM];
    int i = 0;
    int ln = 0;

    if (!st) {
        return -EINVAL;
    }
    if((ret = qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE_11h, QSFP_DD_DATA_PATH_ST_OFFSET, reg, sizeof(reg))) < 0) {
        *st = 0;
        return ret;
    }
    if ((QSFP_DD_DATA_PATH_ST_NUM * 2) != size) {
        return -EINVAL;
    }
    for (i = 0; i < QSFP_DD_DATA_PATH_ST_NUM; i++) {
        st[ln++] = reg[i] & 0x0f;
        st[ln++] = reg[i] >> 4;
    }
    return 0;
}

static int qsfp_dd_is_data_path_activated(struct sff_obj_t *sff_obj, bool *activated)
{
    u8 data_path_st[QSFP_DD_LANE_NUM];
    int ln = 0;
    int ret = 0;
    bool tmp_active = false;
    int lane_num = QSFP_DD_LANE_NUM;

    if ((ret = qsfp_dd_data_path_st_get(sff_obj, data_path_st, sizeof(data_path_st))) < 0) {
        return ret;
    }
    
    for (ln = 0; ln < lane_num; ln++) {
        if (DATA_PATH_ACTIVATED_ST_ENCODE != data_path_st[ln]) {
            break;
        }
    }
    if (ln >= lane_num) {
        MODULE_LOG_DBG("%s ok", sff_obj->name);
        tmp_active = true;
    }
    *activated = tmp_active;
    return 0;
}
/*Table 18- Identifier and Status Summary (Lower Page)
 *  * lower page :offset 0　*/
int qsfp_dd_id_get(struct sff_obj_t *sff_obj, u8 *val)
{
    u8 buf = 0;
    int ret = 0;

    if (!val) {
        return -EINVAL;
    }
    if ((ret = qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE0, QSFP_DD_ID_OFFSET, &buf, sizeof(buf))) < 0) {
        return ret;
    }
    *val = buf;
    return 0;
}
bool qsfp_dd_is_id_matched(struct sff_obj_t *sff_obj)
{
    u8 id = 0;
    bool match = false;
    if (qsfp_dd_id_get(sff_obj, &id) < 0) {
        return match;
    }
    switch (id) {
    case 0x18:
        match = true;
        break;
    default:
        MODULE_LOG_ERR("%s not match id:%d\n", sff_obj->name, id);
        break;
    }

    return match;
}
#if 0
static int qsfp_dd_module_state_is_changed(struct sff_obj_t *sff_obj, bool *is_changed)
{
    u8 page = QSFP_PAGE0;
    u8 offset = 8;
    u8 val = 0;
    int ret = 0;
    if ((ret = qsfp_dd_eeprom_read(sff_obj, page, offset, &val, 1)) < 0) {
        return ret;
    }
    *is_changed = ((val&0x1) ? true : false);
    return 0;
}

/*table 18 : offset 3 bit 0*/
static int qsfp_dd_digital_int_get(struct sff_obj_t *sff_obj, u8 *value)
{
    u8 st = 0;
    int ret = 0;
    if ((ret = qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE0, 0x03, &st, 1)) < 0) {
        return ret;
    }
    *value = st & 0x1;
    return ret;

}
#endif
#if 0
/*table 36: byte 212 :that defines aspects of the device or cable technology*/
static int qsfp_dd_media_interface_tech_get(struct sff_obj_t *sff_obj, u8 *interface)
{

    u8 data = 0;

    if(IS_ERR_OR_NULL(interface)) {
        MODULE_LOG_ERR("invalid para");
        return -1;
    }

    if(qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE0, 212, &data, 1) < 0) {
        MODULE_LOG_ERR("qsfp_dd_eeprom_read fail");
        *interface = 0xff; /*non define*/
        return -1;
    }
    *interface = data;

    return 0;

}
#endif

/*Table 25- Byte 85 Module Type Encodings*/
static int qsfp_dd_module_type_get(struct sff_obj_t *sff_obj, u8 *type)
{
    u8 data = 0;
    int ret = 0;
    
    if ((ret = qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE0, QSFP_DD_MODULE_TYPE_OFFSET, &data, sizeof(data))) < 0) {
        return ret;
    }
    *type = data;
    return 0;
}

static bool is_loopback_module(struct sff_obj_t *sff_obj)
{
    /*<TBD> need check more loopback modules*/
    bool is_loopback = false;
    u8 type = 0;
    if (qsfp_dd_module_type_get(sff_obj, &type) < 0) {
        return false;
    }

    if (0 == type) {
        is_loopback = true;
    }
    return is_loopback;
}
/*table 26*: lower page:00*/
static int app_advertising_field_get(struct sff_obj_t *sff_obj)
{
    u8 module_type = 0;
    union qsfp_dd_app_advert_fields *fields = sff_obj->priv_data.qsfp_dd.fields;
    int ret = 0;
    int apsel = 0;
    int i = 0;
    bool end_of_table = false;

    if ((ret = qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE0, 85, &module_type, 1)) < 0) {

        return ret;
    }
    
    MODULE_LOG_DBG("%s module_type:0x%x\n", sff_obj->name, module_type);
    sff_obj->priv_data.qsfp_dd.module_type = module_type;
    for (apsel = APSEL_1; apsel < APSEL_NUM; apsel++) {
        for (i = 0; i < APP_ADVERT_FIELD_NUM; i++) {

            if ((ret =qsfp_dd_eeprom_read(sff_obj, app_advert_fields_page[i],
                                          app_advert_fields_offset[apsel][i],
                                          &fields[apsel].reg[i], 1)) < 0) {

                MODULE_LOG_ERR("%s qsfp_dd_eeprom_read fail", sff_obj->name);
                return ret;
            }
            if (ret == 0xff) {
                end_of_table = true;
                break;
            }
        }
        if (end_of_table) {
            break;
        }

    }

    for (apsel = 0; apsel < APSEL_NUM; apsel++) {
        MODULE_LOG_DBG("%s host_electrical_interface_code[%d]:0x%x\n",
                   sff_obj->name, apsel, fields[apsel].data.host_electrical_interface_code);
        MODULE_LOG_DBG("%s module_media_interface_code[%d]:0x%x\n",
                   sff_obj->name, apsel, fields[apsel].data.module_media_interface_code);
        MODULE_LOG_DBG("%s lane_count[%d]:0x%x\n",
                   sff_obj->name, apsel, fields[apsel].data.lane_count);
        MODULE_LOG_DBG("%s  host_lane_assignment_options[%d]:0x%x\n",
                   sff_obj->name, apsel, fields[apsel].data.host_lane_assignment_options);
        MODULE_LOG_DBG("%s  media_lane_assignment_options[%d]:0x%x\n",
                   sff_obj->name, apsel, fields[apsel].data.media_lane_assignment_options);
    }
    return 0;
}
#if 0
static int app_advert_field_dump(struct sff_obj_t *sff_obj, u8 *buf, int buf_size)
{
    int apsel = APSEL_1;
    int ret = 0;
    union qsfp_dd_app_advert_fields *fields = sff_obj->priv_data.qsfp_dd.fields;
    /*<TBD> the field data will be cached when module is plugged in , so may not need to capture again in the future*/
    ret = app_advertising_field_get(sff_obj);
    if (ret < 0) {
        return ret;
    }
    scnprintf(buf, buf_size,
              "module_type 0x%x \
                                                                 host electrical intf 0x%x \
                                                                                module media intf 0x%x  \
                                                                                               host_media lane count %d \
                                                                                                              host lane assignment 0x%x \
                                                                                                                             media lane assignment 0x%x\n",
              sff_obj->priv_data.qsfp_dd.module_type,
              fields[apsel].data.host_electrical_interface_code,
              fields[apsel].data.module_media_interface_code,
              fields[apsel].data.lane_count,
              fields[apsel].data.host_lane_assignment_options,
              fields[apsel].data.media_lane_assignment_options

             );
    return 0;

}
#endif
int qsfp_dd_active_ctrl_set_get(struct sff_obj_t *sff_obj, char *buf, int size)
{
    int ret = 0;
    struct stage_set_t stage_set[QSFP_DD_LANE_NUM];
    u8 offset = QSFP_DD_ACTIVE_CTRL_SET_OFFSET;
    int count = 0;
    int ln = 0;
    if((ret = qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE_11h, offset, (u8*)stage_set, sizeof(stage_set))) < 0) {
        return ret;
    }
    
    for (ln = 0; ln < 8; ln++) {
        count += scnprintf(buf+count, size-count,
                           "ln%d: apsel:%d datapath:%d explicit_ctrl:%d\n",
                           ln+1,
                           stage_set[ln].app_code,
                           stage_set[ln].datapath_code,
                           stage_set[ln].explicit_control);
    }
    return 0;
}    
int qsfp_dd_apsel_get(struct sff_obj_t *sff_obj)
{
    //return stage_control_set0(sff_obj, apsel);
    struct qsfp_dd_priv_data *qsfp_dd_priv = qsfp_dd_priv_get(sff_obj);
    return qsfp_dd_priv->apsel;
}        
/*note: so far only page >= 0x10 , need to check bank number*/
static int stage_control_set0(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    int apsel = 0;
    u8 apply_stage_control_set = 0;
    u8 offset = QSFP_DD_STAGE_CTRL_SET_ZERO_APPL_SEL_CTRL_OFFSET;
    //u8 stage_set_offset_begin = QSFP_DD_STAGE_CTRL_SET_ZERO_APPL_SEL_CTRL_OFFSET;
    //u8 stage_set_offset_end = stage_set_offset_begin + QSFP_DD_LANE_NUM - 1;
    int ln = 0;
    int datapath_code = 0;
    struct stage_set_t stage_set[QSFP_DD_LANE_NUM];
    
    if (!is_bank_num_valid(sff_obj)) {
        return -EBADRQC;
    }
    apsel = qsfp_dd_apsel_get(sff_obj);
    /*table 56 stage control set 0, application select controls (Page 10h, active modules only) */
    /*case1: normal case 400G lane = 8*/
    if (1 == apsel) {
        for (ln = 0; ln < 8; ln++) {
            stage_set[ln].explicit_control = 0;
            stage_set[ln].datapath_code = 0x00;
            stage_set[ln].app_code = apsel;
        }

    } else {

        for (ln = 0; ln < 8; ) {
            stage_set[ln].explicit_control = 0;
            stage_set[ln].datapath_code = datapath_code;
            stage_set[ln].app_code = apsel;
            ln++; 
            if (0 == (ln % 2)) {
                datapath_code += 2;
            }
        }
    }
    
    if((ret = qsfp_dd_eeprom_write(sff_obj, QSFP_PAGE_10h, offset, (u8*)stage_set, sizeof(stage_set))) < 0) {
        return ret;
    }
    /*table 55 Apply_DataPathInit*/
    apply_stage_control_set = 0xff;
    if((ret = qsfp_dd_eeprom_write(sff_obj, 
                                   QSFP_PAGE_10h, 
                                   QSFP_DD_STAGE_CTRL_SET_ZERO_DATA_PATH_INIT_OFFSET, 
                                   &apply_stage_control_set, sizeof(apply_stage_control_set))) < 0) {
        return ret;
    }
    
    return 0;
}

int qsfp_dd_apsel_apply(struct sff_obj_t *sff_obj, int apsel)
{
    //return stage_control_set0(sff_obj, apsel);
    struct qsfp_dd_priv_data *qsfp_dd_priv = qsfp_dd_priv_get(sff_obj);
    qsfp_dd_priv->apsel = apsel;
    return 0;
}        
/*qsfp-dd used only*/

/*page 11
 *  * table 71/72 : active modules only
 *   * Table 72- Configuration Error Codes
 *    * para:
 *     * port(in)
 *      * is_pass(out)*/
static int config_error_code_check(struct sff_obj_t *sff_obj, bool *is_pass)
{
    int ret = 0;
    u8 data[4];
    int lane = 0;
    int i = 0;
    int size = sizeof(data);
    /*since the one reg reprsL 2 lanes*/
    int lane_num = valid_lane_num_get(sff_obj);
    u8 config_err_code[lane_num]; /*lane number 8*/

    if (!is_pass) {
        return -EINVAL;
    }
    if (!is_bank_num_valid(sff_obj)) {
        return -EBADRQC;
    }
    if ((ret = qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE_11h, QSFP_DD_CONFIG_ERR_CODE_OFFSET, data, size)) < 0) {
        return ret;
    }

    for (lane = 0, i = 0; i < size; i++) {
        config_err_code[lane++] = data[i] & 0xf;
        config_err_code[lane++] = (data[i] & 0xf0) >> 4;
    }

    for (lane = 0; lane < lane_num; lane++) {

        MODULE_LOG_DBG("%s lane:%d err_code:0x%x\n", sff_obj->name, lane, config_err_code[lane]);
        if (CONFIG_ACCEPTED == config_err_code[lane]) {
        } else {
            break;
        }
    }
    if (lane >= lane_num) {
        *is_pass = true;
    }
    return 0;
}
/*table 54 page 10h offset 130 */
int qsfp_dd_tx_disable_set(struct sff_obj_t *sff_obj, u8 val)
{
    int ret = 0;
    if (!is_bank_num_valid(sff_obj)) {
        return -EBADRQC;
    }
    if((ret = qsfp_dd_eeprom_write(sff_obj, QSFP_PAGE_10h, QSFP_DD_TX_DISABLE_OFFSET, &val, sizeof(val))) < 0) {
        return ret;
    }

    return 0;
}
/*table 54 page 10h offset 130 */
int qsfp_dd_tx_disable_get(struct sff_obj_t *sff_obj, u8 *val)
{
    int ret = 0;
    u8 buf = 0;
    if (!val) {
        return -EINVAL;
    }
    if (!is_bank_num_valid(sff_obj)) {
        return -EBADRQC;
    }
    if((ret = qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE_10h, QSFP_DD_TX_DISABLE_OFFSET, &buf, sizeof(buf))) < 0) {
        return -ret;
    }
    *val = buf;
    return 0;
}
/*table 53 */
static int data_path_power_up(struct sff_obj_t *sff_obj, bool up)
{
    int ret = 0;
    u8 value = 0;

    if (up) {
        value = 0xff;
    }
    if (!is_bank_num_valid(sff_obj)) {
        return -EBADRQC;
    }
    if((ret = qsfp_dd_eeprom_write(sff_obj, QSFP_PAGE_10h, QSFP_DD_DATA_PATH_PWR_UP_OFFSET, &value, sizeof(value))) < 0) {
        return ret;
    }

    return 0;
}
#if 0
/*Table 18- Identifier and Status Summary (Lower Page) offset 2 bit 7:Flat_mem*/
static int qsfp_dd_is_flat_mem(struct sff_obj_t *sff_obj, bool *is_flat)
{
    u8 buf = 0;
    u8 offset = 2;

    if(IS_ERR_OR_NULL(is_flat)) {
        MODULE_LOG_ERR("invalid para");
        return -1;
    }
    if (qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE0, offset, &buf, 1) < 0) {
        *is_flat = false;
        return -1;
    }

    if (buf & 0x80) {
        *is_flat = true;
    } else {
        *is_flat = false;
    }
    return 0;
}
#endif
/*optical: true copper: false*/
static bool is_passive_module(struct sff_obj_t *sff_obj, bool *passive_module)
{
    int ret = 0;
    bool tmp_passive_module = false;
    u8 module_type = 0;

    if ((ret = qsfp_dd_module_type_get(sff_obj, &module_type)) < 0) {
        return ret;
    }

    switch (module_type) {

    case MODULE_PASSIVE_CU:
        tmp_passive_module = true;
        break;
    case MODULE_TYPE_MMF:
    case MODULE_TYPE_SMF:
    case MODULE_ACTIVE_CABLES:
    case MODULE_BASE_T:
    default:
        break;

    }
    *passive_module = tmp_passive_module;
    return 0;
}
#if 0
/*<TBD> so far it's used to de-assert- intL*/
static int qsfp_dd_interrupt_flags_read(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    u8 module_flags[4];
    u8 lane_flags[19];
    /*table 15 module flags*/
    if ((ret = qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE0, 8, module_flags, 4)) < 0) {

        MODULE_LOG_ERR("port:%d qsfp_dd_eeprom_read fail", port);
        return ret;
    }
    /* table 16 Lane-Specific Flag*/
    if ((ret = qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE_11h, 134, lane_flags, 19)) < 0) {

        MODULE_LOG_ERR("port:%d qsfp_dd_eeprom_read fail", port);
        return ret;
    }

    return 0;
}
#endif
static void cnt_increment_limit(u32 *data)
{
    if (*data < U32_MAX) {
        (*data)++;
    }
}
/*table 15*/
static int qsfp_dd_intr_module_flag_update(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    int i = 0;
    u8 module_flag[QSFP_DD_INT_MODULE_FLAG_NUM];
    u32 *cnt = NULL;
    u32 old_cnt = 0;
    struct qsfp_dd_priv_data *qsfp_dd_priv = qsfp_dd_priv_get(sff_obj);

    if ((ret = qsfp_dd_eeprom_read(sff_obj, QSFP_PAGE0, 8, module_flag, sizeof(module_flag))) < 0) {
        return ret;
    }
    for (i = 0; i < QSFP_DD_INT_MODULE_FLAG_NUM; i++) {
        cnt = &(qsfp_dd_priv->intr_module_flag[i].cnt);
        old_cnt = qsfp_dd_priv->intr_module_flag[i].cnt;
        qsfp_dd_priv->intr_module_flag[i].reg = module_flag[i]; 
        
        if (module_flag[i] != 0x00) {
                cnt_increment_limit(cnt);
                if (int_flag_monitor_en) {
                    MODULE_LOG_ERR("%s error %s:0x%x", sff_obj->name, intr_module_flag_str[i], module_flag[i]);
                }
        }
        
        if (old_cnt != *cnt) {
            qsfp_dd_priv->intr_module_flag[i].chg = true;
        } else {
            qsfp_dd_priv->intr_module_flag[i].chg = false;
        }
    }

    return 0;
}

static int qsfp_dd_lane_status_update(struct sff_obj_t *sff_obj, int type, u8 value)
{
    if (type > LN_STATUS_NUM ||
            type < LN_STATUS_RX_LOS_TYPE) {
        return -EINVAL;
    }

    sff_obj->priv_data.qsfp_dd.lane_st[type] = value;

    return 0;
}
/*Table 68 TX Flags , 69- RX Flags (Page 11h, active modules only)*/
static int qsfp_dd_intr_ln_flag_update(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    int i = 0;
    u8 ln_flag[QSFP_DD_INT_LN_FLAG_NUM];
    u32 *cnt = NULL;
    u32 old_cnt = 0;
    struct qsfp_dd_priv_data *qsfp_dd_priv = qsfp_dd_priv_get(sff_obj);
    
    if ((ret = qsfp_dd_eeprom_read(sff_obj, 
                                   QSFP_PAGE_11h, 
                                   QSFP_DD_INTR_LN_FLAG_START_OFFSET, 
                                   ln_flag,
                                   sizeof(ln_flag))) < 0) {
        return ret;
    }
    qsfp_dd_lane_status_update(sff_obj, LN_STATUS_RX_LOS_TYPE, ln_flag[L_RX_LOS_ID]);
    
        for (i = 0; i < QSFP_DD_INT_LN_FLAG_NUM; i++) {
            cnt = (&qsfp_dd_priv->intr_ln_flag[i].cnt);
            old_cnt = qsfp_dd_priv->intr_ln_flag[i].cnt;
            qsfp_dd_priv->intr_ln_flag[i].reg = ln_flag[i]; 
            
            if (ln_flag[i] != 0x00) {
                cnt_increment_limit(cnt);
                if (int_flag_monitor_en) {
                    MODULE_LOG_ERR("%s chg %s:0x%x", sff_obj->name, intr_ln_flag_str[i], ln_flag[i]);
                }
            }
            if (old_cnt != *cnt) {
                qsfp_dd_priv->intr_ln_flag[i].chg = true;
            } else {
                qsfp_dd_priv->intr_ln_flag[i].chg = false;
            }
        }

    return 0;
}

static void qsfp_dd_intr_flag_clear(struct sff_obj_t *sff_obj)
{
    struct qsfp_dd_priv_data *qsfp_dd_priv = qsfp_dd_priv_get(sff_obj);
    memset(qsfp_dd_priv->intr_module_flag, 0, sizeof(qsfp_dd_priv->intr_module_flag));
    memset(qsfp_dd_priv->intr_ln_flag, 0, sizeof(qsfp_dd_priv->intr_ln_flag));
}    

static int qsfp_dd_intr_flag_show(struct sff_obj_t *sff_obj, char *buf, int size)
{
    int count = 0;
    int i = 0;
    u32 cnt = 0;
    char chg = ' ';
    u8 intL = 0;
    int ret = 0;
    struct qsfp_dd_priv_data *qsfp_dd_priv = qsfp_dd_priv_get(sff_obj);

    /*handle transition back to normal state from intL state  {*/
    if ((ret = qsfp_dd_intL_get(sff_obj, &intL)) < 0) {
        return ret;
    }
    if (intL) {
        if ((ret = qsfp_dd_intr_module_flag_update(sff_obj)) < 0) {
            return ret;
        }
        if ((ret = qsfp_dd_intr_ln_flag_update(sff_obj)) < 0 ) {
            return ret;
        }
    }
    /*handle transition back to normal state from intL state  }*/
    
    count += scnprintf(buf+count, size-count,
                       "intr module flag show:\n");
    for (i = 0; i < QSFP_DD_INT_MODULE_FLAG_NUM; i++) {
        
        cnt = qsfp_dd_priv->intr_module_flag[i].cnt;
        if (0 != cnt) {
            if (qsfp_dd_priv->intr_module_flag[i].chg) {
                chg = '*';
            } else {
                chg = ' ';
            }
            count += scnprintf(buf+count, size-count,
                               "%s reg:0x%x cnt:%d%c\n",
                               intr_module_flag_str[i], 
                               qsfp_dd_priv->intr_module_flag[i].reg,
                               cnt,
                               chg);
        }
    }
    count += scnprintf(buf+count, size-count,
                       "intr lane flag show:\n");
    for (i = 0; i < QSFP_DD_INT_LN_FLAG_NUM; i++) {
        cnt = qsfp_dd_priv->intr_ln_flag[i].cnt;
        if (0 != cnt) {
            if (qsfp_dd_priv->intr_ln_flag[i].chg) {
                chg = '*';
            } else {
                chg = ' ';
            }
            count += scnprintf(buf+count, size-count,
                               "%s reg:0x%x cnt:%d%c\n",
                               intr_ln_flag_str[i], 
                               qsfp_dd_priv->intr_ln_flag[i].reg,
                               cnt,
                               chg);
        } 
    }
    return 0;
}
int qsfp_dd_ln_st_get(struct sff_obj_t *sff_obj, int type, u8 *st)
{
    u8 intL = 0;
    int ret = 0;
    struct qsfp_dd_priv_data *qsfp_dd_priv = qsfp_dd_priv_get(sff_obj);

    if ((ret = qsfp_dd_intL_get(sff_obj, &intL)) < 0) {
        return ret;
    }
    if (intL) {
        if ((ret = qsfp_dd_intr_module_flag_update(sff_obj)) < 0) {
            return ret;
        }
        if ((ret = qsfp_dd_intr_ln_flag_update(sff_obj)) < 0 ) {
            return ret;
        }
    }

    *st = qsfp_dd_priv->lane_st[type];
    return 0;
}
static void cache_clear(struct sff_obj_t *sff_obj)
{
    memset(&(sff_obj->priv_data.qsfp_dd), 0, sizeof(struct qsfp_dd_priv_data));
}

void qsfp_dd_rev4_quick_set(struct sff_obj_t *sff_obj, bool en)
{
     sff_obj->priv_data.qsfp_dd.rev4_quick_en = en;
}    

bool qsfp_dd_rev4_quick_get(struct sff_obj_t *sff_obj)
{
     return sff_obj->priv_data.qsfp_dd.rev4_quick_en;
}    
static void qsfp_dd_def_set(struct sff_obj_t *sff_obj)
{
        /*default loading*/
    qsfp_dd_apsel_apply(sff_obj, 1);
} 
int sff_fsm_qsfp_dd_task(struct sff_obj_t *sff_obj)
{
    sff_fsm_state_t st = sff_fsm_st_get(sff_obj);
    int ret = 0;
    u8 module_st = 0;
    u8 lpmode = 0;
    u8 lv = 0;
    u8 rst = 0;
    u8 power = 0;
    bool pass = false;
    bool ready = false;
    bool passive_module = false;
    bool supported = false;
    struct qsfp_dd_priv_data *qsfp_dd_priv = qsfp_dd_priv_get(sff_obj);

    switch (st) {
    case SFF_FSM_ST_IDLE:
        break;
    case SFF_FSM_ST_REMOVED:
        cache_clear(sff_obj);
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_IDLE);
        break;
    case SFF_FSM_ST_INSERTED:
        qsfp_dd_def_set(sff_obj);
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_DETECTING);
        break;
    case SFF_FSM_ST_DETECTING:

        if ((ret = qsfp_dd_reset_get(sff_obj, &rst)) < 0) {
            break;
        }
        if ((ret = sff_power_get(sff_obj, &power)) < 0) {
            break;
        }

        if (!rst || !power) {
            MODULE_LOG_ERR("%s rst:%d power:%d\n",sff_obj->name, rst, power);
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_SUSPEND);
            break;
        }

        if ((ret = qsfp_dd_reset_set(sff_obj, 0)) < 0) {
            break;
        }
        
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_UNRESET);
        break;
    
    case SFF_FSM_ST_MODULE_UNRESET:
        if ((ret = qsfp_dd_reset_set(sff_obj, 1)) < 0) {
            break;
        }
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_MGMT_INIT);
        break;
    case SFF_FSM_ST_MGMT_INIT:
        if ((ret = qsfp_dd_paging_supported(sff_obj, &supported)) < 0) {
            return ret;
        }
        /*use cache instead*/
        sff_obj->priv_data.qsfp_dd.paging_supported = supported;
        /*check module state register if it transit to module low power state*/
        if ((ret = qsfp_dd_module_st_get(sff_obj, &module_st)) < 0) {
            break;
        }

        if ((ret = is_passive_module(sff_obj, &passive_module)) < 0) {
            break;
        }

        if (passive_module) {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_READY);
        } else {

            if ((ret = qsfp_dd_lpmode_get(sff_obj, &lpmode)) < 0) {
                break;
            }
            if (lpmode) {

                if (MODULE_LOW_PWR_ST_ENCODE == module_st) {
                    if (is_loopback_module(sff_obj)) {

                        sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_LOOPBACK_INIT);
                    } else {

                        sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_CMIS_VER_CHECK);
                    }
                }
            } else {
                sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_HW_INIT);
            }
        }

        break;
    case SFF_FSM_ST_MODULE_CMIS_VER_CHECK:

        if ((ret = fsm_func_get(sff_obj, &(qsfp_dd_priv->fsm_func))) < 0) {
            break;
        }

        sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_ADVERT_CHECK);
        break;
    case SFF_FSM_ST_MODULE_ADVERT_CHECK:

        if ((ret = qsfp_dd_priv->fsm_func->advert_check(sff_obj, &pass)) < 0) {
            break;
        }
        if (pass) {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_SW_CONFIG_1);
        }
        break;
    case SFF_FSM_ST_MODULE_SW_CONFIG_1:

        if ((ret = qsfp_dd_priv->fsm_func->sw_config_1(sff_obj)) < 0) {
            break;
        }
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_SW_CONFIG_2);
        break;
    case SFF_FSM_ST_MODULE_SW_CONFIG_2:

        if ((ret = qsfp_dd_priv->fsm_func->sw_config_2(sff_obj, &pass)) < 0) {
            break;
        }
        if (pass) {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_SW_CONFIG_CHECK);
        }
        break;
    case SFF_FSM_ST_MODULE_SW_CONFIG_CHECK:

        if ((ret = qsfp_dd_priv->fsm_func->sw_config_check(sff_obj, &pass)) < 0) {
            break;
        }

        if (pass) {

            sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_SW_CONTROL);
        }
        break;
    case SFF_FSM_ST_MODULE_SW_CONTROL:

        if ((ret = qsfp_dd_priv->fsm_func->sw_control(sff_obj)) < 0) {
            break;
        }

        sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_READY_CHECK);
        break;
    case SFF_FSM_ST_MODULE_READY_CHECK:

        if ((ret = qsfp_dd_priv->fsm_func->module_ready_check(sff_obj, &ready)) < 0) {
            break;
        }
        if (ready) {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_READY);
        }
        break;
    case SFF_FSM_ST_MODULE_READY:

        sff_fsm_st_set(sff_obj, SFF_FSM_ST_READY);
        break;

    case SFF_FSM_ST_MODULE_HW_INIT:
    case SFF_FSM_ST_MODULE_LOOPBACK_INIT:

        if ((ret = qsfp_dd_module_st_get(sff_obj, &module_st)) < 0) {
            break;
        }
        /*check module state register if it transit to module ready*/
        if (MODULE_READY_ST_ENCODE == module_st) {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_READY);
        }
        break;
    case SFF_FSM_ST_READY:
            if ((ret = is_passive_module(sff_obj, &passive_module)) < 0) {
                break;
            }
            if (!passive_module) {
                if ((ret = qsfp_dd_intL_get(sff_obj, &lv)) < 0) {
                    break;
                }
                if (!lv) {
                    if ((ret = qsfp_dd_intr_module_flag_update(sff_obj)) < 0) {
                        break;
                    }
                    if ((ret = qsfp_dd_intr_ln_flag_update(sff_obj)) < 0 ) {
                        break;
                    }
                }
            }
        break;
    case SFF_FSM_ST_SUSPEND:
        break;
    case SFF_FSM_ST_RESTART:

        if ((ret = sff_prs_get(sff_obj, &lv)) < 0) {
            break;
        }
        if (lv) {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_DETECTING);
        } else {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_REMOVED);
        }
        break;
    case SFF_FSM_ST_FAULT:/*i2c bus fail ... etc*/

        break;
    case SFF_FSM_ST_UNKNOWN_TYPE:

        break;
    default:
        MODULE_LOG_ERR("unknown fsm st:%d\n", st);
        break;

    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}

