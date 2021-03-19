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
#include "qsfp.h"

#define QSFP_ID_OFFSET (0)
#define QSFP_ST_INDICATOR2_OFFSET (2)
#define DATA_NOT_READY_BIT (0)
#define FLAT_MEM_BIT    (2)
#define QSFP_ETH_COMP_OFFSET (131)
#define QSFP_ETH_EXT_COMP_OFFSET (192)
#define QSFP_VCC_OFFSET (26)
#define QSFP_TEMP_OFFSET (22)
#define QSFP_EXT_ID_OFFSET (129)
#define CDR_RX_PRS_BIT (2)
#define CDR_TX_PRS_BIT (3)
#define QSFP_LN_RX_PWR_OFFSET (34)
#define QSFP_LN_TX_PWR_OFFSET (50)
#define QSFP_LN_TX_BIAS_OFFSET (42)
#define QSFP_PAGE_SEL_OFFSET (127)

#define QSFP_OPT_LN_CONTROL_TX_EQ_OFFSET (234)
#define QSFP_OPT_LN_CONTROL_RX_EM_OFFSET (236)
#define QSFP_OPT_LN_CONTROL_RX_AM_OFFSET (238)
#define QSFP_CDR_CONTROL_OFFSET (98)
#define QSFP_LN_TX_DISABLE_OFFSET (86)

#define QSFP_VENDOR_NAME_OFFSET (148)
#define QSFP_VENDOR_PN_OFFSET (168)
#define QSFP_VENDOR_SN_OFFSET (196)
#define QSFP_VENDOR_REV_OFFSET (184)

#define QSFP_VENDOR_NAME_LEN (16)
#define QSFP_VENDOR_PN_LEN (16)
#define QSFP_VENDOR_SN_LEN (16)
#define QSFP_VENDOR_REV_LEN (2)




extern u32 logLevel;
extern bool int_flag_monitor_en;
static int qsfp_int_flag_read(struct sff_obj_t *sff_obj);

int qsfp_vendor_info_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
int qsfp_temperature_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
int qsfp_voltage_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
int qsfp_lane_status_get(struct sff_obj_t *sff_obj, int type, u8 *value);
int qsfp_lane_monitor_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
int qsfp_lane_control_set(struct sff_obj_t *sff_obj, int type, u32 value);
int qsfp_lane_control_get(struct sff_obj_t *sff_obj, int type, u32 *value);
int qsfp_tx_disable_set(struct sff_obj_t *sff_obj, u8 value);
int qsfp_tx_disable_get(struct sff_obj_t *sff_obj, u8 *value);
int qsfp_id_get(struct sff_obj_t *sff_obj, u8 *id);
int qsfp_eeprom_dump(struct sff_obj_t *sff_obj, u8 *buf);
int qsfp_page_sel(struct sff_obj_t *sff_obj, int page);
int qsfp_page_get(struct sff_obj_t *sff_obj, u8 *page);
bool qsfp_is_id_matched(struct sff_obj_t *sff_obj);
static int qsfp_lpmode_set(struct sff_obj_t *sff_obj, u8 value);
static int qsfp_lpmode_get(struct sff_obj_t *sff_obj, u8 *value);
static int qsfp_intL_get(struct sff_obj_t *sff_obj, u8 *value);
static int qsfp_reset_set(struct sff_obj_t *sff_obj, u8 value);
static int qsfp_reset_get(struct sff_obj_t *sff_obj, u8 *value);
static int mode_sel_set(struct sff_obj_t *sff_obj, u8 value);
static int mode_sel_get(struct sff_obj_t *sff_obj, u8 *value);

const u8 Qsfp_Eth_Comp_Table[] = {
    ACTIVE_CABLE,
    LR4_40GBASE,
    SR4_40GBASE,
    CR4_40GBASE,
    /*10G Ethernet Compliance Codes*/
    SR_10GBASE,
    LR_10GBASE,
    LRM_10GBASE,
    ER_10GBASE,/*COMPLIANCE_RSVD*/
};

