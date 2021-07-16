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
#include "../inv_swps.h"
#include "qsfp56.h"
/*note : it follows ACMIS rev 0.95a*/

typedef enum {

    MODULE_RESERVED1_ST_ENCODE,
    MODULE_LOW_PWR_ST_ENCODE,
    MODULE_PWR_UP_ST_ENCODE,
    MODULE_READY_ST_ENCODE,
    MODULE_PWR_DN_ST_ENCODE,
    MODULE_FAULT_ST_ENCODE,
    MODULE_RESERVED2_ST_ENCODE,
    MODULE_RESERVED3_ST_ENCODE,
    MODULE_ST_ENCODE_NUM,

} module_st_encode_t;
static const char *module_st_str[MODULE_ST_ENCODE_NUM] = {

    "N/A",
    "LOW_PWR_ST",
    "PWR_UP_ST",
    "READY_ST",
    "PWR_DN_ST",
    "FAULT_ST",
    "N/A",
    "N/A",
};

typedef enum {

    MODULE_TYPE_UNKNOWN = 0x0,
    MODULE_TYPE_MMF = 0x1,
    MODULE_TYPE_SMF = 0x2,
    MODULE_TYPE_PASSIVE_CU = 0x3,
    MODULE_TYPE_ACTIVE_CABLES = 0x4,
    MODULE_TYPE_BASE_T = 0x5,
    MODULE_TYPE_RESERVED,

} module_type_encode_t;

struct module_type_tbl_t {
    int type;
    char *name;
};
static const struct module_type_tbl_t module_type_tbl[] = {
    {.type = MODULE_TYPE_UNKNOWN, .name = "UNKNOWN"},
    {.type = MODULE_TYPE_MMF, .name = "MMF"},
    {.type = MODULE_TYPE_SMF, .name = "SMF"},
    {.type = MODULE_TYPE_PASSIVE_CU, .name = "PASSIVE_CU"},
    {.type = MODULE_TYPE_ACTIVE_CABLES, .name = "ACTIVE_CABLES"},
    {.type = MODULE_TYPE_BASE_T, .name = "BASE_T"},
    {.type = MODULE_TYPE_RESERVED, .name = "RESERVED"}
};
static const char *qsfp56_intr_module_flag_str[QSFP56_INTR_MODULE_FLAG_NUM] = {
    "MODULE_ST_CHG",
    "L_VCC_TEMP_WARN",
    "L_AUX_ALARM_WARN",
    "VENDOR_DEFINED_ERR",
    "RESERVERD",
    "CUSTOM",
};

enum {
    L_TXFAULT_ID,
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
    RESERVED_ID
};

static const char *qsfp56_intr_ln_flag_str[QSFP56_INTR_LN_FLAG_NUM] = {

    "L_TXFAULT",
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
    "reserved"
};

static const struct page_base_reg_t qsfp56_reg_map[QSFP56_REG_NUM] = {

    [QSFP56_REG_ID] = { .page = 0, .offset = 0, .len = 1 },
    [QSFP56_REG_ST_INDICATOR2] = { .page = 0, .offset = 2, .len = 1 },
    [QSFP56_REG_MODULE_ST_INTR] = { .page = 0, .offset = 3, .len = 1 },
    [QSFP56_REG_INTR_MODULE_FLAG] = { .page = 0, .offset = 8, .len = QSFP56_INTR_MODULE_FLAG_NUM },
    [QSFP56_REG_TEMP] = { .page = 0, .offset = 14, .len = WORD_SIZE },
    [QSFP56_REG_VCC] = { .page = 0, .offset = 16, .len = WORD_SIZE },
    [QSFP56_REG_MODULE_TYPE] = { .page = 0, .offset = 85, .len = 1 },
    [QSFP56_REG_PAGE_SEL] = { .page = 0, .offset = 127, .len = 1 },
    [QSFP56_REG_VENDOR_NAME] = { .page = 0, .offset = 129, .len = 16 },
    [QSFP56_REG_VENDOR_PN] = { .page = 0, .offset = 148, .len = 16 },
    [QSFP56_REG_VENDOR_SN] = { .page = 0, .offset = 166, .len = 16 },
    [QSFP56_REG_VENDOR_REV] = { .page = 0, .offset = 164, .len = 2 },
    [QSFP56_REG_TXDISABLE] = { .page = 0x10, .offset = 130, .len = 1 },
    [QSFP56_REG_LN_MONITOR_TX_PWR] = { .page = 0x11, .offset = 154, .len = QSFP56_REG_LN_MONITOR_NUM },
    [QSFP56_REG_LN_MONITOR_RX_PWR] = { .page = 0x11, .offset = 186, .len = QSFP56_REG_LN_MONITOR_NUM },
    [QSFP56_REG_LN_MONITOR_TX_BIAS] = { .page = 0x11, .offset = 170, .len = QSFP56_REG_LN_MONITOR_NUM },
    [QSFP56_REG_INTR_LN_FLAG] = { .page = 0x11, .offset = 135, .len = QSFP56_INTR_LN_FLAG_NUM },

};

