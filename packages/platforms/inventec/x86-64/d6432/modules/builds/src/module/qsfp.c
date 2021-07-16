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
#include "qsfp.h"

static int qsfp_init_op(struct sff_obj_t *sff_obj);
static int qsfp_lpmode_set(struct sff_obj_t *sff_obj, u8 value);
static int qsfp_vendor_info_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
static int qsfp_temperature_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
static int qsfp_voltage_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
static int qsfp_lane_status_get(struct sff_obj_t *sff_obj, int type, u8 *value);
static int qsfp_lane_monitor_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
static int qsfp_lane_control_set(struct sff_obj_t *sff_obj, int type, u32 value);
static int qsfp_lane_control_get(struct sff_obj_t *sff_obj, int type, u32 *value);
static int qsfp_txdisable_set(struct sff_obj_t *sff_obj, u8 value);
static int qsfp_txdisable_get(struct sff_obj_t *sff_obj, u8 *value);
static int qsfp_id_get(struct sff_obj_t *sff_obj, u8 *id);
static bool qsfp_is_id_matched(struct sff_obj_t *sff_obj);
static int qsfp_lpmode_set(struct sff_obj_t *sff_obj, u8 value);
static int qsfp_intr_get(struct sff_obj_t *sff_obj, u8 *value);
static int qsfp_remove_op(struct sff_obj_t *sff_obj);
static void qsfp_intr_flag_clear(struct sff_obj_t *sff_obj);
static int qsfp_intr_flag_show(struct sff_obj_t *sff_obj, char *buf, int size);
static int qsfp_paging_supported(struct sff_obj_t *sff_obj, bool *supported);

static const u8 Qsfp_Eth_Comp_Table[] = {
    ACTIVE_CABLE,
    LR4_40GBASE,
    SR4_40GBASE,
    CR4_40GBASE,
};
static const struct page_base_reg_t qsfp_reg_map[QSFP_REG_NUM] = {

    [QSFP_REG_ID] = { .page = 0, .offset = 0, .len = 1 },
    [QSFP_REG_ST_INDICATOR2] = { .page = 0, .offset = 2, .len = 1 },
    [QSFP_REG_TEMP] = { .page = 0, .offset = 22, .len = WORD_SIZE },
    [QSFP_REG_VCC] = { .page = 0, .offset = 26, .len = WORD_SIZE },
    [QSFP_REG_LN_MONITOR_RX_PWR] = { .page = 0, .offset = 34, .len = QSFP_LN_MONITOR_NUM },
    [QSFP_REG_LN_MONITOR_TX_BIAS] = { .page = 0, .offset = 42, .len = QSFP_LN_MONITOR_NUM },
    [QSFP_REG_LN_MONITOR_TX_PWR] = { .page = 0, .offset = 50, .len = QSFP_LN_MONITOR_NUM },
    [QSFP_REG_TXDISABLE] = { .page = 0, .offset = 86, .len = 1 },
    [QSFP_REG_CDR_CONTROL] = { .page = 0, .offset = 98, .len = 1 },
    [QSFP_REG_EXT_ID] = { .page = 0, .offset = 129, .len = 1 },
    [QSFP_REG_ETH_COMP] = { .page = 0, .offset = 131, .len = 1 },
    [QSFP_REG_EXT_COMP] = { .page = 0, .offset = 192, .len = 1 },
    [QSFP_REG_VENDOR_NAME] = { .page = 0, .offset = 148, .len = 16 },
    [QSFP_REG_VENDOR_PN] = { .page = 0, .offset = 168, .len = 16 },
    [QSFP_REG_VENDOR_REV] = { .page = 0, .offset = 184, .len = 2 },
    [QSFP_REG_VENDOR_SN] = { .page = 0, .offset = 196, .len = 16 },
    [QSFP_REG_OPT_LN_CONTROL_TX_EQ] = { .page = 0x3, .offset = 234, .len = QSFP_LN_CTRL_NUM },
    [QSFP_REG_OPT_LN_CONTROL_RX_EM] = { .page = 0x3, .offset = 236, .len = QSFP_LN_CTRL_NUM },
    [QSFP_REG_OPT_LN_CONTROL_RX_AM] = { .page = 0x3, .offset = 238, .len = QSFP_LN_CTRL_NUM },
    [QSFP_REG_PAGE_SEL] = { .page = 0, .offset = 127, .len = 1 },
    [QSFP_REG_INTR_FLAG] = { .page = 0, .offset = 3, .len = QSFP_INTR_FLAG_NUM },

};

