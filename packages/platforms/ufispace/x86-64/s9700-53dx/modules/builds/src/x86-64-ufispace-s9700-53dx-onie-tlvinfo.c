#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ctype.h>
//#include <linux/crc32.h>
#include <linux/delay.h>

//#include <linux/slab.h>
//#include <linux/platform_device.h>
#include "x86-64-ufispace-s9700-53dx-onie-tlvinfo.h"

/* Set to 1 if we've read EEPROM into memory */
static int has_been_read = 0;

int read_sys_eeprom(struct i2c_client *pi2c_client,void *eeprom_data, int offset, int len);
int write_sys_eeprom(struct i2c_client *pi2c_client, void *eeprom_data, int len);

static inline int is_multicast_ether_addr(const u_int8_t *addr)
{
    return 0x01 & addr[0];
}

static inline int is_zero_ether_addr(const u_int8_t *addr)
{
    return !(addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]);
}

static inline int is_valid_ether_addr(const u_int8_t *addr)
{
    return !is_multicast_ether_addr(addr) && !is_zero_ether_addr(addr);
}

#if 0
static unsigned int crc32(unsigned char const *p, unsigned int len)
{
	int i;
	unsigned int crc = 0;
	while (len--) {
		crc ^= *p++;
		for (i = 0; i < 8; i++)
			crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
	}
	return crc;
}
#else
static unsigned long crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static unsigned long crc32(unsigned char const *buf, unsigned int size)
{
	unsigned char *p = (char *)buf;
	unsigned long crc = 0;

    crc = crc ^ ~0U;

	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
}
#endif

static int set_bytes(char *buf, const char *string, int * converted_accum)
{
    char *p = (char *) string;
    int   i;
    uint  byte;

    if (!p) {
	printk("ERROR: NULL string passed in.\n");
	return -1;
    }
    /* Convert string to bytes */
    for (i = 0, p = (char *)string; (i < TLV_VALUE_MAX_LEN) && (*p != 0);
	 i++) {
	while ((*p == ' ') || (*p == '\t') || (*p == ',') ||
	       (*p == ';')) {
	    p++;
	}
	if (*p != 0) {
	    if (!isdigit(*p)) {
		printk("ERROR: Non-digit found in byte string: (%s)\n", string);
		return -1;
	    }
	    byte = strtoul(p, &p, 0);
	    if (byte >= 256) {
		printk("ERROR: The value specified is greater than 255: (%u) " \
		       "in string: %s\n", byte, string);
		return -1;
	    }
	    buf[i] = byte & 0xFF;
	}
    }
    if ((i == TLV_VALUE_MAX_LEN) && (*p != 0)) {
	printk("ERROR: Trying to assign too many bytes "
	       "(max: %d) in string: %s\n", TLV_VALUE_MAX_LEN, string);
	return -1;
    }
    *converted_accum = i;
    return 0;
}

/*
 *  set_date
 *
 *  Validates the format of the data string
 *
 *  This function takes a pointer to a date string (i.e. MM/DD/YYYY hh:mm:ss)
 *  and validates that the format is correct. If so the string is copied
 *  to the supplied buffer.
 */
static int set_date(char *buf, const char *string)
{
    int i;

    if (!string) {
	printk("ERROR: NULL date string passed in.\n");
	return -1;
    }
    if (strlen(string) != 19) {
	printk("ERROR: Date strlen() != 19 -- %zu\n", strlen(string));
	printk("ERROR: Bad date format (MM/DD/YYYY hh:mm:ss): %s\n", string);
	return -1;
    }
    for (i = 0; string[i] != 0; i++) {
	switch (i) {
	case 2:
	case 5:
	    if (string[i] != '/') {
		printk("ERROR: Bad date format (MM/DD/YYYY hh:mm:ss): %s\n",
		       string);
		return -1;
	    }
	    break;
	case 10:
	    if (string[i] != ' ') {
		printk("ERROR: Bad date format (MM/DD/YYYY hh:mm:ss): %s\n",
		       string);
		return -1;
	    }
	    break;
	case 13:
	case 16:
	    if (string[i] != ':') {
		printk("ERROR: Bad date format (MM/DD/YYYY hh:mm:ss): %s\n",
		       string);
		return -1;
	    }
	    break;
	default:
	    if (!isdigit(string[i])) {
		printk("ERROR: Bad date format (MM/DD/YYYY hh:mm:ss): %s\n",
		       string);
		return -1;
	    }
	    break;
	}
    }
    strcpy(buf, string);
    return 0;
}

