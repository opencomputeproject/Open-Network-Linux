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
 * Thermal Sensor Platform Implementation.
 *
 ***********************************************************/
//#include <unistd.h>
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include "platform_lib.h"

#define THERMAL_PATH_FORMAT 	"/sys/bus/i2c/devices/%s/*temp1_input"
#define PSU_THERMAL_PATH_FORMAT "/sys/bus/i2c/devices/%s/*psu_temp1_input"

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_THERMAL(_id)) {         \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

enum onlp_thermal_id
{
    THERMAL_RESERVED = 0,
    THERMAL_CPU_CORE,
    THERMAL_1_ON_MAIN_BROAD,
    THERMAL_2_ON_MAIN_BROAD,
    THERMAL_3_ON_MAIN_BROAD,
    THERMAL_4_ON_MAIN_BROAD,
    THERMAL_5_ON_MAIN_BROAD,
    THERMAL_6_ON_MAIN_BROAD,
    THERMAL_7_ON_MAIN_BROAD,
    THERMAL_1_ON_PSU1,
    THERMAL_1_ON_PSU2,
};

static char* directory[] =  /* must map with onlp_thermal_id */
{
    NULL,
    NULL,                  /* CPU_CORE files */
    "18-0048",
    "18-0049",
    "18-004a",
    "18-004b",
    "18-004c",
    "18-004e",
    "18-004f",
    "10-0059",
    "9-0058",
};

static char* cpu_coretemp_files[] =
    {
        "/sys/devices/platform/coretemp.0*temp2_input",
        "/sys/devices/platform/coretemp.0*temp3_input",
        "/sys/devices/platform/coretemp.0*temp4_input",
        "/sys/devices/platform/coretemp.0*temp5_input",
        NULL,
    };


/* Static values */
static onlp_thermal_info_t thermal_info_table__[] = {
	{ }, /* Not used */
        ONLP_CHASSIS_THERMAL_INFO_ENTRY_INIT(THERMAL_CPU_CORE, "CPU Core"),
        ONLP_CHASSIS_THERMAL_INFO_ENTRY_INIT(THERMAL_1_ON_MAIN_BROAD, "LM75-1-48"),
        ONLP_CHASSIS_THERMAL_INFO_ENTRY_INIT(THERMAL_2_ON_MAIN_BROAD, "LM75-2-49"),
        ONLP_CHASSIS_THERMAL_INFO_ENTRY_INIT(THERMAL_3_ON_MAIN_BROAD, "LM75-3-4A"),
        ONLP_CHASSIS_THERMAL_INFO_ENTRY_INIT(THERMAL_4_ON_MAIN_BROAD, "LM75-4-4B"),
        ONLP_CHASSIS_THERMAL_INFO_ENTRY_INIT(THERMAL_5_ON_MAIN_BROAD, "LM75-5-4C"),
        ONLP_CHASSIS_THERMAL_INFO_ENTRY_INIT(THERMAL_6_ON_MAIN_BROAD, "LM75-6-4E"),
        ONLP_CHASSIS_THERMAL_INFO_ENTRY_INIT(THERMAL_7_ON_MAIN_BROAD, "LM75-7-4F"),        
        ONLP_PSU_THERMAL_INFO_ENTRY_INIT(THERMAL_1_ON_PSU1, "PSU-1 Thermal Sensor 1", 1),
        ONLP_PSU_THERMAL_INFO_ENTRY_INIT(THERMAL_1_ON_PSU2, "PSU-2 Thermal Sensor 1", 2),
};

/*
 * This will be called to intiialize the thermali subsystem.
 */
int
onlp_thermali_init(void)
{
    return ONLP_STATUS_OK;
}

/*
 * Retrieve the information structure for the given thermal OID.
 *
 * If the OID is invalid, return ONLP_E_STATUS_INVALID.
 * If an unexpected error occurs, return ONLP_E_STATUS_INTERNAL.
 * Otherwise, return ONLP_STATUS_OK with the OID's information.
 *
 * Note -- it is expected that you fill out the information
 * structure even if the sensor described by the OID is not present.
 */
int
onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* info)
{
    int   tid;
    char *format   = NULL;
    char  path[64] = {0};
	
    tid = ONLP_OID_ID_GET(id);
	
    /* Set the onlp_oid_hdr_t and capabilities */		
    *info = thermal_info_table__[tid];
    if(tid == THERMAL_CPU_CORE) {
        return onlp_file_read_int_max(&info->mcelsius, cpu_coretemp_files);
    }

    switch (tid) {
    	case THERMAL_1_ON_MAIN_BROAD:
    	case THERMAL_2_ON_MAIN_BROAD:
    	case THERMAL_3_ON_MAIN_BROAD:
    	case THERMAL_4_ON_MAIN_BROAD:
    	case THERMAL_5_ON_MAIN_BROAD:
    	case THERMAL_6_ON_MAIN_BROAD:
    	case THERMAL_7_ON_MAIN_BROAD:
            format = THERMAL_PATH_FORMAT;
            break;
    	case THERMAL_1_ON_PSU1:
    	case THERMAL_1_ON_PSU2:
            format = PSU_THERMAL_PATH_FORMAT;
            break;
        default:
            return ONLP_STATUS_E_INVALID;
    };
	
    /* get path */
    sprintf(path, format, directory[tid], tid);
    if (onlp_file_read_int(&info->mcelsius, path) < 0) {
        AIM_LOG_ERROR("Unable to read status from file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;								
}

int
onlp_thermali_hdr_get(onlp_oid_id_t id, onlp_oid_hdr_t* hdr)
{
    *hdr = thermal_info_table__[id].hdr;
    return 0;
}