inline static struct qsfp56_priv_data *qsfp56_priv_get(struct sff_obj_t *sff_obj)
{
    return &(sff_obj->priv_data.qsfp56);
}

static int qsfp56_paging_supported(struct sff_obj_t *sff_obj, bool *supported);
static int qsfp56_lpmode_set(struct sff_obj_t *sff_obj, u8 value);
static int qsfp56_intr_get(struct sff_obj_t *sff_obj, u8 *value);
static int qsfp56_lane_monitor_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
static int qsfp56_txdisable_set(struct sff_obj_t *sff_obj, u8 val);
static int qsfp56_txdisable_get(struct sff_obj_t *sff_obj, u8 *val);
static int qsfp56_id_get(struct sff_obj_t *sff_obj, u8 *val);
static bool qsfp56_is_id_matched(struct sff_obj_t *sff_obj);
static int qsfp56_vendor_info_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
static int qsfp56_module_st_get(struct sff_obj_t *sff_obj, u8 *st);
static int qsfp56_module_st_get_text(struct sff_obj_t *sff_obj, char *buf, int buf_size);
static int qsfp56_module_type_get_text(struct sff_obj_t *sff_obj, char *buf, int buf_size);
static int qsfp56_temperature_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
static int qsfp56_voltage_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
static int qsfp56_ln_st_get(struct sff_obj_t *sff_obj, int type, u8 *st);
static void qsfp56_intr_flag_clear(struct sff_obj_t *sff_obj);
static int qsfp56_intr_flag_show(struct sff_obj_t *sff_obj, char *buf, int size);
static int qsfp56_remove_op(struct sff_obj_t *sff_obj);
static int qsfp56_init_op(struct sff_obj_t *sff_obj);

struct func_tbl_t qsfp56_func_tbl = {
    .txdisable_set = qsfp56_txdisable_set,
    .txdisable_get = qsfp56_txdisable_get,
    .temperature_get = qsfp56_temperature_get,
    .voltage_get = qsfp56_voltage_get,
    .lane_monitor_get = qsfp56_lane_monitor_get,
    .vendor_info_get = qsfp56_vendor_info_get,
    .lane_status_get =  qsfp56_ln_st_get,
    .module_st_get = qsfp56_module_st_get_text,
    .module_type_get = qsfp56_module_type_get_text,
    .id_get = qsfp56_id_get,
    .is_id_matched = qsfp56_is_id_matched,
    .paging_supported = qsfp56_paging_supported,
    .intr_flag_show = qsfp56_intr_flag_show,
    .intr_flag_clear = qsfp56_intr_flag_clear,
    .remove_op = qsfp56_remove_op,
    .init_op = qsfp56_init_op
};

static bool qsfp56_reg_len_matched(struct sff_obj_t *sff_obj, int len, int def_len)
{
    if (len == def_len) {
        return true;
    } else {
        MODULE_LOG_ERR("%s fail len:%d def_len:%d\n", sff_obj->name, len, def_len);
        return false;
    }
}