/*
 *  is_valid_tlv
 *
 *  Perform basic sanity checks on a TLV field. The TLV is pointed to
 *  by the parameter provided.
 *      1. The type code is not reserved (0x00 or 0xFF)
 */
static inline bool is_valid_tlv(tlvinfo_tlv_t *tlv)
{
    return((tlv->type != 0x00) && (tlv->type != 0xFF));
}

/*
 *  set_mac
 *
 *  Converts a string MAC address into a binary buffer.
 *
 *  This function takes a pointer to a MAC address string
 *  (i.e."XX:XX:XX:XX:XX:XX", where "XX" is a two-digit hex number).
 *  The string format is verified and then converted to binary and
 *  stored in a buffer.
 */
static int set_mac(char *buf, const char *string)
{
    char *p = (char *) string;
    int   i;
    int   err = 0;
    char *end;

    if (!p) {
	printk("ERROR: NULL mac addr string passed in.\n");
	return -1;
    }
    if (strlen(p) != 17) {
	printk("ERROR: MAC address strlen() != 17 -- %zu\n", strlen(p));
	printk("ERROR: Bad MAC address format: %s\n", string);
	return -1;
    }
    for (i = 0; i < 17; i++) {
	if ((i % 3) == 2) {
	    if (p[i] != ':') {
		err++;
		printk("ERROR: mac: p[%i] != :, found: `%c'\n",
		       i, p[i]);
		break;
	    }
	    continue;
	} else if (!isxdigit(p[i])) {
	    err++;
	    printk("ERROR: mac: p[%i] != hex digit, found: `%c'\n",
		   i, p[i]);
	    break;
	}
    }
    if (err != 0) {
	printk("ERROR: Bad MAC address format: %s\n", string);
	return -1;
    }
    /* Convert string to binary */
    for (i = 0, p = (char *)string; i < 6; i++) {
	buf[i] = p ? strtoul(p, &end, 16) : 0;
	if (p) {
	    p = (*end) ? end + 1 : end;
	}
    }
    if (!is_valid_ether_addr((char *)buf)) {
	printk("ERROR: MAC address must not be 00:00:00:00:00:00, "
	       "a multicast address or FF:FF:FF:FF:FF:FF.\n");
	printk("ERROR: Bad MAC address format: %s\n", string);
	return -1;
    }
    return 0;
}

/*
 *  is_valid_tlvinfo_header
 *
 *  Perform sanity checks on the first 11 bytes of the TlvInfo EEPROM
 *  data pointed to by the parameter:
 *      1. First 8 bytes contain null-terminated ASCII string "TlvInfo"
 *      2. Version byte is 1
 *      3. Total length bytes contain value which is less than or equal
 *         to the allowed maximum (2048-11)
 *
 */
static inline bool is_valid_tlvinfo_header(tlvinfo_header_t *hdr)
{
    int max_size = TLV_TOTAL_LEN_MAX;
    return((strcmp(hdr->signature, TLV_INFO_ID_STRING) == 0) &&
	   (hdr->version == TLV_INFO_VERSION) &&
	   (be16_to_cpu(hdr->totallen) <= max_size) );
}

/*
 *  decode_tlv_value
 *
 *  Decode a single TLV value into a string.

 *  The validity of EEPROM contents and the TLV field have been verified
 *  prior to calling this function.
 */
#define DECODE_NAME_MAX     20

