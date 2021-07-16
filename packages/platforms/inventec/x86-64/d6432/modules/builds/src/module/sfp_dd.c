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
#include "sfp_dd.h"

/* history
 * follow SFP-DD MIS rev1.0 Sep. 18/2019
 */
#define MEDIA_CODE_SR2 (0x0c)
static const char *module_st_str[MODULE_ST_ENCODE_NUM] = {

    [MODULE_RESERVED1_ST_ENCODE] = "N/A",
    [MODULE_LOW_PWR_ST_ENCODE] = "LOW_PWR_ST",
    [MODULE_PWR_UP_ST_ENCODE] = "PWR_UP_ST",
    [MODULE_READY_ST_ENCODE] = "READY_ST",
    [MODULE_PWR_DN_ST_ENCODE] = "PWR_DN_ST",
    [MODULE_FAULT_ST_ENCODE] = "FAULT_ST",
    [MODULE_RESERVED2_ST_ENCODE] = "N/A",
    [MODULE_RESERVED3_ST_ENCODE] = "N/A",
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

static const char *data_path_st_str[ DATA_PATH_ENCODE_NUM] = {
    [DATA_PATH_RESERVED1_ENCODE] = "N/A",
    [DATA_PATH_DEACTIVATED_ST_ENCODE] = "DEACTIVATED_ST",
    [DATA_PATH_INIT_ST_ENCODE] = "INIT_ST",
    [DATA_PATH_DEINIT_ST_ENCODE] = "DEINIT_ST",
    [DATA_PATH_ACTIVATED_ST_ENCODE] = "ACTIVATED_ST",
    "N/A",
    "N/A",
    "N/A",
    "N/A",
    "N/A",
    "N/A",
    "N/A",
    "N/A",
    "N/A",
    "N/A",
    "N/A",
};

static const u8 app_advert_fields_offset[SFP_DD_APSEL_NUM][SFP_DD_APP_ADVERT_FIELD_NUM] = {
    { 86, 87, 88, 89, 169},
    { 90, 91, 92, 93, 170},
};

static const u8 app_advert_fields_page[SFP_DD_APP_ADVERT_FIELD_NUM] = {
    0,
    0,
    0,
    0,
    1
};

static const char *sfp_dd_intr_module_flag_str[SFP_DD_INTR_MODULE_FLAG_NUM] = {
    "FW_FAULT",
    "L_VCC_TEMP_WARN",
    "L_AUX_ALARM_WARN",
    "VENDOR_DEFINED_ERR",
};

static const char *sfp_dd_intr_ln_flag_str[SFP_DD_INTR_LN_FLAG_NUM] = {

    [DATA_PATH_CHG_ID] = "DATA_PATH_CHG",
    [L_TX_RX_LOS_CDR_LOL_ID] = "L_TX_RX_LOS_CDR_LOL",
    [L_RX_POWER_ALARM_WARN_ID] = "L_RX_POWER_ALARM_WARN",
    [L_TX_POWER_ALARM_WARN_ID] = "L_TX_POWER_ALARM_WARN",
    [L_TX_BIAS_ALARM_WARN_ID] = "L_TX_BIAS_ALARM_WARN",
};

static const struct page_base_reg_t sff_dd_reg_map[SFP_DD_REG_NUM] = {

    [SFP_DD_REG_ID] = { .page = 0, .offset = 0, .len = 1 },
    [SFP_DD_REG_CMIS_REV] = { .page = 0, .offset = 1, .len = 1 },
    [SFP_DD_REG_ST_INDICATOR2] = { .page = 0, .offset = 2, .len = 1 },
    [SFP_DD_REG_MODULE_ST_INTR] = { .page = 0, .offset = 3, .len = 1 },
    [SFP_DD_REG_INTR_MODULE_FLAG] = { .page = 0, .offset = 10, .len = SFP_DD_INTR_MODULE_FLAG_NUM },
    [SFP_DD_REG_TEMP] = { .page = 0, .offset = 14, .len = WORD_SIZE },
    [SFP_DD_REG_VCC] = { .page = 0, .offset = 16, .len = WORD_SIZE },
    [SFP_DD_REG_MODULE_GLOBAL_CONTROL] = { .page = 0, .offset = 26, .len = 1 },
    [SFP_DD_REG_MODULE_TYPE] = { .page = 0, .offset = 85, .len = 1 },
    [SFP_DD_REG_BANK_SEL] = { .page = 0, .offset = 126, .len = 1 },
    [SFP_DD_REG_PAGE_SEL] = { .page = 0, .offset = 127, .len = 1 },
    [SFP_DD_REG_VENDOR_NAME] = { .page = 0, .offset = 129, .len = 16 },
    [SFP_DD_REG_VENDOR_PN] = { .page = 0, .offset = 148, .len = 16 },
    [SFP_DD_REG_VENDOR_SN] = { .page = 0, .offset = 166, .len = 16 },
    [SFP_DD_REG_VENDOR_REV] = { .page = 0, .offset = 164, .len = 2 },
    [SFP_DD_REG_DATA_PATH_DEINIT] = { .page = 0x01, .offset = 233, .len = 1 },
    [SFP_DD_REG_TXDISABLE] = { .page = 0x01, .offset = 234, .len = 1 },
    [SFP_DD_REG_STAGE_CTRL_SET0_DATA_PATH_INIT] = { .page = 0x01, .offset = 237, .len = 1 },
    [SFP_DD_REG_STAGE_CTRL_SET0_APP_SEL] = { .page = 0x01, .offset = 238, .len = SFP_DD_LANE_NUM },
    [SFP_DD_REG_DATA_PATH_ST] = { .page = 0x0, .offset = 4, .len = SFP_DD_REG_DATA_PATH_ST_NUM },
    [SFP_DD_REG_CONFIG_ERR_CODE] = { .page = 0x0, .offset = 74, .len = SFP_DD_REG_CONIG_ERR_CODE_NUM },
    [SFP_DD_REG_ACTIVE_CTRL_SET] = { .page = 0x0, .offset = 75, .len = SFP_DD_LANE_NUM },
    [SFP_DD_REG_LN_MONITOR_TX_PWR] = { .page = 0x0, .offset = 62, .len = SFP_DD_REG_LN_MONITOR_NUM },
    [SFP_DD_REG_LN_MONITOR_RX_PWR] = { .page = 0x0, .offset = 70, .len = SFP_DD_REG_LN_MONITOR_NUM },
    [SFP_DD_REG_LN_MONITOR_TX_BIAS] = { .page = 0x0, .offset = 66, .len = SFP_DD_REG_LN_MONITOR_NUM },
    [SFP_DD_REG_INTR_LN_FLAG] = { .page = 0x0, .offset = 5, .len = SFP_DD_INTR_LN_FLAG_NUM },
    [SFP_DD_REG_GLOBAL_CTRL] = { .page = 0x0, .offset = 26, .len = 1 },
    [SFP_DD_REG_MEDIA_INTF_ID] = { .page = 0x0, .offset = 87, .len = 1 },

};

static int app_advertising_field_get(struct sff_obj_t *sff_obj);
static int sfp_dd_data_path_deinit(struct sff_obj_t *sff_obj, u8 val);
static int sfp_dd_is_data_path_activated(struct sff_obj_t *sff_obj, bool *activated);
static int sfp_dd_lowPwr_set(struct sff_obj_t *sff_obj);
static int stage_control_set0(struct sff_obj_t *sff_obj);
static int advert_update(struct sff_obj_t *sff_obj, bool *pass);
static int module_ready_check(struct sff_obj_t *sff_obj, bool *ready);
static int sw_config_1(struct sff_obj_t *sff_obj);
static int sw_config_2(struct sff_obj_t *sff_obj, bool *pass);
static int sw_config_check(struct sff_obj_t *sff_obj, bool *pass);
static int sw_control(struct sff_obj_t *sff_obj);

static int sw_config_1_full(struct sff_obj_t *sff_obj);
static int sw_config_2_full(struct sff_obj_t *sff_obj, bool *pass);
static int sw_config_check_full(struct sff_obj_t *sff_obj, bool *pass);
static int sw_control_full(struct sff_obj_t *sff_obj);
#if 0
static int config_error_code_check(struct sff_obj_t *sff_obj, bool *is_pass);
#endif

static int sfp_dd_priv_data_allocate(struct sff_obj_t *sff_obj)
{
    if (!p_valid(sff_obj->priv_data)) {
        sff_obj->priv_data = kzalloc(sizeof(struct sfp_dd_priv_data), GFP_KERNEL); 
        if (!p_valid(sff_obj->priv_data)) {
            return -ENOMEM;
        }    
    }
    return 0;
}    

static void sfp_dd_priv_data_free(struct sff_obj_t *sff_obj)
{
    if (p_valid(sff_obj->priv_data)) {

        kfree(sff_obj->priv_data);
        sff_obj->priv_data = NULL;
        MODULE_LOG_DBG("%s done\n", sff_obj->name);
    } 
}    

inline static struct sfp_dd_priv_data *sfp_dd_priv_data_get(struct sff_obj_t *sff_obj)
{
    struct sfp_dd_priv_data *priv = NULL;
        
    priv = (struct sfp_dd_priv_data *)sff_obj->priv_data;
    return priv;
}
/*quick sw init*/
struct cmis_func_t sfp_dd_fsmfunc_rev = {

    .advert_update = advert_update,
    .sw_config_1 = sw_config_1,
    .sw_config_2 = sw_config_2,
    .sw_config_check = sw_config_check,
    .sw_control = sw_control,
    .module_ready_check = module_ready_check
};

/*full sw init*/
struct cmis_func_t sfp_dd_fsmfunc_rev_full = {

    .advert_update = advert_update,
    .sw_config_1 = sw_config_1_full,
    .sw_config_2 = sw_config_2_full,
    .sw_config_check = sw_config_check_full,
    .sw_control = sw_control_full,
    .module_ready_check = module_ready_check
};

static int sfp_dd_paging_supported(struct sff_obj_t *sff_obj, bool *supported);
static int sfp_dd_lpmode_get(struct sff_obj_t *sff_obj, u8 *value);
static int sfp_dd_intr_get(struct sff_obj_t *sff_obj, u8 *value);
static int sfp_dd_lane_monitor_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
static int sfp_dd_txdisable_set(struct sff_obj_t *sff_obj, u8 val);
static int sfp_dd_txdisable_get(struct sff_obj_t *sff_obj, u8 *val);
static int sfp_dd_id_get(struct sff_obj_t *sff_obj, u8 *val);
static bool sfp_dd_is_id_matched(struct sff_obj_t *sff_obj);
static int sfp_dd_vendor_info_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
static int sfp_dd_module_st_get(struct sff_obj_t *sff_obj, u8 *st);
static int sfp_dd_module_st_get_text(struct sff_obj_t *sff_obj, char *buf, int buf_size);
static int sfp_dd_module_type_get_text(struct sff_obj_t *sff_obj, char *buf, int buf_size);
static int sfp_dd_temperature_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
static int sfp_dd_voltage_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
static int sfp_dd_ln_st_get(struct sff_obj_t *sff_obj, int type, u8 *st);
static void sfp_dd_intr_flag_clear(struct sff_obj_t *sff_obj);
static int sfp_dd_intr_flag_show(struct sff_obj_t *sff_obj, char *buf, int size);
static int sfp_dd_remove_op(struct sff_obj_t *sff_obj);
static int sfp_dd_apsel_apply(struct sff_obj_t *sff_obj, int apsel);
static int sfp_dd_apsel_get(struct sff_obj_t *sff_obj);
static int sfp_dd_active_ctrl_set_get(struct sff_obj_t *sff_obj, char *buf, int size);
static void sfp_dd_quick_set(struct sff_obj_t *sff_obj, bool en);
static bool sfp_dd_quick_get(struct sff_obj_t *sff_obj);
static int sfp_dd_data_path_st_get_text(struct sff_obj_t *sff_obj, char *buf, int size);
static int sfp_dd_init_op(struct sff_obj_t *sff_obj);

struct func_tbl_t sfp_dd_func_tbl = {
    .txdisable_set = sfp_dd_txdisable_set,
    .txdisable_get = sfp_dd_txdisable_get,
    .temperature_get = sfp_dd_temperature_get,
    .voltage_get = sfp_dd_voltage_get,
    .lane_control_set = dummy_lane_control_set,
    .lane_control_get = dummy_lane_control_get,
    .lane_monitor_get = sfp_dd_lane_monitor_get,
    .vendor_info_get = sfp_dd_vendor_info_get,
    .lane_status_get =  sfp_dd_ln_st_get,
    .module_st_get = sfp_dd_module_st_get_text,
    .module_type_get = sfp_dd_module_type_get_text,
    .data_path_st_get = sfp_dd_data_path_st_get_text,
    .id_get = sfp_dd_id_get,
    .is_id_matched = sfp_dd_is_id_matched,
    .paging_supported = sfp_dd_paging_supported,
    .intr_flag_show = sfp_dd_intr_flag_show,
    .intr_flag_clear = sfp_dd_intr_flag_clear,
    .remove_op = sfp_dd_remove_op,
    .apsel_apply = sfp_dd_apsel_apply,
    .apsel_get = sfp_dd_apsel_get,
    .active_ctrl_set_get = sfp_dd_active_ctrl_set_get,
    .rev4_quick_set = sfp_dd_quick_set,
    .rev4_quick_get = sfp_dd_quick_get,
    .init_op = sfp_dd_init_op
};

static bool sfp_dd_reg_len_matched(struct sff_obj_t *sff_obj, int len, int def_len)
{
    bool matched = false;

    if (!(matched = reg_len_matched(len, def_len))) {
        MODULE_LOG_ERR("%s fail len:%d def_len:%d\n", sff_obj->name, len, def_len);
    }
    return matched;
}

static bool sfp_dd_reg_found(struct sff_obj_t *sff_obj, sfp_dd_reg_t id, const struct page_base_reg_t **reg)
{
    if (id < SFP_DD_REG_ID || id >= SFP_DD_REG_NUM) {
        MODULE_LOG_ERR("%s fail id:%d\n", sff_obj->name, id);
        return false;
    }
    *reg = &sff_dd_reg_map[id];
    return true;
}

static int sfp_dd_lpmode_get(struct sff_obj_t *sff_obj, u8 *value)
{
    return sff_lpmode_get(sff_obj, value);
}

static int sfp_dd_intr_get(struct sff_obj_t *sff_obj, u8 *value)
{
    int ret = 0;
    u8 lv = 0;
    ret = sff_intr_get(sff_obj, &lv);
    if (ret == -ENOSYS) {
        /*get digital intr instead , need to check real module behaviour*/

    }
    *value = lv;
    return ret;
}

static int sfp_dd_eeprom_read(struct sff_obj_t *sff_obj,
                              u8 page,
                              u8 offset,
                              u8 *buf,
                              int len)
{
    return sff_paged_eeprom_read(sff_obj, page, offset, buf, len);
}
static int sfp_dd_eeprom_write(struct sff_obj_t *sff_obj,
                               u8 page,
                               u8 offset,
                               u8 *buf,
                               int len)
{
    return sff_paged_eeprom_write(sff_obj, page, offset, buf, len);
}

static int sfp_dd_paging_support_check(struct sff_obj_t *sff_obj, bool *supported)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data = 0;
    struct sfp_dd_priv_data *priv = sfp_dd_priv_data_get(sff_obj);
    int len = sizeof(data);

    priv->paging_supported.valid = false;

    if(!p_valid(supported)) {
        return -EINVAL;
    }
    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_ST_INDICATOR2, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }

    if (data & bit_mask(SFP_DD_FLAT_MEM_BIT)) {
        *supported = false;
        MODULE_LOG_DBG("%s paging is not supported!\n", sff_obj->name);
    } else {
        *supported = true;
    }
    /*caching*/
    priv->paging_supported.val = *supported;
    priv->paging_supported.valid = true;
    return 0;
}

