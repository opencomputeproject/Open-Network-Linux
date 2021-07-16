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
#include "sfp.h"

const u8 Sfp_Eth_Comp_Table[] = {
    SR_10GBASE,
    LR_10GBASE,
    LRM_10GBASE,
    ER_10GBASE
};

static const struct addr_base_reg_t sfp_reg_map[SFP_REG_NUM] = {

    [SFP_REG_ID] = { .addr = SFF_EEPROM_I2C_ADDR, .offset = 0, .len = 1 },
    [SFP_REG_CONN] = { .addr = SFF_EEPROM_I2C_ADDR, .offset = 2, .len = 1 },
    [SFP_REG_TRANSVR_CODE] = { .addr = SFF_EEPROM_I2C_ADDR, .offset = 3, .len = 8 },
    [SFP_REG_RATE_ID] = { .addr = SFF_EEPROM_I2C_ADDR, .offset = 13, .len = 1 },
    [SFP_REG_EXT_COMP] = { .addr = SFF_EEPROM_I2C_ADDR, .offset = 36, .len = 1 },
    [SFP_REG_VENDOR_NAME] = { .addr = SFF_EEPROM_I2C_ADDR, .offset = 20, .len = 16 },
    [SFP_REG_VENDOR_PN] = { .addr = SFF_EEPROM_I2C_ADDR, .offset = 40, .len = 16 },
    [SFP_REG_VENDOR_SN] = { .addr = SFF_EEPROM_I2C_ADDR, .offset = 68, .len = 16 },
    [SFP_REG_VENDOR_REV] = { .addr = SFF_EEPROM_I2C_ADDR, .offset = 56, .len = 4 },
    [SFP_REG_DDM_TEMP] = { .addr = SFF_DDM_I2C_ADDR, .offset = 96, .len = WORD_SIZE },
    [SFP_REG_DDM_VCC] = { .addr = SFF_DDM_I2C_ADDR, .offset = 98, .len = WORD_SIZE },
    [SFP_REG_DDM_TX_BIAS] = { .addr = SFF_DDM_I2C_ADDR, .offset = 100, .len = WORD_SIZE },
    [SFP_REG_DDM_TX_POWER] = { .addr = SFF_DDM_I2C_ADDR, .offset = 102, .len = WORD_SIZE },
    [SFP_REG_DDM_RX_POWER] = { .addr = SFF_DDM_I2C_ADDR, .offset = 104, .len = WORD_SIZE },
    [SFP_REG_ST_CTRL] = { .addr = SFF_DDM_I2C_ADDR, .offset = 110, .len = 1 },
    [SFP_REG_EXT_ST_CTRL] = { .addr = SFF_DDM_I2C_ADDR, .offset = 118, .len = 1 },
    [SFP_REG_TX_EQ] = { .addr = SFF_DDM_I2C_ADDR, .offset = 114, .len = 1 },
    [SFP_REG_RX_EM] = { .addr = SFF_DDM_I2C_ADDR, .offset = 115, .len = 1 },
};

static int sfp_txdisable_set(struct sff_obj_t *sff_obj, u8 value);
static int sfp_txdisable_get(struct sff_obj_t *sff_obj, u8 *value);
static int sfp_rxlos_get(struct sff_obj_t *sff_obj, u8 *value);
static int sfp_txfault_get(struct sff_obj_t *sff_obj, u8 *value);
static int sfp_vendor_info_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
int sfp_temperature_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
int sfp_voltage_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size);
int sfp_lane_status_get(struct sff_obj_t *sff_obj, int type, u8 *value);
int sfp_lane_monitor_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size);
int sfp_lane_control_set(struct sff_obj_t *sff_obj, int type, u32 value);
int sfp_lane_control_get(struct sff_obj_t *sff_obj, int type, u32 *value);
int sfp_id_get(struct sff_obj_t *sff_obj, u8 *id);
bool sfp_is_id_matched(struct sff_obj_t *sff_obj);
static int sfp_remove_op(struct sff_obj_t *sff_obj)
{
    return 0;
}
static int sfp_init_op(struct sff_obj_t *sff_obj)
{
    return 0;
}
struct func_tbl_t sfp_func_tbl = {
    .txdisable_set = sfp_txdisable_set,
    .txdisable_get = sfp_txdisable_get,
    .temperature_get = sfp_temperature_get,
    .voltage_get = sfp_voltage_get,
    .lane_control_set = sfp_lane_control_set,
    .lane_control_get = sfp_lane_control_get,
    .lane_monitor_get = sfp_lane_monitor_get,
    .vendor_info_get = sfp_vendor_info_get,
    .lane_status_get =  sfp_lane_status_get,
    .id_get = sfp_id_get,
    .is_id_matched = sfp_is_id_matched,
    .remove_op = sfp_remove_op,
    .init_op = sfp_init_op
};
static bool sfp_reg_len_matched(struct sff_obj_t *sff_obj, int len, int def_len)
{
    bool matched = false;

    if (!(matched = reg_len_matched(len, def_len))) {
        MODULE_LOG_ERR("%s fail len:%d def_len:%d\n", sff_obj->name, len, def_len);
    }
    return matched;
}