static bool qsfp56_reg_found(struct sff_obj_t *sff_obj, qsfp56_reg_t id, const struct page_base_reg_t **reg)
{
    if (id < QSFP56_REG_ID || id >= QSFP56_REG_NUM) {
        MODULE_LOG_ERR("%s fail id:%d\n", sff_obj->name, id);
        return false;
    }
    *reg = &qsfp56_reg_map[id];
    return true;
}

static int qsfp56_lpmode_set(struct sff_obj_t *sff_obj, u8 value)
{
    return sff_lpmode_set(sff_obj, value);
}

static int qsfp56_intr_get(struct sff_obj_t *sff_obj, u8 *value)
{
    return sff_intr_get(sff_obj, value);
}

static int qsfp56_eeprom_read(struct sff_obj_t *sff_obj,
                              u8 page,
                              u8 offset,
                              u8 *buf,
                              int len)
{
    return sff_paged_eeprom_read(sff_obj, page, offset, buf, len);
}

static int qsfp56_eeprom_write(struct sff_obj_t *sff_obj,
                               u8 page,
                               u8 offset,
                               u8 *buf,
                               int len)
{
    return sff_paged_eeprom_write(sff_obj, page, offset, buf, len);
}

static int qsfp56_paging_support_check(struct sff_obj_t *sff_obj, bool *supported)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);

    if(!p_valid(supported)) {
        return -EINVAL;
    }
    if (!qsfp56_reg_found(sff_obj, QSFP56_REG_ST_INDICATOR2, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp56_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = qsfp56_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }

    if (data & bit_mask(QSFP56_FLAT_MEM_BIT)) {
        *supported = false;
        MODULE_LOG_DBG("%s paging is not supported!\n", sff_obj->name);
    } else {
        *supported = true;
    }
    /*caching*/
    sff_obj->priv_data.qsfp56.paging_supported = *supported;
    return 0;
}

static int qsfp56_paging_supported(struct sff_obj_t *sff_obj, bool *supported)
{
#if 0
    int ret = 0;
    if ((ret = qsfp56_paging_support_check(sff_obj, supported)) < 0) {
        return ret;
    }
#else
    /*<TBD> use cached instead*/
    *supported = sff_obj->priv_data.qsfp56.paging_supported;
#endif
    return 0;
}

static int qsfp56_module_ready_check(struct sff_obj_t *sff_obj, bool *ready)
{
    u8 module_st = 0;
    int ret = 0;
    bool tmp_ready = false;
    if ((ret = qsfp56_module_st_get(sff_obj, &module_st)) < 0) {
        return ret;
    }
    /*check module state register if it transit to module ready*/
    if (MODULE_READY_ST_ENCODE == module_st) {
        /*set txdisable false, */
        if ((ret = qsfp56_txdisable_set(sff_obj, 0x0)) < 0) {
            return ret;
        }
        tmp_ready = true;
    }

    *ready = tmp_ready;
    return 0;
}
#if 0
static int cmis_ver_get(struct sff_obj_t *sff_obj, u8 *ver)
{
    int ret = 0;
    u8 data = 0;
    const struct page_base_reg_t *reg = NULL;
    int len = sizeof(data);

    if (!p_valid(ver)) {
        return -EINVAL;
    }
    if (!qsfp56_reg_found(sff_obj, QSFP56_REG_CMIS_REV, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp56_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = qsfp56_eeprom_read(sff_obj, reg->page, reg->offset, &data, len)) < 0) {

        MODULE_LOG_ERR("%s qsfp56_eeprom_read fail", sff_obj->name);
        return ret;
    }
    MODULE_LOG_DBG("%s cmis ver:0x%x\n", sff_obj->name, data);
    *ver = data;
    return 0;
}
#endif
#if 0
static bool is_bank_num_valid(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    u8 bank_no = 0xff;
    bool valid = false;
    u8 addr = SFF_EEPROM_I2C_ADDR;
    u8 offset = QSFP56_BANK_SEL_OFFSET;

    /*check if it's bank0? */
    if((ret = sff_eeprom_read(sff_obj, addr, offset, &bank_no, sizeof(bank_no))) < 0) {
        MODULE_LOG_ERR("%s qsfp56_eeprom_read fail", sff_obj->name);
        return valid;
    }
    if (0x00 == bank_no) {
        valid = true;
    } else {
        MODULE_LOG_ERR("%s bank no is not 0", sff_obj->name);
    }

    return valid;
}
#endif
static int qsfp56_temperature_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size)
{
    int ret = 0;
    s16 temp = 0;
    s16 divider = 256;
    s16 decimal = 0;
    char *unit = "c";
    const struct page_base_reg_t *reg = NULL;
    u8 data[WORD_SIZE];
    int len = sizeof(data);

    if (!qsfp56_reg_found(sff_obj, QSFP56_REG_TEMP, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp56_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = qsfp56_eeprom_read(sff_obj, reg->page, reg->offset, data, len)) < 0) {
        return ret;
    }

    temp = (data[0] << 8) | data[1];
    decimal = ((temp/divider)*divider)-temp;
    decimal = abs(decimal);
    decimal = decimal*1000/divider;

    return scnprintf(buf, buf_size,
                     "%d.%d %s\n",
                     temp/divider,
                     decimal, unit);

}

