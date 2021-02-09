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
 *
 *
 ***********************************************************/
#ifndef __PLATFORM_LIB_H__
#define __PLATFORM_LIB_H__

#include <onlp/fan.h>
#include <onlp/psu.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/sysi.h>
#include "x86_64_netberg_aurora_710_int.h"
#include "x86_64_netberg_aurora_710_log.h"

#include <x86_64_netberg_aurora_710/x86_64_netberg_aurora_710_config.h>
#define SYS_CPU_TEMP_PREFIX         "/sys/class/hwmon/hwmon0/"
#define SYS_CORE_TEMP_PREFIX        "/sys/class/hwmon/hwmon2/"
#define SYS_CPU_BOARD_TEMP_PREFIX   "/sys/class/hwmon/hwmon3/"
#define SYS_PSU1_NEAR_TEMP_PREFIX   "/sys/class/hwmon/hwmon4/"
#define SYS_PSU2_NEAR_TEMP_PREFIX   "/sys/class/hwmon/hwmon7/"
#define SYS_MAC_REAR_TEMP_PREFIX    "/sys/class/hwmon/hwmon5/"
#define SYS_QSFP_NEAR_TEMP_PREFIX   "/sys/class/hwmon/hwmon6/"

#define SYS_BMC_CPU_BOARD_TEMP_PREFIX   "/sys/class/hwmon/hwmon1/"

#define SYS_FAN_PREFIX              "/sys/class/hwmon/hwmon1/device/"
#define SYS_EEPROM_PATH             "/sys/bus/i2c/devices/0-0055/eeprom"
#define SYS_EEPROM_SIZE 512
#define PSU1_EEPROM_PATH            "/sys/bus/i2c/devices/58-0050/eeprom"
#define PSU2_EEPROM_PATH            "/sys/bus/i2c/devices/57-0050/eeprom"
#define BMC_EN_FILE_PATH            "/etc/onl/bmc_en"
#define PSU_STATUS_PRESENT          1
#define PSU_STATUS_POWER_GOOD       1
#define FAN_PRESENT                 0
#define FAN_CTRL_SET1               1
#define FAN_CTRL_SET2               2
#define MAX_SYS_FAN_NUM             8
#define BOARD_THERMAL_NUM           6
#define SYS_FAN_NUM                 8
//#define QSFP_NUM                    32
#define PORT_NUM                    34

#define THERMAL_NUM                 15
#define LED_NUM                     4
#define FAN_NUM                     10



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
#define I2C_BUS_59              59      /* FRU  */

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
#define PSU1_PRESENT_OFFSET     0x04
#define PSU2_PRESENT_OFFSET     0x01
#define PSU1_PWGOOD_OFFSET      0x03
#define PSU2_PWGOOD_OFFSET      0x00

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
#define CPLD_REG                0x33
#define CPLD_VER_OFFSET         0x01

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

/** led_oid */
typedef enum led_oid_e {
    LED_OID_SYSTEM = ONLP_LED_ID_CREATE(1),
    LED_OID_FAN = ONLP_LED_ID_CREATE(2),
    LED_OID_PSU1 = ONLP_LED_ID_CREATE(3),
    LED_OID_PSU2 = ONLP_LED_ID_CREATE(4),
    LED_OID_FAN_TRAY1 = ONLP_LED_ID_CREATE(5),
    LED_OID_FAN_TRAY2 = ONLP_LED_ID_CREATE(6),
    LED_OID_FAN_TRAY3 = ONLP_LED_ID_CREATE(7),
    LED_OID_FAN_TRAY4 = ONLP_LED_ID_CREATE(8),
} led_oid_t;

/** led_id */
typedef enum led_id_e {
    LED_SYSTEM_LED = 1,
    LED_FAN_LED = 2,
    LED_PSU1_LED = 3,
    LED_PSU2_LED = 4,
    LED_FAN_TRAY1 = 5,
    LED_FAN_TRAY2 = 6,
    LED_FAN_TRAY3 = 7,
    LED_FAN_TRAY4 = 8,
} led_id_t;