struct func_tbl_t qsfp_func_tbl = {
    .eeprom_read = sff_eeprom_read,
    .eeprom_write = sff_eeprom_write,
    .prs_get = sff_prs_get,
    .lpmode_set = qsfp_lpmode_set,
    .lpmode_get = qsfp_lpmode_get,
    .reset_set = qsfp_reset_set,
    .reset_get = qsfp_reset_get,
    .power_set = sff_power_set,
    .power_get = sff_power_get,
    .mode_sel_set = mode_sel_set,
    .mode_sel_get = mode_sel_get,
    .intL_get = qsfp_intL_get,
    .tx_disable_set = qsfp_tx_disable_set,
    .tx_disable_get = qsfp_tx_disable_get,
    .temperature_get = qsfp_temperature_get,
    .voltage_get = qsfp_voltage_get,
    .lane_control_set = qsfp_lane_control_set,
    .lane_control_get = qsfp_lane_control_get,
    .lane_monitor_get = qsfp_lane_monitor_get,
    .vendor_info_get = qsfp_vendor_info_get,
    .lane_status_get =  qsfp_lane_status_get,
    .module_st_get = dummy_module_st_get,
    .id_get = qsfp_id_get,
    .is_id_matched = qsfp_is_id_matched,
    .eeprom_dump = qsfp_eeprom_dump,
    .page_sel = qsfp_page_sel,
    .page_get = qsfp_page_get,
};
struct func_tbl_t *qsfp_func_load(void)
{
    return &qsfp_func_tbl;
}    
static int qsfp_paging_supported(struct sff_obj_t *sff_obj, bool *supported);
static int _page_switch(struct sff_obj_t *sff_obj, u8 page);