/*Table 22- Module Monitors (Lower Page, active modules only)
 *  * offset 16,17*/
static int qsfp56_voltage_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size)
{
    int ret = 0;
    u16 vol = 0;
    u16 divider = 10000;
    char *unit = "v";
    const struct page_base_reg_t *reg = NULL;
    u8 data[WORD_SIZE];
    int len = sizeof(data);

    if (!qsfp56_reg_found(sff_obj, QSFP56_REG_VCC, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp56_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = qsfp56_eeprom_read(sff_obj, reg->page, reg->offset, data, len)) < 0) {
        return ret;
    }

    vol = (data[0] << 8) | data[1];
    return scnprintf(buf, buf_size,
                     "%d.%d %s\n",
                     vol/divider,
                     vol%divider, unit);
}

/*1.7.7.3  Lane-Specific Monitors */
/*Table 70- Lane-Specific Monitors (Page 11h, active modules only) */
static int qsfp56_lane_monitor_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size)
{
    int ret = 0;
    int count = 0;
    int idx = 0;
    u16 ch_data = 0;
    u16 divider = 10000;
    char *unit = NULL;
    qsfp56_reg_t id = QSFP56_REG_UNKNOWN;
    const struct page_base_reg_t *reg = NULL;
    struct monitor_para_t *para = NULL;
    u8 ch_monitor[QSFP56_REG_LN_MONITOR_NUM]; /* 2(u16 data) *4 (channel)*/
    int len = sizeof(ch_monitor);

    switch(type) {
    case LN_MONITOR_TX_PWR_TYPE:
        id = QSFP56_REG_LN_MONITOR_TX_PWR;
        break;
    case LN_MONITOR_RX_PWR_TYPE:
        id = QSFP56_REG_LN_MONITOR_RX_PWR;
        break;
    case LN_MONITOR_TX_BIAS_TYPE:
        id = QSFP56_REG_LN_MONITOR_TX_BIAS;
        break;
    default:
        break;
    }

    if (!qsfp56_reg_found(sff_obj, id, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp56_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = qsfp56_eeprom_read(sff_obj, reg->page, reg->offset, ch_monitor, len)) < 0) {
        return ret;
    }
    para = monitor_para_find(type);
    if (!p_valid(para)) {
        return -EBADRQC;
    }
    divider = para->divider;
    unit = para->unit;
    for (idx = 0; idx < len; idx += WORD_SIZE) {
        /*big edian*/
        ch_data = (ch_monitor[idx] << 8) | ch_monitor[idx+1];

        count += scnprintf(buf+count, buf_size-count,
                           "ln%d: %d.%d %s\n",
                           (idx >> 1) + 1, ch_data/divider,
                           ch_data%divider, unit);
    }

    return count;
}
/*rev3 1.7.3.2*/
static int qsfp56_vendor_info_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size)
{
    int ret = 0;
    qsfp56_reg_t id = QSFP56_REG_UNKNOWN;
    const struct page_base_reg_t *reg = NULL;
    u8 byte_data[VENDOR_INFO_BUF_SIZE];
    int len = sizeof(byte_data);

    switch(type) {
    case VENDOR_NAME_TYPE:
        id = QSFP56_REG_VENDOR_NAME;
        break;
    case VENDOR_PN_TYPE:
        id = QSFP56_REG_VENDOR_PN;
        break;
    case VENDOR_SN_TYPE:
        id = QSFP56_REG_VENDOR_SN;
        break;
    case VENDOR_REV_TYPE:
        id = QSFP56_REG_VENDOR_REV;
        break;
    default:
        break;
    }

    if (!qsfp56_reg_found(sff_obj, id, &reg)) {
        return -EBADRQC;
    }
    /*use a big enough buf to handle all the vendor info*/
    if (reg->len >= len) {
        return -EBADRQC;
    }

    if((ret = qsfp56_eeprom_read(sff_obj, reg->page, reg->offset, byte_data, reg->len)) < 0) {
        return ret;
    }
    /*add terminal of string*/
    byte_data[reg->len] = '\0';
    MODULE_LOG_DBG("%s page:0x%x offset:0x%x %s", sff_obj->name, reg->page, reg->offset, byte_data);
    return scnprintf(buf, buf_size, "%s\n", byte_data);

}
/*Table 18- Identifier and Status Summary (Lower Page) byte3 , 3-1 Module state*/
static int qsfp56_module_st_get(struct sff_obj_t *sff_obj, u8 *st)
{
    u8 data = 0;
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    int len = sizeof(data);

    if (!st) {
        return -EINVAL;
    }
    if (!qsfp56_reg_found(sff_obj, QSFP56_REG_MODULE_ST_INTR, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp56_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = qsfp56_eeprom_read(sff_obj, reg->page, reg->offset, &data, len)) < 0) {

        MODULE_LOG_ERR("%s qsfp56_eeprom_read fail", sff_obj->name);
        return ret;
    }

    *st = bits_get(data, QSFP56_MODULE_ST_BIT_MIN, QSFP56_MODULE_ST_BIT_NUM);
    return 0;
}
static int qsfp56_module_st_get_text(struct sff_obj_t *sff_obj, char *buf, int buf_size)
{
    int ret = 0;
    u8 st = 0;

    if ((ret = qsfp56_module_st_get(sff_obj, &st)) < 0) {
        return ret;
    }
    return scnprintf(buf, buf_size,"%s\n", module_st_str[st]);
}
/*Table 18- Identifier and Status Summary (Lower Page)
 *  * lower page :offset 0　*/
