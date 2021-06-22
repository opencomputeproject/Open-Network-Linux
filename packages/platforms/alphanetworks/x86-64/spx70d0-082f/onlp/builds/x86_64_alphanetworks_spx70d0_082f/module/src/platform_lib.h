/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2020 Alpha Networks Incorporation.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 * </bsn.cl>
 ************************************************************
 *
 *
 *
 ***********************************************************/
#ifndef __PLATFORM_LIB_H__
#define __PLATFORM_LIB_H__

#include "x86_64_alphanetworks_spx70d0_082f_log.h"

//for ne10032 
#define IDPROM_PATH "/sys/devices/pci0000:00/0000:00:1f.3/i2c-0/0-0056/eeprom"
//#define IDPROM_PATH "/sys/devices/pci0000:00/0000:00:1f.3/i2c-0/i2c-6/i2c-10/10-0051/eeprom"

#define PSU1_ID 1
#define PSU2_ID 2
#define PSUI_BUS_ID_OFFSET      1
#define PSU1_BUS_ID             (PSUI_BUS_ID_OFFSET + PSU1_ID)
#define PSU2_BUS_ID             (PSUI_BUS_ID_OFFSET + PSU2_ID)

#define CHASSIS_THERMAL_COUNT 2
#define CHASSIS_FAN_COUNT     5
#define CHASSIS_PSU_COUNT     1

enum onlp_bmc_i2c_id
{
    BMC_I2C1 = 0,
    BMC_I2C2,
    BMC_I2C3,
    BMC_I2C4,
    BMC_I2C5,
    BMC_I2C6,
};

#define IPMI_I2C_BUS0_ID                 0x01
#define IPMI_I2C_BUS1_ID                 0x03
#define IPMI_I2C_BUS2_ID                 0x05
#define IPMI_I2C_BUS3_ID                 0x07
#define IPMI_I2C_BUS4_ID                 0x09
#define IPMI_I2C_BUS5_ID                 0x0B

#define BMC_CPLD_I2C_BUS_ID            BMC_I2C5
#define CPLD_I2C_ADDR                   0x5F  /* Power CPLD Physical Address in the I2C */ 
/* IPMI Table 22-14 define Request Data byte 2 as I2C address. 
  * [7:1] -Slave address and [0] - reserved.
  * So Power CPLD Physical Address in the I2C 0x5F( 0101 1111) should transfer to 0xBE (1011 1110).
  */
#define BMC_CPLD_I2C_ADDR                   0xBE

#define PCA9548_1_ADDR                      0x70  /* PSU EEPROM Physical Address in the I2C */ 
/* IPMI Table 22-14 define Request Data byte 2 as I2C address. 
  * [7:1] -Slave address and [0] - reserved.
  * So Power CPLD Physical Address in the I2C 0x70( 0111 0000) should transfer to 0xE0 (1110 0000).
  */
#define BMC_PCA9548_1_ADDR                  0xE0

#define PSUD_EEPROM_ADDR                    0x58  /* PSU EEPROM Physical Address in the I2C */ 
/* IPMI Table 22-14 define Request Data byte 2 as I2C address. 
  * [7:1] -Slave address and [0] - reserved.
  * So Power CPLD Physical Address in the I2C 0x58( 0101 1000) should transfer to 0xB0 (1011 0000).
  */
#define BMC_PSUD_EEPROM_ADDR                0xB0
#define PSU1_AC_PMBUS_PREFIX "/sys/bus/i2c/devices/57-003c/"
#define PSU2_AC_PMBUS_PREFIX "/sys/bus/i2c/devices/58-003f/"

#define PSU1_AC_HWMON_PREFIX "/sys/bus/i2c/devices/2-0050/"
#define PSU1_DC_HWMON_PREFIX "/sys/bus/i2c/devices/57-0050/"
#define PSU2_AC_HWMON_PREFIX "/sys/bus/i2c/devices/3-0050/"
#define PSU2_DC_HWMON_PREFIX "/sys/bus/i2c/devices/58-0053/"

#define PSU1_AC_HWMON_NODE(node) PSU1_AC_HWMON_PREFIX#node
#define PSU1_DC_HWMON_NODE(node) PSU1_DC_HWMON_PREFIX#node
#define PSU2_AC_HWMON_NODE(node) PSU2_AC_HWMON_PREFIX#node
#define PSU2_DC_HWMON_NODE(node) PSU2_DC_HWMON_PREFIX#node