static const char *qsfp_intr_flag_str[QSFP_INTR_FLAG_NUM] = {
    [L_RX_TX_LOS_ID] = "L_RX_TX_LOS",
    [L_TXFAULT_ID] = "L_TXFAULT",
    [L_RX_TX_LOL_ID] = "L_RX_TX_LOL",
    [L_TEMP_ALARM_WARN_ID] = "L_TEMP_ALARM_WARN",
    [L_VCC_ALARM_WARN_ID] = "L_VCC_ALARM_WARN",
    [L_VENDOR_SPEC_ID] = "L_VENDOR_SPEC",
    [L_RX_CH12_POWER_ALARM_WARN_ID] = "L_RX_CH12_POWER_ALARM_WARN",
    [L_RX_CH34_POWER_ALARM_WARN_ID] = "L_RX_CH34_POWER_ALARM_WARN",
    [L_TX_CH12_BIAS_ALARM_WARN_ID] = "L_TX_CH12_BIAS_ALARM_WARN",
    [L_TX_CH34_BIAS_ALARM_WARN_ID] = "L_TX_CH34_BIAS_ALARM_WARN",
    [L_TX_CH12_POWER_ALARM_WARN_ID] = "L_TX_CH12_POWER_ALARM_WARN",
    [L_TX_CH34_POWER_ALARM_WARN_ID] = "L_TX_CH34_POWER_ALARM_WARN",
    [L_RESERVED1_0_ID] = "L_RESERVED1_0",
    [L_RESERVED1_1_ID] = "L_RESERVED1_1",
    [L_RESERVED2_0_ID] = "L_RESERVED2_0",
    [L_RESERVED2_1_ID] = "L_RESERVED2_1",
    [L_VENDOR_SPEC2_0_ID] = "L_VENDOR_SPEC2_0",
    [L_VENDOR_SPEC2_1_ID] = "L_VENDOR_SPEC2_1",
    [L_VENDOR_SPEC2_2_ID] = "L_VENDOR_SPEC2_2",
};

struct func_tbl_t qsfp_func_tbl = {
    .txdisable_set = qsfp_txdisable_set,
    .txdisable_get = qsfp_txdisable_get,
    .temperature_get = qsfp_temperature_get,
    .voltage_get = qsfp_voltage_get,
    .lane_control_set = qsfp_lane_control_set,
    .lane_control_get = qsfp_lane_control_get,
    .lane_monitor_get = qsfp_lane_monitor_get,
    .vendor_info_get = qsfp_vendor_info_get,
    .lane_status_get =  qsfp_lane_status_get,
    .id_get = qsfp_id_get,
    .is_id_matched = qsfp_is_id_matched,
    .paging_supported = qsfp_paging_supported,
    .remove_op = qsfp_remove_op,
    .intr_flag_show = qsfp_intr_flag_show,
    .intr_flag_clear = qsfp_intr_flag_clear,
    .init_op = qsfp_init_op 
};
static int qsfp_priv_data_allocate(struct sff_obj_t *sff_obj)
{
    if (!p_valid(sff_obj->priv_data)) {
        sff_obj->priv_data = kzalloc(sizeof(struct qsfp_priv_data), GFP_KERNEL);
        if (!p_valid(sff_obj->priv_data)) {
            return -ENOMEM;
        }
    }
    return 0;
}

static void qsfp_priv_data_free(struct sff_obj_t *sff_obj)
{
    if (p_valid(sff_obj->priv_data)) {
        kfree(sff_obj->priv_data);
        sff_obj->priv_data = NULL;
    }
}

inline static struct qsfp_priv_data *qsfp_priv_data_get(struct sff_obj_t *sff_obj)
{
    struct qsfp_priv_data *priv = NULL;

    priv = (struct qsfp_priv_data *)sff_obj->priv_data;
    return priv;
}

static int qsfp_init_op(struct sff_obj_t *sff_obj)
{
    int ret = 0;

    if ((ret = qsfp_priv_data_allocate(sff_obj)) < 0) {
        return ret;
    }
    MODULE_LOG_DBG("%s\n", sff_obj->name);
    return 0;
}

static int qsfp_remove_op(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    /*always make sure, module plugged in with lower power mode*/
    if((ret = qsfp_lpmode_set(sff_obj, 1)) < 0) {
        return ret;
    }
    qsfp_priv_data_free(sff_obj);

    return 0;
}

