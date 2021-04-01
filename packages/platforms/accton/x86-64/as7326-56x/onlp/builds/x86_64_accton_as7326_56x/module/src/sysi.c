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
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#include <onlp/platformi/sysi.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/fani.h>
#include <onlp/platformi/psui.h>
#include "platform_lib.h"
#include "x86_64_accton_as7326_56x_int.h"
#include "x86_64_accton_as7326_56x_log.h"

#define NUM_OF_FAN_ON_MAIN_BROAD      6

#define PREFIX_PATH_ON_CPLD_DEV          "/sys/bus/i2c/devices/"
#define NUM_OF_CPLD                      3
#define FAN_DUTY_CYCLE_MAX         (100)
#define FAN_DUTY_CYCLE_DEFAULT     (32)
#define FAN_DUTY_PLUS_FOR_DIR      (13)
/* Note, all chassis fans share 1 single duty setting.
 * Here use fan 1 to represent global fan duty value.*/
#define FAN_ID_FOR_SET_FAN_DUTY    (1)
#define CELSIUS_RECORD_NUMBER      (2)  /*Must >= 2*/


static char arr_cplddev_name[NUM_OF_CPLD][10] =
{
 "18-0060",
 "12-0062",
 "19-0064"
};

const char*
onlp_sysi_platform_get(void)
{
    return "x86-64-accton-as7326-56x-r0";
}

int
onlp_sysi_onie_data_get(uint8_t** data, int* size)
{
    const int len = 256;
    uint8_t* rdata = aim_zmalloc(len);
    char *paths[] = {IDPROM_PATH_2, IDPROM_PATH_1};
    int  ret = ONLP_STATUS_OK;
    int i;
    
    for (i = 0 ; i < AIM_ARRAYSIZE(paths); i++ ){
        ret = onlp_file_open(O_RDONLY, 0, paths[i]);
        if (ret >= 0) {
            close(ret);
            if(onlp_file_read(rdata, len, size, paths[i]) == ONLP_STATUS_OK) {
                if(*size == len) {
                    *data = rdata;
                    return ONLP_STATUS_OK;
                }
            }
        }
    }
    aim_free(rdata);
    *size = 0;
    return ONLP_STATUS_E_INTERNAL;
}

int
onlp_sysi_oids_get(onlp_oid_t* table, int max)
{
    int i;
    onlp_oid_t* e = table;
    memset(table, 0, max*sizeof(onlp_oid_t));

    /* 5 Thermal sensors on the chassis */
    for (i = 1; i <= CHASSIS_THERMAL_COUNT; i++) {
        *e++ = ONLP_THERMAL_ID_CREATE(i);
    }

    /* 5 LEDs on the chassis */
    for (i = 1; i <= CHASSIS_LED_COUNT; i++) {
        *e++ = ONLP_LED_ID_CREATE(i);
    }

    /* 2 PSUs on the chassis */
    for (i = 1; i <= CHASSIS_PSU_COUNT; i++) {
        *e++ = ONLP_PSU_ID_CREATE(i);
    }

    /* 6 Fans on the chassis */
    for (i = 1; i <= CHASSIS_FAN_COUNT; i++) {
        *e++ = ONLP_FAN_ID_CREATE(i);
    }

    return 0;
}

int
onlp_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int   i, v[NUM_OF_CPLD]={0};

    for (i = 0; i < NUM_OF_CPLD; i++) {
        v[i] = 0;

        if(onlp_file_read_int(v+i, "%s%s/version", PREFIX_PATH_ON_CPLD_DEV, arr_cplddev_name[i]) < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }
    }
    pi->cpld_versions = aim_fstrdup("%d.%d.%d", v[0], v[1], v[2]);

    return 0;
}

void
onlp_sysi_platform_info_free(onlp_platform_info_t* pi)
{
    aim_free(pi->cpld_versions);
}

/* Thermal policy
 * Both B2F and F2B direction use the same policy
 *1.	(Thermal sensor_LM75_49 + Thermal sensor_LM75_CPU) /2 =< 39C    , Keep 37.5%(0x05) Fan speed
 *2.	(Thermal sensor_LM75_49 + Thermal sensor_LM75_CPU) /2 > 39C    , Change Fan speed from 37.5%(0x05) to 75%(0x0B)
 *3.	(Thermal sensor_LM75_49 + Thermal sensor_LM75_CPU) /2 > 45C    , Change Fan speed from 75%(0x0B) to 100%(0x0F)
 *4.	(Thermal sensor_LM75_49 + Thermal sensor_LM75_CPU) /2 > 61C     , Send alarm message
 *5.	(Thermal sensor_LM75_49 + Thermal sensor_LM75_CPU) /2 > 66C     , Shut down system
 *6.	One Fan fail      , Change Fan speed to 100%(0x0F)
 */