#define SFP_START_INDEX         0
#define NUM_OF_XFP_PORT         8  /* 8 10G XFP Port 1-8  */
#define NUM_OF_QSFP28_PORT      2  /* 2 100G QSFP28 Port (9-12), (13-16) */
#define NUM_OF_SFP_PORT (NUM_OF_XFP_PORT + NUM_OF_QSFP28_PORT)  

#define SFP_PLUS_EEPROM_I2C_ADDR        0x50  /* SFP+ EEPROM Physical Address in the I2C */  
#define SFP_DOM_EEPROM_I2C_ADDR         0x51

#define QSFP28_EEPROM_TXRX_LOS_OFFSET     3
#define QSFP28_EEPROM_TX_FAULT_OFFSET     4
#define QSFP28_EEPROM_TX_DISABLE_OFFSET   86
#define QSFP28_EEPROM_POWERSET_OFFSET     93
#define QSFP28_EEPROM_PAGE_SELECT_OFFSET  127

typedef long long               I64_T;      /* 64-bit signed   */
typedef unsigned long long      UI64_T;     /* 64-bit unsigned */
typedef long                    I32_T;      /* 32-bit signed   */
typedef unsigned long           UI32_T;     /* 32-bit unsigned */
typedef short int               I16_T;      /* 16-bit signed   */
typedef unsigned short int      UI16_T;     /* 16-bit unsigned */
typedef char                    I8_T;       /* 8-bit signed    */
typedef unsigned char           UI8_T;      /* 8-bit unsigned  */

/*----------------------------------------------------------*/
/*  BIT operation                                           */
/*----------------------------------------------------------*/
#define UTL_TEST_BITS(__type__,__var__,__pos__)                         \
        (__type__)((((__type__)(__var__)>>(__type__)(__pos__))&(__type__)1)?(__type__)1:(__type__)0)

#define UTL_LEFT_SHIFT_BITS(__type__,__range__,__var__,__sft_bits__)    \
        (((__type__)(__sft_bits__)>=(__range__))?0:((__type__)(__var__)<<(__type__)(__sft_bits__)))

#define UTL_RIGHT_SHIFT_BITS(__type__,__range__,__var__,__sft_bits__)   \
        (((__type__)__sft_bits__>=(__range__))?(__type__)(__var__):((__type__)(__var__)>>(__type__)(__sft_bits__)))

#define UTL_SET_BITS(__type__,__range__,__var__,__pos__)                \
        ((__type__)(__var__)|UTL_LEFT_SHIFT_BITS(__type__,__range__,(__type__)1U,(__type__)(__pos__)))

#define UTL_RESET_BITS(__type__,__range__,__var__,__pos__)              \
        ((__type__)(__var__)&~UTL_LEFT_SHIFT_BITS(__type__,__range__,(__type__)1U,(__type__)(__pos__)))

/*----------------------------------------------------------*/
/*  64 BIT operation                                        */
/*----------------------------------------------------------*/
#define UTL_TEST_BITS64(__var__,__pos__)     UTL_TEST_BITS(UI64_T,__var__,__pos__)

#define UTL_SET_BITS64(__var__,__pos__)      (__var__) = UTL_SET_BITS(UI64_T,64,__var__,__pos__)
#define UTL_RESET_BITS64(__var__,__pos__)    (__var__) = UTL_RESET_BITS(UI64_T,64,__var__,__pos__)

/*----------------------------------------------------------*/
/*  32 BIT operation                                        */
/*----------------------------------------------------------*/
/*   Usage: if( UTL_TEST_BITS32(val,2) )*/
/*   pos = 0~7, 0~15, 0~31              */
/*   0x1234 = 0001 0010 0011 0100       */
/*   UTL_TEST_BITS32( 0x1234, 2 ) ==> 1 */
/*   UTL_TEST_BITS32( 0x1234, 1 ) ==> 0 */
#define UTL_TEST_BITS32(__var__,__pos__)     UTL_TEST_BITS(UI32_T,__var__,__pos__)


