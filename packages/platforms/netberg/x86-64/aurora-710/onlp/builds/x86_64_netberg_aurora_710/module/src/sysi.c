/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *        Copyright 2014, 2015 Big Switch Networks, Inc.
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
#include <onlp/platformi/sysi.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/psui.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/fani.h>
#include <onlplib/file.h>
#include <onlplib/crc32.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "platform_lib.h"

bool bmc_enable = false;

const char*
onlp_sysi_platform_get(void)
{   
    return "x86-64-netberg-aurora-710-r0";
}

int
onlp_sysi_init(void)
{
    /* check if the platform is bmc enabled */
    if ( onlp_sysi_bmc_en_get() ) {
        bmc_enable = true;
        //AIM_LOG_INFO("BMC detected on platform!");
        AIM_SYSLOG_INFO(
                                "BMC detected on platform!",
                                "BMC detected on platform!",
                                "BMC detected on platform!");
    } else {
        bmc_enable = false;
        //AIM_LOG_INFO("BMC NOT detected on platform!");
        AIM_SYSLOG_INFO(
                                "BMC NOT detected on platform!",
                                "BMC NOT detected on platform!",
                                "BMC NOT detected on platform!");
    }

    return ONLP_STATUS_OK;
}

int
onlp_sysi_onie_data_get(uint8_t** data, int* size)
{
    uint8_t* rdata = aim_zmalloc(SYS_EEPROM_SIZE);
    if(onlp_file_read(rdata, SYS_EEPROM_SIZE, size, SYS_EEPROM_PATH) == ONLP_STATUS_OK) {
        if(*size == SYS_EEPROM_SIZE) {
            *data = rdata;
            return ONLP_STATUS_OK;
        }
    }
    
    AIM_LOG_INFO("Unable to get data from eeprom \n");    
    aim_free(rdata);
    *size = 0;
    return ONLP_STATUS_E_INTERNAL;
}

int
onlp_sysi_oids_get(onlp_oid_t* table, int max)
{
    onlp_oid_t* e = table;
    memset(table, 0, max*sizeof(onlp_oid_t));
    int i;

    if ( !bmc_enable ) {
        /* 2 PSUs */
        *e++ = ONLP_PSU_ID_CREATE(1);
        *e++ = ONLP_PSU_ID_CREATE(2);

        /* LEDs Item */
        for (i=1; i<=LED_NUM; i++) {
            *e++ = ONLP_LED_ID_CREATE(i);
        }

        /* Fans Item */
        for (i=1; i<=FAN_NUM; i++) {
            *e++ = ONLP_FAN_ID_CREATE(i);
        }
    }

    /* THERMALs Item */
    if ( !bmc_enable ) {
        for (i=1; i<=THERMAL_NUM; i++) {
            *e++ = ONLP_THERMAL_ID_CREATE(i);
        }
    } else {
        *e++ = THERMAL_OID_CPU1;
        *e++ = THERMAL_OID_CPU2;
        *e++ = THERMAL_OID_CPU3;
        *e++ = THERMAL_OID_CPU4;
        *e++ = THERMAL_OID_CPU_BOARD;
    }

    return ONLP_STATUS_OK;
}

int
decide_fan_percentage(int is_up, int new_temp)
{
    int new_perc;
    if (is_up) {
        if (new_temp >= THERMAL_ERROR_DEFAULT) {
            new_perc = THERMAL_ERROR_FAN_PERC;
        } else if (new_temp >= THERMAL_WARNING_DEFAULT) {
            new_perc = THERMAL_WARNING_FAN_PERC;
        } else {
            new_perc = THERMAL_NORMAL_FAN_PERC;
        }
    } else {
        if (new_temp <= THERMAL_NORMAL_DEFAULT) {
            new_perc = THERMAL_NORMAL_FAN_PERC;
        } else if (new_temp <= THERMAL_WARNING_DEFAULT) {
            new_perc = THERMAL_WARNING_FAN_PERC;
        } else {
            new_perc = THERMAL_ERROR_FAN_PERC;
        }
    }

    return new_perc;
}

