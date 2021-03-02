/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2013 Accton Technology Corporation.
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
 * Platform Library
 *
 ***********************************************************/
#ifndef __PLATFORM_LIB_H__
#define __PLATFORM_LIB_H__

#include <onlp/fan.h>
#include <onlp/psu.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/sysi.h>
#include "x86_64_ufispace_s9700_23d_int.h"
#include "x86_64_ufispace_s9700_23d_log.h"

#include <x86_64_ufispace_s9700_23d/x86_64_ufispace_s9700_23d_config.h>
#define SYS_DEV                     "/sys/bus/i2c/devices/"
#define SYS_CPU_CORETEMP_PREFIX         "/sys/devices/platform/coretemp.0/hwmon/hwmon0/"
#define SYS_CPU_CORETEMP_PREFIX2      "/sys/devices/platform/coretemp.0/"
#define SYS_CORE_TEMP_PREFIX        "/sys/class/hwmon/hwmon2/"
#define SYS_CPU_BOARD_TEMP_PREFIX   "/sys/bus/i2c/devices/0-004f/hwmon/hwmon1/"
#define SYS_CPU_BOARD_TEMP_PREFIX2   "/sys/bus/i2c/devices/0-004f/"

#define SYS_FAN_PREFIX              "/sys/class/hwmon/hwmon1/device/"
#define SYS_EEPROM_PATH             "/sys/bus/i2c/devices/0-0057/eeprom"
#define SYS_EEPROM_SIZE 512
#define PSU1_EEPROM_PATH            "/sys/bus/i2c/devices/58-0050/eeprom"
#define PSU2_EEPROM_PATH            "/sys/bus/i2c/devices/57-0050/eeprom"
#define BMC_EN_FILE_PATH            "/etc/onl/bmc_en"
#define BMC_SENSOR_CACHE            "/tmp/bmc_sensor_cache"
#define MB_CPLD1_ID_PATH            "/sys/bus/i2c/devices/1-0030/cpld_id"
#define CPU_MUX_RESET_PATH          "/sys/devices/platform/x86_64_ufispace_s9700_23d_lpc/cpu_cpld/mux_reset"
#define CMD_BIOS_VER                "dmidecode -s bios-version | tail -1 | tr -d '\r\n'"
#define CMD_BMC_VER_1               "expr `ipmitool mc info | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f1` + 0"
#define CMD_BMC_VER_2               "expr `ipmitool mc info | grep 'Firmware Revision' | cut -d':' -f2 | cut -d'.' -f2` + 0"
#define CMD_BMC_VER_3               "echo $((`ipmitool mc info | grep 'Aux Firmware Rev Info' -A 2 | sed -n '2p'`))"
#define CMD_BMC_SENSOR_CACHE        "ipmitool sdr -c get TEMP_CPU_PECI TEMP_OP2_ENV TEMP_J2_ENV_1 TEMP_J2_DIE_1 TEMP_J2_ENV_2 TEMP_J2_DIE_2 PSU0_TEMP PSU1_TEMP FAN0_RPM FAN1_RPM FAN2_RPM FAN3_RPM PSU0_FAN1 PSU0_FAN2 PSU1_FAN1 PSU1_FAN2 FAN0_PRSNT_H FAN1_PRSNT_H FAN2_PRSNT_H FAN3_PRSNT_H PSU0_VIN PSU0_VOUT PSU0_IIN PSU0_IOUT PSU0_STBVOUT PSU0_STBIOUT PSU1_VIN PSU1_VOUT PSU1_IIN PSU1_IOUT PSU1_STBVOUT PSU1_STBIOUT > /tmp/bmc_sensor_cache"
#define CMD_UCD_VER                 "ipmitool raw 0x3c 0x08"
#define CMD_BMC_SDR_GET             "ipmitool sdr -c get %s"
#define CMD_FRU_INFO_GET            "ipmitool fru print %d | grep '%s' | cut -d':' -f2 | awk '{$1=$1};1' | tr -d '\n'"
#define CMD_BMC_CACHE_GET           "cat "BMC_SENSOR_CACHE" | grep %s | awk -F',' '{print $%d}'"
#define PSU_STATUS_PRESENT          1
#define PSU_STATUS_POWER_GOOD       1
#define FAN_PRESENT                 0
#define FAN_CTRL_SET1               1
#define FAN_CTRL_SET2               2
#define MAX_SYS_FAN_NUM             8
#define BOARD_THERMAL_NUM           6
#define SYS_FAN_NUM                 8
#define QSFP_NUM                    40
#define QSFPDD_NIF_NUM              10
#define QSFPDD_FAB_NUM              13
#define SFP_NUM                     2
#define PORT_NUM                    25