static int sfp_dd_paging_supported(struct sff_obj_t *sff_obj, bool *supported)
{
    int ret = 0;
    struct sfp_dd_priv_data *priv = sfp_dd_priv_data_get(sff_obj);

    check_p(supported);
    if (priv->paging_supported.valid) {
        /*use cache*/
        *supported = priv->paging_supported.val;
        MODULE_LOG_DBG("%s cached is used!\n", sff_obj->name);

    } else {
        if ((ret = sfp_dd_paging_support_check(sff_obj, supported)) < 0) {
            return ret;
        }
    }

    return 0;
}

static int advert_update(struct sff_obj_t *sff_obj, bool *pass)
{
    int ret = 0;
    if ((ret = app_advertising_field_get(sff_obj)) < 0) {
        /*print error code*/
        return ret;
    }
    *pass = true;
    return 0;
}
static int module_ready_check(struct sff_obj_t *sff_obj, bool *ready)
{
    u8 module_st = 0;
    int ret = 0;
    bool tmp_ready = false;
    bool activated = false;

    if ((ret = sfp_dd_module_st_get(sff_obj, &module_st)) < 0) {
        return ret;
    }
    if ((ret = sfp_dd_is_data_path_activated(sff_obj, &activated)) < 0) {
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
static int sw_config_1(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    if ((ret = sfp_dd_lowPwr_set(sff_obj)) < 0) {
        return ret;
    }
    return 0;
}
static int sw_config_2(struct sff_obj_t *sff_obj, bool *pass)
{
    *pass = true;
    return 0;
}

static int sw_config_check(struct sff_obj_t *sff_obj, bool *pass)
{
    *pass = true;
    return 0;
}

static int sw_control(struct sff_obj_t *sff_obj)
{
    return 0;
}

static int sw_config_1_full(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    if ((ret = sfp_dd_data_path_deinit(sff_obj, 0xff)) < 0) {
        return ret;
    }
    if ((ret = sfp_dd_lowPwr_set(sff_obj)) < 0) {
        return ret;
    }
    return 0;
}

static int sw_config_2_full(struct sff_obj_t *sff_obj, bool *pass)
{
    u8 module_st = 0;
    int ret = 0;
    
    if ((ret = sfp_dd_module_st_get(sff_obj, &module_st)) < 0) {
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
#if 0 /*cmis*/ 
static int sw_config_check_full(struct sff_obj_t *sff_obj, bool *pass)
{
    int ret = 0;
    bool is_pass = false;

    if ((ret = config_error_code_check(sff_obj, &is_pass)) < 0) {

        return ret;
    }
    *pass = is_pass;
    return 0;
}
#else 
/*luxshare module doesn't have correct configure code so just skip this check*/
static int sw_config_check_full(struct sff_obj_t *sff_obj, bool *pass)
{
    *pass = true;
    return 0;
}

#endif
static int sw_control_full(struct sff_obj_t *sff_obj)
{
    int ret = 0;

    if ((ret = sfp_dd_data_path_deinit(sff_obj, 0x0)) < 0) {
        return ret;
    }

    return 0;
}

static int cmis_ver_get(struct sff_obj_t *sff_obj, u8 *ver)
{
    int ret = 0;
    u8 data = 0;
    const struct page_base_reg_t *reg = NULL;
    int len = sizeof(data);

    if (!p_valid(ver)) {
        return -EINVAL;
    }
    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_CMIS_REV, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {

        MODULE_LOG_ERR("%s sfp_dd_eeprom_read fail", sff_obj->name);
        return ret;
    }
    MODULE_LOG_DBG("%s cmis ver:0x%x\n", sff_obj->name, data);
    *ver = data;
    return 0;
}
static int fsm_func_get(struct sff_obj_t *sff_obj, struct cmis_func_t **ppfunc)
{
    int ret = 0;
    u8 ver = 0;
    struct cmis_func_t *func = &sfp_dd_fsmfunc_rev;
    bool quick = sfp_dd_quick_get(sff_obj);
    if ((ret = cmis_ver_get(sff_obj, &ver)) < 0) {
        return ret;
    }

    if (quick) {
        func = &sfp_dd_fsmfunc_rev;
        MODULE_LOG_DBG("%s quick is loaded\n", sff_obj->name);
    } else {
        func = &sfp_dd_fsmfunc_rev_full;
        MODULE_LOG_DBG("%s full is loaded\n", sff_obj->name);
    }
    *ppfunc = func;
    return 0;
}
#if 0
/*TBD lan number can be changed*/
static int valid_lane_num_get(struct sff_obj_t *sff_obj)
{

    int num = SFP_DD_LANE_NUM;
    return num;

}
#endif
static bool is_bank_num_valid(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    u8 bank_no = 0xff;
    bool valid = false;
    const struct page_base_reg_t *reg = NULL;
    int len = sizeof(bank_no);

    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_BANK_SEL, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, &bank_no, reg->len)) < 0) {
        MODULE_LOG_ERR("%s sfp_dd_eeprom_read fail", sff_obj->name);
        return ret;
    }
    /*check if it's bank0? */
    if (0x00 == bank_no) {
        valid = true;
    } else {
        MODULE_LOG_ERR("%s bank no is not 0", sff_obj->name);
    }

    return valid;
}
/*CMIS REV 4.0*/
static int sfp_dd_lowPwr_set(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    u8 buf = 0;
    const struct page_base_reg_t *reg = NULL;
    int len = sizeof(buf);

    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_MODULE_GLOBAL_CONTROL, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, &buf, reg->len)) < 0) {
        return ret;
    }

    clear_bit(SFP_DD_MODULE_GLOBAL_CONTROL_LOW_PWR_BIT, (unsigned long *)&buf);

    if( (ret = sfp_dd_eeprom_write(sff_obj, reg->page, reg->offset, &buf, reg->len)) < 0) {
        return ret;
    }

    return 0;
}
/*Table 8-44  Page 10h Overview offset 128*/
static int sfp_dd_data_path_deinit(struct sff_obj_t *sff_obj, u8 val)
{
    int ret = 0;
    u8 buf = val;
    const struct page_base_reg_t *reg = NULL;
    int len = sizeof(buf);

    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_DATA_PATH_DEINIT, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if( (ret = sfp_dd_eeprom_write(sff_obj, reg->page, reg->offset, &buf, reg->len)) < 0) {
        return ret;
    }

    return 0;
}