static bool qsfp_reg_len_matched(struct sff_obj_t *sff_obj, int len, int def_len)
{
    bool matched = false;

    if (!(matched = reg_len_matched(len, def_len))) {
        MODULE_LOG_ERR("%s fail len:%d def_len:%d\n", sff_obj->name, len, def_len);
    }
    return matched;
}

static bool qsfp_reg_found(struct sff_obj_t *sff_obj, qsfp_reg_t id, const struct page_base_reg_t **reg)
{
    if (id < QSFP_REG_ID || id >= QSFP_REG_NUM) {
        MODULE_LOG_ERR("%s fail id:%d\n", sff_obj->name, id);
        return false;
    }
    *reg = &qsfp_reg_map[id];
    return true;
}
static int qsfp_lpmode_set(struct sff_obj_t *sff_obj, u8 value)
{
    return sff_lpmode_set(sff_obj, value);
}

static int qsfp_lpmode_get(struct sff_obj_t *sff_obj, u8 *value)
{
    return sff_lpmode_get(sff_obj, value);
}

static int qsfp_intr_get(struct sff_obj_t *sff_obj, u8 *value)
{
    return sff_intr_get(sff_obj, value);
}

static int qsfp_eeprom_read(struct sff_obj_t *sff_obj,
                            u8 page,
                            u8 offset,
                            u8 *buf,
                            int len)
{
    return sff_paged_eeprom_read(sff_obj, page, offset, buf, len);
}
static int qsfp_eeprom_write(struct sff_obj_t *sff_obj,
                             u8 page,
                             u8 offset,
                             u8 *buf,
                             int len)
{
    return sff_paged_eeprom_write(sff_obj, page, offset, buf, len);
}
static int qsfp_paging_support_check(struct sff_obj_t *sff_obj, bool *supported)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);
    struct qsfp_priv_data *qsfp_priv = qsfp_priv_data_get(sff_obj);

    qsfp_priv->paging_supported.valid = false;
    
    if(!p_valid(supported)) {
        return -EINVAL;
    }
    if (!qsfp_reg_found(sff_obj, QSFP_REG_ST_INDICATOR2, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = qsfp_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }

    if (data & bit_mask(FLAT_MEM_BIT)) {
        *supported = false;
        MODULE_LOG_DBG("%s paging is not supported!\n", sff_obj->name);
    } else {
        *supported = true;
    }
    /*caching*/
    qsfp_priv->paging_supported.val = *supported;
    qsfp_priv->paging_supported.valid = true;

    return 0;
}

static int qsfp_paging_supported(struct sff_obj_t *sff_obj, bool *supported)
{
    int ret = 0;
    struct qsfp_priv_data *qsfp_priv = qsfp_priv_data_get(sff_obj);
    
    if (qsfp_priv->paging_supported.valid) {
        /*use cache*/
        *supported = qsfp_priv->paging_supported.val;
        MODULE_LOG_DBG("%s cached is used!\n", sff_obj->name);

    } else {
        if ((ret = qsfp_paging_support_check(sff_obj, supported)) < 0) {
            return ret;
        }
    }    
    
    return 0;
}