#define THERMAL_NUM                 21
#define LED_NUM                     4
#define FAN_NUM                     8



#define THERMAL_SHUTDOWN_DEFAULT    105000

#define THERMAL_ERROR_DEFAULT       95000
#define THERMAL_ERROR_FAN_PERC      100

#define THERMAL_WARNING_DEFAULT     77000
#define THERMAL_WARNING_FAN_PERC    80

#define THERMAL_NORMAL_DEFAULT      72000
#define THERMAL_NORMAL_FAN_PERC     50
    
/* I2C bus */
#define I2C_BUS_0               0
#define I2C_BUS_1               1
#define I2C_BUS_2               2
#define I2C_BUS_3               3
#define I2C_BUS_4               4
#define I2C_BUS_5               5
#define I2C_BUS_6               6
#define I2C_BUS_7               7
#define I2C_BUS_8               8  
#define I2C_BUS_9               9  
#define I2C_BUS_44              44      /* cpld */
#define I2C_BUS_50              50      /* SYS LED */
#define I2C_BUS_57              (57)      /* PSU2 */
#define I2C_BUS_58              (58)      /* PSU1 */
#define I2C_BUS_59              59      /* FRU */

#define I2C_BUS_PSU1            I2C_BUS_58      /* PSU1 */
#define I2C_BUS_PSU2            I2C_BUS_57      /* PSU2 */
    
/* PSU */
#define PSU_MUX_MASK            0x01

#define PSU_THERMAL1_OFFSET     0x8D
#define PSU_THERMAL2_OFFSET     0x8E
#define PSU_THERMAL_REG         0x58
#define PSU_FAN_RPM_REG         0x58
#define PSU_FAN_RPM_OFFSET      0x90
#define PSU_REG                 0x58
#define PSU_VOUT_OFFSET1        0x20
#define PSU_VOUT_OFFSET2        0x8B
#define PSU_IOUT_OFFSET         0x8C
#define PSU_POUT_OFFSET         0x96
#define PSU_PIN_OFFSET          0x97

#define PSU_STATE_REG           0x25
#define PSU1_PRESENT_MASK       0x40
#define PSU2_PRESENT_MASK       0x80
#define PSU1_PWGOOD_MASK        0x10
#define PSU2_PWGOOD_MASK        0x20

/* LED */
#define LED_REG                 0x75
#define LED_OFFSET              0x02
#define LED_PWOK_OFFSET         0x03

#define LED_SYS_AND_MASK        0xFE
#define LED_SYS_GMASK           0x01
#define LED_SYS_YMASK           0x00

#define LED_FAN_AND_MASK        0xF9
#define LED_FAN_GMASK           0x02
#define LED_FAN_YMASK           0x06