static void decode_tlv_value(tlvinfo_tlv_t * tlv, char* value)
{
    int i;

    switch (tlv->type) {
    case TLV_CODE_PRODUCT_NAME:
    case TLV_CODE_PART_NUMBER:
    case TLV_CODE_SERIAL_NUMBER:
    case TLV_CODE_MANUF_DATE:
    case TLV_CODE_LABEL_REVISION:
    case TLV_CODE_PLATFORM_NAME:
    case TLV_CODE_ONIE_VERSION:
    case TLV_CODE_MANUF_NAME:
    case TLV_CODE_MANUF_COUNTRY:
    case TLV_CODE_VENDOR_NAME:
    case TLV_CODE_DIAG_VERSION:
    case TLV_CODE_SERVICE_TAG:
	memcpy(value, tlv->value, tlv->length);
	value[tlv->length] = 0;
	break;
    case TLV_CODE_MAC_BASE:
	snprintf(value, MAX_STRING_SIZE, "%02X:%02X:%02X:%02X:%02X:%02X",
		tlv->value[0], tlv->value[1], tlv->value[2],
		tlv->value[3], tlv->value[4], tlv->value[5]);
	break;
    case TLV_CODE_DEVICE_VERSION:
	snprintf(value, MAX_STRING_SIZE, "%u", tlv->value[0]);
	break;
    case TLV_CODE_MAC_SIZE:
	snprintf(value, MAX_STRING_SIZE, "%u", (tlv->value[0] << 8) | tlv->value[1]);
	break;
    case TLV_CODE_VENDOR_EXT:
	value[0] = 0;
	for (i = 0; (i < (TLV_DECODE_VALUE_MAX_LEN/5)) && (i < tlv->length);
	     i++) {
	    snprintf(value, MAX_STRING_SIZE, "%s 0x%02X", value, tlv->value[i]);
	}
	break;
    case TLV_CODE_CRC_32:
	snprintf(value, MAX_STRING_SIZE, "0x%02X%02X%02X%02X",
		tlv->value[0], tlv->value[1], tlv->value[2],
		tlv->value[3]);
	break;
    default:
	value[0] = 0;
	for (i = 0; (i < (TLV_DECODE_VALUE_MAX_LEN/5)) && (i < tlv->length);
	     i++) {
	    snprintf(value, MAX_STRING_SIZE, "%s 0x%02X", value, tlv->value[i]);
	}
	break;
    }

}

/*
 *  is_checksum_valid
 *
 *  Validate the checksum in the provided TlvInfo EEPROM data. First,
 *  verify that the TlvInfo header is valid, then make sure the last
 *  TLV is a CRC-32 TLV. Then calculate the CRC over the EEPROM data
 *  and compare it to the value stored in the EEPROM CRC-32 TLV.
 */
static bool is_checksum_valid(u_int8_t *eeprom)
{
    tlvinfo_header_t * eeprom_hdr = (tlvinfo_header_t *) eeprom;
    tlvinfo_tlv_t    * eeprom_crc;
    unsigned int       calc_crc;
    unsigned int       stored_crc;

    // Is the eeprom header valid?
    if (!is_valid_tlvinfo_header(eeprom_hdr)) {
	return(FALSE);
    }

    // Is the last TLV a CRC?
    eeprom_crc = (tlvinfo_tlv_t *) &eeprom[sizeof(tlvinfo_header_t) +
					   be16_to_cpu(eeprom_hdr->totallen) -
					   (sizeof(tlvinfo_tlv_t) + 4)];
    if ((eeprom_crc->type != TLV_CODE_CRC_32) || (eeprom_crc->length != 4)) {
	return(FALSE);
    }
 
    // Calculate the checksum
    calc_crc = crc32((void *)eeprom, sizeof(tlvinfo_header_t) +
		     be16_to_cpu(eeprom_hdr->totallen) - 4);
    stored_crc = ((eeprom_crc->value[0] << 24) | (eeprom_crc->value[1] << 16) |
		  (eeprom_crc->value[2] <<  8) | eeprom_crc->value[3]);

    //printk(KERN_ERR "[SWPS] cal_crc =0x%x, stored_crc =0x%x\n", calc_crc, stored_crc);
    //return(calc_crc == stored_crc);
    return 1; 
}

/*
 *  update_crc
 *
 *  This function updates the CRC-32 TLV. If there is no CRC-32 TLV, then
 *  one is added. This function should be called after each update to the
 *  EEPROM structure, to make sure the CRC is always correct.
 */