static int sfp_dd_temperature_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size)
{
    int ret = 0;
    s16 temp = 0;
    s16 divider = 256;
    s16 decimal = 0;
    char *unit = "c";
    const struct page_base_reg_t *reg = NULL;
    u8 data[WORD_SIZE];
    int len = sizeof(data);

    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_TEMP, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, data, reg->len)) < 0) {
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
static int sfp_dd_voltage_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size)
{
    int ret = 0;
    u16 vol = 0;
    u16 divider = 10000;
    char *unit = "v";
    const struct page_base_reg_t *reg = NULL;
    u8 data[WORD_SIZE];
    int len = sizeof(data);

    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_VCC, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, data, reg->len)) < 0) {
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
static int sfp_dd_lane_monitor_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size)
{
    int ret = 0;
    int count = 0;
    int idx = 0;
    u16 ch_data = 0;
    u16 divider = 10000;
    char *unit = NULL;
    sfp_dd_reg_t id = SFP_DD_REG_UNKNOWN;
    const struct page_base_reg_t *reg = NULL;
    struct monitor_para_t *para = NULL;
    u8 ch_monitor[SFP_DD_REG_LN_MONITOR_NUM]; /* 2(u16 data) *4 (channel)*/
    int len = sizeof(ch_monitor);

    switch(type) {
    case LN_MONITOR_TX_PWR_TYPE:
        id = SFP_DD_REG_LN_MONITOR_TX_PWR;
        break;
    case LN_MONITOR_RX_PWR_TYPE:
        id = SFP_DD_REG_LN_MONITOR_RX_PWR;
        break;
    case LN_MONITOR_TX_BIAS_TYPE:
        id = SFP_DD_REG_LN_MONITOR_TX_BIAS;
        break;
    default:
        break;
    }

    if (!sfp_dd_reg_found(sff_obj, id, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, ch_monitor, reg->len)) < 0) {
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

static int sfp_dd_vendor_info_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size)
{
    int ret = 0;
    sfp_dd_reg_t id = SFP_DD_REG_UNKNOWN;
    const struct page_base_reg_t *reg = NULL;
    u8 byte_data[VENDOR_INFO_BUF_SIZE];
    int len = sizeof(byte_data);

    switch(type) {
    case VENDOR_NAME_TYPE:
        id = SFP_DD_REG_VENDOR_NAME;
        break;
    case VENDOR_PN_TYPE:
        id = SFP_DD_REG_VENDOR_PN;
        break;
    case VENDOR_SN_TYPE:
        id = SFP_DD_REG_VENDOR_SN;
        break;
    case VENDOR_REV_TYPE:
        id = SFP_DD_REG_VENDOR_REV;
        break;
    default:
        break;
    }

    if (!sfp_dd_reg_found(sff_obj, id, &reg)) {
        return -EBADRQC;
    }
    /*use a big enough buf to handle all the vendor info*/
    if (reg->len >= len) {
        return -EBADRQC;
    }

    if((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, byte_data, reg->len)) < 0) {
        return ret;
    }
    /*add terminal of string*/
    byte_data[reg->len] = '\0';
    MODULE_LOG_DBG("%s page:0x%x offset:0x%x %s", sff_obj->name, reg->page, reg->offset, byte_data);
    return scnprintf(buf, buf_size, "%s\n", byte_data);
}
/*Table 18- Identifier and Status Summary (Lower Page) byte3 , 3-1 Module state*/
static int sfp_dd_module_st_get(struct sff_obj_t *sff_obj, u8 *st)
{
    u8 data = 0;
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    int len = sizeof(data);

    if (!st) {
        return -EINVAL;
    }
    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_MODULE_ST_INTR, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {

        MODULE_LOG_ERR("%s sfp_dd_eeprom_read fail", sff_obj->name);
        return ret;
    }

    *st = bits_get(data, SFP_DD_MODULE_ST_BIT_MIN, SFP_DD_MODULE_ST_BIT_NUM);
    return 0;
}
static int sfp_dd_module_st_get_text(struct sff_obj_t *sff_obj, char *buf, int buf_size)
{
    int ret = 0;
    u8 st = 0;

    if ((ret = sfp_dd_module_st_get(sff_obj, &st)) < 0) {
        return ret;
    }
    return scnprintf(buf, buf_size,"%s\n", module_st_str[st]);
}
/*value : the status of 8 lane, one lane: 4 bits*/
/*table 65 page 11h offset 128~131*/
static int sfp_dd_data_path_st_get(struct sff_obj_t *sff_obj, u8 st[], int st_size)
{
    int ret = 0;
    int i = 0;
    int ln = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data[SFP_DD_REG_DATA_PATH_ST_NUM];
    int len = sizeof(data);

    if (!p_valid(st)) {
        return -EINVAL;
    }
    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_DATA_PATH_ST, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, data, reg->len)) < 0) {
        *st = 0;
        return ret;
    }
    if ((len * 2) != st_size) {
        return -EINVAL;
    }
    for (i = 0; i < len; i++) {
        st[ln++] = data[i] & 0x0f;
        st[ln++] = data[i] >> 4;
    }
    return 0;
}

