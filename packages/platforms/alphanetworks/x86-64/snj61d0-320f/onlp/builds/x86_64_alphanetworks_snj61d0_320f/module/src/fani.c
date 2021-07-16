/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2017 Accton Technology Corporation.
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
 * Fan Platform Implementation Defaults.
 *
 ***********************************************************/
#include <onlp/platformi/fani.h>
#include "platform_lib.h"

#define MAX_FAN_SPEED       21000     /* FAN_FRONT_RPM_MAX = 21000 , FAN_REAR_RPM_MAX = 19000 */
#define MAX_PSU_FAN_SPEED   26592

#define CHASSIS_FAN_INFO(fid)		\
    { \
        { ONLP_FAN_ID_CREATE(FAN_##fid##_ON_FAN_BOARD), "Chassis Fan - "#fid, 0 },\
        0x0,\
        ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,\
        0,\
        0,\
        ONLP_FAN_MODE_INVALID,\
    }

#define PSU_FAN_INFO(pid, fid) \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##fid##_ON_PSU_##pid), "PSU "#pid" - Fan "#fid, 0 },\
        0x0,\
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,\
        0,\
        0,\
        ONLP_FAN_MODE_INVALID,\
    }


/* Static fan information */
onlp_fan_info_t finfo[] = {
    { }, /* Not used */
    CHASSIS_FAN_INFO(1),
    CHASSIS_FAN_INFO(2),
    CHASSIS_FAN_INFO(3),
    CHASSIS_FAN_INFO(4),
    CHASSIS_FAN_INFO(5),
    CHASSIS_FAN_INFO(6),
    PSU_FAN_INFO(1,1),
    PSU_FAN_INFO(2,1),
};

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_FAN(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

static int
_onlp_fani_info_get_fan(int fid, onlp_fan_info_t* info)
{
    int   value;

    /* get fan present status
     */
    if (onlp_file_read_int(&value, "%s""fan%d_present", FAN_BOARD_PATH, fid) < 0) {
        AIM_LOG_ERROR("Unable to read status from (%s)\r\n", FAN_BOARD_PATH);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (value == 0) {
        return ONLP_STATUS_OK; /* fan is not present */
    }
    info->status |= ONLP_FAN_STATUS_PRESENT;


    /* get fan fault status (turn on when any one fails)
     */
    if (onlp_file_read_int(&value, "%s""fan%d_fault", FAN_BOARD_PATH, fid) < 0) {
        AIM_LOG_ERROR("Unable to read status from (%s)\r\n", FAN_BOARD_PATH);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (value > 0) {
        info->status |= ONLP_FAN_STATUS_FAILED;
    }

    /* get front fan speed
     */
    if (onlp_file_read_int(&value, "%s""fan%d_input", FAN_BOARD_PATH, fid) < 0) {
        AIM_LOG_ERROR("Unable to read status from (%s)\r\n", FAN_BOARD_PATH);
        return ONLP_STATUS_E_INTERNAL;
    }
    info->rpm = value;

    /* get rear fan speed
     */
    if (onlp_file_read_int(&value, "%s""fan%d_input", FAN_BOARD_PATH, fid+10) < 0) {
        AIM_LOG_ERROR("Unable to read status from (%s)\r\n", FAN_BOARD_PATH);
        return ONLP_STATUS_E_INTERNAL;
    }

    /* take the min value from front/rear fan speed
     */
    if (info->rpm > value) {
        info->rpm = value;
    }

    /* get speed percentage from rpm
     */
    info->percentage = (info->rpm * 100)/MAX_FAN_SPEED;

    return ONLP_STATUS_OK;
}

static uint32_t
_onlp_get_fan_direction_on_psu(void)
{
    int i = 0;

    for (i = PSU1_ID; i <= PSU2_ID; i++) {
        psu_type_t psu_type;
        psu_type = psu_type_get(i, NULL, 0);

        if (psu_type == PSU_TYPE_UNKNOWN) {
            continue;
        }

        if (PSU_TYPE_AC_F2B == psu_type) {
            return ONLP_FAN_STATUS_F2B;
        }
        else {
            return ONLP_FAN_STATUS_B2F;
        }
    }

    return 0;
}

static int
_onlp_fani_info_get_fan_on_psu(int pid, onlp_fan_info_t* info)
{
    int val = 0;

    info->status |= ONLP_FAN_STATUS_PRESENT;

    /* get fan direction
     */
    info->status |= _onlp_get_fan_direction_on_psu();

    /* get fan fault status
     */
    if (psu_yesm1300am_pmbus_info_get(pid, "psu_fan1_fault", &val) == ONLP_STATUS_OK) {
        info->status |= (val > 0) ? ONLP_FAN_STATUS_FAILED : 0;
    }

    /* get fan speed
     */
    if (psu_yesm1300am_pmbus_info_get(pid, "psu_fan1_speed_rpm", &val) == ONLP_STATUS_OK) {
        info->rpm = val;
        info->percentage = (info->rpm * 100) / MAX_PSU_FAN_SPEED;
    }

    return ONLP_STATUS_OK;
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
onlp_fani_percentage_set(onlp_oid_t id, int p)
{
    int  fid;

    DIAG_PRINT("%s, id=%d", __FUNCTION__, id);

    VALIDATE(id);

    fid = ONLP_OID_ID_GET(id);

    /* reject p=0 (p=0, stop fan) */
    if (p == 0) {
        return ONLP_STATUS_E_INVALID;
    }

    switch (fid)
    {
    case FAN_1_ON_PSU_1:
        return psu_yesm1300am_pmbus_info_set(PSU1_ID, "psu_fan1_duty_cycle_percentage", p);
    case FAN_1_ON_PSU_2:
        return psu_yesm1300am_pmbus_info_set(PSU2_ID, "psu_fan1_duty_cycle_percentage", p);
    case FAN_1_ON_FAN_BOARD:
    case FAN_2_ON_FAN_BOARD:
    case FAN_3_ON_FAN_BOARD:
    case FAN_4_ON_FAN_BOARD:
    case FAN_5_ON_FAN_BOARD:
    case FAN_6_ON_FAN_BOARD:
        break;
    default:
        return ONLP_STATUS_E_INVALID;
    }

    /* Transform p to fan_pwm */
    p = (p * 255) / 100 ;
    if (onlp_file_write_int(p, "%s%s", FAN_BOARD_PATH, "fan_pwm") < 0)
    {
        AIM_LOG_ERROR("Unable to write data to file (%s%s)\r\n", FAN_BOARD_PATH, "FAN_BOARD_PATH");
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}


/*
 * This function will be called prior to all of onlp_fani_* functions.
 */
int
onlp_fani_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* info)
{
    DIAG_PRINT("%s, id=%d", __FUNCTION__, id);

    int rc = 0;
    int fid;
    VALIDATE(id);

    fid = ONLP_OID_ID_GET(id);
    *info = finfo[fid];

    switch (fid)
    {
    case FAN_1_ON_PSU_1:
        rc = _onlp_fani_info_get_fan_on_psu(PSU1_ID, info);
        break;
    case FAN_1_ON_PSU_2:
        rc = _onlp_fani_info_get_fan_on_psu(PSU2_ID, info);
        break;
    case FAN_1_ON_FAN_BOARD:
    case FAN_2_ON_FAN_BOARD:
    case FAN_3_ON_FAN_BOARD:
    case FAN_4_ON_FAN_BOARD:
    case FAN_5_ON_FAN_BOARD:
    case FAN_6_ON_FAN_BOARD:
        rc =_onlp_fani_info_get_fan(fid, info);
        break;
    default:
        rc = ONLP_STATUS_E_INVALID;
        break;
    }

    return rc;
}
