/*
 *  The Definition of the TlvInfo EEPROM format can be found at onie.org or
 *  github.com/onie
 */
#include <linux/types.h>
#define strtoul simple_strtoul

#define FALSE   0
#define TRUE    (!FALSE)
#define MAX_STRING_SIZE	128
/*
 * Tlvinf header: Layout of the header for the TlvInfo format
 *
 * See the end of this file for details of this eeprom format
 */
struct __attribute__ ((__packed__)) tlvinfo_header_s {
    char    signature[8];   /* 0x00 - 0x07 EEPROM Tag "TlvInfo" */
    u_int8_t      version;  /* 0x08        Structure version */
    u_int16_t     totallen; /* 0x09 - 0x0A Length of all data which follows */
};
typedef struct tlvinfo_header_s tlvinfo_header_t;

// Header Field Constants
#define TLV_INFO_ID_STRING      "TlvInfo"
#define TLV_INFO_VERSION        0x01
#define TLV_TOTAL_LEN_MAX       (SYS_EEPROM_SIZE - sizeof(tlvinfo_header_t))

/*
 * TlvInfo TLV: Layout of a TLV field
 */
struct __attribute__ ((__packed__)) tlvinfo_tlv_s {
    u_int8_t  type;
    u_int8_t  length;
    u_int8_t  value[0];
};
typedef struct tlvinfo_tlv_s tlvinfo_tlv_t;

/* Maximum length of a TLV value in bytes */
#define TLV_VALUE_MAX_LEN        255

/**
 *  The TLV Types.
 *
 *  Keep these in sync with tlv_code_list in cmd_sys_eeprom.c
 */
#define TLV_CODE_PRODUCT_NAME   0x21
#define TLV_CODE_PART_NUMBER    0x22
#define TLV_CODE_SERIAL_NUMBER  0x23
#define TLV_CODE_MAC_BASE       0x24
#define TLV_CODE_MANUF_DATE     0x25
#define TLV_CODE_DEVICE_VERSION 0x26
#define TLV_CODE_LABEL_REVISION 0x27
#define TLV_CODE_PLATFORM_NAME  0x28
#define TLV_CODE_ONIE_VERSION   0x29
#define TLV_CODE_MAC_SIZE       0x2A
#define TLV_CODE_MANUF_NAME     0x2B
#define TLV_CODE_MANUF_COUNTRY  0x2C
#define TLV_CODE_VENDOR_NAME    0x2D
#define TLV_CODE_DIAG_VERSION   0x2E
#define TLV_CODE_SERVICE_TAG    0x2F
#define TLV_CODE_VENDOR_EXT     0xFD
#define TLV_CODE_CRC_32         0xFE

/*
 *  Struct for displaying the TLV codes and names.
 */
struct tlv_code_desc {
    u_int8_t m_code;
    char* m_name;
};

/*
 *  List of TLV codes and names.
 */
static const struct tlv_code_desc tlv_code_list[] = {
    { TLV_CODE_PRODUCT_NAME	 , "Product Name"},
    { TLV_CODE_PART_NUMBER	 , "Part Number"},
    { TLV_CODE_SERIAL_NUMBER     , "Serial Number"},
    { TLV_CODE_MAC_BASE	         , "Base MAC Address"},
    { TLV_CODE_MANUF_DATE	 , "Manufacture Date"},
    { TLV_CODE_DEVICE_VERSION    , "Device Version"},
    { TLV_CODE_LABEL_REVISION    , "Label Revision"},
    { TLV_CODE_PLATFORM_NAME     , "Platform Name"},
    { TLV_CODE_ONIE_VERSION	 , "Loader Version"},
    { TLV_CODE_MAC_SIZE	         , "MAC Addresses"},
    { TLV_CODE_MANUF_NAME	 , "Manufacturer"},
    { TLV_CODE_MANUF_COUNTRY     , "Country Code"},
    { TLV_CODE_VENDOR_NAME	 , "Vendor Name"},
    { TLV_CODE_DIAG_VERSION	 , "Diag Version"},
    { TLV_CODE_SERVICE_TAG       , "Service Tag"},
    { TLV_CODE_VENDOR_EXT	 , "Vendor Extension"},
    { TLV_CODE_CRC_32	         , "CRC-32"},
};

static inline const char* tlv_type2name(u_int8_t type)
{
    char* name = "Unknown";
    int   i;

    for (i = 0; i < sizeof(tlv_code_list)/sizeof(tlv_code_list[0]); i++) {
	if (tlv_code_list[i].m_code == type) {
	    name = tlv_code_list[i].m_name;
	    break;
	}
    }
    return name;
}

/*
 * The max decode value is currently for the 'raw' type or the 'vendor
 * extension' type, both of which have the same decode format.  The
 * max decode string size is computed as follows:
 *
 *   strlen(" 0xFF") * TLV_VALUE_MAX_LEN + 1
 *
 */
#define TLV_DECODE_VALUE_MAX_LEN    ((5 * TLV_VALUE_MAX_LEN) + 1)


/*
 * Each platform must define the following platform-specific macros
 * in sys_eeprom_platform.h:
 * SYS_EEPROM_SIZE: size of usable eeprom
 * SYS_EEPROM_I2C_DEVICE: i2c-bus
 * SYS_EEPROM_I2C_ADDR: address on the bus
 * The following may also be defined in sys_eeprom_platform.h, else
 * the defaults with take over:
 * SYS_EEPROM_MAX_SIZE: Total size of the eeprom
 * SYS_EEPROM_OFFSET: offset from where the ONIE header starts
 */
#define SYS_EEPROM_MAX_SIZE   2048
#define SYS_EEPROM_OFFSET     0
#define SYS_EEPROM_SIZE       SYS_EEPROM_MAX_SIZE
#define SYS_EEPROM_I2C_DEVICE "/dev/i2c-0"
#define SYS_EEPROM_I2C_ADDR   0x51

#if (SYS_EEPROM_SIZE + SYS_EEPROM_OFFSET > SYS_EEPROM_MAX_SIZE)
  #error SYS_EEPROM_SIZE + SYS_EEPROM_OFFSET is greater than SYS_EEPROM_MAX_SIZE
#endif

// Access functions to onie_tlvinfo
void show_eeprom(u_int8_t *eeprom);
int read_eeprom(struct i2c_client *pi2c_client, u_int8_t *eeprom);
int prog_eeprom(struct i2c_client *pi2c_client,u_int8_t * eeprom);
void update_eeprom_header(u_int8_t *eeprom);
bool tlvinfo_find_tlv(u_int8_t *eeprom, u_int8_t tcode, int *eeprom_index);
bool tlvinfo_delete_tlv(u_int8_t * eeprom, u_int8_t code);
bool tlvinfo_add_tlv(u_int8_t * eeprom, int tcode, char * strval);
bool tlvinfo_decode_tlv(u_int8_t *eeprom, u_int8_t tcode, char* value);
//int  find_vpd_data(u_int8_t *eeprom, int i_offset, char *c_buf);