typedef struct fan_ctrl_policy {
   int duty_cycle;
   int pwm;
   int temp_down; /* The boundary temperature to down adjust fan speed */
   int temp_up;   /* The boundary temperature to up adjust fan speed */
   int state;
} fan_ctrl_policy_t;

enum
{
   LEVEL_FAN_DEF=0,
   LEVEL_FAN_MID,
   LEVEL_FAN_MAX,
   LEVEL_TEMP_HIGH,
   LEVEL_TEMP_CRITICAL
};

fan_ctrl_policy_t  fan_thermal_policy[] = {
{38,  0x4, 0,     39000,   LEVEL_FAN_DEF},
{75,  0xB, 39000, 45000,   LEVEL_FAN_MID},
{100, 0xE, 45000, 61000,   LEVEL_FAN_MAX},
{100, 0xE, 61000, 66000,   LEVEL_TEMP_HIGH},
{100, 0xE, 66000, 200000,  LEVEL_TEMP_CRITICAL}
};

#define FAN_SPEED_CTRL_PATH "/sys/bus/i2c/devices/11-0066/fan_duty_cycle_percentage"

static int fan_state=LEVEL_FAN_DEF;
static int alarm_state = 0; /* 0->default or clear, 1-->alarm detect */
int
onlp_sysi_platform_manage_fans(void)
{
    int i=0, ori_state=LEVEL_FAN_DEF, current_state=LEVEL_FAN_DEF;
    int  fd, len;
    int cur_duty_cycle, new_duty_cycle, temp=0;
    onlp_thermal_info_t thermal_3, thermal_5;
    char  buf[10] = {0};

    /* Get current temperature
     */
    if (onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(3), &thermal_3) != ONLP_STATUS_OK  )
    {
        AIM_LOG_ERROR("Unable to read thermal status, set fans to 75 %% speed");
        onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(1), fan_thermal_policy[LEVEL_FAN_MID].duty_cycle);
        return ONLP_STATUS_E_INTERNAL;
    }
    if(onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(5), &thermal_5) != ONLP_STATUS_OK)
    {
        AIM_LOG_ERROR("Unable to read thermal status, set fans to 75 %% speed");
        onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(1), fan_thermal_policy[LEVEL_FAN_MID].duty_cycle);
        return ONLP_STATUS_E_INTERNAL;
    }

    temp = (thermal_3.mcelsius + thermal_5.mcelsius)/2;

    /* Get current fan pwm percent
     */
    fd = open(FAN_SPEED_CTRL_PATH, O_RDONLY);
    if (fd == -1){
        AIM_LOG_ERROR("Unable to open fan speed control node (%s)", FAN_SPEED_CTRL_PATH);
        return ONLP_STATUS_E_INTERNAL;
    }
    len = read(fd, buf, sizeof(buf));
    close(fd);
    if (len <= 0) {
        AIM_LOG_ERROR("Unable to read fan speed from (%s)", FAN_SPEED_CTRL_PATH);
        return ONLP_STATUS_E_INTERNAL;
    }
    cur_duty_cycle = atoi(buf);
    ori_state=fan_state;
    /* Inpunt temp to get theraml_polyc state and new pwm percent. */
    for(i=0; i < sizeof(fan_thermal_policy)/sizeof(fan_ctrl_policy_t); i++)
    {
        if (temp > fan_thermal_policy[i].temp_down)
        {
            if (temp <= fan_thermal_policy[i].temp_up)
            {
                current_state =i;
            }
        }
    }

    /* Decision 3: Decide new fan pwm percent.
     */
    if (cur_duty_cycle!=fan_thermal_policy[current_state].duty_cycle)
    {
        new_duty_cycle = fan_thermal_policy[current_state].duty_cycle;
        onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(1), new_duty_cycle);
    }

    /* Get each fan status
     */
    for (i = 1; i <= NUM_OF_FAN_ON_MAIN_BROAD; i++)
    {
        onlp_fan_info_t fan_info;

        if (onlp_fani_info_get(ONLP_FAN_ID_CREATE(i), &fan_info) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("Unable to get fan(%d) status, try to set the other fans as full speed\r\n", i);
            onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(1), FAN_DUTY_CYCLE_MAX);
            if (fan_state < LEVEL_FAN_MAX)
            {
                fan_state=LEVEL_FAN_MAX;
                current_state=fan_state;
            }
            if(current_state <LEVEL_FAN_MAX )
                current_state=LEVEL_FAN_MAX;
            break;
        }
        /* Decision 1: Set fan as full speed if any fan is failed.
         */
        if (fan_info.status & ONLP_FAN_STATUS_FAILED || !(fan_info.status & ONLP_FAN_STATUS_PRESENT)) {
            AIM_LOG_ERROR("Fan(%d) is not working, set the other fans as full speed\r\n", i);
            onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(1), FAN_DUTY_CYCLE_MAX);
            if (fan_state < LEVEL_FAN_MAX)
            {
                fan_state=LEVEL_FAN_MAX;
                current_state=fan_state;
            }
            if(current_state <LEVEL_FAN_MAX )
                current_state=LEVEL_FAN_MAX;
            break;
        }
    }

    if(current_state!=ori_state)
    {
         fan_state=current_state;

         switch (ori_state)
         {
             case LEVEL_FAN_DEF:
                 if(current_state==LEVEL_TEMP_HIGH)
                 {
                     if(alarm_state==0)
                     {
                        AIM_SYSLOG_WARN("Temperature high", "Temperature high","Alarm for temperature high is detected");
                        alarm_state=1;
                     }
                 }
                 if(current_state==LEVEL_TEMP_CRITICAL)
                 {
                     AIM_SYSLOG_CRIT("Temperature critical", "Temperature critical", "Alarm for temperature critical is detected, reboot DUT");
                     system("sync;sync;sync");
                     system("reboot");
                 }
                 break;
             case LEVEL_FAN_MID:
                 if(current_state==LEVEL_TEMP_HIGH)
                 {
                     if(alarm_state==0)
                     {
                        AIM_SYSLOG_WARN("Temperature high", "Temperature high","Alarm for temperature high is detected");
                        alarm_state=1;
                     }
                 }
                 if(current_state==LEVEL_TEMP_CRITICAL)
                 {
                     AIM_SYSLOG_CRIT("Temperature critical", "Temperature critical", "Alarm for temperature critical is detected, reboot DUT");
                     system("sync;sync;sync");
                     system("reboot");
                 }
                 break;
             case LEVEL_FAN_MAX:
                 if(current_state==LEVEL_TEMP_HIGH)
                 {
                     if(alarm_state==0)
                     {
                        AIM_SYSLOG_WARN("Temperature high", "Temperature high","Alarm for temperature high is detected");
                        alarm_state=1;
                     }
                 }
                 if(current_state==LEVEL_TEMP_CRITICAL)
                 {
                     AIM_SYSLOG_CRIT("Temperature critical", "Temperature critical ", "Alarm for temperature critical is detected, reboot DUT");
                     system("sync;sync;sync");
                     system("reboot");
                 }
                 break;
             case LEVEL_TEMP_HIGH:
                 if(current_state==LEVEL_TEMP_CRITICAL)
                 {
                     AIM_SYSLOG_CRIT("Temperature critical", "Temperature critical ", "Alarm for temperature critical is detected, reboot DUT");
                     system("sync;sync;sync");
                     system("reboot");
                 }
                 break;
             case LEVEL_TEMP_CRITICAL:
                 break;
             default:
                AIM_SYSLOG_WARN("onlp_sysi_platform_manage_fans abnormal state", "onlp_sysi_platform_manage_fans  abnormal state", "onlp_sysi_platform_manage_fans at abnormal state\n");
                 break;
         }

    }
    if(alarm_state==1 && current_state < LEVEL_TEMP_HIGH)
    {
       if (temp < (fan_thermal_policy[3].temp_down - 5000)) /*below 65 C, clear alarm*/
       {
           AIM_SYSLOG_INFO("Temperature high is clean", "Temperature high is clear", "Alarm for temperature high is cleared");
           alarm_state=0;
       }
    }

    return 0;
}


int
onlp_sysi_platform_manage_leds(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