#define LED_PSU2_AND_MASK       0xEF
#define LED_PSU2_GMASK          0x00
#define LED_PSU2_YMASK          0x10
#define LED_PSU2_ON_AND_MASK    0xFD
#define LED_PSU2_ON_OR_MASK     0x02
#define LED_PSU2_OFF_AND_MASK   0xFD
#define LED_PSU2_OFF_OR_MASK    0x00
#define LED_PSU1_AND_MASK       0xF7
#define LED_PSU1_GMASK          0x00
#define LED_PSU1_YMASK          0x08
#define LED_PSU1_ON_AND_MASK    0xFE
#define LED_PSU1_ON_OR_MASK     0x01
#define LED_PSU1_OFF_AND_MASK   0xFE
#define LED_PSU1_OFF_OR_MASK    0x00
#define LED_SYS_ON_MASK         0x00
#define LED_SYS_OFF_MASK        0x33

/* SYS */
#define CPLD_MAX                3
//#define CPLD_BASE_ADDR          0x30
#define CPLD_REG_VER            0x02
extern const int CPLD_BASE_ADDR[CPLD_MAX];

/* QSFP */
#define QSFP_PRES_REG1          0x20
#define QSFP_PRES_REG2          0x21
#define QSFP_PRES_OFFSET1       0x00
#define QSFP_PRES_OFFSET2       0x01

/* FAN */
#define FAN_GPIO_ADDR           0x20
#define FAN_1_2_PRESENT_MASK    0x40
#define FAN_3_4_PRESENT_MASK    0x04
#define FAN_5_6_PRESENT_MASK    0x40
#define FAN_7_8_PRESENT_MASK    0x04

/* BMC CMD */
#define BMC_CACHE_EN            1
#define BMC_CACHE_CYCLE         30
#define BMC_CMD_SDR_SIZE        48
#define BMC_TOKEN_SIZE          20

#define FAN_CACHE_TIME          5
#define PSU_CACHE_TIME          5
#define THERMAL_CACHE_TIME      10

enum sensor
{
    FAN_SENSOR = 0,
    PSU_SENSOR,
    THERMAL_SENSOR,
};

typedef struct bmc_info_s
{
    char name[20];
    float data;
}bmc_info_t;

/** led_oid */
typedef enum led_oid_e {
    LED_OID_SYSTEM = ONLP_LED_ID_CREATE(1),    
    LED_OID_PSU0 = ONLP_LED_ID_CREATE(2),
    LED_OID_PSU1 = ONLP_LED_ID_CREATE(3),
    LED_OID_FAN = ONLP_LED_ID_CREATE(4),
    LED_OID_FAN_TRAY1 = ONLP_LED_ID_CREATE(5),
    LED_OID_FAN_TRAY2 = ONLP_LED_ID_CREATE(6),
    LED_OID_FAN_TRAY3 = ONLP_LED_ID_CREATE(7),
    LED_OID_FAN_TRAY4 = ONLP_LED_ID_CREATE(8),
} led_oid_t;

/** led_id */
typedef enum led_id_e {
    LED_ID_SYS_SYS = 1,    
    LED_ID_SYS_PSU0 = 2,
    LED_ID_SYS_PSU1 = 3,
    LED_ID_SYS_FAN = 4,
    LED_ID_FAN_TRAY1 = 5,
    LED_ID_FAN_TRAY2 = 6,
    LED_ID_FAN_TRAY3 = 7,
    LED_ID_FAN_TRAY4 = 8,
} led_id_t;