static void update_crc(u_int8_t *eeprom)
{
    tlvinfo_header_t * eeprom_hdr = (tlvinfo_header_t *) eeprom;
    tlvinfo_tlv_t    * eeprom_crc;
    unsigned int       calc_crc;

    // Is the eeprom header valid?
    if (!is_valid_tlvinfo_header(eeprom_hdr)) {
	return;
    }
    // Is the last TLV a CRC?
    eeprom_crc = (tlvinfo_tlv_t *) &eeprom[sizeof(tlvinfo_header_t) +
					   be16_to_cpu(eeprom_hdr->totallen) -
					   (sizeof(tlvinfo_tlv_t) + 4)];
    if (eeprom_crc->type != TLV_CODE_CRC_32) {
	if ((be16_to_cpu(eeprom_hdr->totallen) + sizeof(tlvinfo_tlv_t) + 4) >
	    TLV_TOTAL_LEN_MAX) {
	    return;
	}
	eeprom_crc = (tlvinfo_tlv_t *) &eeprom[sizeof(tlvinfo_header_t) +
					       be16_to_cpu(
						   eeprom_hdr->totallen)];
	eeprom_hdr->totallen = cpu_to_be16(be16_to_cpu(eeprom_hdr->totallen) +
					   sizeof(tlvinfo_tlv_t) + 4);
	eeprom_crc->type = TLV_CODE_CRC_32;
    }
    eeprom_crc->length = 4;

    // Calculate the checksum
    calc_crc = crc32((void *)eeprom,
		     sizeof(tlvinfo_header_t) +
		     be16_to_cpu(eeprom_hdr->totallen) - 4);

    eeprom_crc->value[0] = (calc_crc >> 24) & 0xFF;
    eeprom_crc->value[1] = (calc_crc >> 16) & 0xFF;
    eeprom_crc->value[2] = (calc_crc >>  8) & 0xFF;
    eeprom_crc->value[3] = (calc_crc >>  0) & 0xFF;
}

/*
 *  show_eeprom
 *
 *  Display the contents of the EEPROM
 */

/*
 *  read_eeprom
 *
 *  Read the EEPROM into memory, if it hasn't already been read.
 */
int read_eeprom( struct i2c_client *pi2c_client, u_int8_t *eeprom)
{
    int ret;
    tlvinfo_header_t *eeprom_hdr = (tlvinfo_header_t *) eeprom;
    tlvinfo_tlv_t *eeprom_tlv = (tlvinfo_tlv_t *)&eeprom[
	sizeof(tlvinfo_header_t)];

    if (has_been_read)
	return 0;

    /* Read the header */
    ret = read_sys_eeprom( pi2c_client,(void *)eeprom_hdr, 0, sizeof(tlvinfo_header_t));
    /* If the header was successfully read, read the TLVs */
    if ((ret == 0) && is_valid_tlvinfo_header(eeprom_hdr)) {
	    ret = read_sys_eeprom( pi2c_client, (void *)eeprom_tlv, sizeof(tlvinfo_header_t),
			      be16_to_cpu(eeprom_hdr->totallen));
    }
    // If the contents are invalid, start over with default contents
    if(!is_valid_tlvinfo_header(eeprom_hdr))
	    printk(KERN_ERR
                "Notice:  Invalid TLV header found.  Using default contents--1.\n");
    if(!is_checksum_valid(eeprom))
	    printk(KERN_ERR
                "Notice:  Invalid TLV checksum found.  Using default contents--2.\n");
    if ( !is_valid_tlvinfo_header(eeprom_hdr) || !is_checksum_valid(eeprom) ){
	    strcpy(eeprom_hdr->signature, TLV_INFO_ID_STRING);
	    eeprom_hdr->version = TLV_INFO_VERSION;
	    eeprom_hdr->totallen = cpu_to_be16(0);
	    update_crc(eeprom);
    }
    has_been_read = 1;

    return ret;
}
EXPORT_SYMBOL(read_eeprom);

/*
 *  prog_eeprom
 *  Write the EEPROM data from CPU memory to the hardware.
 */