static int sfp_dd_is_data_path_activated(struct sff_obj_t *sff_obj, bool *activated)
{
    int ln = 0;
    int ret = 0;
    bool tmp_active = false;
    u8 data_path_st[SFP_DD_LANE_NUM];
    int len = sizeof(data_path_st);

    if ((ret = sfp_dd_data_path_st_get(sff_obj, data_path_st, len)) < 0) {
        return ret;
    }

    for (ln = 0; ln < len; ln++) {
        if (DATA_PATH_ACTIVATED_ST_ENCODE != data_path_st[ln]) {
            break;
        }
    }
    if (ln >= len) {
        MODULE_LOG_DBG("%s ok", sff_obj->name);
        tmp_active = true;
    }
    *activated = tmp_active;
    return 0;
}

static int sfp_dd_data_path_st_get_text(struct sff_obj_t *sff_obj, char *buf, int size)
{
    int ln = 0;
    int ret = 0;
    int cnt = 0;
    u8 data_path_st[SFP_DD_LANE_NUM];
    int len = sizeof(data_path_st);

    if ((ret = sfp_dd_data_path_st_get(sff_obj, data_path_st, len)) < 0) {
        return ret;
    }

    for (ln = 0; ln < len; ln++) {
        cnt += scnprintf(buf+cnt, size-cnt,
                         "ln%d: %s\n", ln+1, data_path_st_str[data_path_st[ln]]);
    }
    return cnt;
}
/*Table 18- Identifier and Status Summary (Lower Page)
 *  * lower page :offset 0　*/
