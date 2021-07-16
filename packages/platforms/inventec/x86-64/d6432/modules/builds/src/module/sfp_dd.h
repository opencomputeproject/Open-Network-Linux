#ifndef __SFF_SFP_DD_H
#define __SFF_SFP_DD_H

#define SFP_DD_FLAT_MEM_BIT (7)
#define SFP_DD_MODULE_ST_BIT_MIN (1)
#define SFP_DD_MODULE_ST_BIT_NUM (3)
#define SFP_DD_MODULE_GLOBAL_CONTROL_LOW_PWR_BIT (6)
#define SFP_DD_INTR_LN_FLAG_NUM (5)
#define SFP_DD_INTR_MODULE_FLAG_NUM (4)
#define SFP_DD_LANE_NUM (2)
#define SFP_DD_REG_DATA_PATH_ST_NUM (SFP_DD_LANE_NUM / 2)
#define SFP_DD_REG_CONIG_ERR_CODE_NUM (SFP_DD_LANE_NUM / 2)
#define SFP_DD_REG_LN_MONITOR_NUM (SFP_DD_LANE_NUM * WORD_SIZE)

#define SFP_DD_REG_TX1_DISABLE_BIT (4)
#define SFP_DD_REG_TX2_DISABLE_BIT (5)

#define SFP_DD_RXLOS_BIT_MIN (4)
#define SFP_DD_RXLOS_BIT_NUM (SFP_DD_LANE_NUM)
#define SFP_DD_REG_GLOBAL_CTRL_FORCE_LOWPWR_BIT (4)
#define SFP_DD_REG_GLOBAL_CTRL_LOWPWR_BIT (6)
/*
union active_control_set_t {

    struct stage_set_t data;
    u8 reg;
};
*/
#define TX_CDR_BIT (0x01)
#define TX_CDR_BYPASS_BIT (0x02)
#define TX_INPUT_FIX_MANUAL_BIT (0x04)
#define ADAPTIVE_TX_INPUT_FIX_MANUAL_BIT (0x08)
#define TX_INPUT_FREEZE_BIT (0x10)

/*offset: 162*/

#define RX_CDR_BIT (0x01)
#define RX_CDR_BYPASS_BIT (0x02)
#define RX_OUTPUT_AMP_CONTROL_BIT (0x04)
#define RX_OUTPUT_EQ_CONTROL_BIT (0x08 | 0x10)
#define STAGE_SET1_BIT (0x20)
typedef enum {

    MODULE_RESERVED1_ST_ENCODE,
    MODULE_LOW_PWR_ST_ENCODE,
    MODULE_PWR_UP_ST_ENCODE,
    MODULE_READY_ST_ENCODE,
    MODULE_PWR_DN_ST_ENCODE,
    MODULE_FAULT_ST_ENCODE,
    MODULE_RESERVED2_ST_ENCODE,
    MODULE_RESERVED3_ST_ENCODE,
    MODULE_ST_ENCODE_NUM

} Module_State_Encoding_t;
typedef enum {
    SFP_DD_REG_UNKNOWN,
    SFP_DD_REG_ID,
    SFP_DD_REG_CMIS_REV,
    SFP_DD_REG_ST_INDICATOR2,
    SFP_DD_REG_MODULE_GLOBAL_CONTROL,
    SFP_DD_REG_BANK_SEL,
    SFP_DD_REG_MODULE_TYPE,
    SFP_DD_REG_PAGE_SEL,
    SFP_DD_REG_INTR_MODULE_FLAG,
    SFP_DD_REG_DATA_PATH_DEINIT,
    SFP_DD_REG_DATA_PATH_PWR_UP,
    SFP_DD_REG_TXDISABLE,
    SFP_DD_REG_VCC,
    SFP_DD_REG_TEMP,
    SFP_DD_REG_STAGE_CTRL_SET0_APP_SEL,
    SFP_DD_REG_STAGE_CTRL_SET0_DATA_PATH_INIT,
    SFP_DD_REG_DATA_PATH_ST,
    SFP_DD_REG_CONFIG_ERR_CODE,
    SFP_DD_REG_ACTIVE_CTRL_SET,
    SFP_DD_REG_LN_MONITOR_TX_PWR,
    SFP_DD_REG_LN_MONITOR_RX_PWR,
    SFP_DD_REG_LN_MONITOR_TX_BIAS,
    SFP_DD_REG_VENDOR_NAME,
    SFP_DD_REG_VENDOR_PN,
    SFP_DD_REG_VENDOR_SN,
    SFP_DD_REG_VENDOR_REV,
    SFP_DD_REG_MODULE_ST_INTR,
    SFP_DD_REG_INTR_LN_FLAG,
    SFP_DD_REG_GLOBAL_CTRL,
    SFP_DD_REG_MEDIA_INTF_ID,
    SFP_DD_REG_NUM
} sfp_dd_reg_t;

