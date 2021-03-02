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
 * ONLP System Platform Interface.
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
    int mb_cpld1_addr = 0x700;
    int mb_cpld1_board_type_rev;
    int mb_cpld1_hw_rev, mb_cpld1_build_rev;

    if (read_ioport(mb_cpld1_addr, &mb_cpld1_board_type_rev) < 0) {
        AIM_LOG_ERROR("unable to read MB CPLD1 Board Type Revision\n");
        return "x86-64-ufispace-s9705-48d-rx";
    }   
    mb_cpld1_hw_rev = (((mb_cpld1_board_type_rev) >> 2 & 0x03));
    mb_cpld1_build_rev = (((mb_cpld1_board_type_rev) & 0x03) | ((mb_cpld1_board_type_rev) >> 5 & 0x04));
    
    if (mb_cpld1_hw_rev == 0 && mb_cpld1_build_rev == 0) {
        return "x86-64-ufispace-s9705-48d-r0";
    } else if (mb_cpld1_hw_rev == 1 && mb_cpld1_build_rev == 0) {
        return "x86-64-ufispace-s9705-48d-r1";
    } else if (mb_cpld1_hw_rev == 1 && mb_cpld1_build_rev == 1) {
        return "x86-64-ufispace-s9705-48d-r2";
    } else if (mb_cpld1_hw_rev == 2 && mb_cpld1_build_rev == 0) {
        return "x86-64-ufispace-s9705-48d-r3";
    } else if (mb_cpld1_hw_rev == 2 && mb_cpld1_build_rev == 1) {
        return "x86-64-ufispace-s9705-48d-r4";
    } else if (mb_cpld1_hw_rev == 2 && mb_cpld1_build_rev == 2) {
        return "x86-64-ufispace-s9705-48d-r5";
    } else if (mb_cpld1_hw_rev == 3 && mb_cpld1_build_rev == 0) {
        return "x86-64-ufispace-s9705-48d-r6";
    } else if (mb_cpld1_hw_rev == 3 && mb_cpld1_build_rev == 1) {
        return "x86-64-ufispace-s9705-48d-r7";
    } else {        
        return "x86-64-ufispace-s9705-48d-r7";
    }
}

int
onlp_sysi_platform_set(const char* platform)
{
    return ONLP_STATUS_OK;
}

int
onlp_sysi_init(void)
{    
    /* check if the platform is bmc enabled */
    if ( onlp_sysi_bmc_en_get() ) {
        bmc_enable = true;
    } else {
        bmc_enable = false;
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

void
onlp_sysi_onie_data_free(uint8_t* data)
{
    if (data) {
        aim_free(data);
    }
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
        *e++ = THERMAL_OID_CPU_PECI;
        //*e++ = THERMAL_OID_BMC_ENV;
        //*e++ = THERMAL_OID_ENV_FRONT_T;
        //*e++ = THERMAL_OID_ENV_FRONT_B;        
        *e++ = THERMAL_OID_RAMON_ENV_T;
        *e++ = THERMAL_OID_RAMON_DIE_T;
        *e++ = THERMAL_OID_RAMON_ENV_B;
        *e++ = THERMAL_OID_RAMON_DIE_B;
        //*e++ = THERMAL_OID_CPU_PKG;        
        //*e++ = THERMAL_OID_CPU1;
        //*e++ = THERMAL_OID_CPU2;
        //*e++ = THERMAL_OID_CPU3;
        //*e++ = THERMAL_OID_CPU4;
        //*e++ = THERMAL_OID_CPU5;
        //*e++ = THERMAL_OID_CPU6;
        //*e++ = THERMAL_OID_CPU7;
        //*e++ = THERMAL_OID_CPU8;
        //*e++ = THERMAL_OID_CPU_BOARD;
        
        *e++ = LED_OID_SYSTEM;        
        *e++ = LED_OID_PSU0;
        *e++ = LED_OID_PSU1;
        *e++ = LED_OID_FAN;
	
        *e++ = PSU_OID_PSU0;
        *e++ = PSU_OID_PSU1;

        *e++ = FAN_OID_FAN1;
        *e++ = FAN_OID_FAN2;
        *e++ = FAN_OID_FAN3;
        *e++ = FAN_OID_FAN4;   
    }
    
    return ONLP_STATUS_OK;
}

int
onlp_sysi_platform_manage_fans(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_sysi_platform_manage_leds(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
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

void
onlp_sysi_platform_info_free(onlp_platform_info_t* pi)
{
    if (pi->cpld_versions) {
        aim_free(pi->cpld_versions);
    }

    if (pi->other_versions) {
        aim_free(pi->other_versions);
    }
}