static int qsfp_id_get(struct sff_obj_t *sff_obj, u8 *id)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);

    if (!p_valid(id)) {
        return -EINVAL;
    }
    if (!qsfp_reg_found(sff_obj, QSFP_REG_ID, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = qsfp_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }
    *id = data;
    return 0;
}
static bool qsfp_is_id_matched(struct sff_obj_t *sff_obj)
{
    u8 id = 0;
    bool match = false;
    if (qsfp_id_get(sff_obj, &id) < 0) {
        return match;
    }
    switch (id) {
    case 0x11:
    case 0xc:
        match = true;
        break;
    default:
        MODULE_LOG_ERR("%s not match id:%d\n", sff_obj->name, id);
        break;
    }

    return match;
}
static int qsfp_ext_id_get(struct sff_obj_t *sff_obj, u8 *ext_id)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);

    if(!p_valid(ext_id)) {
        return -EINVAL;
    }

    if (!qsfp_reg_found(sff_obj, QSFP_REG_EXT_ID, &reg)) {
        return -EBADRQC;
    }

    if (!qsfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }

    if ((ret = qsfp_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }
    *ext_id = data;
    return 0;
}
static int _qsfp_data_ready_check(struct sff_obj_t *sff_obj, bool *ready)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);
    bool tmp_ready = false;
    if(!p_valid(ready)) {
        return -EINVAL;
    }

    if (!qsfp_reg_found(sff_obj, QSFP_REG_ST_INDICATOR2, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = qsfp_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }
    if (data & bit_mask(DATA_NOT_READY_BIT)) {
        tmp_ready = false;
    } else {
        tmp_ready = true;
    }

    *ready = tmp_ready;
    return 0;
}
static int qsfp_eth_comp_get(struct sff_obj_t *sff_obj, u8 *eth_comp)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);

    if(!p_valid(eth_comp)) {
        MODULE_LOG_ERR("invalid para");
        return -EINVAL;
    }
    if (!qsfp_reg_found(sff_obj, QSFP_REG_ETH_COMP, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = qsfp_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }
    *eth_comp = data;

    return 0;
}
static int _qsfp_ext_comp_get(struct sff_obj_t *sff_obj, u8 *ext_comp)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);

    if(!p_valid(ext_comp)) {
        MODULE_LOG_ERR("invalid para");
        return -EINVAL;
    }

    if (!qsfp_reg_found(sff_obj, QSFP_REG_EXT_COMP, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = qsfp_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }
    *ext_comp = data;

    return 0;
}
static bool qsfp_eth_comp_is_supported(struct sff_obj_t *sff_obj, u8 eth_comp)
{
    bool is_supported = false;
    int size = ARRAY_SIZE(Qsfp_Eth_Comp_Table);
    int idx = 0;
    u8 comp_codes = 0;
    for (idx = 0; idx < size; idx++) {
        if(eth_comp & Qsfp_Eth_Comp_Table[idx]) {
            comp_codes = eth_comp & Qsfp_Eth_Comp_Table[idx];
            MODULE_LOG_DBG("%s known eth_cmp:0x%x\n", sff_obj->name, eth_comp & Qsfp_Eth_Comp_Table[idx]);
            break;
        }
    }
    if(idx >= size) {

        MODULE_LOG_DBG("%s unknown eth_cmp:0x%x\n", sff_obj->name, eth_comp);
    }

    switch (comp_codes) {
    case ACTIVE_CABLE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_40G_AOC);
        break;
    case LR4_40GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_40G_LR4);
        break;
    case SR4_40GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_40G_SR4);
        break;
    case CR4_40GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_COPPER_L4_40G);
        break;
    default:
        break;
    }

    if (TRANSVR_CLASS_UNKNOWN != transvr_type_get(sff_obj)) {
        is_supported = true;
        MODULE_LOG_DBG("%s known eth_ext_cmp:0x%x transvr:%d\n",
                       sff_obj->name, eth_comp, transvr_type_get(sff_obj));
    }
    return is_supported;
}
static bool qsfp_ext_comp_is_supported(struct sff_obj_t *sff_obj, u8 ext_comp)
{
    bool is_supported = false;
    ext_comp_encode_t eth_ext_code = ext_comp;

    switch(eth_ext_code) {
    case AOC_100G_BER_5:
    case AOC_100G_BER_12:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_100G_AOC);
        break;
    case SR4_100GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_100G_SR4);
        break;
    case LR4_100GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_100G_LR4);
        break;
    case ER4_100GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_100G_ER4);
        break;
    case PSM4_100G_SMF:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_100G_PSM4);
        break;
    case CR4_100GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_COPPER_L4_100G);
        break;
    case CWDM4_100G:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_100G_CWDM4);
        break;
    case SR10_100GBASE:
    case ACC_100G_BER_5:
    case ACC_100G_BER_12:
    case ER4_40GBASE:
    case SR_10GBASE_4:
    case PSM4_40G_SMF:
    case CLR4_100G:
    case DWDM2_100GE:
        MODULE_LOG_DBG("%s known eth_ext_cmp:0x%x\n", sff_obj->name, ext_comp);

        break;
    /*200G qsfp56 part*/
    case AOC_200G_BER_6:
    case AOC_200G_BER_6_V2:
    case AOC_200G_BER_5:
    case AOC_200G_BER_5_V2:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_200G_AOC);
        break;
    case SR4_200GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_200G_SR4);
        break;
    case DR4_200GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_200G_DR4);
        break;
    case FR4_200GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_200G_DR4);
        break;
    case CR4_200GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_COPPER_L4_200G);
        break;
    case LR4_200GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_200G_LR4);
    case PSM4_200G:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_200G_PSM4);
        break;
    default:
        MODULE_LOG_DBG("%s unknown eth_ext_cmp:0x%x\n", sff_obj->name, ext_comp);

        break;
    }

    if (TRANSVR_CLASS_UNKNOWN != transvr_type_get(sff_obj)) {
        is_supported = true;
        MODULE_LOG_DBG("%s known eth_ext_cmp:0x%x transvr:%d\n",
                       sff_obj->name, ext_comp, transvr_type_get(sff_obj));
    }
    return is_supported;
}