typedef enum {

    MODULE_TYPE_UNKNOWN = 0x0,
    MODULE_TYPE_MMF = 0x1,
    MODULE_TYPE_SMF = 0x2,
    MODULE_TYPE_PASSIVE_CU = 0x3,
    MODULE_TYPE_ACTIVE_CABLES = 0x4,
    MODULE_TYPE_BASE_T = 0x5,
    MODULE_TYPE_RESERVED,

} Module_Type_Encoding_t;

struct module_type_tbl_t {
    int type;
    char *name;
};

typedef enum {
    DATA_PATH_RESERVED1_ENCODE,
    DATA_PATH_DEACTIVATED_ST_ENCODE,
    DATA_PATH_INIT_ST_ENCODE,
    DATA_PATH_DEINIT_ST_ENCODE,
    DATA_PATH_ACTIVATED_ST_ENCODE,
    DATA_PATH_RESERVED2_ENCODE,
    DATA_PATH_RESERVED3_ENCODE,
    DATA_PATH_RESERVED4_ENCODE,
    DATA_PATH_RESERVED5_ENCODE,
    DATA_PATH_RESERVED6_ENCODE,
    DATA_PATH_RESERVED7_ENCODE,
    DATA_PATH_RESERVED8_ENCODE,
    DATA_PATH_RESERVED9_ENCODE,
    DATA_PATH_RESERVED10_ENCODE,
    DATA_PATH_RESERVED11_ENCODE,
    DATA_PATH_RESERVED12_ENCODE,
    DATA_PATH_ENCODE_NUM,

} DataPathStateEncoding_t;

typedef enum {
    NO_STATUS = 0,
    CONFIG_ACCEPTED,
    CONFIG_REJECTED_UNKNOWN,
    CONFIG_REJECTED_INVALID_CODE,
    CONFIG_REJECTED_INVALID_COMBO,
    CONFIG_REJECTED_INVALID_SI,
    CONFIG_REJECTED_IN_USE,
    CONFIG_REJECTED_INCOMPLETE_LANE_INFO,
} config_err_code_t;

enum {
    DATA_PATH_CHG_ID,
    L_TX_RX_LOS_CDR_LOL_ID,
    L_RX_POWER_ALARM_WARN_ID,
    L_TX_POWER_ALARM_WARN_ID,
    L_TX_BIAS_ALARM_WARN_ID,
};

struct stage_set_t {
    unsigned char explicit_control : 1;
    unsigned char datapath_code : 3;
    unsigned char app_code : 4;
};


typedef enum {
    SFP_DD_APSEL_1,
    SFP_DD_APSEL_2,
    SFP_DD_APSEL_NUM,
} sfp_dd_apsel_code_t;
/*app_advert_field*/
#define SFP_DD_APP_ADVERT_FIELD_NUM (5)
struct sfp_dd_app_advert_fields_t {

    u8 host_electrical_interface_code;
    u8 module_media_interface_code;
    u8 lane_count;
    u8 host_lane_assignment_options;
    u8 media_lane_assignment_options;
};

union sfp_dd_app_advert_fields {

    struct sfp_dd_app_advert_fields_t data;
    u8 reg[SFP_DD_APP_ADVERT_FIELD_NUM];
};

struct cmis_func_t {
    int (*advert_update)(struct sff_obj_t *sff_obj, bool *pass);
    int (*sw_config_1)(struct sff_obj_t *sff_obj);
    int (*sw_config_2)(struct sff_obj_t *sff_obj, bool *pass);
    int (*sw_config_check)(struct sff_obj_t *sff_obj, bool *pass);
    int (*sw_control)(struct sff_obj_t *sff_obj);
    int (*module_ready_check)(struct sff_obj_t *sff_obj, bool *ready);
};

struct sfp_dd_priv_data {
    int valid_ln_num;
    struct u8_format_t module_type;
    union sfp_dd_app_advert_fields fields;
    struct paging_supported_t paging_supported;
    bool quick_en;
    u8 eeprom_cache[PAGE_NUM][EEPROM_SIZE];
    struct intr_flag_t intr_module_flag[SFP_DD_INTR_MODULE_FLAG_NUM];
    struct intr_flag_t intr_ln_flag[SFP_DD_INTR_LN_FLAG_NUM];
    u8 lane_st[LN_STATUS_NUM];
    struct cmis_func_t *fsm_func;
    int apsel;
    struct advert_tx_eq_t advert_tx_eq;
    tx_eq_type_t tx_eq_type;
    major_module_type_t major_module_type;
};
#endif /*__SFF_SFP_DD_H*/