static bool sfp_reg_found(struct sff_obj_t *sff_obj, sfp_reg_t id, const struct addr_base_reg_t **reg)
{
    if (id <= SFP_REG_UNKNOWN || id >= SFP_REG_NUM) {
        MODULE_LOG_ERR("%s fail id:%d\n", sff_obj->name, id);
        return false;
    }
    *reg = &sfp_reg_map[id];
    return true;
}

static int sfp_txdisable_set(struct sff_obj_t *sff_obj, u8 value)
{
    return sff_txdisable_set(sff_obj, value);
}

static int sfp_txdisable_get(struct sff_obj_t *sff_obj, u8 *value)
{
    return sff_txdisable_get(sff_obj, value);
}

static int sfp_rxlos_get(struct sff_obj_t *sff_obj, u8 *value)
{
    return sff_rxlos_get(sff_obj, value);
}

static int sfp_txfault_get(struct sff_obj_t *sff_obj, u8 *value)
{
    return sff_txfault_get(sff_obj, value);
}

static int sfp_eeprom_read(struct sff_obj_t *sff_obj,
                           u8 addr,
                           u8 offset,
                           u8 *buf,
                           int len)
{
    return sff_eeprom_read(sff_obj, addr, offset, buf, len);
}
static int sfp_eeprom_write(struct sff_obj_t *sff_obj,
                            u8 addr,
                            u8 offset,
                            u8 *buf,
                            int len)
{
    return sff_eeprom_write(sff_obj, addr, offset, buf, len);
}

static int sfp_transvr_codes_read(struct sff_obj_t *sff_obj, u8 *transvr_codes, int size)
{
    int ret = 0;
    const struct addr_base_reg_t *reg = NULL;

    if(!p_valid(transvr_codes)) {
        MODULE_LOG_ERR("invalid para");
        return -EINVAL;
    }
    if (!sfp_reg_found(sff_obj, SFP_REG_TRANSVR_CODE, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_reg_len_matched(sff_obj, size, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_eeprom_read(sff_obj, reg->addr, reg->offset, transvr_codes, reg->len)) < 0) {
        return ret;
    }

    return 0;
}

static int sfp_ext_comp_get(struct sff_obj_t *sff_obj, u8 *ext_comp)
{
    int ret = 0;
    const struct addr_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);

    if(!p_valid(ext_comp)) {
        MODULE_LOG_ERR("invalid para");
        return -EBADRQC;
    }

    if (!sfp_reg_found(sff_obj, SFP_REG_EXT_COMP, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_eeprom_read(sff_obj, reg->addr, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }

    *ext_comp = data;

    return 0;
}
static int sfp_conn_type_get(struct sff_obj_t *sff_obj, u8 *conn_type)
{
    int ret = 0;
    const struct addr_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);

    if(!p_valid(conn_type)) {
        MODULE_LOG_ERR("invalid para");
        return -EBADRQC;
    }
    if (!sfp_reg_found(sff_obj, SFP_REG_CONN, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_eeprom_read(sff_obj, reg->addr, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }

    *conn_type = data;

    return 0;
}
static bool sfp_eth_comp_is_supported(struct sff_obj_t *sff_obj, u8 eth_comp)
{
    bool is_supported = false;
    int size = ARRAY_SIZE(Sfp_Eth_Comp_Table);
    int idx = 0;
    u8 comp_codes = 0;
    for (idx = 0; idx < size; idx++) {
        if(eth_comp & Sfp_Eth_Comp_Table[idx]) {
            MODULE_LOG_DBG("%s known eth_cmp:0x%x\n", sff_obj->name, eth_comp & Sfp_Eth_Comp_Table[idx]);
            comp_codes = eth_comp & Sfp_Eth_Comp_Table[idx];
            break;
        }
    }
    if(idx >= size) {

        MODULE_LOG_DBG("%s unknown eth_cmp:0x%x\n", sff_obj->name, (eth_comp & 0xf0) >> 4);
    }

    switch (comp_codes) {
    case SR_10GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_10G_S_SR);
        break;
    case LR_10GBASE:
    case LRM_10GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_10G_S_LR);
        break;
    case ER_10GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_10G_S_ER);
        break;
    default:
        break;

    }
    if (TRANSVR_CLASS_UNKNOWN != transvr_type_get(sff_obj)) {
        is_supported = true;
        MODULE_LOG_DBG("%s known eth_comp:0x%x transvr:%d\n",
                       sff_obj->name, eth_comp, transvr_type_get(sff_obj));
    }
    return is_supported;
}