static int qsfp_type_identify(struct sff_obj_t *sff_obj, bool *supported)
{
    u8 id = 0;
    u8 eth_comp = 0;
    u8 ext_comp = 0;
    int ret = 0;
    bool is_supported = false;

    if (!p_valid(supported)) {
        return -EINVAL;
    }
    *supported = is_supported;
    if((ret = qsfp_id_get(sff_obj, &id)) < 0) {
        return ret;
    }

    if((ret = qsfp_eth_comp_get(sff_obj, &eth_comp)) < 0) {
        return ret;
    }
    /*check extended comp from 8024*/
    if (test_bit(QSFP_ETH_COMP_EXT_BIT, (unsigned long *)&eth_comp)) {
        if((ret = _qsfp_ext_comp_get(sff_obj, &ext_comp)) < 0) {
            return ret;
        }

        is_supported = qsfp_ext_comp_is_supported(sff_obj, ext_comp);

    } else {
        /*40G* spec check*/
        is_supported = qsfp_eth_comp_is_supported(sff_obj, eth_comp);
    }
    *supported = is_supported;
    return 0;
}

static int qsfp_lane_control_set(struct sff_obj_t *sff_obj, int type, u32 value)
{
    int ret = 0;
    qsfp_reg_t id = QSFP_REG_UNKNOWN;
    const struct page_base_reg_t *reg = NULL;
    u8 ch_ctrl[QSFP_LN_CTRL_NUM];
    int len = sizeof(ch_ctrl);

    ch_ctrl[0] = (value >> 8) & 0xff;
    ch_ctrl[1] = value & 0xff;

    switch(type) {

    case TX_EQ_TYPE:
        id = QSFP_REG_OPT_LN_CONTROL_TX_EQ;
        break;

    case RX_EM_TYPE:
        id = QSFP_REG_OPT_LN_CONTROL_RX_EM;
        break;

    case RX_AM_TYPE:
        id = QSFP_REG_OPT_LN_CONTROL_RX_AM;
        break;
    default:
        break;
    }

    if (!qsfp_reg_found(sff_obj, id, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = qsfp_eeprom_write(sff_obj, reg->page, reg->offset, ch_ctrl, reg->len)) < 0) {
        return ret;
    }

    return 0;
}

static int qsfp_lane_control_get(struct sff_obj_t *sff_obj, int type, u32 *value)
{
    int ret = 0;
    qsfp_reg_t id = QSFP_REG_UNKNOWN;
    const struct page_base_reg_t *reg = NULL;
    u8 ch_ctrl[QSFP_LN_CTRL_NUM];
    int len = sizeof(ch_ctrl);

    if (!p_valid(value)) {
        return -EINVAL;
    }
    switch(type) {

    case TX_EQ_TYPE:
        id = QSFP_REG_OPT_LN_CONTROL_TX_EQ;
        break;

    case RX_EM_TYPE:
        id = QSFP_REG_OPT_LN_CONTROL_RX_EM;
        break;

    case RX_AM_TYPE:
        id = QSFP_REG_OPT_LN_CONTROL_RX_AM;
        break;
    default:
        break;
    }

    if (!qsfp_reg_found(sff_obj, id, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = qsfp_eeprom_read(sff_obj, reg->page, reg->offset, ch_ctrl, reg->len)) < 0) {
        return ret;
    }

    *value = ((u32)ch_ctrl[0] << 8) | ((u32)ch_ctrl[1]);
    return 0;
}

static int qsfp_lane_status_update(struct sff_obj_t *sff_obj, int type, u8 value)
{
    struct qsfp_priv_data *qsfp_priv = qsfp_priv_data_get(sff_obj);
    if (type > LN_STATUS_NUM ||
            type < LN_STATUS_RXLOS_TYPE) {
        return -EINVAL;
    }

    qsfp_priv->lane_st[type] = value;

    return 0;
}

