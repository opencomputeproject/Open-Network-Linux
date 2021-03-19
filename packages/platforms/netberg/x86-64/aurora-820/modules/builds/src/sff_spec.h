#ifndef __SFF_SPEC_H
#define __SFF_SPEC_H

//sff spec , 8472 8436
typedef enum {
    /* Shared QSFP and SFP fields: */
    IDENTIFIER, /* Type of Transceiver */
    STATUS,     /* Support flags for upper pages */
    INTERRUPT_FLAG,
    TEMPERATURE_ALARMS,
    VCC_ALARMS, /* Voltage */
    CHANNEL_RX_PWR_ALARMS,
    CHANNEL_TX_BIAS_ALARMS,
    TEMPERATURE,
    VCC, /* Voltage */
    CHANNEL_RX_PWR,
    CHANNEL_TX_PWR,/* nba820 add*/
    CHANNEL_TX_BIAS,
    CHANNEL_TX_DISABLE,
    POWER_CONTROL,
    ETHERNET_COMPLIANCE,
    EXTENDED_IDENTIFIER,
    PAGE_SELECT_BYTE,
    LENGTH_SM_KM, /* Single mode, in km */
    LENGTH_SM,    /* Single mode in 100m (not in QSFP) */
    LENGTH_OM3,
    LENGTH_OM2,
    LENGTH_OM1,
    LENGTH_COPPER,
    VENDOR_NAME,     /* QSFP Vendor Name (ASCII) */
    VENDOR_OUI,      /* QSFP Vendor IEEE company ID */
    PART_NUMBER,     /* Part NUmber provided by QSFP vendor (ASCII) */
    REVISION_NUMBER, /* Revision number */
    ETHERNET_EXTENDED_COMPLIANCE, /* ethernet extended compliance code */
    VENDOR_SERIAL_NUMBER,         /* Vendor Serial Number (ASCII) */
    MFG_DATE,                     /* Manufacturing date code */
    DIAGNOSTIC_MONITORING_TYPE,   /* Diagnostic monitoring implemented */
    TEMPERATURE_THRESH,
    VCC_THRESH,
    RX_PWR_THRESH,
    TX_BIAS_THRESH,

    /* SFP-specific Fields */
    /* 0xA0 Address Fields */
    EXT_IDENTIFIER,        /* Extended type of transceiver */
    CONNECTOR_TYPE,        /* Code for Connector Type */
    TRANSCEIVER_CODE,      /* Code for Electronic or optical capability */
    ENCODING_CODE,         /* High speed Serial encoding algo code */
    SIGNALLING_RATE,       /* nominal signalling rate */
    RATE_IDENTIFIER,       /* type of rate select functionality */
    TRANCEIVER_CAPABILITY, /* Code for Electronic or optical capability */
    WAVELENGTH,            /* laser wavelength */
    CHECK_CODE_BASEID,     /* Check code for the above fields */
    /* Extended options */
    ENABLED_OPTIONS, /* Indicates the optional transceiver signals enabled */
    UPPER_BIT_RATE_MARGIN,   /* upper bit rate margin */
    LOWER_BIT_RATE_MARGIN,   /* lower but rate margin */
    ENHANCED_OPTIONS,        /* Enhanced options implemented */
    SFF_COMPLIANCE,          /* revision number of SFF compliance */
    CHECK_CODE_EXTENDED_OPT, /* check code for the extended options */
    VENDOR_EEPROM,
    /* nba820 add*/
    CDR_CONTROL,
    OPTIIONAL_CHANNEL_CONTROL_TX_EQ,
    OPTIIONAL_CHANNEL_CONTROL_RX_EM,
    OPTIIONAL_CHANNEL_CONTROL_RX_AM,
    OPTIIONAL_INDICATOR,
    OPTIIONAL_INDICATOR_EM,
    /* 0xA2 address Fields */
    /* Diagnostics */
    ALARM_THRESHOLD_VALUES,  /* diagnostic flag alarm and warning thresh values */
    EXTERNAL_CALIBRATION,    /* diagnostic calibration constants */
    CHECK_CODE_DMI,          /* Check code for base Diagnostic Fields */
    DIAGNOSTICS,             /* Diagnostic Monitor Data */
    STATUS_CONTROL,          /* Optional Status and Control bits */
    ALARM_WARN_FLAGS,        /* Diagnostic alarm and warning flag */
    EXTENDED_STATUS_CONTROL, /* Extended status and control bytes */
    /* General Purpose */
    VENDOR_MEM_ADDRESS, /* Vendor Specific memory address */
    USER_EEPROM,        /* User Writable NVM */
    VENDOR_CONTROL,     /* Vendor Specific Control */
    /*qsfp-dd*/
    ID_STATUS_SUMMARY,
    MODULE_FLAGS,
    SFF_FIELD_MAX /* keep this the last */
} Sff_field;