static bool sfp_ext_comp_supported(struct sff_obj_t *sff_obj, u8 ext_comp)
{
    bool is_supported = false;
    ext_comp_encode_t eth_ext_code = ext_comp;
    switch(eth_ext_code) {
    case AOC_100G_BER_5:
    case AOC_100G_BER_12: /* 100G AOC or 25G AUI C2M AOC 10^^-12 BER */
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_25G_AOC);
        break;
    case SR4_100GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_25G_SR);
        break;
    case LR4_100GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_25G_LR);
        break;
    case ER4_100GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_OPTICAL_25G_ER);
        break;
    case CR4_100GBASE:
        transvr_type_set(sff_obj, TRANSVR_CLASS_COPPER_L1_25G);
        break;
    case ACC_100G_BER_5:
    case ACC_100G_BER_12: /* 100G ACC or 25G AUI C2M ACC 10^^-12 BER */
    case CR_25GBASE_CA_S:
    case CR_25GBASE_CA_N:
    case T_10BASE_SFI: { /* 10BASE-T with SFI electrical interface */
        /*<to do> how about these types!?*/
        MODULE_LOG_DBG("%s known ext_comp:0x%x\n", sff_obj->name, ext_comp);
    }
    break;
    default:
        MODULE_LOG_DBG("%s unknown ext_comp:%d\n", sff_obj->name, ext_comp);

        break;
    }
    if (TRANSVR_CLASS_UNKNOWN != transvr_type_get(sff_obj)) {
        is_supported = true;
        MODULE_LOG_DBG("%s known ext_comp:0x%x transvr_type:%d\n",
                       sff_obj->name, ext_comp, transvr_type_get(sff_obj));
    }
    return is_supported;
}
int sfp_id_get(struct sff_obj_t *sff_obj, u8 *id)
{
    int ret = 0;
    const struct addr_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);

    if (!sfp_reg_found(sff_obj, SFP_REG_ID, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_eeprom_read(sff_obj, reg->addr, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }

    *id = data;
    return 0;
}
bool sfp_is_id_matched(struct sff_obj_t *sff_obj)
{
    u8 id = 0;
    bool match = false;
    if (sfp_id_get(sff_obj, &id) < 0) {
        return match;
    }
    switch (id) {
    case 0x3:
        match = true;
        break;
    default:
        MODULE_LOG_ERR("%s not match id:%d\n", sff_obj->name, id);
        break;
    }

    return match;
}
static int sfp_type_identify(struct sff_obj_t *sff_obj, bool *is_found)
{
    u8 id = 0;
    u8 code = 0;
    u8 conn_type = 0;
    u8 ext_comp = 0;
    int idx = 0;
    int ret = 0;
    u8 transvr_codes[TRANSVR_CODE_NUM];
    int len = sizeof(transvr_codes);

    if((ret = sfp_id_get(sff_obj, &id)) < 0) {
        goto exit_err;
    }
    if(SFF_8024_ID_SFP != id) {
        MODULE_LOG_ERR("unknown id:0x%x\n", id);
        goto exit_non_support_err; /*<TBD> error handling in the future*/
    }

    if((ret = sfp_ext_comp_get(sff_obj, &ext_comp)) < 0) {
        goto exit_err;
    }

    if(!sfp_ext_comp_supported(sff_obj, ext_comp)) {

        if((ret = sfp_transvr_codes_read(sff_obj, transvr_codes, len)) < 0) {
            goto exit_err;
        }

        for (idx = 0; idx < len; idx++) {
            MODULE_LOG_DBG("%s transvr[%d]:0x%x\n", sff_obj->name, idx, transvr_codes[idx]);
        }

        if((ret = sfp_conn_type_get(sff_obj, &conn_type)) < 0) {
            goto exit_err;
        }

        MODULE_LOG_DBG("%s conn_type:0x%x\n", sff_obj->name,conn_type);

        /*check connector type first*/
        /*check transvr codes start from offset:3 */
        if (CONN_TYPE_LC == conn_type) {
            if(!sfp_eth_comp_is_supported(sff_obj, transvr_codes[0])) {
                goto exit_non_support_err;
            }
        } else if (CONN_TYPE_OPTICAL_PIGTAIL == conn_type) {

            MODULE_LOG_ERR("%s CONN_TYPE_OPTICAL_PIGTAIL\n", sff_obj->name);
            goto exit_non_support_err;

        } else if (CONN_TYPE_COPPER_PIGTAIL == conn_type) {
            code = transvr_codes[TRANSVR_CODE_SFP_PLUS_TECH_FIBRE_CH_TECH2];

            if (code & bit_mask(PASSIVE_CABLE_BIT)) {
                MODULE_LOG_ERR("%s sfp+ passive cable tech\n", sff_obj->name);
                goto exit_non_support_err;
            } else if (code & bit_mask(ACTIVE_CABLE_BIT)) {
                MODULE_LOG_ERR("%s sfp+ active cable tech\n", sff_obj->name);
                goto exit_non_support_err;

            } else {

                goto exit_non_support_err; /*<TBD> error handling in the future*/
            }
        }

    }
    *is_found = true;
    return 0;
exit_non_support_err:
    return 0;
exit_err: /*could be i2c fail , need to define err code in the future <TBD>*/
    return ret;
}
/*reference sff 8472: TABLE 9-12  ALARM AND WARNING FLAG BITS
 * start from offset 112 -> 117 : total size 6 bytes
 * offset 114 : 7-4 : tx input equalization control RATE=HIGH, 3-0: tx input equalization control RATE=LOW
 * offset 115 : 7-4 : RX output emphasis control RATE=HIGH, 3-0: RX output emphasis control RATE=LOW

 * */
int sfp_lane_control_set(struct sff_obj_t *sff_obj, int type, u32 value)
{
    int ret = 0;
    sfp_reg_t id = SFP_REG_UNKNOWN;
    const struct addr_base_reg_t *reg = NULL;
    u8 data = (u8)value;
    int len = sizeof(data);

    switch(type) {
    case TX_EQ_TYPE:
        id = SFP_REG_TX_EQ;
        break;
    case RX_EM_TYPE:
        id = SFP_REG_RX_EM;
        break;

    default:
        break;
    }

    if (!sfp_reg_found(sff_obj, id, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_eeprom_write(sff_obj, reg->addr, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }

    return 0;
}
/*reference sff 8472: TABLE 9-12  ALARM AND WARNING FLAG BITS
 * start from offset 112 -> 117 : total size 6 bytes
 * offset 114 : 7-4 : tx input equalization control RATE=HIGH, 3-0: tx input equalization control RATE=LOW
 * offset 115 : 7-4 : RX output emphasis control RATE=HIGH, 3-0: RX output emphasis control RATE=LOW

 * */
int sfp_lane_control_get(struct sff_obj_t *sff_obj, int type, u32 *value)
{
    int ret = 0;
    sfp_reg_t id = SFP_REG_UNKNOWN;
    const struct addr_base_reg_t *reg = NULL;
    u8 data = 0;
    int len = sizeof(data);

    switch(type) {
    case TX_EQ_TYPE:
        id = SFP_REG_TX_EQ;
        break;
    case RX_EM_TYPE:
        id = SFP_REG_RX_EM;
        break;

    default:
        break;
    }

    if (!sfp_reg_found(sff_obj, id, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_eeprom_read(sff_obj, reg->addr, reg->offset, &data, reg->len)) < 0) {
        return ret;
    }

    *value = data;
    return 0;
}
int sfp_lane_status_get(struct sff_obj_t *sff_obj, int type, u8 *value)
{
    u8 reg = 0;
    int ret = 0;

    switch(type) {
    case LN_STATUS_RXLOS_TYPE:
        ret = sfp_rxlos_get(sff_obj, &reg);
        break;
    case LN_STATUS_TXFAULT_TYPE:
        ret = sfp_txfault_get(sff_obj, &reg) ;
        break;
    case LN_STATUS_TXLOS_TYPE:
    default:
        break;
    }
    if (ret < 0) {
        return ret;
    }
    *value = reg;
    return 0;

}
int sfp_temperature_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size)
{
    int ret = 0;
    s16 temp = 0;
    s16 divider = 256;
    s16 decimal = 0;
    char *unit = "c";
    const struct addr_base_reg_t *reg = NULL;
    u8 data[WORD_SIZE];
    int len = sizeof(data);

    if (!sfp_reg_found(sff_obj, SFP_REG_DDM_TEMP, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_eeprom_read(sff_obj, reg->addr, reg->offset, data, reg->len)) < 0) {
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
int sfp_voltage_get(struct sff_obj_t *sff_obj, u8 *buf, int buf_size)
{
    int ret = 0;
    u16 divider = 10000;
    u16 vol = 0;
    char *unit = "v";
    const struct addr_base_reg_t *reg = NULL;
    u8 data[WORD_SIZE];
    int len = sizeof(data);

    if (!sfp_reg_found(sff_obj, SFP_REG_DDM_VCC, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_eeprom_read(sff_obj, reg->addr, reg->offset, data, reg->len)) < 0) {
        return ret;
    }
    vol = (data[0] << 8) | data[1];
    return scnprintf(buf, buf_size,
                     "%d.%d %s\n",
                     vol/divider,
                     vol%divider, unit);
}
int sfp_lane_monitor_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size)
{
    u16 divider = 0;
    u16 ch_monitor = 0;
    char *unit = NULL;
    struct monitor_para_t *para = NULL;
    int ret = 0;
    sfp_reg_t id = SFP_REG_UNKNOWN;
    const struct addr_base_reg_t *reg = NULL;
    u8 data[WORD_SIZE];
    int len = sizeof(data);

    switch(type) {
    case LN_MONITOR_TX_PWR_TYPE:
        id = SFP_REG_DDM_TX_POWER;
        break;
    case LN_MONITOR_RX_PWR_TYPE:
        id = SFP_REG_DDM_RX_POWER;
        break;
    case LN_MONITOR_TX_BIAS_TYPE:
        id = SFP_REG_DDM_TX_BIAS;
        break;
    default:
        break;
    }

    if (!sfp_reg_found(sff_obj, id, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_eeprom_read(sff_obj, reg->addr, reg->offset, data, reg->len)) < 0) {
        return ret;
    }
    ch_monitor = (data[0] << 8) | data[1];

    para = monitor_para_find(type);
    if (!p_valid(para)) {
        return -EINVAL;
    }
    divider = para->divider;
    unit = para->unit;
    /*big edian*/
    return scnprintf(buf, buf_size,
                     "ch1: %d.%d %s\n",
                     ch_monitor/divider,
                     ch_monitor%divider, unit);
}

static int sfp_vendor_info_get(struct sff_obj_t *sff_obj, int type, u8 *buf, int buf_size)
{
    int ret = 0;
    sfp_reg_t id = SFP_REG_UNKNOWN;
    const struct addr_base_reg_t *reg = NULL;
    u8 data[VENDOR_INFO_BUF_SIZE];
    int len = sizeof(data);

    switch(type) {
    case VENDOR_NAME_TYPE:
        id = SFP_REG_VENDOR_NAME;
        break;
    case VENDOR_PN_TYPE:
        id = SFP_REG_VENDOR_PN;
        break;
    case VENDOR_SN_TYPE:
        id = SFP_REG_VENDOR_SN;
        break;
    case VENDOR_REV_TYPE:
        id = SFP_REG_VENDOR_REV;
        break;
    default:
        break;
    }

    if (!sfp_reg_found(sff_obj, id, &reg)) {
        return -EBADRQC;
    }
    /*use a big enough buf to handle all the vendor info*/
    if (reg->len >= len) {
        return -EBADRQC;
    }
    if((ret = sfp_eeprom_read(sff_obj, reg->addr, reg->offset, data, reg->len)) < 0) {
        return ret;
    }
    /*add terminal of string*/
    data[reg->len] = '\0';

    return scnprintf(buf, buf_size, "%s\n", data);
}

static int sfp_rate_select(struct sff_obj_t *sff_obj, u8 rate_bitmap)
{
    int ret = 0;
    const struct addr_base_reg_t *reg = NULL;
    u8 status = 0;
    int len = sizeof(status);

    if(rate_bitmap & SOFT_RX_RATE_RS0) {

        if (!sfp_reg_found(sff_obj, SFP_REG_ST_CTRL, &reg)) {
            return -EBADRQC;
        }
        if (!sfp_reg_len_matched(sff_obj, len, reg->len)) {
            return -EBADRQC;
        }
        if ((ret = sfp_eeprom_read(sff_obj, reg->addr, reg->offset, &status, reg->len)) < 0) {
            return ret;
        }

        MODULE_LOG_DBG("%s status:0x%x\n", sff_obj->name, status);
        set_bit(SFP_SOFT_RS0_SEL_BIT, (unsigned long *)&status);
        if ((ret = sfp_eeprom_write(sff_obj, reg->addr, reg->offset, &status, reg->len)) < 0) {
            return ret;
        }
    }

    if(rate_bitmap & SOFT_TX_RATE_RS1) {

        if (!sfp_reg_found(sff_obj, SFP_REG_EXT_ST_CTRL, &reg)) {
            return -EBADRQC;
        }
        if (!sfp_reg_len_matched(sff_obj, len, reg->len)) {
            return -EBADRQC;
        }
        if ((ret = sfp_eeprom_read(sff_obj, reg->addr, reg->offset, &status, reg->len)) < 0) {
            return ret;
        }
        MODULE_LOG_DBG("%s ext_status:0x%x\n", sff_obj->name, status);
        set_bit(SFP_SOFT_RS1_SEL_BIT, (unsigned long *)&status);
        if ((ret = sfp_eeprom_write(sff_obj, reg->addr, reg->offset, &status, reg->len)) < 0) {
            return ret;
        }
    }
    return 0;
}
static int sfp_rate_select_control(struct sff_obj_t *sff_obj)
{
    int ret = 0;
    u8 rate = 0;
    const struct addr_base_reg_t *reg = NULL;
    u8 rate_id = 0;
    int len = sizeof(rate_id);

    /*read rate id*/
    if (!sfp_reg_found(sff_obj, SFP_REG_RATE_ID, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_eeprom_read(sff_obj, reg->addr, reg->offset, &rate_id, reg->len)) < 0) {
        return ret;
    }
    MODULE_LOG_DBG("%s rate_id:0x%x\n", sff_obj->name, rate_id);
    switch (rate_id) {
    case 0x00: /* Unspecified */
    case 0x03: /* Unspecified */
    case 0x05: /* Unspecified */
    case 0x07: /* Unspecified */
    case 0x09: /* Unspecified */
    case 0x0B: /* Unspecified */
    case 0x0D: /* Unspecified */
    case 0x0F: /* Unspecified */
        break;
    case 0x02: /* SFF-8431 (8/4/2G Rx Rate_Select only) */
        rate = SOFT_RX_RATE_RS0;
        break;
    case 0x04: /* SFF-8431 (8/4/2G Tx Rate_Select only) */
        rate = SOFT_TX_RATE_RS1;
        break;
    case 0x06: /* SFF-8431 (8/4/2G Independent Rx & Tx Rate_select) */
        rate = SOFT_RX_RATE_RS0 | SOFT_TX_RATE_RS1;
        break;
    case 0x01: /* SFF-8079 (4/2/1G Rate_Select & AS0/AS1) */
    case 0x08: /* FC-PI-5 (16/8/4G Rx Rate_select only)
                  * High=16G only, Low=8G/4G
                  *         *                     */
    case 0x0A: /* FC-PI-5 (16/8/4G Independent Rx, Tx Rate_select)
                  * High=16G only, Low=8G/4G
                  *         *                     */
    case 0x0C: /* FC-PI-6 (32/16/8G Independent Rx, Tx Rate_Select)
                  * High=32G only, Low = 16G/8G
                  *         *                     */
    case 0x0E: /* 10/8G Rx and Tx Rate_Select controlling the operation or
                  * locking modes of the internal signal conditioner, retimer
                  *         *                     * or CDR, according to the logic table defined in Table 10-2,
                  *                 *                                         * High Bit Rate (10G) =9.95-11.3 Gb/s; Low Bit Rate (8G) =
                  *                         *                                                             * 8.5 Gb/s. In this mode, the default value of bit 110.3 (Soft
                  *                                 *                                                                                 * Rate Select RS(0), Table 9-11) and of bit 118.3 (Soft Rate
                  *                                         *                                                                                                     * Select RS(1), Table 10-1) is 1.
                  *                                                 *                                                                                                                         */
    default:
        break;
    }

    if (rate != 0) {
        if((ret = sfp_rate_select(sff_obj, rate)) < 0) {
            return ret;
        }
    } else {
        MODULE_LOG_DBG("%s no support\n", sff_obj->name);
    }
    return 0;
}

static int sfp_data_ready_check(struct sff_obj_t *sff_obj, u8 *is_ready)
{
    int ret = 0;
    const struct addr_base_reg_t *reg = NULL;
    u8 status = 0;
    int len = sizeof(status);

    if(!p_valid(is_ready)) {
        MODULE_LOG_ERR("invalid para");
        return -EINVAL;
    }
    if (!sfp_reg_found(sff_obj, SFP_REG_ST_CTRL, &reg)) {
        return -EBADRQC;
    }
    if (!sfp_reg_len_matched(sff_obj, len, reg->len)) {
        return -EBADRQC;
    }
    if ((ret = sfp_eeprom_read(sff_obj, reg->addr, reg->offset, &status, reg->len)) < 0) {
        return ret;
    }

    status = ~(status & 0x01) & 0x01;
    *is_ready = status;

    return 0;
}
int sff_fsm_sfp_task(struct sff_obj_t *sff_obj)
{
    sff_fsm_state_t st = sff_fsm_st_get(sff_obj);
    int ret = 0;
    u8 is_ready = 0;
    bool is_found = false;
    switch (st) {

    case SFF_FSM_ST_DETECTED:
        ret = sfp_data_ready_check(sff_obj, &is_ready);
        if (ret < 0) {
            break;
        }

        if(is_ready) {
            sff_fsm_st_set(sff_obj, SFF_FSM_ST_INIT);
        }
        break;

    case SFF_FSM_ST_IDENTIFY:
        if((ret = sfp_type_identify(sff_obj, &is_found)) < 0) {
            break;
        }

        if (!is_found) {
            /*just print out error log, transit to ready state anyway*/
            MODULE_LOG_ERR("%s sfp type is not detected\n", sff_obj->name);
        }
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_READY);

        break;

    case SFF_FSM_ST_READY:

        break;
    case SFF_FSM_ST_INIT:

        if ((ret = sfp_rate_select_control(sff_obj)) < 0) {
            break;
        }
        sff_fsm_st_set(sff_obj, SFF_FSM_ST_IDENTIFY);
        break;

    case SFF_FSM_ST_UNKNOWN_TYPE:
    case SFF_FSM_ST_IDLE:
    case SFF_FSM_ST_REMOVED:
    case SFF_FSM_ST_ISOLATED:
    default:
        break;

    }
    if(ret < 0) {
        return ret;
    }
    return 0;
}