int 
platform_thermal_temp_get(int *thermal_temp)
{
    int i, temp, max_temp, rc;
    onlp_thermal_info_t thermal_info;
    memset(&thermal_info, 0, sizeof(thermal_info));
    uint32_t thermal_arr[] = { THERMAL_OID_FRONT_MAC,
                               THERMAL_OID_ASIC,
                               THERMAL_OID_CPU1, 
                               THERMAL_OID_CPU2, 
                               THERMAL_OID_CPU3, 
                               THERMAL_OID_CPU4 };
    max_temp = 0;

    for (i=0; i<BOARD_THERMAL_NUM; i++) {   
        
        if ((rc = onlp_thermali_info_get(thermal_arr[i], &thermal_info)) != ONLP_STATUS_OK) {
            return rc;
        }
        
        temp = thermal_info.mcelsius;
        if (temp > max_temp) {
            max_temp = temp;
        }
    }
    *thermal_temp = max_temp;

    return ONLP_STATUS_OK;
}

int
onlp_sysi_platform_manage_fans(void)
{
    int rc, is_up ,new_temp, thermal_temp, diff;
    static int new_perc = 0, ori_perc = 0;
    static int ori_temp = 0;
    onlp_thermal_info_t thermal_info;
    memset(&thermal_info, 0, sizeof(thermal_info));

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
 
    /* get new temperature */
    if ((rc = platform_thermal_temp_get(&thermal_temp)) != ONLP_STATUS_OK) {
        goto _EXIT;
    }

    new_temp = thermal_temp;
    diff = new_temp - ori_temp;

    if (diff == 0) {
        goto _EXIT;
    } else {
        is_up = (diff > 0 ? 1 : 0);    
    }
    
    new_perc = decide_fan_percentage(is_up, new_temp);
    
    if (ori_perc == new_perc) {
        goto _EXIT;
    }


    AIM_LOG_INFO("The Fan Speeds Percent are now at %d%%", new_perc);

    if ((rc = onlp_fani_percentage_set(THERMAL_OID_ASIC, new_perc)) != ONLP_STATUS_OK) {
        goto _EXIT;
    }
            
    /* update */
    ori_perc = new_perc;
    ori_temp = new_temp;
       
_EXIT :
    return rc;
}