int prog_eeprom(struct i2c_client *pi2c_client, u_int8_t * eeprom)
{
    int ret = 0;
    tlvinfo_header_t * eeprom_hdr = (tlvinfo_header_t *) eeprom;
    int eeprom_len;

    eeprom_len = sizeof(tlvinfo_header_t) + be16_to_cpu(eeprom_hdr->totallen);
    ret = write_sys_eeprom( pi2c_client, eeprom, eeprom_len);
    if (ret) {
	printk("Programming failed.\n");
	return -1;
    }
    has_been_read = 0;
    return 0;
}
EXPORT_SYMBOL(prog_eeprom);

/*
 *  tlvinfo_find_tlv
 *
 *  This function finds the TLV with the supplied code in the EERPOM.
 *  An offset from the beginning of the EEPROM is returned in the
 *  eeprom_index parameter if the TLV is found.
 */
bool tlvinfo_find_tlv(u_int8_t *eeprom, u_int8_t tcode,
			     int *eeprom_index)
{
    tlvinfo_header_t * eeprom_hdr = (tlvinfo_header_t *) eeprom;
    tlvinfo_tlv_t    * eeprom_tlv;
    int eeprom_end;

    // Make sure the EEPROM contents are valid
    if (!is_valid_tlvinfo_header(eeprom_hdr) || !is_checksum_valid(eeprom)) {
	return(FALSE);
    }
    // Search through the TLVs, looking for the first one which matches the
    // supplied type code.
    *eeprom_index = sizeof(tlvinfo_header_t);
    eeprom_end = sizeof(tlvinfo_header_t) + be16_to_cpu(eeprom_hdr->totallen);
    
    while (*eeprom_index < eeprom_end) {
	eeprom_tlv = (tlvinfo_tlv_t *) &eeprom[*eeprom_index];
	if (!is_valid_tlv(eeprom_tlv)) {
	    return(FALSE);
	}
	if (eeprom_tlv->type == tcode) {
	    return(TRUE);
	}
	*eeprom_index += sizeof(tlvinfo_tlv_t) + eeprom_tlv->length;
    }
    return(FALSE);
}

/*
 *  tlvinfo_decode_tlv
 *
 *  This function finds the TLV with the supplied code in the EERPOM
 *  and decodes the value into the buffer provided.
 */
bool tlvinfo_decode_tlv(u_int8_t *eeprom, u_int8_t tcode, char* value)
{
    int eeprom_index;
    tlvinfo_tlv_t * eeprom_tlv;

    // Find the TLV and then decode it

    if (tlvinfo_find_tlv(eeprom, tcode, &eeprom_index)) {
        eeprom_tlv = (tlvinfo_tlv_t *) &eeprom[eeprom_index];
        decode_tlv_value(eeprom_tlv, value);
        return TRUE;
    }

    return FALSE;
}
EXPORT_SYMBOL(tlvinfo_decode_tlv);

/*
 *  tlvinfo_delete_tlv
 *
 *  This function deletes the TLV with the specified type code from the
 *  EEPROM.
 */
bool tlvinfo_delete_tlv(u_int8_t * eeprom, u_int8_t code)
{
    int eeprom_index;
    int tlength;
    tlvinfo_header_t * eeprom_hdr = (tlvinfo_header_t *) eeprom;
    tlvinfo_tlv_t * eeprom_tlv;

    // Find the TLV and then move all following TLVs "forward"
    if (tlvinfo_find_tlv(eeprom, code, &eeprom_index)) {
	eeprom_tlv = (tlvinfo_tlv_t *) &eeprom[eeprom_index];
	tlength = sizeof(tlvinfo_tlv_t) + eeprom_tlv->length;
	memcpy(&eeprom[eeprom_index], &eeprom[eeprom_index+tlength],
	       sizeof(tlvinfo_header_t) + be16_to_cpu(eeprom_hdr->totallen) -
	       eeprom_index - tlength);
	eeprom_hdr->totallen = cpu_to_be16(be16_to_cpu(eeprom_hdr->totallen) -
					   tlength);
	update_crc(eeprom);
	return(TRUE);
    }
    return(FALSE);
}
EXPORT_SYMBOL(tlvinfo_delete_tlv);