static int qsfp56_id_get(struct sff_obj_t *sff_obj, u8 *val)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);

    if (!p_valid(val)) {
        return -EINVAL;
    }
    if (!qsfp56_reg_found(sff_obj, QSFP56_REG_ID, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp56_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = qsfp56_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }
    *val = data;
    return 0;
}
static bool qsfp56_is_id_matched(struct sff_obj_t *sff_obj)
{
    u8 id = 0;
    bool match = false;
    if (qsfp56_id_get(sff_obj, &id) < 0) {
        return match;
    }
    switch (id) {
    case SFF_8024_ID_QSFP56:
        match = true;
        break;
    default:
        MODULE_LOG_ERR("%s not match id:%d\n", sff_obj->name, id);
        break;
    }

    return match;
}

/*Table 25- Byte 85 Module Type Encodings*/
static int qsfp56_module_type_get(struct sff_obj_t *sff_obj, u8 *type)
{
    u8 data = 0;
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    int len = sizeof(data);

    if (!p_valid(type)) {
        return -EINVAL;
    }
    if (!qsfp56_reg_found(sff_obj, QSFP56_REG_MODULE_TYPE, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp56_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = qsfp56_eeprom_read(sff_obj, reg->page, reg->offset, &data, len)) < 0) {
        return ret;
    }
    *type = data;
    return 0;
}
static int qsfp56_module_type_get_text(struct sff_obj_t *sff_obj, char *buf, int buf_size)
{
    int ret = 0;
    u8 type = 0;
    int i = 0;
    if ((ret = qsfp56_module_type_get(sff_obj, &type)) < 0) {
        return ret;
    }
    for (i = 0; MODULE_TYPE_RESERVED != module_type_tbl[i].type; i++) {
        if (type == module_type_tbl[i].type) {
            break;
        }
    }
    return scnprintf(buf, buf_size,"%s\n", module_type_tbl[i].name);
}