int
onlp_sysi_platform_manage_leds(void)
{
    int psu1_status, psu2_status, rc, i;
    static int pre_psu1_status = 0, pre_psu2_status = 0, pre_fan_status = 0;
    
    //-------------------------------
    static int pre_fan_tray_status[4] = {0};
    int fan_tray_id, sum, total = 0;
    onlp_led_status_t fan_tray_status[SYS_FAN_NUM];
    //-------------------------------
    
    onlp_psu_info_t psu_info;
    onlp_fan_info_t fan_info;
    
    //-------- -----------------------    
    memset(&fan_tray_status, 0, sizeof(fan_tray_status));
    //-------------------------------
 
    memset(&psu_info, 0, sizeof(onlp_psu_info_t));   
    memset(&fan_info, 0, sizeof(onlp_fan_info_t));
    uint32_t fan_arr[] = { FAN_OID_FAN1, 
                           FAN_OID_FAN2, 
                           FAN_OID_FAN3, 
                           FAN_OID_FAN4, 
                           FAN_OID_FAN5, 
                           FAN_OID_FAN6, 
                           FAN_OID_FAN7, 
                           FAN_OID_FAN8, };

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }


    /* PSU LED CTRL */
    if ((rc = onlp_psui_info_get(PSU_OID_PSU1, &psu_info)) != ONLP_STATUS_OK) {
        goto _EXIT;
    }

    psu1_status = psu_info.status;
    if (psu1_status != pre_psu1_status) {
        if((psu1_status & ONLP_PSU_STATUS_PRESENT) == 0) {
            rc = onlp_ledi_mode_set(LED_OID_PSU1, ONLP_LED_MODE_OFF);
        }
        else if(psu1_status != ONLP_PSU_STATUS_PRESENT) {
            rc = onlp_ledi_mode_set(LED_OID_PSU1, ONLP_LED_MODE_ORANGE);
        } else {
            rc = onlp_ledi_mode_set(LED_OID_PSU1, ONLP_LED_MODE_GREEN);
        }

        if (rc != ONLP_STATUS_OK) {
            goto _EXIT;
        }
        pre_psu1_status = psu1_status;
    }

    if ((rc = onlp_psui_info_get(PSU_OID_PSU2, &psu_info)) != ONLP_STATUS_OK) {
        goto _EXIT;
    }

    psu2_status = psu_info.status;
    if( psu2_status != pre_psu2_status) {
        if((psu2_status & ONLP_PSU_STATUS_PRESENT) == 0) {
            rc = onlp_ledi_mode_set(LED_OID_PSU2, ONLP_LED_MODE_OFF);
        }
        else if(psu2_status != ONLP_PSU_STATUS_PRESENT) {
            rc = onlp_ledi_mode_set(LED_OID_PSU2, ONLP_LED_MODE_ORANGE);
        } else {
            rc = onlp_ledi_mode_set(LED_OID_PSU2, ONLP_LED_MODE_GREEN);
        }

        if (rc != ONLP_STATUS_OK) {
            goto _EXIT;
        }
        pre_psu2_status = psu2_status;
    }
    
    /* FAN LED CTRL */
    for (i=0; i<SYS_FAN_NUM; i++) {
        if ((rc = onlp_fani_info_get(fan_arr[i], &fan_info)) != ONLP_STATUS_OK) {
            goto _EXIT;
        }

        /* FAN TRAY LED CTRL */
        fan_tray_status[i] = fan_info.status;
        if (i%2 == 1) {
            sum = fan_tray_status[i-1] + fan_tray_status[i];
            total = total + sum;
            
            switch (i) {
                case 1:
                    fan_tray_id = LED_FAN_TRAY1;
                    break;
                case 3:
                    fan_tray_id = LED_FAN_TRAY2;
                    break;
                case 5:
                    fan_tray_id = LED_FAN_TRAY3;
                    break;
                case 7:
                    fan_tray_id = LED_FAN_TRAY4;
                    break;
            }

    
            /* the enum of fan_tray id is start from 5 to 8, 
             * the "-5" means mapping to array index 0 to 3
             */
            if (sum != pre_fan_tray_status[fan_tray_id - 5]) {
                if (sum > ONLP_LED_STATUS_FAILED) {
                    rc = onlp_ledi_mode_set(fan_tray_id, ONLP_LED_MODE_ORANGE);
                    
                } else {
                    rc = onlp_ledi_mode_set(fan_tray_id, ONLP_LED_MODE_GREEN);
                }
                
                if (rc != ONLP_STATUS_OK) {
                    goto _EXIT;
                }
                
                pre_fan_tray_status[fan_tray_id - 5] = sum;
            }
        }
    }    

    
    if (total != pre_fan_status) {
        if (total == (ONLP_LED_STATUS_PRESENT * 8)) {
            rc = onlp_ledi_mode_set(LED_OID_FAN, ONLP_LED_MODE_GREEN);
        } else {
            rc = onlp_ledi_mode_set(LED_OID_FAN, ONLP_LED_MODE_ORANGE);
        }
        
        if (rc != ONLP_STATUS_OK) {
            goto _EXIT;
        }
        
        pre_fan_status = total;
    }

_EXIT :
    return rc;
}

int
onlp_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int rc;
    if ((rc = sysi_platform_info_get(pi)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    return ONLP_STATUS_OK;
}