static int qsfp_intr_flag_update(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    int i = 0;
    u32 *cnt = NULL;
    u32 old_cnt = 0;
    struct qsfp_priv_data *qsfp_priv = qsfp_priv_data_get(sff_obj);
    const struct page_base_reg_t *reg = NULL;
    u8 intr_flag[QSFP_INTR_FLAG_NUM];
    int len = sizeof(intr_flag);
    u8 rxlos = 0;

    if (!qsfp_reg_found(sff_obj, QSFP_REG_INTR_FLAG, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = qsfp_eeprom_read(sff_obj,
                                reg->page,
                                reg->offset,
                                intr_flag,
                                reg->len)) < 0) {
        return ret;
    }

    rxlos = bits_get(intr_flag[L_RX_TX_LOS_ID], QSFP_RXLOS_BIT_1ST, QSFP_RXLOS_BIT_NUM);
    qsfp_lane_status_update(sff_obj, LN_STATUS_RXLOS_TYPE, rxlos);

    for (i = 0; i < len; i++) {
        cnt = (&qsfp_priv->intr_flag[i].cnt);
        old_cnt = qsfp_priv->intr_flag[i].cnt;
        qsfp_priv->intr_flag[i].reg = intr_flag[i];

        if (intr_flag[i] != 0x00) {
            cnt_increment_limit(cnt);
            if (int_flag_monitor_en) {
                MODULE_LOG_ERR("%s chg %s:0x%x", sff_obj->name, qsfp_intr_flag_str[i], intr_flag[i]);
            }
        }
        if (old_cnt != *cnt) {
            qsfp_priv->intr_flag[i].chg = true;
        } else {
            qsfp_priv->intr_flag[i].chg = false;
        }
    }

    return 0;
}

static void qsfp_intr_flag_clear(struct sff_obj_t *sff_obj)
{
    struct qsfp_priv_data *qsfp_priv = qsfp_priv_data_get(sff_obj);
    memset(qsfp_priv->intr_flag, 0, sizeof(qsfp_priv->intr_flag));
}

static int qsfp_intr_flag_show(struct sff_obj_t *sff_obj, char *buf, int size)
{
    int count = 0;
    int i = 0;
    u32 cnt = 0;
    char chg = ' ';
    u8 intr = 0;
    int ret = 0;
    struct qsfp_priv_data *qsfp_priv = qsfp_priv_data_get(sff_obj);

    /*handle transition back to normal state from intr state  {*/
    if ((ret = qsfp_intr_get(sff_obj, &intr)) < 0) {
        return ret;
    }
    if (intr) {
        if ((ret = qsfp_intr_flag_update(sff_obj)) < 0) {
            return ret;
        }
    }
    /*handle transition back to normal state from intr state  }*/

    count += scnprintf(buf+count, size-count,
                       "intr flag show:\n");
    for (i = 0; i < QSFP_INTR_FLAG_NUM; i++) {

        cnt = qsfp_priv->intr_flag[i].cnt;
        if (0 != cnt) {
            if (qsfp_priv->intr_flag[i].chg) {
                chg = '*';
            } else {
                chg = ' ';
            }
            count += scnprintf(buf+count, size-count,
                               "%s reg:0x%x cnt:%d%c\n",
                               qsfp_intr_flag_str[i],
                               qsfp_priv->intr_flag[i].reg,
                               cnt,
                               chg);
        }
    }
    return count;
}

static int qsfp_lane_status_get(struct sff_obj_t *sff_obj, int type, u8 *st)
{
    u8 intr = 0;
    int ret = 0;
    struct qsfp_priv_data *qsfp_priv = qsfp_priv_data_get(sff_obj);

    if ((ret = qsfp_intr_get(sff_obj, &intr)) < 0) {
        return ret;
    }
    if (intr) {
        if ((ret = qsfp_intr_flag_update(sff_obj)) < 0) {
            return ret;
        }
    }

    *st = qsfp_priv->lane_st[type];
    return 0;
}

