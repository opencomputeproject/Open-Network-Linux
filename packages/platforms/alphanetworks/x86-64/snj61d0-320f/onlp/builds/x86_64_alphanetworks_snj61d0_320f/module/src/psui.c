/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2018 Alpha Networks Incorporation.
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
#include <onlp/platformi/psui.h>
#include <onlplib/file.h>
#include <stdio.h>
#include <string.h>
#include "platform_lib.h"

#define PSU_STATUS_PRESENT      1
#define PSU_STATUS_POWER_GOOD   1

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_PSU(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)


int
onlp_psui_init(void)
{
    DIAG_PRINT("%s", __FUNCTION__);
    return ONLP_STATUS_OK;
}

static int
psu_yesm1300am_info_get(onlp_psu_info_t *info)
{
    DIAG_PRINT("%s, Model:%s", __FUNCTION__, info->model);
    int val   = 0;
    int index = ONLP_OID_ID_GET(info->hdr.id);

    /* Set capability
     */
    info->caps = ONLP_PSU_CAPS_AC;
    psu_serial_number_get(index, info->serial, sizeof(info->serial));

    if (info->status & ONLP_PSU_STATUS_FAILED)
    {
        return ONLP_STATUS_OK;
    }

    /* Set the associated oid_table */
    info->hdr.coids[0] = ONLP_FAN_ID_CREATE(index + CHASSIS_FAN_COUNT);
    info->hdr.coids[1] = ONLP_THERMAL_ID_CREATE(index + CHASSIS_THERMAL_COUNT);

    /* Read voltage, current and power */
    if (psu_yesm1300am_pmbus_info_get(index, "psu_v_out", &val) == 0) {
        info->mvout = val;
        info->caps |= ONLP_PSU_CAPS_VOUT;
    }

    if (psu_yesm1300am_pmbus_info_get(index, "psu_v_in", &val) == 0) {
        info->mvin = val;
        info->caps |= ONLP_PSU_CAPS_VIN;
    }

    if (psu_yesm1300am_pmbus_info_get(index, "psu_i_out", &val) == 0) {
        info->miout = val;
        info->caps |= ONLP_PSU_CAPS_IOUT;
    }
    
    if (psu_yesm1300am_pmbus_info_get(index, "psu_i_in", &val) == 0) {
        info->miin = val;
        info->caps |= ONLP_PSU_CAPS_IIN;
    }

    if (psu_yesm1300am_pmbus_info_get(index, "psu_p_out", &val) == 0) {
        info->mpout = val;
        info->caps |= ONLP_PSU_CAPS_POUT;
    }   

    if (psu_yesm1300am_pmbus_info_get(index, "psu_p_in", &val) == 0) {
        info->mpin = val;
        info->caps |= ONLP_PSU_CAPS_PIN;
    }  

    return ONLP_STATUS_OK;
}

/*
 * Get all information about the given PSU oid.
 */
static onlp_psu_info_t pinfo[] =
{
    { }, /* Not used */
    {    /* PSU-1 is on i2c channel 9 for snj61d0*/
        { ONLP_PSU_ID_CREATE(PSU1_ID), "PSU-1", 0 },
    },
    {    /* PSU-2 is on i2c channel 10 for snj61d0*/
        { ONLP_PSU_ID_CREATE(PSU2_ID), "PSU-2", 0 },
    }
};


int
onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* info)
{
    int val   = 0;
    int ret   = ONLP_STATUS_OK;
    int index = ONLP_OID_ID_GET(id);
    psu_type_t psu_type;
    char *prefix = NULL;

    VALIDATE(id);

    memset(info, 0, sizeof(onlp_psu_info_t));
    *info = pinfo[index]; /* Set the onlp_oid_hdr_t */

    /* Get PSU present status */
    switch(index)
    {
    case PSU1_ID:
        prefix = PSU1_AC_HWMON_PREFIX;
        if (onlp_file_read_int(&val, "%s%s", prefix, "psu1_present") < 0) 
        {
            AIM_LOG_ERROR("Unable to read present status from PSU(%d)\r\n", index);
            return ONLP_STATUS_E_INTERNAL;
        }
        break;
    case PSU2_ID:
        prefix = PSU2_AC_HWMON_PREFIX;
        if (onlp_file_read_int(&val, "%s%s", prefix, "psu2_present") < 0) 
        {
            AIM_LOG_ERROR("Unable to read present status from PSU(%d)\r\n", index);
            return ONLP_STATUS_E_INTERNAL;
        }
        break;
    default:
        break;
    }

    if (val != PSU_STATUS_PRESENT) {
        info->status &= ~ONLP_PSU_STATUS_PRESENT;
        return ONLP_STATUS_OK;
    }

    info->status |= ONLP_PSU_STATUS_PRESENT;

    /* Get PSU power good status */
    switch(index)
    {
    case PSU1_ID:
        prefix = PSU1_AC_HWMON_PREFIX;
        if (onlp_file_read_int(&val, "%s%s", prefix, "psu1_power_good") < 0) 
        {
            AIM_LOG_ERROR("Unable to read power status from PSU(%d)\r\n", index);
            return ONLP_STATUS_E_INTERNAL;
        }
        break;
    case PSU2_ID:
        prefix = PSU2_AC_HWMON_PREFIX;
        if (onlp_file_read_int(&val, "%s%s", prefix, "psu2_power_good") < 0) 
        {
            AIM_LOG_ERROR("Unable to read power status from PSU(%d)\r\n", index);
            return ONLP_STATUS_E_INTERNAL;
        }
        break;
    default:
        break;
    }

    if (val != PSU_STATUS_POWER_GOOD) 
    {
        info->status |=  ONLP_PSU_STATUS_FAILED;
    }

    /* Get PSU type
     */
    psu_type = psu_type_get(index, info->model, sizeof(info->model));
    switch (psu_type) {
    case PSU_TYPE_AC_F2B:
    case PSU_TYPE_AC_B2F:
        ret = psu_yesm1300am_info_get(info);
        break;
    case PSU_TYPE_UNKNOWN:  /* User insert a unknown PSU or unplugged.*/
        info->status |= ONLP_PSU_STATUS_UNPLUGGED;
        info->status &= ~ONLP_PSU_STATUS_FAILED;
        ret = ONLP_STATUS_OK;
        break;
    default:
        ret = ONLP_STATUS_E_UNSUPPORTED;
        break;
    }

    return ret;
}

