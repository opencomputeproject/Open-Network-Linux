#ifndef __SFF_SPEC_H
#define __SFF_SPEC_H
/*sff_spec mainly contains
 * -sff 8024 spec
 * -other module common structure
 */
#define SFF_8024_ID_OFFSET (0)
#define SFF_REV_OFFSET (1)
#define SFF_STATUS_OFFSET (2)
#define SFF_PAGE_SEL_OFFSET (127)

#define PAGE_NUM  (256)
#define EEPROM_SIZE (256)
#define EEPROM_HALF_SIZE (128)
#define SFF_EEPROM_I2C_ADDR (0x50)
#define SFF_DDM_I2C_ADDR (0x51)
#define VENDOR_INFO_BUF_SIZE (32)

typedef enum {
    SFF_UNKNOWN_TYPE,
    SFP_TYPE,
    SFP_DD_TYPE,
    QSFP_TYPE,
    QSFP56_TYPE,
    QSFP_DD_TYPE,
    SFF_TYPE_NUM
} sff_type;

enum {
    SFF_8024_ID_SFP = 0x3,
    SFF_8024_ID_SFP2 = 0x0b,
    SFF_8024_ID_QSFP = 0xc,
    SFF_8024_ID_QSFP_PLUS = 0xd,
    SFF_8024_ID_QSFP28 = 0x11,
    SFF_8024_ID_QSFP_DD = 0x18,
    SFF_8024_ID_SFP_DD = 0x1a, /*<TBD>*/
    SFF_8024_ID_QSFP56 = 0x1e,  /* <TBD> QSFP+ or later with Common Management Interface Specification (CMIS) <TBD>*/
};

enum {
    CONN_TYPE_LC = 0x07,
    CONN_TYPE_OPTICAL_PIGTAIL = 0x0b,
    CONN_TYPE_COPPER_PIGTAIL = 0x21
};
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
/*sff-8024 rev 4.6 Table 4-4 Extended Specification Compliance Codes */
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
    AOC_200G_BER_6 = 0x30,
    AOC_200G_BER_6_V2 = 0x31,
    AOC_200G_BER_5 = 0x32,
    AOC_200G_BER_5_V2 = 0x33,
    CR4_200GBASE = 0x40, /*50GBASE-CR, 100GBASE-CR2, or 200GBASE-CR4*/
    SR4_200GBASE = 0x41,/*50GBASE-SR, 100GBASE-SR2, or 200GBASE-SR4*/
    DR4_200GBASE = 0x42,/*50GBASE-FR or 200GBASE-DR4*/
    FR4_200GBASE = 0x43,/*200GBASE-FR4*/
    PSM4_200G = 0x44,/*200G 1550 nm PSM4 */
    LR4_200GBASE = 0x46/*200GBASE-LR4*/

} ext_comp_encode_t;

