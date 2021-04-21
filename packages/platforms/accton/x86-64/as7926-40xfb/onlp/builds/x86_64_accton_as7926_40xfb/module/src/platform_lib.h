/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2014 Accton Technology Corporation.
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

#include <onlplib/file.h>
#include "x86_64_accton_as7926_40xfb_log.h"

#define CHASSIS_FAN_COUNT     5
#define CHASSIS_THERMAL_COUNT 12
#define CHASSIS_LED_COUNT     4
#define CHASSIS_PSU_COUNT     2

#define PSU1_ID 1
#define PSU2_ID 2

#define PSU_SYSFS_PATH "/sys/devices/platform/as7926_40xfb_psu/"
#define FAN_BOARD_PATH "/sys/devices/platform/as7926_40xfb_fan/"
#define IDPROM_PATH "/sys/class/i2c-adapter/i2c-0/0-0057/eeprom"

enum onlp_led_id
{
    LED_RESERVED = 0,
    LED_PSU,
    LED_FAN,
    LED_DIAG,
    LED_LOC
};

enum onlp_thermal_id {
    THERMAL_RESERVED = 0,
    THERMAL_CPU_CORE,
    THERMAL_1_ON_ASIC,       /* Main Board Bottom ASIC */
    THERMAL_1_ON_MAINBOARD,  /* Main Board Bottom TMP432_1 Temp */
    THERMAL_2_ON_MAINBOARD,  /* Main Board Bottom TMP432_2 Temp */
    THERMAL_3_ON_MAINBOARD,  /* Main Board Bottom TMP432_3 Temp */
    THERMAL_4_ON_MAINBOARD,  /* Main Board Bottom LM75_1 Temp */
    THERMAL_5_ON_MAINBOARD,  /* Main Board Bottom LM75_2 Temp */
    THERMAL_6_ON_MAINBOARD,  /* Main Board Bottom LM75_3 Temp */
    THERMAL_7_ON_MAINBOARD,  /* Main Board Bottom LM75_4 Temp */
    THERMAL_8_ON_MAINBOARD,  /* QSFP-DD Board LM75_1 Temp */
    THERMAL_9_ON_MAINBOARD,  /* QSFP-DD Board LM75_2 Temp */
    THERMAL_10_ON_MAINBOARD,  /* QSFP-DD Board LM753 Temp */
    THERMAL_1_ON_PSU1,
    THERMAL_1_ON_PSU2,
};

#endif  /* __PLATFORM_LIB_H__ */
