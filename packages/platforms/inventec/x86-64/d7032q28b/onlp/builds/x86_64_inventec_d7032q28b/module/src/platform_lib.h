/************************************************************
 * platform_lib.h
 *
 *           Copyright 2018 Inventec Technology Corporation.
 *
 ************************************************************
 *
 ***********************************************************/
#ifndef __PLATFORM_LIB_H__
#define __PLATFORM_LIB_H__

#include "x86_64_inventec_d7032q28b_log.h"

/* This is definitions for x86-64-inventec-d7032q28b*/
/* OID map*/
/*
 *  SYS---------ONLP_THERMAL_CPU_PHY
 *         |----ONLP_THERMAL_CPU_CORE0
 *         |----ONLP_THERMAL_CPU_CORE1
 *         |----ONLP_THERMAL_CPU_CORE2
 *         |----ONLP_THERMAL_1_ON_MAIN_BROAD
 *         |----ONLP_THERMAL_2_ON_MAIN_BROAD
 *         |----ONLP_THERMAL_3_ON_MAIN_BROAD
 *         |----ONLP_THERMAL_4_ON_MAIN_BROAD
 *         |----ONLP_THERMAL_5_ON_MAIN_BROAD
 *         |
 *         |----ONLP_FAN_1
 *         |----ONLP_FAN_2
 *         |----ONLP_FAN_3
 *         |----ONLP_FAN_4
 *         |----ONLP_FAN_5
 *         |----ONLP_FAN_6
 *         |----ONLP_FAN_7
 *         |----ONLP_FAN_8
 *         |
 *         |----ONLP_LED_FAN1
 *         |
 *         |----ONLP_LED_FAN2
 *         |
 *         |----ONLP_LED_FAN3
 *         |
 *         |----ONLP_LED_FAN4
 *         |
 *         |----ONLP_PSU_1--------ONLP_THERMAL_1_ON_PSU1
 *         |                   |--ONLP_FAN_PSU_1
 *         |
 *         |----ONLP_PSU_2--------ONLP_THERMAL_1_ON_PSU2
 *         |                   |--ONLP_FAN_PSU_2
 *         |
 *         |----ONLP_LED_MGMT
 */

#define INV_INFO_PREFIX                 "/sys/bus/i2c/devices/0-0055/"
#define INV_HWMON_PREFIX                "/sys/bus/i2c/devices/0-0066/"
#define INV_FAN_PREFIX                  "/sys/bus/i2c/devices/0-0066/"
#define INV_THERMAL_PREFIX              "/sys/bus/i2c/devices/0-0066/"
#define INV_LED_PREFIX                  "/sys/bus/i2c/devices/0-0066/"
#define INV_PSU_PREFIX                  "/sys/bus/i2c/devices/0-0066/"
#define INV_SFP_PREFIX		            "/sys/class/swps/"
#define INV_EEPROM_PATH                 "/sys/bus/i2c/devices/0-0053/eeprom"

#define ADD_STATE(orig_state,new_state)  orig_state | new_state
#define REMOVE_STATE(orig_state, target) orig_state & (~target)
#define LOCAL_ID_TO_INFO_IDX(id)  (id-1)

/* Thermal definitions*/
enum onlp_thermal_id {
    ONLP_THERMAL_CPU_PHY = 1,
    ONLP_THERMAL_CPU_CORE0,
    ONLP_THERMAL_CPU_CORE1,
    ONLP_THERMAL_CPU_CORE2,
    ONLP_THERMAL_1_ON_MAIN_BROAD,
    ONLP_THERMAL_2_ON_MAIN_BROAD,
    ONLP_THERMAL_3_ON_MAIN_BROAD,
    ONLP_THERMAL_4_ON_MAIN_BROAD,
    ONLP_THERMAL_5_ON_MAIN_BROAD,
    ONLP_THERMAL_1_ON_PSU1,
    ONLP_THERMAL_1_ON_PSU2,
    ONLP_THERMAL_MAX  /*num limit include reserved*/
};

/* Fan definitions*/
enum onlp_fan_id {
    ONLP_FAN_1 = 1,
    ONLP_FAN_2,
    ONLP_FAN_3,
    ONLP_FAN_4,
    ONLP_FAN_5,
    ONLP_FAN_6,
    ONLP_FAN_7,
    ONLP_FAN_8,
    ONLP_FAN_PSU_1,
    ONLP_FAN_PSU_2,
    ONLP_FAN_MAX	/*num limit include reserved*/
};

/* PSU definitions*/
enum onlp_psu_id {
    ONLP_PSU_1 = 1,
    ONLP_PSU_2,
    ONLP_PSU_MAX	/*num limit include reserved*/
};

/* LED definitions*/
enum onlp_led_id {
    ONLP_LED_MGMT = 1,
    ONLP_LED_FAN1,
    ONLP_LED_FAN2,
    ONLP_LED_FAN3,
    ONLP_LED_FAN4,
    ONLP_LED_MAX	/*num limit include reserved*/
};

/* platform functions*/
#define PLATFORM_HWMON_DIAG_LOCK
#define PLATFORM_HWMON_DIAG_UNLOCK
int platform_hwmon_diag_enable_read(int *enable);
int platform_hwmon_diag_enable_write(int enable);
char* hwmon_path( char* parent_dir);



#endif  /* __PLATFORM_LIB_H__ */