typedef enum {
    TRANSVR_CLASS_UNKNOWN=0,
    /* Transceiver class for Optical 10G */
    TRANSVR_CLASS_OPTICAL_10G_S_AOC =(27011),
    TRANSVR_CLASS_OPTICAL_10G_S_SR  =(27012),
    TRANSVR_CLASS_OPTICAL_10G_S_LR  =(27013),
    TRANSVR_CLASS_OPTICAL_10G_S_ER  =(27014),
    TRANSVR_CLASS_OPTICAL_10G_Q_AOC =(27015),
    TRANSVR_CLASS_OPTICAL_10G_Q_SR  =(27016),
    TRANSVR_CLASS_OPTICAL_10G_Q_LR  =(27017),
    TRANSVR_CLASS_OPTICAL_10G_Q_ER  =(27018),
    /* Transceiver class for Optical 25G */
    TRANSVR_CLASS_OPTICAL_25G_AOC   =(27021),
    TRANSVR_CLASS_OPTICAL_25G_SR    =(27022),
    TRANSVR_CLASS_OPTICAL_25G_LR    =(27023),
    TRANSVR_CLASS_OPTICAL_25G_ER    =(27024),
    /* Transceiver class for Optical 40G */
    TRANSVR_CLASS_OPTICAL_40G_AOC   =(27041),
    TRANSVR_CLASS_OPTICAL_40G_SR4   =(27042),
    TRANSVR_CLASS_OPTICAL_40G_LR4   =(27043),
    TRANSVR_CLASS_OPTICAL_40G_ER4   =(27044),
    /* Transceiver class for Optical 100G */
    TRANSVR_CLASS_OPTICAL_100G_AOC  =(27101),
    TRANSVR_CLASS_OPTICAL_100G_SR4  =(27102),
    TRANSVR_CLASS_OPTICAL_100G_LR4  =(27103),
    TRANSVR_CLASS_OPTICAL_100G_ER4  =(27104),
    TRANSVR_CLASS_OPTICAL_100G_PSM4 =(27105),
    TRANSVR_CLASS_OPTICAL_100G_CWDM4 =(27106),
    /* Transceiver class for Copper */
    TRANSVR_CLASS_COPPER_L1_1G      =(28001),
    TRANSVR_CLASS_COPPER_L1_10G     =(28011),
    TRANSVR_CLASS_COPPER_L4_10G     =(28012),
    TRANSVR_CLASS_COPPER_L1_25G     =(28021),
    TRANSVR_CLASS_COPPER_L4_40G     =(28041),
    TRANSVR_CLASS_COPPER_L4_100G    =(28101),
    TRANSVR_CLASS_COPPER_L4_200G    =(28201),
    /* Transceiver class for Optical 200G */
    TRANSVR_CLASS_OPTICAL_200G_AOC  =(27201),
    TRANSVR_CLASS_OPTICAL_200G_SR4  =(27202),
    TRANSVR_CLASS_OPTICAL_200G_LR4  =(27203),
    TRANSVR_CLASS_OPTICAL_200G_ER4  =(27204),
    TRANSVR_CLASS_OPTICAL_200G_PSM4 =(27205),
    TRANSVR_CLASS_OPTICAL_200G_CWDM4 =(27206),
    TRANSVR_CLASS_OPTICAL_200G_DR4 = 27207

} transvr_type_t;
typedef enum {
    POWER_CLASS_1_MODULE = 0,
    POWER_CLASS_2_MODULE = 1,
    POWER_CLASS_3_MODULE = 2,
    POWER_CLASS_4_MODULE = 3,

} Power_Class_t;

struct page_base_reg_t {
    u8 page;
    u8 offset;
    int len;
};

struct addr_base_reg_t {
    u8 addr;
    u8 offset;
    int len;
};

typedef enum {
    TX_EQ_AUTO,
    TX_EQ_MANUAL,
    TX_EQ_TYPE_NUM
} tx_eq_type_t; 

struct advert_tx_eq_t {
    bool is_supported[TX_EQ_TYPE_NUM];
            
};   

struct intr_flag_t {
    u8 reg;
    u32 cnt;
    bool chg;
};

struct paging_supported_t {
    bool val;
    bool valid;
};

typedef enum {
    TX_EQ_TYPE,
    RX_EM_TYPE,
    RX_AM_TYPE,
} sysfs_attrbute_index_ln_control_t;

typedef enum {
    LN_MONITOR_RX_PWR_TYPE,
    LN_MONITOR_TX_PWR_TYPE,
    LN_MONITOR_TX_BIAS_TYPE,
} sysfs_attrbute_index_ln_monitor_t;

typedef enum {
    LN_STATUS_RXLOS_TYPE,
    LN_STATUS_TXLOS_TYPE,
    LN_STATUS_TXFAULT_TYPE,
    LN_STATUS_NUM,
} sysfs_attrbute_index_ln_status_t;

typedef enum {
    VENDOR_NAME_TYPE,
    VENDOR_PN_TYPE,
    VENDOR_SN_TYPE,
    VENDOR_REV_TYPE,
} sysfs_attrbute_index_vendor_info_t;

typedef enum {
    ACTIVE_MODULE_TYPE,
    PASSIVE_MODULE_TYPE,
    LOOPBACK_MODULE_TYPE,
    UNKNOWN_MODULE_TYPE,
    BRIEF_MODULE_TYPE_NUM
} major_module_type_t;

#define reg_len_matched(len, def_len) ((len == def_len) ? true : false)
#endif /* __SFF_SPEC_H */