typedef struct {
    u8 slave_addr;
    u8 offset;
    int length;
} sff_sfp_field_info_t;

typedef struct {
    Sff_field field;
    sff_sfp_field_info_t data;
} sff_sfp_field_map_t;

typedef enum {
    //QSFP_PAGE0 = 0xff,
    QSFP_PAGE0 = 0,
    QSFP_PAGE3 = 3 ,
    QSFP_PAGE_01h = 0x01,
    QSFP_PAGE_10h = 0x10,
    QSFP_PAGE_11h = 0x11,
} Qsfp_page;

typedef struct {
    Qsfp_page page;
    u8 offset;
    int length;
} sff_qsfp_field_info_t;

typedef struct {
    Sff_field field;
    sff_qsfp_field_info_t data;
} sff_qsfp_field_map_t;


typedef struct {
    unsigned rx_output_em: 4;
    unsigned tx_input_eq: 4;
    unsigned rx_output_amp1: 1;
    unsigned rx_output_amp2: 1;
    unsigned rx_output_amp3: 1;
    unsigned rx_output_amp4: 1;
    unsigned rx_output_em_type: 2;
    unsigned reserved: 2;

} qsfp_optional_indicator_t; /*u16 size*/

/* following complianbce codes are derived from SFF-8436 document */
typedef enum {
    COMPLIANCE_NONE = 0,
    ACTIVE_CABLE = 1 << 0,
    LR4_40GBASE = 1 << 1,
    SR4_40GBASE = 1 << 2,
    CR4_40GBASE = 1 << 3,
    /*10G Ethernet Compliance Codes*/
    SR_10GBASE = 1 << 4,
    LR_10GBASE = 1 << 5,
    LRM_10GBASE = 1 << 6,
    ER_10GBASE = 1 << 7,/*COMPLIANCE_RSVD*/
} Ethernet_compliance;

/* following complianbce codes are derived from SFF-8024 document */
typedef enum {
    EXT_COMPLIANCE_NONE = 0,
    AOC_100G_BER_5 = 0x01, /* 100G AOC or 25G AUI C2M AOC 5 * 10^^-5 BER */
    SR4_100GBASE = 0x02,   /* or SR-25GBASE */
    LR4_100GBASE = 0x03,   /* or LR-25GBASE */
    ER4_100GBASE = 0x04,   /* or ER-25GBASE */
    SR10_100GBASE = 0x05,
    CWDM4_100G = 0x06,
    PSM4_100G_SMF = 0x07,
    ACC_100G_BER_5 = 0x08, /* 100G ACC or 25G AUI C2M ACC 5 * 10^^-5 BER */
    EXT_COMPLIANCE_OBSOLETE = 0x09,
    EXT_COMPLIANCE_RSVD1 = 0x0A,
    CR4_100GBASE = 0x0B, /* or CR-25GBASE CA-L */
    CR_25GBASE_CA_S = 0x0C,
    CR_25GBASE_CA_N = 0x0D,
    EXT_COMPLIANCE_RSVD2 = 0x0E,
    EXT_COMPLIANCE_RSVD3 = 0x0F,
    ER4_40GBASE = 0x10,
    SR_10GBASE_4 = 0x11,
    PSM4_40G_SMF = 0x12,
    G959_P1I1_2D1 = 0x13, /* 10709 Mbd, 2 km, 1310nm SM */
    G959_P1S1_2D2 = 0x14, /* 10709 Mbd, 40 km, 1550nm SM */
    G959_P1L1_2D2 = 0x15, /* 10709 Mbd, 80 km, 1550nm SM */
    T_10BASE_SFI = 0x16,  /* 10BASE-T with SFI electrical interface */
    CLR4_100G = 0x17,
    AOC_100G_BER_12 = 0x18, /* 100G AOC or 25G AUI C2M AOC 10^^-12 BER */
    ACC_100G_BER_12 = 0x19, /* 100G ACC or 25G AUI C2M ACC 10^^-12 BER */
    DWDM2_100GE = 0x1A,     /* DMWM module using 1550nm, 80 km */
} Ethernet_extended_compliance;