/*table 54 page 10h offset 130 */
static int qsfp56_txdisable_set(struct sff_obj_t *sff_obj, u8 val)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    int len = sizeof(val);
#if 0
    if (!is_bank_num_valid(sff_obj)) {
        return -EBADRQC;
    }
#endif
    if (!qsfp56_reg_found(sff_obj, QSFP56_REG_TXDISABLE, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp56_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = qsfp56_eeprom_write(sff_obj, reg->page, reg->offset, &val, len)) < 0) {
        return ret;
    }

    return 0;
}
/*table 54 page 10h offset 130 */
static int qsfp56_txdisable_get(struct sff_obj_t *sff_obj, u8 *val)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);

    if (!p_valid(val)) {
        return -EINVAL;
    }
#if 0
    if (!is_bank_num_valid(sff_obj)) {
        return -EBADRQC;
    }
#endif
    if (!qsfp56_reg_found(sff_obj, QSFP56_REG_TXDISABLE, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp56_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = qsfp56_eeprom_read(sff_obj, reg->page, reg->offset, &data, len)) < 0) {
        return -ret;
    }
    *val = data;
    return 0;
}
/*optical: true copper: false*/
static bool is_passive_module(struct sff_obj_t *sff_obj, bool *passive_module)
{
    int ret = 0;
    bool tmp_passive_module = false;
    u8 module_type = 0;

    if ((ret = qsfp56_module_type_get(sff_obj, &module_type)) < 0) {
        return ret;
    }

    switch (module_type) {

    case MODULE_TYPE_PASSIVE_CU:
        tmp_passive_module = true;
        break;
    case MODULE_TYPE_UNKNOWN:
    case MODULE_TYPE_MMF:
    case MODULE_TYPE_SMF:
    case MODULE_TYPE_ACTIVE_CABLES:
    case MODULE_TYPE_BASE_T:
    default:
        break;

    }
    *passive_module = tmp_passive_module;
    return 0;
}

/*table 15*/
static int qsfp56_intr_module_flag_update(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    int i = 0;
    u32 *cnt = NULL;
    u32 old_cnt = 0;
    struct qsfp56_priv_data *qsfp56_priv = qsfp56_priv_get(sff_obj);
    const struct page_base_reg_t *reg = NULL;
    u8 module_flag[QSFP56_INTR_MODULE_FLAG_NUM];
    int len = sizeof(module_flag);

    if (!qsfp56_reg_found(sff_obj, QSFP56_REG_INTR_MODULE_FLAG, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp56_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = qsfp56_eeprom_read(sff_obj, reg->page, reg->offset, module_flag, reg->len)) < 0) {
        return ret;
    }
    for (i = 0; i < len; i++) {
        cnt = &(qsfp56_priv->intr_module_flag[i].cnt);
        old_cnt = qsfp56_priv->intr_module_flag[i].cnt;
        qsfp56_priv->intr_module_flag[i].reg = module_flag[i];

        if (module_flag[i] != 0x00) {
            cnt_increment_limit(cnt);
            if (int_flag_monitor_en) {
                MODULE_LOG_ERR("%s error %s:0x%x", sff_obj->name, qsfp56_intr_module_flag_str[i], module_flag[i]);
            }
        }

        if (old_cnt != *cnt) {
            qsfp56_priv->intr_module_flag[i].chg = true;
        } else {
            qsfp56_priv->intr_module_flag[i].chg = false;
        }
    }

    return 0;
}