static int sfp_dd_id_get(struct sff_obj_t *sff_obj, u8 *val)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);

    if (!p_valid(val)) {
        return -EINVAL;
    }
    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_ID, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }
    *val = data;
    return 0;
}
static bool sfp_dd_is_id_matched(struct sff_obj_t *sff_obj)
{
    u8 id = 0;
    bool match = false;
    if (sfp_dd_id_get(sff_obj, &id) < 0) {
        return match;
    }
    switch (id) {
    case SFF_8024_ID_SFP_DD:
        match = true;
        break;
    default:
        MODULE_LOG_ERR("%s not match id:%d\n", sff_obj->name, id);
        break;
    }

    return match;
}

/*Table 25- Byte 85 Module Type Encodings*/
static int sfp_dd_module_type_get(struct sff_obj_t *sff_obj, u8 *type)
{
    u8 data = 0;
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    int len = sizeof(data);

    if (!p_valid(type)) {
        return -EINVAL;
    }
    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_MODULE_TYPE, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }
    *type = data;
    return 0;
}

static int sfp_dd_module_type_get_text(struct sff_obj_t *sff_obj, char *buf, int buf_size)
{
    int ret = 0;
    u8 type = 0;
    int i = 0;
    if ((ret = sfp_dd_module_type_get(sff_obj, &type)) < 0) {
        return ret;
    }
    for (i = 0; MODULE_TYPE_RESERVED != module_type_tbl[i].type; i++) {
        if (type == module_type_tbl[i].type) {
            break;
        }
    }
    return scnprintf(buf, buf_size,"%s\n", module_type_tbl[i].name);
}
static int sfp_dd_is_optic_media_intf(struct sff_obj_t *sff_obj, bool *is_optic)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);

    check_p(is_optic);
    *is_optic = false;
    
    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_MEDIA_INTF_ID, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }
    
    if (data == MEDIA_CODE_SR2) {
        *is_optic = true;
    } 

    return 0;
}    
static void sfp_dd_major_module_type_set(struct sff_obj_t *sff_obj, u8 module_type)
{
    major_module_type_t major_module_type = UNKNOWN_MODULE_TYPE;
    struct sfp_dd_priv_data *sfp_dd_priv = sfp_dd_priv_data_get(sff_obj);
    bool is_optic_media_intf = false;
    switch (module_type) {

    case MODULE_TYPE_PASSIVE_CU:
        major_module_type = PASSIVE_MODULE_TYPE;
        break;
    case MODULE_TYPE_MMF:
    case MODULE_TYPE_SMF:
    case MODULE_TYPE_ACTIVE_CABLES:
    case MODULE_TYPE_BASE_T:
        major_module_type = ACTIVE_MODULE_TYPE;
        break;
    case MODULE_TYPE_UNKNOWN:
    default:
        break;

    }
    if (MODULE_TYPE_UNKNOWN == module_type) {
        /*luxshare sfp_dd sr2 module doen't have corresponding module type in eeprom 
         * check media interface code instead*/
        if (sfp_dd_is_optic_media_intf(sff_obj, &is_optic_media_intf) < 0) {
            sfp_dd_priv->major_module_type = MODULE_TYPE_UNKNOWN;    
            return;
        }
        
        if (is_optic_media_intf) {
            major_module_type = ACTIVE_MODULE_TYPE;
        } else {
            major_module_type = LOOPBACK_MODULE_TYPE;
        }
    }
    sfp_dd_priv->major_module_type = major_module_type;    
}    