/** Thermal_oid */
typedef enum thermal_oid_e {
    THERMAL_OID_CPU_PECI = ONLP_THERMAL_ID_CREATE(1),
    THERMAL_OID_OP2_ENV = ONLP_THERMAL_ID_CREATE(2),    
    THERMAL_OID_J2_ENV_1 = ONLP_THERMAL_ID_CREATE(3),
    THERMAL_OID_J2_DIE_1 = ONLP_THERMAL_ID_CREATE(4),
    THERMAL_OID_J2_ENV_2 = ONLP_THERMAL_ID_CREATE(5),
    THERMAL_OID_J2_DIE_2 = ONLP_THERMAL_ID_CREATE(6),
    THERMAL_OID_PSU0 = ONLP_THERMAL_ID_CREATE(7),
    THERMAL_OID_PSU1 = ONLP_THERMAL_ID_CREATE(8),    
    THERMAL_OID_CPU_PKG = ONLP_THERMAL_ID_CREATE(9),
    THERMAL_OID_CPU1 = ONLP_THERMAL_ID_CREATE(10),
    THERMAL_OID_CPU2 = ONLP_THERMAL_ID_CREATE(11),
    THERMAL_OID_CPU3 = ONLP_THERMAL_ID_CREATE(12),
    THERMAL_OID_CPU4 = ONLP_THERMAL_ID_CREATE(13), 
    THERMAL_OID_CPU5 = ONLP_THERMAL_ID_CREATE(14), 
    THERMAL_OID_CPU6 = ONLP_THERMAL_ID_CREATE(15), 
    THERMAL_OID_CPU7 = ONLP_THERMAL_ID_CREATE(16), 
    THERMAL_OID_CPU8 = ONLP_THERMAL_ID_CREATE(17), 
    THERMAL_OID_CPU_BOARD = ONLP_THERMAL_ID_CREATE(18),
    THERMAL_OID_BMC_ENV = ONLP_THERMAL_ID_CREATE(19),
    THERMAL_OID_ENV = ONLP_THERMAL_ID_CREATE(20),
    THERMAL_OID_ENV_FRONT = ONLP_THERMAL_ID_CREATE(21),
} thermal_oid_t;

/** thermal_id */
typedef enum thermal_id_e {
    THERMAL_ID_CPU_PECI = 1,    
    THERMAL_ID_OP2_ENV = 2,
    THERMAL_ID_J2_ENV_1 = 3, 
    THERMAL_ID_J2_DIE_1 = 4,
    THERMAL_ID_J2_ENV_2 = 5,      
    THERMAL_ID_J2_DIE_2 = 6,
    THERMAL_ID_PSU0 = 7, 
    THERMAL_ID_PSU1 = 8,
    THERMAL_ID_CPU_PKG = 9,
    THERMAL_ID_CPU1 = 10,
    THERMAL_ID_CPU2 = 11,
    THERMAL_ID_CPU3 = 12,
    THERMAL_ID_CPU4 = 13,
    THERMAL_ID_CPU5 = 14,
    THERMAL_ID_CPU6 = 15,
    THERMAL_ID_CPU7 = 16,
    THERMAL_ID_CPU8 = 17,
    THERMAL_ID_CPU_BOARD = 18,  
    THERMAL_ID_BMC_ENV = 19,
    THERMAL_ID_ENV = 20,
    THERMAL_ID_ENV_FRONT = 21,
    THERMAL_ID_MAX = 22,
} thermal_id_t;

/* Shortcut for CPU thermal threshold value. */
#define THERMAL_THRESHOLD_INIT_DEFAULTS  \
    { THERMAL_WARNING_DEFAULT, \
      THERMAL_ERROR_DEFAULT,   \
      THERMAL_SHUTDOWN_DEFAULT }
      
/** Fan_oid */
typedef enum fan_oid_e {
    FAN_OID_FAN1 = ONLP_FAN_ID_CREATE(1),
    FAN_OID_FAN2 = ONLP_FAN_ID_CREATE(2),
    FAN_OID_FAN3 = ONLP_FAN_ID_CREATE(3),
    FAN_OID_FAN4 = ONLP_FAN_ID_CREATE(4),    
    FAN_OID_PSU0_FAN1 = ONLP_FAN_ID_CREATE(5),
    FAN_OID_PSU0_FAN2 = ONLP_FAN_ID_CREATE(6),
    FAN_OID_PSU1_FAN1 = ONLP_FAN_ID_CREATE(7),
    FAN_OID_PSU1_FAN2 = ONLP_FAN_ID_CREATE(8),
} fan_oid_t;