static int qsfp56_lane_status_update(struct sff_obj_t *sff_obj, int type, u8 value)
{
    if (type > LN_STATUS_NUM ||
            type < LN_STATUS_RXLOS_TYPE) {
        return -EINVAL;
    }

    sff_obj->priv_data.qsfp56.lane_st[type] = value;

    return 0;
}
/*Table 68 TX Flags , 69- RX Flags (Page 11h, active modules only)*/
static int qsfp56_intr_ln_flag_update(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    int i = 0;
    u32 *cnt = NULL;
    u32 old_cnt = 0;
    struct qsfp56_priv_data *qsfp56_priv = qsfp56_priv_get(sff_obj);
    const struct page_base_reg_t *reg = NULL;
    u8 ln_flag[QSFP56_INTR_LN_FLAG_NUM];
    int len = sizeof(ln_flag);

    if (!qsfp56_reg_found(sff_obj, QSFP56_REG_INTR_LN_FLAG, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp56_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = qsfp56_eeprom_read(sff_obj,
                                  reg->page,
                                  reg->offset,
                                  ln_flag,
                                  reg->len)) < 0) {
        return ret;
    }
    qsfp56_lane_status_update(sff_obj, LN_STATUS_RXLOS_TYPE, ln_flag[L_RX_LOS_ID]);

    for (i = 0; i < len; i++) {
        cnt = (&qsfp56_priv->intr_ln_flag[i].cnt);
        old_cnt = qsfp56_priv->intr_ln_flag[i].cnt;
        qsfp56_priv->intr_ln_flag[i].reg = ln_flag[i];

        if (ln_flag[i] != 0x00) {
            cnt_increment_limit(cnt);
            if (int_flag_monitor_en) {
                MODULE_LOG_ERR("%s chg %s:0x%x", sff_obj->name, qsfp56_intr_ln_flag_str[i], ln_flag[i]);
            }
        }
        if (old_cnt != *cnt) {
            qsfp56_priv->intr_ln_flag[i].chg = true;
        } else {
            qsfp56_priv->intr_ln_flag[i].chg = false;
        }
    }

    return 0;
}

static void qsfp56_intr_flag_clear(struct sff_obj_t *sff_obj)
{
    struct qsfp56_priv_data *qsfp56_priv = qsfp56_priv_get(sff_obj);
    memset(qsfp56_priv->intr_module_flag, 0, sizeof(qsfp56_priv->intr_module_flag));
    memset(qsfp56_priv->intr_ln_flag, 0, sizeof(qsfp56_priv->intr_ln_flag));
}

static int qsfp56_intr_flag_show(struct sff_obj_t *sff_obj, char *buf, int size)
{
    int count = 0;
    int i = 0;
    u32 cnt = 0;
    char chg = ' ';
    u8 intr = 0;
    int ret = 0;
    struct qsfp56_priv_data *qsfp56_priv = qsfp56_priv_get(sff_obj);

    /*handle transition back to normal state from intr state  {*/
    if ((ret = qsfp56_intr_get(sff_obj, &intr)) < 0) {
        return ret;
    }
    if (intr) {
        if ((ret = qsfp56_intr_module_flag_update(sff_obj)) < 0) {
            return ret;
        }
        if ((ret = qsfp56_intr_ln_flag_update(sff_obj)) < 0 ) {
            return ret;
        }
    }
    /*handle transition back to normal state from intr state  }*/

    count += scnprintf(buf+count, size-count,
                       "intr module flag show:\n");
    for (i = 0; i < QSFP56_INTR_MODULE_FLAG_NUM; i++) {

        cnt = qsfp56_priv->intr_module_flag[i].cnt;
        if (0 != cnt) {
            if (qsfp56_priv->intr_module_flag[i].chg) {
                chg = '*';
            } else {
                chg = ' ';
            }
            count += scnprintf(buf+count, size-count,
                               "%s reg:0x%x cnt:%d%c\n",
                               qsfp56_intr_module_flag_str[i],
                               qsfp56_priv->intr_module_flag[i].reg,
                               cnt,
                               chg);
        }
    }
    count += scnprintf(buf+count, size-count,
                       "intr lane flag show:\n");
    for (i = 0; i < QSFP56_INTR_LN_FLAG_NUM; i++) {
        cnt = qsfp56_priv->intr_ln_flag[i].cnt;
        if (0 != cnt) {
            if (qsfp56_priv->intr_ln_flag[i].chg) {
                chg = '*';
            } else {
                chg = ' ';
            }
            count += scnprintf(buf+count, size-count,
                               "%s reg:0x%x cnt:%d%c\n",
                               qsfp56_intr_ln_flag_str[i],
                               qsfp56_priv->intr_ln_flag[i].reg,
                               cnt,
                               chg);
        }
    }
    return count;
}
static int qsfp56_ln_st_get(struct sff_obj_t *sff_obj, int type, u8 *st)
{
    u8 intr = 0;
    int ret = 0;
    struct qsfp56_priv_data *qsfp56_priv = qsfp56_priv_get(sff_obj);

    if ((ret = qsfp56_intr_get(sff_obj, &intr)) < 0) {
        return ret;
    }
    if (intr) {
        if ((ret = qsfp56_intr_module_flag_update(sff_obj)) < 0) {
            return ret;
        }
        if ((ret = qsfp56_intr_ln_flag_update(sff_obj)) < 0 ) {
            return ret;
        }
    }

    *st = qsfp56_priv->lane_st[type];
    return 0;
}
static void cache_clear(struct sff_obj_t *sff_obj)
{
    memset(&(sff_obj->priv_data.qsfp56), 0, sizeof(struct qsfp56_priv_data));
}