static major_module_type_t sfp_dd_major_module_type_get(struct sff_obj_t *sff_obj)
{
    struct sfp_dd_priv_data *sfp_dd_priv = sfp_dd_priv_data_get(sff_obj);
    
    return sfp_dd_priv->major_module_type;
}    
#if 0
/*table 26*: lower page:00*/
static int app_advertising_field_get(struct sff_obj_t *sff_obj)
{
    u8 module_type = 0;
    union sfp_dd_app_advert_fields *fields = sff_obj->priv_data.sfp_dd.fields;
    int ret = 0;
    int apsel = 0;
    int i = 0;
    bool end_of_table = false;


    if ((ret = sfp_dd_module_type_get(sff_obj, &module_type)) < 0) {
        return ret;
    }
    MODULE_LOG_DBG("%s module_type:0x%x\n", sff_obj->name, module_type);
    sff_obj->priv_data.sfp_dd.module_type = module_type;
    for (apsel = SFP_DD_APSEL_1; apsel < SFP_DD_APSEL_NUM; apsel++) {
        for (i = 0; i < SFP_DD_APP_ADVERT_FIELD_NUM; i++) {

            if ((ret =sfp_dd_eeprom_read(sff_obj, app_advert_fields_page[i],
                                         app_advert_fields_offset[apsel][i],
                                         &fields[apsel].reg[i], 1)) < 0) {

                MODULE_LOG_ERR("%s sfp_dd_eeprom_read fail", sff_obj->name);
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

    for (apsel = 0; apsel < SFP_DD_APSEL_NUM; apsel++) {
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
#else

static int app_advertising_field_get(struct sff_obj_t *sff_obj)
{
    return 0;
}
#endif

static int sfp_dd_active_ctrl_set_get(struct sff_obj_t *sff_obj, char *buf, int size)
{
    int ret = 0;
    int count = 0;
    int ln = 0;
    const struct page_base_reg_t *reg = NULL;
    struct stage_set_t stage_set[SFP_DD_LANE_NUM];
    int len = sizeof(stage_set);

    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_ACTIVE_CTRL_SET, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, (u8*)stage_set, reg->len)) < 0) {
        return ret;
    }

    for (ln = 0; ln < len; ln++) {
        count += scnprintf(buf+count, size-count,
                           "ln%d: apsel:%d datapath:%d explicit_ctrl:%d\n",
                           ln+1,
                           stage_set[ln].app_code,
                           stage_set[ln].datapath_code,
                           stage_set[ln].explicit_control);
    }
    return count;
}
static int sfp_dd_apsel_get(struct sff_obj_t *sff_obj)
{
    //return stage_control_set0(sff_obj, apsel);
    struct sfp_dd_priv_data *sfp_dd_priv = sfp_dd_priv_data_get(sff_obj);
    return sfp_dd_priv->apsel;
}
/*note: so far only page >= 0x10 , need to check bank number*/
static int stage_control_set0(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    int apsel = 0;
    u8 apply_stage_control_set = 0;
    int ln = 0;
    int datapath_code = 0;
    const struct page_base_reg_t *reg = NULL;
    struct stage_set_t stage_set[SFP_DD_LANE_NUM];
    int len = sizeof(stage_set);

    if (!is_bank_num_valid(sff_obj)) {
        return -EBADRQC;
    }
    apsel = sfp_dd_apsel_get(sff_obj);
    /*table 56 stage control set 0, application select controls (Page 10h, active modules only) */
    /*case1: normal case 400G lane = 8*/
    if (1 == apsel) {
        for (ln = 0; ln < len; ln++) {
            stage_set[ln].explicit_control = 0;
            stage_set[ln].datapath_code = 0x00;
            stage_set[ln].app_code = apsel;
        }

    } else {

        for (ln = 0; ln < len; ) {
            stage_set[ln].explicit_control = 0;
            stage_set[ln].datapath_code = datapath_code;
            stage_set[ln].app_code = apsel;
            ln++;
            if (0 == (ln % 2)) {
                datapath_code += 2;
            }
        }
    }

    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_STAGE_CTRL_SET0_APP_SEL, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }

    if((ret = sfp_dd_eeprom_write(sff_obj, reg->page, reg->offset, (u8*)stage_set, reg->len)) < 0) {
        return ret;
    }
    /*Apply_DataPathInit*/
    apply_stage_control_set = 0x30;
    len = sizeof(apply_stage_control_set);
    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_STAGE_CTRL_SET0_DATA_PATH_INIT, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }

    if((ret = sfp_dd_eeprom_write(sff_obj,
                                  reg->page,
                                  reg->offset,
                                  &apply_stage_control_set,
                                  reg->len)) < 0) {
        return ret;
    }

    return 0;
}

static int sfp_dd_apsel_apply(struct sff_obj_t *sff_obj, int apsel)
{
    //return stage_control_set0(sff_obj, apsel);
    struct sfp_dd_priv_data *sfp_dd_priv = sfp_dd_priv_data_get(sff_obj);
    sfp_dd_priv->apsel = apsel;
    return 0;
}
#if 0 /*reserved*/
static int config_error_code_check(struct sff_obj_t *sff_obj, bool *is_pass)
{
    int ret = 0;
    int ln = 0;
    int i = 0;
    int ln_num = valid_lane_num_get(sff_obj);
    u8 config_err_code[ln_num];
    const struct page_base_reg_t *reg = NULL;
    u8 data[SFP_DD_REG_CONIG_ERR_CODE_NUM];
    int len = sizeof(data);

    if (!p_valid(is_pass)) {
        return -EINVAL;
    }
    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_CONFIG_ERR_CODE, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if (!is_bank_num_valid(sff_obj)) {
        return -EBADRQC;
    }
    if ((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, data, reg->len)) < 0) {
        return ret;
    }

    for (ln = 0, i = 0; i < len; i++) {
        config_err_code[ln++] = data[i] & 0xf;
        config_err_code[ln++] = (data[i] & 0xf0) >> 4;
    }

    for (ln = 0; ln < ln_num; ln++) {

        MODULE_LOG_DBG("%s lane:%d err_code:0x%x\n", sff_obj->name, ln, config_err_code[ln]);
        if (CONFIG_ACCEPTED == config_err_code[ln]) {
        } else {
            break;
        }
    }
    if (ln >= ln_num) {
        *is_pass = true;
    }
    return 0;
}
#endif
static int sfp_dd_txdisable_set(struct sff_obj_t *sff_obj, u8 val)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    int len = sizeof(val);
    u8 set = 0;
    u8 data = 0;
    u8 mask = ~((1 << SFP_DD_REG_TX1_DISABLE_BIT) | (1 << SFP_DD_REG_TX2_DISABLE_BIT));
    set = val << SFP_DD_REG_TX1_DISABLE_BIT;

    if (!is_bank_num_valid(sff_obj)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_TXDISABLE, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }

    if((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }
    data &= mask;
    data |= set;

    if((ret = sfp_dd_eeprom_write(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }

    return 0;
}

static int sfp_dd_txdisable_get(struct sff_obj_t *sff_obj, u8 *val)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);

    if (!p_valid(val)) {
        return -EINVAL;
    }
    if (!is_bank_num_valid(sff_obj)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_TXDISABLE, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }
    *val = data;
    return 0;
}

/*table 15*/
static int sfp_dd_intr_module_flag_update(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    int i = 0;
    u32 *cnt = NULL;
    u32 old_cnt = 0;
    struct sfp_dd_priv_data *sfp_dd_priv = sfp_dd_priv_data_get(sff_obj);
    const struct page_base_reg_t *reg = NULL;
    u8 module_flag[SFP_DD_INTR_MODULE_FLAG_NUM];
    int len = sizeof(module_flag);

    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_INTR_MODULE_FLAG, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, module_flag, reg->len)) < 0) {
        return ret;
    }
    for (i = 0; i < len; i++) {
        cnt = &(sfp_dd_priv->intr_module_flag[i].cnt);
        old_cnt = sfp_dd_priv->intr_module_flag[i].cnt;
        sfp_dd_priv->intr_module_flag[i].reg = module_flag[i];

        if (module_flag[i] != 0x00) {
            cnt_increment_limit(cnt);
            if (int_flag_monitor_en) {
                MODULE_LOG_ERR("%s error %s:0x%x", sff_obj->name, sfp_dd_intr_module_flag_str[i], module_flag[i]);
            }
        }

        if (old_cnt != *cnt) {
            sfp_dd_priv->intr_module_flag[i].chg = true;
        } else {
            sfp_dd_priv->intr_module_flag[i].chg = false;
        }
    }

    return 0;
}

