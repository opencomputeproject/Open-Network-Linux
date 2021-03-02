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
 * Fan Platform Implementation.
 *
 ***********************************************************/
#include <onlp/platformi/fani.h>
#include "x86_64_ufispace_s9705_48d_int.h"
#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include "platform_lib.h"

onlp_fan_info_t fan_info[] = {
    { }, /* Not used */
    {
        { FAN_OID_FAN1, "Chassis Fan - 1", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
    },
    {
        { FAN_OID_FAN2, "Chassis Fan - 2", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
    },
    {
        { FAN_OID_FAN3, "Chassis Fan - 3", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
    },
    {
        { FAN_OID_FAN4, "Chassis Fan - 4", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
    },
    {
        { FAN_OID_PSU0_FAN1, "PSU 1 - Fan 1", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM,
    },
    {
        { FAN_OID_PSU0_FAN2, "PSU 1 - Fan 2", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM,
    }
        ,
    {
        { FAN_OID_PSU1_FAN1, "PSU 2 - Fan 1", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM,
    },
    {
        { FAN_OID_PSU1_FAN2, "PSU 2 - Fan 2", 0 },
        ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_F2B,
        ONLP_FAN_CAPS_GET_RPM,
    }
};

/*
 * This function will be called prior to all of onlp_fani_* functions.
 */
int
onlp_fani_init(void)
{
    lock_init();
    return ONLP_STATUS_OK;
}

int sys_fan_present_get(onlp_fan_info_t* info, int id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int 
sys_fan_info_get(onlp_fan_info_t* info, int id)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
sys_fan_rpm_percent_set(int perc)
{  
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_fani_rpm_set(onlp_oid_t id, int rpm)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function sets the fan speed of the given OID as a percentage.
 *
 * This will only be called if the OID has the PERCENTAGE_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int
onlp_fani_percentage_set(onlp_oid_t id, int percentage)
{
    return ONLP_STATUS_E_UNSUPPORTED;  
}

int
onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* rv)
{
    int fan_id ,rc;
        
    fan_id = ONLP_OID_ID_GET(id);
    *rv = fan_info[fan_id];
    rv->caps |= ONLP_FAN_CAPS_GET_RPM;
       
    switch (fan_id) {
        case FAN_ID_FAN1:
        case FAN_ID_FAN2:
        case FAN_ID_FAN3:
        case FAN_ID_FAN4:
        case FAN_ID_PSU0_FAN1:
        case FAN_ID_PSU0_FAN2:    
        case FAN_ID_PSU1_FAN1:
        case FAN_ID_PSU1_FAN2:
            rc = bmc_fan_info_get(rv, fan_id);
            break;
        default:            
            return ONLP_STATUS_E_INTERNAL;
            break;
    }

    return rc;
}