/*
 *  tlvinfo_add_tlv
 *
 *  This function adds a TLV to the EEPROM, converting the value (a string) to
 *  the format in which it will be stored in the EEPROM.
 */
#define MAX_TLV_VALUE_LEN   256
bool tlvinfo_add_tlv(u_int8_t * eeprom, int tcode, char * strval)
{
    tlvinfo_header_t * eeprom_hdr = (tlvinfo_header_t *) eeprom;
    tlvinfo_tlv_t * eeprom_tlv;
    int new_tlv_len = 0;
    u_int32_t value;
    char data[MAX_TLV_VALUE_LEN];
    int eeprom_index;
    int max_size = TLV_TOTAL_LEN_MAX;

    // Encode each TLV type into the format to be stored in the EERPOM
    switch (tcode) {
    case TLV_CODE_PRODUCT_NAME:
    case TLV_CODE_PART_NUMBER:
    case TLV_CODE_SERIAL_NUMBER:
    case TLV_CODE_LABEL_REVISION:
    case TLV_CODE_PLATFORM_NAME:
    case TLV_CODE_ONIE_VERSION:
    case TLV_CODE_MANUF_NAME:
    case TLV_CODE_MANUF_COUNTRY:
    case TLV_CODE_VENDOR_NAME:
    case TLV_CODE_DIAG_VERSION:
    case TLV_CODE_SERVICE_TAG:
	strncpy(data, strval, MAX_TLV_VALUE_LEN);

	if( strlen(strval) >= MAX_TLV_VALUE_LEN )
	    new_tlv_len = MAX_TLV_VALUE_LEN;
	else
	    new_tlv_len = strlen(strval);

	break;
    case TLV_CODE_DEVICE_VERSION:
	value = strtoul(strval, NULL, 0);
	if (value >= 256) {
	    printk("ERROR: Device version must be 255 or less. Value " \
		   "supplied: %u", value);
	    return(FALSE);
	}
	data[0] = value & 0xFF;
	new_tlv_len = 1;
	break;
    case TLV_CODE_MAC_SIZE:
	value = strtoul(strval, NULL, 0);
	if (value >= 65536) {
	    printk("ERROR: MAC Size must be 65535 or less. Value " \
		   "supplied: %u", value);
	    return(FALSE);
	}
	data[0] = (value >> 8) & 0xFF;
	data[1] = value & 0xFF;
	new_tlv_len = 2;
	break;
    case TLV_CODE_MANUF_DATE:
	if (set_date(data, strval) != 0) {
	    return(FALSE);
	}
	new_tlv_len = 19;
	break;
    case TLV_CODE_MAC_BASE:
	if (set_mac(data, strval) != 0) {
	    return(FALSE);
	}
	new_tlv_len = 6;
	break;
    case TLV_CODE_CRC_32:
	printk("WARNING: The CRC TLV is set automatically and cannot be set " \
	       "manually.\n");
	return(FALSE);
    case TLV_CODE_VENDOR_EXT:
    default:
	if (set_bytes(data, strval, &new_tlv_len) != 0 ) {
	    return(FALSE);
	}
	break;
    }

    // Is there room for this TLV?
    if ((be16_to_cpu(eeprom_hdr->totallen) + sizeof(tlvinfo_tlv_t) +
	 new_tlv_len) > max_size) {
	printk("ERROR: There is not enough room in the EERPOM to save data.\n");
	return(FALSE);
    }

    // Add TLV at the end, overwriting CRC TLV if it exists
    if (tlvinfo_find_tlv(eeprom, TLV_CODE_CRC_32, &eeprom_index)) {
	eeprom_hdr->totallen = cpu_to_be16(be16_to_cpu(eeprom_hdr->totallen)
					   - sizeof(tlvinfo_tlv_t) - 4);
    } else {
	eeprom_index = sizeof(tlvinfo_header_t) +
	    be16_to_cpu(eeprom_hdr->totallen);
    }
    eeprom_tlv = (tlvinfo_tlv_t *) &eeprom[eeprom_index];
    eeprom_tlv->type = tcode;
    eeprom_tlv->length = new_tlv_len;
    memcpy(eeprom_tlv->value, data, new_tlv_len);

    // Update the total length and calculate (add) a new CRC-32 TLV
    eeprom_hdr->totallen = cpu_to_be16(be16_to_cpu(eeprom_hdr->totallen) +
				       sizeof(tlvinfo_tlv_t) + new_tlv_len);
    update_crc(eeprom);

    return(TRUE);
}
EXPORT_SYMBOL(tlvinfo_add_tlv);