static int sfp_dd_lane_status_update(struct sff_obj_t *sff_obj, int type, u8 value)
{
    struct sfp_dd_priv_data *priv = sfp_dd_priv_data_get(sff_obj);
    if (type > LN_STATUS_NUM ||
            type < LN_STATUS_RXLOS_TYPE) {
        return -EINVAL;
    }

    priv->lane_st[type] = value;

    return 0;
}

static int sfp_dd_intr_ln_flag_update(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    int i = 0;
    u32 *cnt = NULL;
    u32 old_cnt = 0;
    struct sfp_dd_priv_data *sfp_dd_priv = sfp_dd_priv_data_get(sff_obj);
    const struct page_base_reg_t *reg = NULL;
    u8 ln_flag[SFP_DD_INTR_LN_FLAG_NUM];
    int len = sizeof(ln_flag);
    u8 rxlos = 0;
    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_INTR_LN_FLAG, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_dd_eeprom_read(sff_obj,
                                  reg->page,
                                  reg->offset,
                                  ln_flag,
                                  reg->len)) < 0) {
        return ret;
    }


    rxlos = bits_get(ln_flag[L_TX_RX_LOS_CDR_LOL_ID], SFP_DD_RXLOS_BIT_MIN, SFP_DD_RXLOS_BIT_NUM);
    sfp_dd_lane_status_update(sff_obj, LN_STATUS_RXLOS_TYPE, rxlos);

    for (i = 0; i < len; i++) {
        cnt = (&sfp_dd_priv->intr_ln_flag[i].cnt);
        old_cnt = sfp_dd_priv->intr_ln_flag[i].cnt;
        sfp_dd_priv->intr_ln_flag[i].reg = ln_flag[i];

        if (ln_flag[i] != 0x00) {
            cnt_increment_limit(cnt);
            if (int_flag_monitor_en) {
                MODULE_LOG_ERR("%s chg %s:0x%x", sff_obj->name, sfp_dd_intr_ln_flag_str[i], ln_flag[i]);
            }
        }
        if (old_cnt != *cnt) {
            sfp_dd_priv->intr_ln_flag[i].chg = true;
        } else {
            sfp_dd_priv->intr_ln_flag[i].chg = false;
        }
    }

    return 0;
}

static void sfp_dd_intr_flag_clear(struct sff_obj_t *sff_obj)
{
    struct sfp_dd_priv_data *sfp_dd_priv = sfp_dd_priv_data_get(sff_obj);
    memset(sfp_dd_priv->intr_module_flag, 0, sizeof(sfp_dd_priv->intr_module_flag));
    memset(sfp_dd_priv->intr_ln_flag, 0, sizeof(sfp_dd_priv->intr_ln_flag));
}

static int sfp_dd_intr_flag_show(struct sff_obj_t *sff_obj, char *buf, int size)
{
    int count = 0;
    int i = 0;
    u32 cnt = 0;
    char chg = ' ';
    u8 intr = 0;
    int ret = 0;
    struct sfp_dd_priv_data *sfp_dd_priv = sfp_dd_priv_data_get(sff_obj);

    /*handle transition back to normal state from intr state  {*/
    if ((ret = sfp_dd_intr_get(sff_obj, &intr)) < 0) {
        return ret;
    }
    if (intr) {
        if ((ret = sfp_dd_intr_module_flag_update(sff_obj)) < 0) {
            return ret;
        }
        if ((ret = sfp_dd_intr_ln_flag_update(sff_obj)) < 0 ) {
            return ret;
        }
    }
    /*handle transition back to normal state from intr state  }*/

    count += scnprintf(buf+count, size-count,
                       "intr module flag show:\n");
    for (i = 0; i < SFP_DD_INTR_MODULE_FLAG_NUM; i++) {

        cnt = sfp_dd_priv->intr_module_flag[i].cnt;
        if (0 != cnt) {
            if (sfp_dd_priv->intr_module_flag[i].chg) {
                chg = '*';
            } else {
                chg = ' ';
            }
            count += scnprintf(buf+count, size-count,
                               "%s reg:0x%x cnt:%d%c\n",
                               sfp_dd_intr_module_flag_str[i],
                               sfp_dd_priv->intr_module_flag[i].reg,
                               cnt,
                               chg);
        }
    }
    count += scnprintf(buf+count, size-count,
                       "intr lane flag show:\n");
    for (i = 0; i < SFP_DD_INTR_LN_FLAG_NUM; i++) {
        cnt = sfp_dd_priv->intr_ln_flag[i].cnt;
        if (0 != cnt) {
            if (sfp_dd_priv->intr_ln_flag[i].chg) {
                chg = '*';
            } else {
                chg = ' ';
            }
            count += scnprintf(buf+count, size-count,
                               "%s reg:0x%x cnt:%d%c\n",
                               sfp_dd_intr_ln_flag_str[i],
                               sfp_dd_priv->intr_ln_flag[i].reg,
                               cnt,
                               chg);
        }
    }
    return count;
}
static int sfp_dd_ln_st_get(struct sff_obj_t *sff_obj, int type, u8 *st)
{
    u8 intr = 0;
    int ret = 0;
    struct sfp_dd_priv_data *sfp_dd_priv = sfp_dd_priv_data_get(sff_obj);

    if ((ret = sfp_dd_intr_get(sff_obj, &intr)) < 0) {
        return ret;
    }
    if (intr) {
        if ((ret = sfp_dd_intr_module_flag_update(sff_obj)) < 0) {
            return ret;
        }
        if ((ret = sfp_dd_intr_ln_flag_update(sff_obj)) < 0 ) {
            return ret;
        }
    }

    *st = sfp_dd_priv->lane_st[type];
    return 0;
}

void sfp_dd_quick_set(struct sff_obj_t *sff_obj, bool en)
{
    struct sfp_dd_priv_data *priv = sfp_dd_priv_data_get(sff_obj);
    priv->quick_en = en;
}

bool sfp_dd_quick_get(struct sff_obj_t *sff_obj)
{
    struct sfp_dd_priv_data *priv = sfp_dd_priv_data_get(sff_obj);
    return priv->quick_en;
}

static int sfp_dd_init_op(struct sff_obj_t *sff_obj)
{
     int ret = 0;
    struct sfp_dd_priv_data *sfp_dd_priv = NULL;

    if ((ret = sfp_dd_priv_data_allocate(sff_obj)) < 0) {
        return ret;
    }

    sfp_dd_priv = sfp_dd_priv_data_get(sff_obj);

    /*default loading*/
    sfp_dd_apsel_apply(sff_obj, 1);
    return 0;
}
static int sfp_dd_remove_op(struct sff_obj_t *sff_obj)
{
    sfp_dd_priv_data_free(sff_obj);
    return 0;
}
#if 0
/*on :true: low power mode
 *    false: high power mode*/ 