/*   Usage: UTL_SET_BITS32( val, 6 )                    */
/*   pos = 0~7, 0~15, 0~31                                  */
/*   val = 0x0100 =               0000 0001 0000 0000       */
/*   UTL_SET_BITS32( val,6 )==>   0000 0001 0100 0000   */
/*   UTL_RESET_BITS32( val,8 )=>  0000 0000 0000 0000   */
#define UTL_SET_BITS32(__var__,__pos__)      (__var__) = UTL_SET_BITS(UI32_T,32,__var__,__pos__)
#define UTL_RESET_BITS32(__var__,__pos__)    (__var__) = UTL_RESET_BITS(UI32_T,32,__var__,__pos__)

/*----------------------------------------------------------*/
/*  16 BIT operation                                        */
/*----------------------------------------------------------*/
#define UTL_TEST_BITS16(__var__,__pos__)     UTL_TEST_BITS(UI16_T,__var__,__pos__)

#define UTL_SET_BITS16(__var__,__pos__)      (__var__) = UTL_SET_BITS(UI16_T,16,__var__,__pos__)
#define UTL_RESET_BITS16(__var__,__pos__)    (__var__) = UTL_RESET_BITS(UI16_T,16,__var__,__pos__)

/*----------------------------------------------------------*/
/*  8 BIT operation                                         */
/*----------------------------------------------------------*/
#define UTL_TEST_BITS8(__var__,__pos__)     UTL_TEST_BITS(UI8_T,__var__,__pos__)

#define UTL_SET_BITS8(__var__,__pos__)      (__var__) = UTL_SET_BITS(UI8_T,8,__var__,__pos__)
#define UTL_RESET_BITS8(__var__,__pos__)    (__var__) = UTL_RESET_BITS(UI8_T,16,__var__,__pos__)


int deviceNodeWriteInt(char *filename, int value, int data_len);
int deviceNodeReadBinary(char *filename, char *buffer, int buf_size, int data_len);
int deviceNodeReadString(char *filename, char *buffer, int buf_size, int data_len);

typedef enum psu_type {
    PSU_TYPE_UNKNOWN,
    PSU_TYPE_AC_F2B,
    PSU_TYPE_AC_B2F,
    PSU_TYPE_DC_48V_F2B,
    PSU_TYPE_DC_48V_B2F
} psu_type_t;

//psu_type_t get_psu_type(int id, char* modelname, int modelname_len);
int psu_two_complement_to_int(uint16_t data, uint8_t valid_bit, int mask);

/* FAN related data
 */
enum onlp_fan_id
{
    FAN_RESERVED = 0,
    FAN_1,
    FAN_2,
    FAN_3,
    FAN_4,
    FAN_5
};

/* 
 * LED ID (need to sync with "enum onlp_led_id" defined in ledi.c)
 */
enum onlp_led_id
{
    LED_RESERVED = 0,
    LED_POWER,
    LED_PSU,
    LED_SYSTEM,
    LED_FAN,
    LED_LOC
};

/*
* TLV type code
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
i2c APIs: access i2c device by ioctl 
*/ 
#include <errno.h>
#include <linux/i2c-dev.h>

int i2c_sequential_read(int i2cbus, int addr, int offset, int length, char* data);

int bmc_i2c_read_byte(int bus, int devaddr, int offset, char* data);
int bmc_i2c_write_byte(int bus, int devaddr, int offset, char value);
int bmc_i2c_pmb_cmd_read_byte(int bus, int devaddr, int pmb_cmd, int offset, char* data);


#define DIAG_FLAG_ON 1
#define DIAG_FLAG_OFF 0
char diag_flag_set(char d);
char diag_flag_get(void);

char diag_debug_trace_on(void);
char diag_debug_trace_off(void);
char diag_debug_trace_check(void);

char diag_debug_pause_platform_manage_on(void);
char diag_debug_pause_platform_manage_off(void);
char diag_debug_pause_platform_manage_check(void);
/*
* TLV parsering for specific type code
*/
int eeprom_tlv_read(uint8_t *rdata, char type, char *data);


#define DIAG_TRACE(fmt,args...) if(diag_debug_trace_check()) printf("\n[TRACE]"fmt"\n", args)
#define DIAG_PRINT(fmt,args...) DIAG_TRACE(fmt,args);else if(diag_flag_get()) printf("[DIAG]"fmt"\n", args) 

char* sfp_control_to_str(int value);

int device_hw_version_get(void);

void dump_stack(void);

#endif  /* __PLATFORM_LIB_H__ */