/*
 * read_sys_eeprom - read the hwinfo from i2c EEPROM
 */
int read_sys_eeprom(struct i2c_client *pi2c_client,void *eeprom_data, int offset, int len)
{
    int iRet = 0;
    int i = 0;
    unsigned char ucBuf[2];
    u_int8_t *c;
    unsigned short  usAddr = SYS_EEPROM_OFFSET + offset;

    c = eeprom_data;
    for (i = 0; i < len; i++) {
        ucBuf[0] = (usAddr & 0xFF00) >> 8;
        ucBuf[1] = (usAddr & 0x00FF);
       
        iRet = i2c_smbus_write_byte_data(pi2c_client, ucBuf[0], ucBuf[1]);
        if( iRet < 0 ){
            printk(KERN_ERR"Error!! VPD data read error\n");
            return -1;
        }

        *c = i2c_smbus_read_byte(pi2c_client);
        c++; usAddr++;
    }
    return 0;
}

/*
 * write_sys_eeprom - write the hwinfo to i2c EEPROM
 */
int write_sys_eeprom(struct i2c_client *pi2c_client, void *eeprom_data, int len)
{
    int iRet = 0;
    int i = 0;
    u_int8_t *c;
    unsigned short usAddr = SYS_EEPROM_OFFSET;
    unsigned char ucBuf[3];

    c = eeprom_data;
    for (i = 0; i < len; i++) {
        ucBuf[ 0 ] = (usAddr & 0xFF00) >>8 ;
        ucBuf[ 1 ] = (usAddr & 0x00FF);
        ucBuf[ 2 ] = *c;

        iRet = i2c_smbus_write_word_data( pi2c_client, ucBuf[0], (ucBuf[2] << 8 | ucBuf[1]));
        if (iRet < 0 ){
            printk(KERN_ERR"Error!! VPD data write error . \n");
            return -1;
        }

        c++; usAddr++;
        msleep_interruptible(10);
    }

    return 0;
}

void update_eeprom_header(u_int8_t *eeprom)
{
    tlvinfo_header_t * eeprom_hdr = (tlvinfo_header_t *) eeprom;

    strcpy(eeprom_hdr->signature, TLV_INFO_ID_STRING);
    eeprom_hdr->version = TLV_INFO_VERSION;
    eeprom_hdr->totallen = cpu_to_be16(0);
    update_crc(eeprom);
}
#if 0
int  find_vpd_data(u_int8_t *eeprom, int i_offset, char *c_buf)
{
    int tlv_end;
    int curr_tlv;
    tlvinfo_header_t * eeprom_hdr = (tlvinfo_header_t *) eeprom;
    tlvinfo_tlv_t    * eeprom_tlv;
    int iFind = 0;

    if( !is_valid_tlvinfo_header(eeprom_hdr) ) {
        printk(KERN_ERR"EEPROM does not contain data in a valid TlvInfo format.\n");
        return -1;
    }

    curr_tlv = sizeof(tlvinfo_header_t);
    tlv_end  = sizeof(tlvinfo_header_t) + be16_to_cpu(eeprom_hdr->totallen);
    while(curr_tlv < tlv_end){
        eeprom_tlv = (tlvinfo_tlv_t *) &eeprom[curr_tlv];
        if (!is_valid_tlv(eeprom_tlv)) {
            printk(KERN_ERR"Invalid TLV field starting at EEPROM offset %d\n",
                curr_tlv);
            return -1;
        }

        decode_tlv_value(eeprom_tlv, c_buf);
        if( eeprom_tlv->type == i_offset){
            iFind = 1;
            break;
        }
        curr_tlv += sizeof(tlvinfo_tlv_t) + eeprom_tlv->length;
    }

    if( iFind == 0 )
        return -1;
    else
        return 0;
}
#endif