static int sfp_dd_sw_lpmode_set(struct sff_obj_t *sff_obj, bool on)
{
    int ret = 0;
    const struct page_base_reg_t *reg = NULL;
    u8 set = 0;
    int len = sizeof(set);

    if (!sfp_dd_reg_found(sff_obj, SFP_DD_REG_GLOBAL_CTRL, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_dd_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    
    if((ret = sfp_dd_eeprom_read(sff_obj, reg->page, reg->offset, &set, reg->len)) < 0) {
        return ret;
    }
    if (on) {
        set_bit(SFP_DD_REG_GLOBAL_CTRL_FORCE_LOWPWR_BIT, (unsigned long *)&set);
    } else {
        clear_bit(SFP_DD_REG_GLOBAL_CTRL_FORCE_LOWPWR_BIT, (unsigned long *)&set);
        clear_bit(SFP_DD_REG_GLOBAL_CTRL_LOWPWR_BIT, (unsigned long *)&set);
    }
    if((ret = sfp_dd_eeprom_write(sff_obj, reg->page, reg->offset, &set, reg->len)) < 0) {
        return ret;
    }

    return 0;
}
#endif
int sff_fsm_sfp_dd_task(struct sff_obj_t *sff_obj)
{
    sff_fsm_state_t st = sff_fsm_st_get(sff_obj);
    int ret = 0;
    u8 module_st = 0;
    u8 module_type = 0;
    u8 lpmode = 0;
    u8 lv = 0;
    bool pass = false;
    bool ready = false;
    bool supported = false;
    struct sfp_dd_priv_data *sfp_dd_priv = sfp_dd_priv_data_get(sff_obj);

    switch (st) {

    case SFF_FSM_ST_DETECTED:
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_MGMT_INIT);
        break;
    case SFF_FSM_ST_MGMT_INIT:
        if ((ret = sfp_dd_paging_support_check(sff_obj, &supported)) < 0) {
            return ret;
        }
        if ((ret = sfp_dd_module_type_get(sff_obj, &module_type)) < 0) {
            break;
        }
        sfp_dd_major_module_type_set(sff_obj, module_type);

        if (UNKNOWN_MODULE_TYPE == sfp_dd_major_module_type_get(sff_obj)) {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_UNKNOWN_TYPE);
            break;
        }
        if (PASSIVE_MODULE_TYPE != sfp_dd_major_module_type_get(sff_obj) && !supported) {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_UNKNOWN_TYPE);
            break;
        } 
        if ((ret = fsm_func_get(sff_obj, &(sfp_dd_priv->fsm_func))) < 0) {
            break;
        }
        
        if (PASSIVE_MODULE_TYPE == sfp_dd_major_module_type_get(sff_obj)) {        
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_READY);
        } else {
            if (ACTIVE_MODULE_TYPE == sfp_dd_major_module_type_get(sff_obj)) {         
                if ((ret = sfp_dd_priv->fsm_func->advert_update(sff_obj, &pass)) < 0) {
                    break;
                }
                if (!pass) {
                    break;
                }

                if ((ret = sfp_dd_lpmode_get(sff_obj, &lpmode)) < 0) {
                    break;
                }
                if (lpmode) {
                    /*check module state register if it transit to module low power state*/
                    if ((ret = sfp_dd_module_st_get(sff_obj, &module_st)) < 0) {
                        break;
                    }
                    
                    if (MODULE_LOW_PWR_ST_ENCODE == module_st) {
                        MODULE_LOG_DBG("%s low pwr code\n", sff_obj->name);
                        sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_SW_CONFIG_1);
                    } else {
                         /*<TBD> if module st is ready in some cases , transit to READY state right away*/
                        MODULE_LOG_DBG("%s high pwr code\n", sff_obj->name);
                        sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_READY_CHECK);
                    }
                } else {
                    sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_READY_CHECK);
                }
            } else {
                sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_LOOPBACK_INIT);
            }

        }

        break;
    case SFF_FSM_ST_MODULE_SW_CONFIG_1:

        if ((ret = sfp_dd_priv->fsm_func->sw_config_1(sff_obj)) < 0) {
            break;
        }
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_SW_CONFIG_1_WAIT);
        break;
    case SFF_FSM_ST_MODULE_SW_CONFIG_1_WAIT: 
        /*need to wait for luxshare module transition finished, otherwise i2c access will fail*/
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_SW_CONFIG_2);
        break;
    case SFF_FSM_ST_MODULE_SW_CONFIG_2:

        if ((ret = sfp_dd_priv->fsm_func->sw_config_2(sff_obj, &pass)) < 0) {
            break;
        }
        if (pass) {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_SW_CONFIG_CHECK);
        }
        break;
    case SFF_FSM_ST_MODULE_SW_CONFIG_CHECK:

        if ((ret = sfp_dd_priv->fsm_func->sw_config_check(sff_obj, &pass)) < 0) {
            break;
        }

        if (pass) {

            sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_SW_CONTROL);
        }
        break;
    case SFF_FSM_ST_MODULE_SW_CONTROL:

        if ((ret = sfp_dd_priv->fsm_func->sw_control(sff_obj)) < 0) {
            break;
        }

        sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_READY_CHECK);
        break;
    case SFF_FSM_ST_MODULE_READY_CHECK:

        if ((ret = sfp_dd_priv->fsm_func->module_ready_check(sff_obj, &ready)) < 0) {
            break;
        }
        if (ready) {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_MODULE_READY);
        }
        break;
    case SFF_FSM_ST_MODULE_READY:

        sff_fsm_st_set(sff_obj, SFF_FSM_ST_READY);
        break;

    case SFF_FSM_ST_MODULE_LOOPBACK_INIT:
    #if 0
        /*set pwm if needed?*/
        if ((ret = sfp_dd_module_st_get(sff_obj, &module_st)) < 0) {
            break;
        }
        /*check module state register if it transit to module ready*/
        if (MODULE_READY_ST_ENCODE == module_st) {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_READY);
        }
    #endif
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_READY);

        break;
    case SFF_FSM_ST_READY:
        
        if (ACTIVE_MODULE_TYPE == sfp_dd_major_module_type_get(sff_obj)) {
            if ((ret = sfp_dd_intr_get(sff_obj, &lv)) < 0) {
                break;
            }
            if (!lv) {
                if ((ret = sfp_dd_intr_module_flag_update(sff_obj)) < 0) {
                    break;
                }
                if ((ret = sfp_dd_intr_ln_flag_update(sff_obj)) < 0 ) {
                    break;
                }
            }
        }
        break;
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