static int qsfp_temperature_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size)
{
    int ret = 0;
    s16 temp = 0;
    s16 divider = 256;
    s16 decimal = 0;
    char *unit = "c";
    const struct page_base_reg_t *reg = NULL;
    u8 data[WORD_SIZE];
    int len = sizeof(data);

    if (!p_valid(buf)) {
        return -EINVAL;
    }

    if (!qsfp_reg_found(sff_obj, QSFP_REG_TEMP, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = qsfp_eeprom_read(sff_obj, reg->page, reg->offset, data, reg->len)) < 0) {
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

static int qsfp_voltage_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size)
{
    u16 vol = 0;
    int ret = 0;
    u16 divider = 10000;
    char *unit = "v";
    const struct page_base_reg_t *reg = NULL;
    u8 data[WORD_SIZE];
    int len = sizeof(data);

    if (!qsfp_reg_found(sff_obj, QSFP_REG_VCC, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = qsfp_eeprom_read(sff_obj, reg->page, reg->offset, data, reg->len)) < 0) {
        return ret;
    }

    vol = (data[0] << 8) | data[1];
    return scnprintf(buf, buf_size,
                     "%d.%d %s\n",
                     vol/divider,
                     vol%divider, unit);

}
static int qsfp_lane_monitor_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size)
{
    int idx = 0;
    int ret = 0;
    u16 divider = 0;
    char *unit = NULL;
    struct monitor_para_t *para = NULL;
    qsfp_reg_t id = QSFP_REG_UNKNOWN;
    const struct page_base_reg_t *reg = NULL;
    u16 ch_data = 0;
    u8 ch_monitor[QSFP_LN_MONITOR_NUM];
    int len = sizeof(ch_monitor);
    int count = 0;

    if (!p_valid(buf)) {
        return -EINVAL;
    }
    switch(type) {
    case LN_MONITOR_TX_PWR_TYPE:
        id = QSFP_REG_LN_MONITOR_TX_PWR;
        break;
    case LN_MONITOR_RX_PWR_TYPE:
        id = QSFP_REG_LN_MONITOR_RX_PWR;
        break;
    case LN_MONITOR_TX_BIAS_TYPE:
        id = QSFP_REG_LN_MONITOR_TX_BIAS;
        break;
    default:
        break;
    }

    if (!qsfp_reg_found(sff_obj, id, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = qsfp_eeprom_read(sff_obj, reg->page, reg->offset, ch_monitor, reg->len)) < 0) {
        return ret;
    }

    para = monitor_para_find(type);
    if (!p_valid(para)) {
        return -EBADRQC;
    }
    divider = para->divider;
    unit = para->unit;

    for (idx = 0; idx < len; idx += WORD_SIZE) {

        ch_data = (ch_monitor[idx] << 8) | ch_monitor[idx+1];
        count += scnprintf(buf+count, buf_size-count,
                           "ch%d: %d.%d %s\n",
                           (idx >> 1) + 1, ch_data/divider,
                           ch_data%divider, unit);
    }

    return count;
}
static int qsfp_vendor_info_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size)
{
    int ret = 0;
    qsfp_reg_t id = QSFP_REG_UNKNOWN;
    const struct page_base_reg_t *reg = NULL;
    u8 data[VENDOR_INFO_BUF_SIZE];
    int len = sizeof(data);

    if (!p_valid(buf)) {
        return -EINVAL;
    }

    switch(type) {
    case VENDOR_NAME_TYPE:
        id = QSFP_REG_VENDOR_NAME;
        break;
    case VENDOR_PN_TYPE:
        id = QSFP_REG_VENDOR_PN;
        break;
    case VENDOR_SN_TYPE:
        id = QSFP_REG_VENDOR_SN;
        break;
    case VENDOR_REV_TYPE:
        id = QSFP_REG_VENDOR_REV;
        break;
    default:
        break;
    }
    if (!qsfp_reg_found(sff_obj, id, &reg)) {
        return -EBADRQC;
    }
    /*use a big enough buf to handle all the vendor info*/
    if (reg->len >= len) {
        return -EBADRQC;
    }
    if((ret = qsfp_eeprom_read(sff_obj, reg->page, reg->offset, data, reg->len)) < 0) {
        return ret;
    }
    /*add terminal of string*/
    data[reg->len] = '\0';

    return scnprintf(buf, buf_size, "%s\n", data);
}

static int qsfp_lpmode_control(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    u8 ext_id = 0;
    u8 power_class = 0;
    u8 lpmode = 0;

    if ((ret = qsfp_ext_id_get(sff_obj, &ext_id)) < 0) {
        return ret;
    }

    /*sff-8436 power class takes up bit 6,7 in  Extended Identifier Values
    *      *
    *           *
    *                *
    *                     * */

    power_class = ext_id >> 6;
    MODULE_LOG_DBG("%s power_class:0x%x\n", sff_obj->name, power_class);

    switch(power_class) {
    case POWER_CLASS_1_MODULE:/*lower power mode*/
        lpmode = 1;
        break;
    case POWER_CLASS_2_MODULE:
    case POWER_CLASS_3_MODULE:
    case POWER_CLASS_4_MODULE:
    default:
        lpmode = 0;
        break;
    }
    if((ret = qsfp_lpmode_set(sff_obj, lpmode)) < 0) {
        return ret;
    }

    return 0;
}

