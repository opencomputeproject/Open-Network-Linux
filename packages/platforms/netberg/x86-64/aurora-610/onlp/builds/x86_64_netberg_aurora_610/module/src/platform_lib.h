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

#include "x86_64_netberg_aurora_610_log.h"

/* This is definitions for x86-64-netberg-aurora-610*/
/* OID map*/
/*
 *  SYS---------ONLP_THERMAL_CPU_PHY
 *         |----ONLP_THERMAL_CPU_CORE0
 *         |----ONLP_THERMAL_CPU_CORE1
 *         |----ONLP_THERMAL_CPU_CORE2
 *         |----ONLP_THERMAL_CPU_CORE3
 *         |----ONLP_THERMAL_1_ON_MAIN_BROAD
 *         |----ONLP_THERMAL_2_ON_MAIN_BROAD
 *         |----ONLP_THERMAL_3_ON_MAIN_BROAD
 *         |----ONLP_THERMAL_4_ON_MAIN_BROAD
 *         |----ONLP_THERMAL_5_ON_MAIN_BROAD
 *         |----ONLP_FAN_1--------ONLP_LED_FAN1
 *         |
 *         |----ONLP_FAN_2--------ONLP_LED_FAN2
 *         |
 *         |----ONLP_FAN_3--------ONLP_LED_FAN3
 *         |
 *         |----ONLP_FAN_4--------ONLP_LED_FAN4
 *         |
 *         |
 *         |----ONLP_PSU_1--------ONLP_THERMAL_1_ON_PSU1
 *         |                   |--ONLP_THERMAL_2_ON_PSU1
 *         |                   |--ONLP_FAN_PSU_1
 *         |
 *         |----ONLP_PSU_2--------ONLP_THERMAL_1_ON_PSU2
 *         |                   |--ONLP_THERMAL_2_ON_PSU2
 *         |                   |--ONLP_FAN_PSU_2
 *         |
 *         |----ONLP_LED_MGMT
 */

#define NET_SYSLED_PREFIX	"/sys/class/hwmon/hwmon2/device/"
#define NET_HWMON_PREFIX	"/sys/class/hwmon/hwmon1/device/"
#define NET_CTMP_PREFIX		"/sys/class/hwmon/hwmon0/"
#define NET_SFP_PREFIX		"/sys/class/swps/"
#define NET_SYS_PREFIX		"/sys/class/eeprom/vpd/"

#define OID_MAP_TO_INFO_IDX(oid)         ONLP_OID_ID_GET(oid)-1
#define LOCAL_ID_TO_INFO_IDX(id)         (id-1)
#define ADD_STATE(orig_state,new_state)  orig_state | new_state
#define REMOVE_STATE(orig_state, target) orig_state & (~target)

/* Thermal definitions*/
enum onlp_thermal_id {
    ONLP_THERMAL_CPU_PHY = 1,
    ONLP_THERMAL_CPU_CORE0,
    ONLP_THERMAL_CPU_CORE1,
    ONLP_THERMAL_CPU_CORE2,
    ONLP_THERMAL_CPU_CORE3,
    ONLP_THERMAL_1_ON_MAIN_BROAD,
    ONLP_THERMAL_2_ON_MAIN_BROAD,
    ONLP_THERMAL_3_ON_MAIN_BROAD,
    ONLP_THERMAL_4_ON_MAIN_BROAD,
    ONLP_THERMAL_5_ON_MAIN_BROAD,
    ONLP_THERMAL_1_ON_PSU1,
    ONLP_THERMAL_2_ON_PSU1,
    ONLP_THERMAL_1_ON_PSU2,
    ONLP_THERMAL_2_ON_PSU2,
    ONLP_THERMAL_MAX
};

#define ONLP_THERMAL_COUNT 14 /*include "reserved"*/

/* Fan definitions*/
enum onlp_fan_id {
    ONLP_FAN_1 = 1,
    ONLP_FAN_2,
    ONLP_FAN_3,
    ONLP_FAN_4,
    ONLP_FAN_PSU_1,
    ONLP_FAN_PSU_2,
    ONLP_FAN_MAX
};

#define ONLP_FAN_COUNT 7 /*include "reserved"*/

/* PSU definitions*/
enum onlp_psu_id {
    ONLP_PSU_1 = 1,
    ONLP_PSU_2,
    ONLP_PSU_MAX
};

#define ONLP_PSU_COUNT 2 /*include "reserved"*/

/* LED definitions*/
enum onlp_led_id {
    ONLP_LED_MGMT = 1,
    ONLP_LED_FAN1,
    ONLP_LED_FAN2,
    ONLP_LED_FAN3,
    ONLP_LED_FAN4,
    ONLP_LED_MAX
};

#define ONLP_LED_COUNT 6 /*include "reserved"*/


/* platform functions*/
#define PLATFORM_HWMON_DIAG_LOCK
#define PLATFORM_HWMON_DIAG_UNLOCK
//#define PLATFORM_HWMON_DIAG_LOCK platform_hwmon_diag_enable_write(0)
//#define PLATFORM_HWMON_DIAG_UNLOCK platform_hwmon_diag_enable_write(1)
int platform_hwmon_diag_enable_read(int *enable);
int platform_hwmon_diag_enable_write(int enable);




#endif  /* __PLATFORM_LIB_H__ */