typedef enum {
    POWER_CLASS_1_MODULE = 0,
    POWER_CLASS_2_MODULE = 1,
    POWER_CLASS_3_MODULE = 2,
    POWER_CLASS_4_MODULE = 3,

} Power_Class_t;

struct qsfp_interrupt_flag_t {

    u8 los_ind;         //  3. [7:4] Tx LOS : [3:0] Rx LOS
    u8 eq_laser_fault;  //  4. [7:4] Tx Adapt Fault : [3:0] Tx laser fault
    u8 lol_ind;         //  5. [7:4] Tx LOL : [3:0] Rx LOL
    u8 temp_alarm;   //  6. [7:4] Temp alarm/warning : [0:0] init complete fl
    u8 vcc_alarm;    //  7. [7:4] Vcc alarm/warning
    u8 vendor_spec;  //  8. unused
    u8 rx_power_alarm12;  //  9. Rx power alarm/warning, Ch1 and Ch2
    u8 rx_power_alarm34;  // 10. Rx power alarm/warning, Ch3 and Ch4
    u8 tx_bias_alarm12;   // 11. Tx Bias  alarm/warning, Ch1 and Ch2
    u8 tx_bias_alarm34;   // 12. Tx Bias  alarm/warning, Ch3 and Ch4
    u8 tx_power_alarm12;  // 13. Rx power alarm/warning, Ch1 and Ch2
    u8 tx_power_alarm34;  // 14. Rx power alarm/warning, Ch3 and Ch4

};

/*qsfp-dd*/
/*rev 3.0*/
typedef enum {

    MODULE_LOW_PWR_ST_ENCODE = 0x1,
    MODULE_PWR_UP_ST_ENCODE = 0x2,
    MODULE_READY_ST_ENCODE = 0x3,
    MODULE_PWR_DOWN_ST_ENCODE = 0x4,
    MODULE_FAULT_ST_ENCODE = 0x5,

} Module_State_Encoding_t;

typedef enum {

    MODULE_TYPE_MMF = 0x1,
    MODULE_TYPE_SMF = 0x2,
    MODULE_PASSIVE_CU = 0x3,
    MODULE_ACTIVE_CABLES = 0x4,
    MODULE_BASE_T = 0x5,

} Module_Type_Encoding_t;

typedef enum {

    DATA_PATH_DEACTIVATED_ST_ENCODE = 0x1,
    DATA_PATH_INIT_ST_ENCODE = 0x2,
    DATA_PATH_DEINIT_ST_ENCODE = 0x3,
    DATA_PATH_ACTIVATED_ST_ENCODE = 0x4,

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

#if 0
struct id_status_field_offset2_t {
    unsigned reserved : 2;
    unsigned twi_max_speed : 2;
    unsigned reserved2 : 2;
    unsigned clei_present : 1;
    unsigned flat_mem : 1;
};
struct id_status_field_offset3_t {
    unsigned intL_sw : 1;/*digital state of interrupt output signal*/
    unsigned module_state : 3;
    unsigned reserved : 4;
};
struct qsfp_dd_id_status_t {
    u8 id;
    u8 revision_compliance;
    struct id_status_field_offset2_t offset2;
    struct id_status_field_offset3_t offset3;
};

struct qsfp_dd_module_flags_t {
    unsigned char state_change_flag : 1;
    unsigned char module_fw_fault : 1;
    unsigned char data_path_fw_fault : 1;
    unsigned char reserved : 2;
};
#endif
/*app_advert_field*/
#define APP_ADVERT_FIELD_NUM (5)
struct qsfp_dd_app_advert_fields_t {

    u8 host_electrical_interface_code;
    u8 module_media_interface_code;
    u8 lane_count;
    u8 host_lane_assignment_options;
    u8 media_lane_assignment_options;
};

union qsfp_dd_app_advert_fields {

    struct qsfp_dd_app_advert_fields_t data;
    u8 reg[APP_ADVERT_FIELD_NUM];
};
typedef enum {
    APSEL_1,
    APSEL_2,
    APSEL_NUM,

} apsel_code_t;

struct stage_set_t {
    unsigned char explicit_control : 1;
    unsigned char datapath_code : 3;
    unsigned char app_code : 4;

};
#define RESET_ASSERT_TIME (10) /*ms*/
/*
union active_control_set_t {

    struct stage_set_t data;
    u8 reg;
};
*/
/*qsfp-dd Signal Integrity Control define
 *  * Table 46- Implemented Signal Integrity Controls (Page 01h) */
/*offset: 161*/
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
#endif /* __SFF_SPEC_H */