static int qsfp56_init_op(struct sff_obj_t *sff_obj)
{
    /*default loading*/
    return 0;
}
static int qsfp56_remove_op(struct sff_obj_t *sff_obj)
{
    cache_clear(sff_obj);
    return 0;
}
int sff_fsm_qsfp56_task(struct sff_obj_t *sff_obj)
{
    sff_fsm_state_t st = sff_fsm_st_get(sff_obj);
    int ret = 0;
    u8 lv = 0;
    bool ready = false;
    bool passive_module = false;
    bool supported = false;

    switch (st) {

    case SFF_FSM_ST_DETECTED:
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_DETECTING);
        break;
    case SFF_FSM_ST_DETECTING:
        if ((ret = qsfp56_paging_support_check(sff_obj, &supported)) < 0) {
            return ret;
        }

        sff_fsm_st_set(sff_obj, SFF_FSM_ST_INIT);
        break;

    case SFF_FSM_ST_INIT:
        /*set txdisable true to avoid link flapping <TBD> 4 lane 0xf ?*/
        if ((ret = qsfp56_txdisable_set(sff_obj, 0xff)) < 0) {
            break;
        }

        if ((ret = qsfp56_lpmode_set(sff_obj, 0)) < 0) {
            break;
        }
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_READY_CHECK);
        break;
    case SFF_FSM_ST_MODULE_READY_CHECK:

        if ((ret = qsfp56_module_ready_check(sff_obj, &ready)) < 0) {
            break;
        }
        if (ready) {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_READY);
        }
        break;
    case SFF_FSM_ST_MODULE_READY:

        sff_fsm_st_set(sff_obj, SFF_FSM_ST_READY);
        break;

    case SFF_FSM_ST_READY:
        if ((ret = is_passive_module(sff_obj, &passive_module)) < 0) {
            break;
        }
        if (!passive_module) {
            if ((ret = qsfp56_intr_get(sff_obj, &lv)) < 0) {
                break;
            }
            if (!lv) {
                if ((ret = qsfp56_intr_module_flag_update(sff_obj)) < 0) {
                    break;
                }
                if ((ret = qsfp56_intr_ln_flag_update(sff_obj)) < 0 ) {
                    break;
                }
            }
        }
        break;
    case SFF_FSM_ST_FAULT:/*i2c bus fail ... etc*/
    case SFF_FSM_ST_IDLE:
    case SFF_FSM_ST_REMOVED:
    case SFF_FSM_ST_ISOLATED:
    case SFF_FSM_ST_SUSPEND:
    case SFF_FSM_ST_RESTART:
    case SFF_FSM_ST_UNKNOWN_TYPE:
    default:
        break;

    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}