static int qsfp_cdr_control(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 ext_id = 0;
    u8 cdr_ctrl = 0x00;
    int len = sizeof(cdr_ctrl);

    if ((ret = qsfp_ext_id_get(sff_obj, &ext_id)) < 0) {
        return ret;
    }

    if ((ext_id & bit_mask(CDR_RX_PRS_BIT)) &&
            (ext_id & bit_mask(CDR_TX_PRS_BIT))) {

        if (!qsfp_reg_found(sff_obj, QSFP_REG_CDR_CONTROL, &reg)) {
            return -EBADRQC;
        }
        if (!qsfp_reg_len_matched(sff_obj, len, reg->len)) {
            return -EBADRQC;
        }

        if((ret = qsfp_eeprom_read(sff_obj, reg->page, reg->offset, &cdr_ctrl, reg->len)) < 0) {
            return ret;
        }
        MODULE_LOG_DBG("default cdr:0x%x\n", cdr_ctrl);

        if(cdr_ctrl != 0xff) {
            cdr_ctrl = 0xff;
            if((ret = qsfp_eeprom_write(sff_obj, reg->page, reg->offset, &cdr_ctrl, reg->len)) < 0) {
                return ret;
            }
        }
        MODULE_LOG_DBG("OK\n");
    } else {

        MODULE_LOG_DBG("not supported:ext_id:0x%x\n", ext_id);
    }
    return 0;
}
static int qsfp_txdisable_set(struct sff_obj_t *sff_obj, u8 value)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data = value;
    int len = sizeof(data);

    MODULE_LOG_DBG("%s 0x%x\n", sff_obj->name, value);
    if (!qsfp_reg_found(sff_obj, QSFP_REG_TXDISABLE, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = qsfp_eeprom_write(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }
    return 0;
}

static int qsfp_txdisable_get(struct sff_obj_t *sff_obj, u8 *value)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);

    if (!p_valid(value)) {
        return -EINVAL;
    }
    if (!qsfp_reg_found(sff_obj, QSFP_REG_TXDISABLE, &reg)) {
        return -EBADRQC;
    }
    if (!qsfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = qsfp_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }
    *value = data;
    return 0;

}

int sff_fsm_qsfp_task(struct sff_obj_t *sff_obj)
{
    sff_fsm_state_t st = sff_fsm_st_get(sff_obj);
    int ret = 0;
    bool ready = false;
    u8 lv = 0;
    bool supported = false;

    switch (st) {

    case SFF_FSM_ST_DETECTED:
        if ((ret = qsfp_paging_support_check(sff_obj, &supported)) < 0) {
            return ret;
        }
        
        if((ret = qsfp_type_identify(sff_obj, &supported)) < 0) {
            break;
        }
        if ((ret = qsfp_lpmode_get(sff_obj, &lv)) < 0) {
            break;
        }
        if (1 == lv) {
            if(supported) {
                sff_fsm_st_set(sff_obj, SFF_FSM_ST_INIT);
            } else {
                /*unknown type: special handling*/
                //sff_fsm_st_set(sff_obj, SFF_FSM_ST_UNKNOWN_TYPE);
                MODULE_LOG_DBG("%s unknown type, init anyway\n", sff_obj->name);
                sff_fsm_st_set(sff_obj, SFF_FSM_ST_INIT);
            }
        } else {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_DATA_READY_CHECK);
            MODULE_LOG_DBG("%s it's high power already jumup to ready check\n", sff_obj->name);
        }
        break;

    case SFF_FSM_ST_READY:
        if ((ret = qsfp_intr_get(sff_obj, &lv)) < 0) {
            break;
        }
        if (!lv) {
            if ((ret = qsfp_intr_flag_update(sff_obj)) < 0 ) {
                break;
            }
        }
        break;

    case SFF_FSM_ST_INIT:
        /*cdr control*/
        if((ret = qsfp_cdr_control(sff_obj)) < 0) {
            break;
        }

        /*low power mode control*/
        if((ret = qsfp_lpmode_control(sff_obj)) < 0) {
            break;

        }
        sff_fsm_kobj_change_event(sff_obj);
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_DATA_READY_CHECK);
        break;
    case SFF_FSM_ST_DATA_READY_CHECK:
       
        if ((ret = _qsfp_data_ready_check(sff_obj, &ready)) < 0) {
            break;
        }

        if(ready) {
             sff_fsm_st_set(sff_obj, SFF_FSM_ST_READY);
        }

        break;
    case SFF_FSM_ST_UNKNOWN_TYPE:
    case SFF_FSM_ST_IDLE:
    case SFF_FSM_ST_REMOVED:
    case SFF_FSM_ST_ISOLATED:
    case SFF_FSM_ST_SUSPEND:
    case SFF_FSM_ST_RESTART:
    case SFF_FSM_ST_TIMEOUT:
    default:
        break;

    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}