/** Thermal_oid */
typedef enum thermal_oid_e {
    THERMAL_OID_FRONT_MAC = ONLP_THERMAL_ID_CREATE(1),
    THERMAL_OID_ASIC = ONLP_THERMAL_ID_CREATE(2),
    THERMAL_OID_CPU1 = ONLP_THERMAL_ID_CREATE(3),
    THERMAL_OID_CPU2 = ONLP_THERMAL_ID_CREATE(4),
    THERMAL_OID_CPU3 = ONLP_THERMAL_ID_CREATE(5),
    THERMAL_OID_CPU4 = ONLP_THERMAL_ID_CREATE(6), 
    THERMAL_OID_PSU1_1 = ONLP_THERMAL_ID_CREATE(7), 
    THERMAL_OID_PSU1_2 = ONLP_THERMAL_ID_CREATE(8), 
    THERMAL_OID_PSU2_1 = ONLP_THERMAL_ID_CREATE(9), 
    THERMAL_OID_PSU2_2 = ONLP_THERMAL_ID_CREATE(10),
    THERMAL_OID_CPU_BOARD = ONLP_THERMAL_ID_CREATE(11),
    THERMAL_OID_PSU1_NEAR = ONLP_THERMAL_ID_CREATE(12),
    THERMAL_OID_PSU2_NEAR = ONLP_THERMAL_ID_CREATE(13),
    THERMAL_OID_MAC_REAR = ONLP_THERMAL_ID_CREATE(14),
    THERMAL_OID_QSFP_NEAR = ONLP_THERMAL_ID_CREATE(15),
} thermal_oid_t;

/** thermal_id */
typedef enum thermal_id_e {
    THERMAL_ID_FRONT_MAC = 1,
    THERMAL_ID_ASIC = 2,
    THERMAL_ID_CPU1 = 3,
    THERMAL_ID_CPU2 = 4,
    THERMAL_ID_CPU3 = 5,
    THERMAL_ID_CPU4 = 6,
    THERMAL_ID_PSU1_1 = 7,
    THERMAL_ID_PSU1_2 = 8,
    THERMAL_ID_PSU2_1 = 9,
    THERMAL_ID_PSU2_2 = 10,
    THERMAL_ID_CPU_BOARD = 11,
    THERMAL_ID_PSU1_NEAR = 12,
    THERMAL_ID_PSU2_NEAR = 13,
    THERMAL_ID_MAC_REAR = 14,
    THERMAL_ID_QSFP_NEAR = 15,
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
    FAN_OID_FAN5 = ONLP_FAN_ID_CREATE(5),
    FAN_OID_FAN6 = ONLP_FAN_ID_CREATE(6),
    FAN_OID_FAN7 = ONLP_FAN_ID_CREATE(7),
    FAN_OID_FAN8 = ONLP_FAN_ID_CREATE(8),
    FAN_OID_PSU_FAN1 = ONLP_FAN_ID_CREATE(9),
    FAN_OID_PSU_FAN2 = ONLP_FAN_ID_CREATE(10)
} fan_oid_t;

/** fan_id */
typedef enum fan_id_e {
    FAN_ID_FAN1 = 1,
    FAN_ID_FAN2 = 2,
    FAN_ID_FAN3 = 3,
    FAN_ID_FAN4 = 4,
    FAN_ID_FAN5 = 5,
    FAN_ID_FAN6 = 6,
    FAN_ID_FAN7 = 7,
    FAN_ID_FAN8 = 8,
    FAN_ID_PSU_FAN1 = 9,
    FAN_ID_PSU_FAN2 = 10
} fan_id_t;

/** led_oid */
typedef enum psu_oid_e {
    PSU_OID_PSU1 = ONLP_PSU_ID_CREATE(1),
    PSU_OID_PSU2 = ONLP_PSU_ID_CREATE(2)
} psu_oid_t;

/** fan_id */
typedef enum psu_id_e {
    PSU_ID_PSU1 = 1,
    PSU_ID_PSU2 = 2
} psu_id_t;

int psu_thermal_get(onlp_thermal_info_t* info, int id);

int psu_fan_info_get(onlp_fan_info_t* info, int id);

int psu_vout_get(onlp_psu_info_t* info, int i2c_bus);

int psu_iout_get(onlp_psu_info_t* info, int i2c_bus);

int psu_pout_get(onlp_psu_info_t* info, int i2c_bus);

int psu_pin_get(onlp_psu_info_t* info, int i2c_bus);

int psu_eeprom_get(onlp_psu_info_t* info, int id);

int psu_present_get(int *pw_exist, int exist_offset, 
                    int i2c_bus, int psu_mask);

int psu_pwgood_get(int *pw_good, int good_offset, int i2c_bus, int psu_mask);

int psu2_led_set(onlp_led_mode_t mode);

int psu1_led_set(onlp_led_mode_t mode);

int fan_led_set(onlp_led_mode_t mode);

int system_led_set(onlp_led_mode_t mode);

int fan_tray_led_set(onlp_oid_t id, onlp_led_mode_t mode);

int sysi_platform_info_get(onlp_platform_info_t* pi);

int qsfp_present_get(int port, int *pres_val);

bool onlp_sysi_bmc_en_get(void);

extern bool bmc_enable;
#endif  /* __PLATFORM_LIB_H__ */