/** fan_id */
typedef enum fan_id_e {
    FAN_ID_FAN1 = 1,
    FAN_ID_FAN2 = 2,
    FAN_ID_FAN3 = 3,
    FAN_ID_FAN4 = 4,
    FAN_ID_PSU0_FAN1 = 5,
    FAN_ID_PSU0_FAN2 = 6,
    FAN_ID_PSU1_FAN1 = 7,
    FAN_ID_PSU1_FAN2 = 8,
    FAN_ID_MAX = 9,
} fan_id_t;

/** led_oid */
typedef enum psu_oid_e {
    PSU_OID_PSU0 = ONLP_PSU_ID_CREATE(1),
    PSU_OID_PSU1 = ONLP_PSU_ID_CREATE(2)
} psu_oid_t;

/** psu_id */
typedef enum psu_id_e {
    PSU_ID_PSU0 = 1,
    PSU_ID_PSU1 = 2,
    PSU_ID_PSU0_VIN = 3,
    PSU_ID_PSU0_VOUT = 4,
    PSU_ID_PSU0_IIN = 5,
    PSU_ID_PSU0_IOUT = 6,
    PSU_ID_PSU0_STBVOUT = 7,
    PSU_ID_PSU0_STBIOUT = 8,
    PSU_ID_PSU1_VIN = 9,
    PSU_ID_PSU1_VOUT = 10,
    PSU_ID_PSU1_IIN = 11,
    PSU_ID_PSU1_IOUT = 12,
    PSU_ID_PSU1_STBVOUT = 13,
    PSU_ID_PSU1_STBIOUT = 14,
    PSU_ID_MAX = 15,
} psu_id_t;

int psu_thermal_get(onlp_thermal_info_t* info, int id);

int psu_fan_info_get(onlp_fan_info_t* info, int id);

int psu_vin_get(onlp_psu_info_t* info, int id);

int psu_vout_get(onlp_psu_info_t* info, int id);

int psu_iin_get(onlp_psu_info_t* info, int id);

int psu_iout_get(onlp_psu_info_t* info, int id);

int psu_pout_get(onlp_psu_info_t* info, int id);

int psu_pin_get(onlp_psu_info_t* info, int id);

int psu_eeprom_get(onlp_psu_info_t* info, int id);

int psu_present_get(int *pw_present, int id);

int psu_pwgood_get(int *pw_good, int id);

int psu2_led_set(onlp_led_mode_t mode);

int psu1_led_set(onlp_led_mode_t mode);

int fan_led_set(onlp_led_mode_t mode);

int system_led_set(onlp_led_mode_t mode);

int fan_tray_led_set(onlp_oid_t id, onlp_led_mode_t mode);

int sysi_platform_info_get(onlp_platform_info_t* pi);

int qsfp_present_get(int port, int *pres_val);

int sfp_present_get(int port, int *pres_val);

bool onlp_sysi_bmc_en_get(void);

int qsfp_port_to_cpld_addr(int port);

int qsfp_port_to_sysfs_attr_offset(int port);

int read_ioport(int addr, int *reg_val);

int bmc_thermal_info_get(onlp_thermal_info_t* info, int id);

int bmc_fan_info_get(onlp_fan_info_t* info, int id);

int exec_cmd(char *cmd, char* out, int size);

int file_read_hex(int* value, const char* fmt, ...);

int file_vread_hex(int* value, const char* fmt, va_list vargs);

int parse_bmc_sdr_cmd(char *cmd_out, int cmd_out_size,
                  char *tokens[], int token_size, 
                  const char *sensor_id_str, int *idx);

int
sys_led_info_get(onlp_led_info_t* info, int id);

int
psu_fru_get(onlp_psu_info_t* info, int id);

int 
psu_stbiout_get(int* stbiout, int id);

int 
psu_stbvout_get(int* stbvout, int id);

void 
lock_init();

int 
bmc_sensor_read(int bmc_cache_index, int sensor_type, float *data);

void check_and_do_i2c_mux_reset(int port);

extern bool bmc_enable;
#endif  /* __PLATFORM_LIB_H__ */