static int qsfp_lpmode_set(struct sff_obj_t *sff_obj, u8 value)
{
    int ret = 0;
    check_pfunc(sff_obj->mgr->io_drv->lpmode_set);
    if ((ret = sff_obj->mgr->io_drv->lpmode_set(sff_obj->lc_id, sff_obj->port, value)) < 0) {
        return ret;
    }
    return 0;
}
static int qsfp_lpmode_get(struct sff_obj_t *sff_obj, u8 *value)
{
    int ret = 0;

    check_pfunc(sff_obj->mgr->io_drv->lpmode_get);
    if ((ret = sff_obj->mgr->io_drv->lpmode_get(sff_obj->lc_id, sff_obj->port, value)) < 0) {
        return ret;
    }
    return 0;
}
static int mode_sel_set(struct sff_obj_t *sff_obj, u8 value)
{
    int ret = 0;

    check_pfunc(sff_obj->mgr->io_drv->mode_sel_set);
    if ((ret = sff_obj->mgr->io_drv->mode_sel_set(sff_obj->lc_id, sff_obj->port, value)) < 0) {
        return ret;
    }
    return 0;

}
static int mode_sel_get(struct sff_obj_t *sff_obj, u8 *value)
{
    int ret = 0;

    check_pfunc(sff_obj->mgr->io_drv->mode_sel_get);
    if ((ret = sff_obj->mgr->io_drv->mode_sel_get(sff_obj->lc_id, sff_obj->port, value)) < 0) {
        return ret;
    }
    return 0;
}
static int qsfp_reset_set(struct sff_obj_t *sff_obj, u8 value)
{
    int ret = 0;

    check_pfunc(sff_obj->mgr->io_drv->reset_set); 
    if ((ret = sff_obj->mgr->io_drv->reset_set(sff_obj->lc_id, sff_obj->port, value)) < 0) {
        return ret;
    }
    return 0;
}
static int qsfp_reset_get(struct sff_obj_t *sff_obj, u8 *value)
{
    int ret = 0;

    check_pfunc(sff_obj->mgr->io_drv->reset_get); 
    if ((ret = sff_obj->mgr->io_drv->reset_get(sff_obj->lc_id, sff_obj->port, value)) < 0) {
        return ret;
    }
    return 0;
}
static int qsfp_intL_get(struct sff_obj_t *sff_obj, u8 *value)
{
    int ret = 0;
    unsigned long bitmap = 0;

    if (!p_valid(value)) {
        return -EINVAL;
    }

    check_pfunc(sff_obj->mgr->io_drv->intr_all_get);
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
int qsfp_page_get(struct sff_obj_t *sff_obj, u8 *page)
{
    int ret = 0;
    u8 addr = SFF_EEPROM_I2C_ADDR;
    u8 offset_page_sel = QSFP_PAGE_SEL_OFFSET;
    u8 reg = 0;

    ret = sff_eeprom_read(sff_obj, addr, offset_page_sel, &reg, 1);
    if (ret < 0) {
        MODULE_LOG_ERR("fail\n");
        return ret;
    }
    *page = reg;
    return 0;
}
static int _page_switch(struct sff_obj_t *sff_obj, u8 page)
{
    int rec = 0;
    u8 slave_addr = SFF_EEPROM_I2C_ADDR;
    u8 offset_page_sel = QSFP_PAGE_SEL_OFFSET;
    u8 cur_page = 0;
    bool supported = false;
#if 0
    if ((rec = qsfp_paging_supported(sff_obj, &supported)) < 0) {
        return rec;
    }
#endif
    /*<TBD> use cached instead*/
    memcpy(&supported, &(sff_obj->priv_data.qsfp.paging_supported), sizeof(supported));

    if (!supported &&
            QSFP_PAGE0 != page) {
        MODULE_LOG_DBG("%s paging is not supported!\n", sff_obj->name);
        return -ENOSYS;
    }
    rec = qsfp_page_get(sff_obj, &cur_page);
    if (rec < 0) {
        return rec;
    }
    if (cur_page == page) {
        return 0;
    }
    rec = sff_eeprom_write(sff_obj, slave_addr, offset_page_sel, &page, 1);

    if (rec < 0) {
        MODULE_LOG_ERR("switch page fail\n");
        return rec;
    }
    return 0;
}

int qsfp_page_sel(struct sff_obj_t *sff_obj, int page)
{
    if (page < QSFP_PAGE0 ||  page >= PAGE_NUM) {

        return -EINVAL;
    }

    return _page_switch(sff_obj, page);
}
static int qsfp_eeprom_read(struct sff_obj_t *sff_obj,
                            u8 page,
                            u8 offset,
                            u8 *buf,
                            int len)
{
    int ret = 0;
    u8 addr = SFF_EEPROM_I2C_ADDR; //it's fixed

    if (offset > QSFP_PAGE_SEL_OFFSET) {
        ret = _page_switch(sff_obj, page);
        if (ret < 0) {
            return ret;
        }
    }
    return sff_eeprom_read(sff_obj, addr, offset, buf, len);
}
static int qsfp_eeprom_write(struct sff_obj_t *sff_obj,
                             u8 page,
                             u8 offset,
                             u8 *buf,
                             int len)
{
    int ret = 0;
    u8 addr = SFF_EEPROM_I2C_ADDR; //it's fixed

    if (offset > QSFP_PAGE_SEL_OFFSET) {
        ret = _page_switch(sff_obj, page);
        if (ret < 0) {
            return ret;
        }
    }
    return sff_eeprom_write(sff_obj, addr, offset, buf, len);
}

int qsfp_eeprom_dump(struct sff_obj_t *sff_obj, u8 *buf)
{
    int ret = 0;
    u8 page = 0;
    int i = 0;
    int j = 0;
    int cnt = 0;
    u8 *data = NULL;
    u8 offset = 0;
    int last = EEPROM_SIZE;
    if ((ret = qsfp_page_get(sff_obj, &page)) < 0) {

        return ret;
    }
    if (page < QSFP_PAGE0 ||  page >= PAGE_NUM) {

        return -EBFONT;
    }
    cnt += snprintf(buf+cnt, BUF_SIZE - cnt, "page%d\n", page);
    data = sff_obj->priv_data.qsfp.eeprom_cache[page];
    memset(data, 0, sizeof(EEPROM_SIZE));
    if (QSFP_PAGE0 == page) {

        offset = 0;
        ret = qsfp_eeprom_read(sff_obj, page, 0, data, EEPROM_SIZE);
        /*ret = qsfp_eeprom_read(sff_obj, page, 128, data+128, 64);
        ret = qsfp_eeprom_read(sff_obj, page, 192, data+192, 64);
        */

    } else {

        offset = 128;
        ret = qsfp_eeprom_read(sff_obj, page, offset, data+offset, EEPROM_HALF_SIZE);

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

static int qsfp_paging_supported(struct sff_obj_t *sff_obj, bool *supported)
{
    int ret = 0;
    u8 reg = 0;
    u8 addr = SFF_EEPROM_I2C_ADDR; //it's fixed

    if(!supported) {
        return -EINVAL;
    }
    if((ret = sff_eeprom_read(sff_obj, addr, QSFP_ST_INDICATOR2_OFFSET, &reg, 1)) < 0) {
        return ret;
    }

    if (reg & bit_mask(FLAT_MEM_BIT)) {
        *supported = false;
        MODULE_LOG_DBG("%s paging is not supported!\n", sff_obj->name);
    } else {
        *supported = true;
    }

    return 0;
}
int qsfp_id_get(struct sff_obj_t *sff_obj, u8 *id)
{
    if(!id) {
        return -EINVAL;
    }
    return qsfp_eeprom_read(sff_obj, QSFP_PAGE0, QSFP_ID_OFFSET, id, 1);
}
bool qsfp_is_id_matched(struct sff_obj_t *sff_obj)
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
int qsfp_ext_id_get(struct sff_obj_t *sff_obj, u8 *ext_id)
{
    if(!ext_id) {
        return -EINVAL;
    }

    return qsfp_eeprom_read(sff_obj, QSFP_PAGE0, QSFP_EXT_ID_OFFSET, ext_id, 1);
}
static int _qsfp_data_ready_check(struct sff_obj_t *sff_obj, bool *ready)
{
    int ret = 0;
    u8 st = 0;
    if(!ready) {
        return -EINVAL;
    }

    if((ret = qsfp_eeprom_read(sff_obj, QSFP_PAGE0, QSFP_ST_INDICATOR2_OFFSET, &st, 1)) < 0) {
        return ret;
    }
    if (st & bit_mask(DATA_NOT_READY_BIT)) {
        *ready = false;
    } else {
        *ready = true;
    }
    return 0;

#if 0
    /*<TBD> special case:ex: AVAGO sn:AF1741GG04J pn: AFBR-89CDDZ*/

    MODULE_LOG_DBG("special_case of checking data ready\n");
    if((ret = sfp_id_get(sff_obj, &val)) < 0) {
        return ret;
    }
    *is_ready = 1;
    return 0;
#endif
}
static int _qsfp_ethernet_compliance_read(struct sff_obj_t *sff_obj, u8 *eth_comp)
{
    int ret = 0;
    u8 data = 0;

    if(!eth_comp) {
        MODULE_LOG_ERR("invalid para");
        return -EINVAL;
    }
    if((ret = qsfp_eeprom_read(sff_obj, QSFP_PAGE0, QSFP_ETH_COMP_OFFSET, &data, 1)) < 0) {
        MODULE_LOG_ERR("qsfp_eeprom_read fail");
        return ret;
    }
    *eth_comp = data;

    return 0;
}
static int _qsfp_eth_extended_compliance_get(struct sff_obj_t *sff_obj, u8 *eth_ext_comp)
{
    u8 data = 0;
    int ret = 0;

    if(!eth_ext_comp) {
        MODULE_LOG_ERR("invalid para");
        return -EINVAL;
    }

    if((ret = qsfp_eeprom_read(sff_obj, QSFP_PAGE0, QSFP_ETH_EXT_COMP_OFFSET, &data, 1)) < 0) {
        return ret;
    }
    *eth_ext_comp = data;

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
static bool qsfp_eth_ext_is_supported(struct sff_obj_t *sff_obj, u8 eth_ext_comp)
{
    bool is_supported = false;
    Ethernet_extended_compliance eth_ext_code = eth_ext_comp;

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
        MODULE_LOG_DBG("%s known eth_ext_cmp:0x%x\n", sff_obj->name, eth_ext_comp);

        break;
    default:
        MODULE_LOG_DBG("%s unknown eth_ext_cmp:0x%x\n", sff_obj->name, eth_ext_comp);

        break;
    }

    if (TRANSVR_CLASS_UNKNOWN != transvr_type_get(sff_obj)) {
        is_supported = true;
        MODULE_LOG_DBG("%s known eth_ext_cmp:0x%x transvr:%d\n",
                       sff_obj->name, eth_ext_comp, transvr_type_get(sff_obj));
    }
    return is_supported;
}

static int qsfp_type_identify(struct sff_obj_t *sff_obj, bool *found)
{
    u8 id = 0;
    u8 eth_comp = 0;
    u8 eth_ext_comp = 0;
    int ret = 0;

    if (!found) {
        return -EINVAL;
    }
    if((ret = qsfp_id_get(sff_obj, &id)) < 0) {
        goto exit_err;
    }
    if(0x0d != id &&
            0x11 != id &&
            0x0c != id) { // not qsfp+ (0x0d), qsfp28 (0x11), qsfp(INF 8438)(0xc)
        MODULE_LOG_ERR("unknown id:0x%x\n", id);
        goto exit_non_support_err; /*<TBD> error handling in the future*/
    }

    if((ret = _qsfp_ethernet_compliance_read(sff_obj, &eth_comp)) < 0) {
        goto exit_err;
    }

    if (eth_comp & 0x80) { //bit7 == 1
        if((ret = _qsfp_eth_extended_compliance_get(sff_obj, &eth_ext_comp)) < 0) {
            goto exit_err;
        }

        if(!qsfp_eth_ext_is_supported(sff_obj, eth_ext_comp)) {
            goto exit_non_support_err;
        }
    } else { /*40G*/
        /* ACTIVE_CABLE = 1 << 0,
        *         *     LR4_40GBASE = 1 << 1,
        *                 *         SR4_40GBASE = 1 << 2,
        *                         *             CR4_40GBASE = 1 << 3,
        *                                 *
        *                                         */

        if(!qsfp_eth_comp_is_supported(sff_obj, eth_comp)) {
            goto exit_non_support_err;
        }
    }
    *found = true;
    return 0;

exit_non_support_err:
    *found = false;
    return 0;
exit_err: /*could be i2c fail , need to define err code in the future <TBD>*/
    return ret;
}

int qsfp_lane_control_set(struct sff_obj_t *sff_obj, int type, u32 value)
{
    int ret = 0;
    u8 page = QSFP_PAGE3;
    u8 offset = 0;
    bool fail = false;
    u8 ch_ctrl[WORD_SIZE];

    ch_ctrl[0] = (value >> 8) & 0xff;
    ch_ctrl[1] = value & 0xff;

    switch(type) {

    case TX_EQ_TYPE:
        offset = QSFP_OPT_LN_CONTROL_TX_EQ_OFFSET;
        break;

    case RX_EM_TYPE:
        offset = QSFP_OPT_LN_CONTROL_RX_EM_OFFSET;
        break;

    case RX_AM_TYPE:
        offset = QSFP_OPT_LN_CONTROL_RX_AM_OFFSET;
        break;
    default:
        fail = true;
        break;
    }

    if(fail) {
        return -EBADRQC;
    }
    if((ret = qsfp_eeprom_write(sff_obj, page, offset, ch_ctrl, WORD_SIZE)) < 0) {
        return ret;
    }

    return 0;
}

int qsfp_lane_control_get(struct sff_obj_t *sff_obj, int type, u32 *value)
{
    int ret = 0;
    u8 page = QSFP_PAGE3;
    u8 offset = 0;
    bool fail = false;
    u8 ch_ctrl[WORD_SIZE];
    if (!value) {
        return -EINVAL;
    }
    switch(type) {

    case TX_EQ_TYPE:
        offset = QSFP_OPT_LN_CONTROL_TX_EQ_OFFSET;
        break;

    case RX_EM_TYPE:
        offset = QSFP_OPT_LN_CONTROL_RX_EM_OFFSET;
        break;

    case RX_AM_TYPE:
        offset = QSFP_OPT_LN_CONTROL_RX_AM_OFFSET;
        break;
    default:
        fail = true;
        break;
    }

    if(fail) {
        return -EBADRQC;
    }

    if((ret = qsfp_eeprom_read(sff_obj, page, offset, ch_ctrl, sizeof(ch_ctrl))) < 0) {
        return ret;
    }
    *value = ((u32)ch_ctrl[0] << 8) | ((u32)ch_ctrl[1]);
    return 0;
}
int qsfp_lane_status_get(struct sff_obj_t *sff_obj, int type, u8 *st)
{
    u8 intL = 0;
    int ret = 0;

    if ((ret = qsfp_intL_get(sff_obj, &intL)) < 0) {
        return ret;
    }
    if (intL) {
        if ((ret = qsfp_int_flag_read(sff_obj)) < 0) {
            return ret;
        }
    }

    *st = sff_obj->priv_data.qsfp.lane_st[type];
    return 0;
}
static int qsfp_lane_status_update(struct sff_obj_t *sff_obj, int type, u8 value)
{
    if (type > LN_STATUS_NUM ||
            type < LN_STATUS_RX_LOS_TYPE) {
        return -EINVAL;
    }

    sff_obj->priv_data.qsfp.lane_st[type] = value;

    return 0;
}

int qsfp_temperature_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size)
{
    int ret = 0;
    u8 reg[WORD_SIZE];
    s16 temp = 0;
    s16 data_h = 0;
    s16 data_l = 0;
    s16 divider = 256;
    s16 decimal = 0;
    char *unit = "c";

    if (!buf) {
        return -EINVAL;
    }

    if((ret = qsfp_eeprom_read(sff_obj, QSFP_PAGE0, QSFP_TEMP_OFFSET, reg, sizeof(reg))) < 0) {
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

int qsfp_voltage_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size)
{
    u8 reg[WORD_SIZE];
    u16 vol_data = 0;
    u16 data_h;
    u16 data_l;
    int ret = 0;
    u16 divider = 10000;
    char *unit = "v";

    if((ret = qsfp_eeprom_read(sff_obj, QSFP_PAGE0, QSFP_VCC_OFFSET, reg, sizeof(reg))) < 0) {
        MODULE_LOG_ERR("NG1\n");
        return ret;
    }
    data_h = reg[0];
    data_l = reg[1];
    vol_data = (data_h << 8) | data_l;

    scnprintf(buf, buf_size,
              "%d.%d %s\n",
              vol_data/divider,
              vol_data%divider, unit);

    return 0;
}
int qsfp_lane_monitor_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size)
{
    u8 offset = 0;
    u8 page = QSFP_PAGE0;
    int idx = 0;
    int ret = 0;
    bool fail = false;
    u8 ch_monitor[QSFP_LANE_NUM*WORD_SIZE]; /* 2(u16 data) *4 (lane)*/
    u16 divider = 0;
    u16 data_h = 0;
    u16 data_l = 0;
    u16 ch_data = 0;
    char *unit = NULL;
    struct monitor_para_t *para = NULL;
    int len = 0;
    int count = 0;

    if (!buf) {
        return -EINVAL;
    }
    len = sizeof(ch_monitor);
    switch(type) {
    case LN_MONITOR_TX_PWR_TYPE:
        offset = QSFP_LN_TX_PWR_OFFSET;
        break;
    case LN_MONITOR_RX_PWR_TYPE:
        offset = QSFP_LN_RX_PWR_OFFSET;
        break;
    case LN_MONITOR_TX_BIAS_TYPE:
        offset = QSFP_LN_TX_BIAS_OFFSET;
        break;
    default:
        fail = true;
        break;
    }
    if (fail) {
        return -EINVAL;
    }
    if((ret = qsfp_eeprom_read(sff_obj, page, offset, ch_monitor, len)) < 0) {
        return ret;
    }
    para = monitor_para_find(type);
    if (!para) {
        return -EBADRQC;
    }
    divider = para->divider;
    unit = para->unit;

    for (idx = 0; idx < len; idx += WORD_SIZE) {

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
int qsfp_vendor_info_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size)
{
    int ret = 0;
    u8 page = QSFP_PAGE0;
    u8 offset = 0;
    int len = 0;
    bool fail = false;
    u8 byte_data[VENDOR_INFO_BUF_SIZE];

    switch(type) {
    case VENDOR_NAME_TYPE:
        offset = QSFP_VENDOR_NAME_OFFSET;
        len = QSFP_VENDOR_NAME_LEN;
        break;
    case VENDOR_PN_TYPE:
        offset = QSFP_VENDOR_PN_OFFSET;
        len = QSFP_VENDOR_PN_LEN;
        break;
    case VENDOR_SN_TYPE:
        offset = QSFP_VENDOR_SN_OFFSET;
        len = QSFP_VENDOR_SN_LEN;
        break;
    case VENDOR_REV_TYPE:
        offset = QSFP_VENDOR_REV_OFFSET;
        len = QSFP_VENDOR_REV_LEN;
        break;
    default:
        fail = true;
        break;
    }

    if (fail) {
        return -EINVAL;
    }
    if((ret = qsfp_eeprom_read(sff_obj, page, offset, byte_data, len)) < 0) {
        return ret;
    }
    /*add terminal of string*/
    byte_data[len] = '\0';

    scnprintf(buf, buf_size, "%s\n", byte_data);

    return 0;
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

/*6.2.3  Interrupt Flags Bytes 3-21 consist of interrupt flags for LOS, TX Fault, warnings and alarms.*/
/*TABLE 6-4  LNANNEL STATUS INTERRUPT FLAGS (PAGE 00H BYTES 3-5) */
/*offset 3 , 0-4: rxlos; 4-7 txlos*/
/*offset 4 , 0-4: txfault*/
static int qsfp_int_flag_read(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    int i = 0;
    u8 buf = 0;
    u8 data = 0;
    u8 reg[17];

    if ((ret = qsfp_eeprom_read(sff_obj, QSFP_PAGE0, 0x03, &buf, 1)) < 0) {

        MODULE_LOG_ERR("%s qsfp_eeprom_read fail", sff_obj->name);
        return ret;
    }
    data = bits_get(buf, 0, 4);
    qsfp_lane_status_update(sff_obj, LN_STATUS_RX_LOS_TYPE, data);

    if (int_flag_monitor_en) {
        MODULE_LOG_DBG("%s rxlos 0x%x",  sff_obj->name, data);
    }
    if ((ret = qsfp_eeprom_read(sff_obj, QSFP_PAGE0, 0x04, &buf, 1)) < 0) {

        MODULE_LOG_ERR("%s qsfp_eeprom_read fail",  sff_obj->name);
        return ret;
    }
    data = bits_get(buf, 0, 4);

    if ((ret = qsfp_eeprom_read(sff_obj, QSFP_PAGE0, 0x05, reg, 17)) < 0) {

        MODULE_LOG_ERR("%s qsfp_eeprom_read fail",  sff_obj->name);
        return ret;
    }

    for (i = 0; i < 17; i++) {
        if (0 != reg[i]) {
            if (int_flag_monitor_en) {
                MODULE_LOG_DBG("%s reg[%d] 0x%x", sff_obj->name, i+5, reg[i]);
            }
        }
    }

    return 0;
}
static int qsfp_cdr_control(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    u8 ext_id = 0;
    u8 cdr_ctrl = 0x00;

    if ((ret = qsfp_ext_id_get(sff_obj, &ext_id)) < 0) {
        return ret;
    }

    if ((ext_id & bit_mask(CDR_RX_PRS_BIT)) &&
            (ext_id & bit_mask(CDR_TX_PRS_BIT))) {

        if((ret = qsfp_eeprom_read(sff_obj, QSFP_PAGE0, QSFP_CDR_CONTROL_OFFSET, &cdr_ctrl, 1)) < 0) {
            return ret;
        }
        MODULE_LOG_DBG("default cdr:0x%x\n", cdr_ctrl);

        if(cdr_ctrl != 0xff) {
            cdr_ctrl = 0xff;
            if((ret = qsfp_eeprom_write(sff_obj, QSFP_PAGE0, QSFP_CDR_CONTROL_OFFSET, &cdr_ctrl, 1)) < 0) {
                return ret;
            }
        }
        MODULE_LOG_DBG("OK\n");
    } else {

        MODULE_LOG_DBG("not supported:ext_id:0x%x\n", ext_id);
    }
    return 0;
}
int qsfp_tx_disable_set(struct sff_obj_t *sff_obj, u8 value)
{
    int ret = 0;
    u8 reg = value;

    if((ret = qsfp_eeprom_write(sff_obj, QSFP_PAGE0, QSFP_LN_TX_DISABLE_OFFSET, &reg, 1)) < 0) {
        return ret;
    }
    return 0;
}

int qsfp_tx_disable_get(struct sff_obj_t *sff_obj, u8 *value)
{
    int ret = 0;
    u8 reg = 0;
    if (!value) {
        return -EINVAL;
    }
    if((ret = qsfp_eeprom_read(sff_obj, QSFP_PAGE0, QSFP_LN_TX_DISABLE_OFFSET, &reg, 1)) < 0) {
        return ret;
    }
    *value = reg;
    return 0;

}
static void cache_clear(struct sff_obj_t *sff_obj)
{
    memset(&(sff_obj->priv_data.qsfp), 0, sizeof(struct qsfp_priv_data));
}
int sff_fsm_qsfp_task(struct sff_obj_t *sff_obj)
{
    sff_fsm_state_t st = sff_fsm_st_get(sff_obj);
    int ret = 0;
    bool ready = false;
    bool found = false;
    u8 lv = 0;
    bool supported = false;
    u8 rst = 0;
    u8 power = 0;
    switch (st) {

    case SFF_FSM_ST_IDLE:
        break;
    case SFF_FSM_ST_REMOVED:
        cache_clear(sff_obj);
        /*always make sure, module plugged in with lower power mode*/
        if((ret = qsfp_lpmode_set(sff_obj, 1)) < 0) {
            break;
        }
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_IDLE);
        break;
    case SFF_FSM_ST_INSERTED:

        sff_fsm_st_set(sff_obj, SFF_FSM_ST_DETECTING);
        break;

    case SFF_FSM_ST_DETECTING:

        if ((ret = qsfp_reset_get(sff_obj, &rst)) < 0) {
            break;
        }

        if ((ret = sff_power_get(sff_obj, &power)) < 0) {
            break;
        }

        if (!rst || !power) {
            MODULE_LOG_INFO("%s rst:%d power:%d\n",sff_obj->name, rst, power);
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_SUSPEND);
            break;
        }

        if ((ret = _qsfp_data_ready_check(sff_obj, &ready)) < 0) {
            break;
        }

        if(!ready) {
            break;
        }
        if ((ret = qsfp_paging_supported(sff_obj, &supported)) < 0) {
            return ret;
        }
        memcpy(&(sff_obj->priv_data.qsfp.paging_supported), &supported, sizeof(supported));
#if 0
        if ((ret = qsfp_tx_disable_set(sff_obj, 0xf)) < 0) {

            break;
        }
#endif        
        if((ret = qsfp_type_identify(sff_obj, &found)) < 0) {
            break;
        }

        if(found) {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_INIT);
        } else {
            /*unknown type: special handling*/
            //sff_fsm_st_set(sff_obj, SFF_FSM_ST_UNKNOWN_TYPE);
            MODULE_LOG_DBG("%s unknown type, init anyway\n", sff_obj->name);
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_INIT);
        }

        break;

    case SFF_FSM_ST_ISOLATED:
        break;
    case SFF_FSM_ST_READY:
#if defined (QSFP_INT_FLAG_SUPPORT)
        if ((ret = qsfp_intL_get(sff_obj, &lv)) < 0) {
            break;
        }
        if (!lv) {
            if ((ret = qsfp_int_flag_read(sff_obj)) < 0) {
                break;
            }
        }
#endif
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
#if 0
        /*tx_disable control*/
        if((ret = qsfp_tx_disable_set(sff_obj, 0x0)) < 0) {
            break;
        }
#endif        
        sff_fsm_kobj_change_event(sff_obj);
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_READY);
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

