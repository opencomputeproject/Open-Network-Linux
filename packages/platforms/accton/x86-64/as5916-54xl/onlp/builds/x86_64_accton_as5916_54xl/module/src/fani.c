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
 * Fan Platform Implementation Defaults.
 *
 ***********************************************************/
#include <onlplib/file.h>
#include <onlp/platformi/fani.h>
#include "platform_lib.h"

enum fan_id {
	FAN_1_ON_FAN_BOARD = 1,
	FAN_2_ON_FAN_BOARD,
	FAN_3_ON_FAN_BOARD,
	FAN_4_ON_FAN_BOARD,
	FAN_5_ON_FAN_BOARD,
	FAN_6_ON_FAN_BOARD,
	FAN_7_ON_FAN_BOARD,
	FAN_8_ON_FAN_BOARD,
	FAN_9_ON_FAN_BOARD,
	FAN_10_ON_FAN_BOARD,
	FAN_11_ON_FAN_BOARD,
	FAN_12_ON_FAN_BOARD,
	FAN_1_ON_PSU_1,
	FAN_1_ON_PSU_2,
};

#define MAX_FAN_SPEED     25500
#define MAX_PSU_FAN_SPEED 25500

#define CHASSIS_FAN_INFO(fid)		\
    { \
        { ONLP_FAN_ID_CREATE(FAN_##fid##_ON_FAN_BOARD), "Chassis Fan - "#fid" Front", 0 },\
        0x0,\
        ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,\
        0,\
        0,\
        ONLP_FAN_MODE_INVALID,\
    }

#define CHASSIS_REAR_FAN_INFO(fid, tid)		\
    { \
        { ONLP_FAN_ID_CREATE(FAN_##fid##_ON_FAN_BOARD), "Chassis Fan - "#tid" Rear", 0 },\
        0x0,\
        ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,\
        0,\
        0,\
        ONLP_FAN_MODE_INVALID,\
    }

#define PSU_FAN_INFO(pid, fid) 		\
    { \
        { ONLP_FAN_ID_CREATE(FAN_##fid##_ON_PSU_##pid), "PSU "#pid" - Fan "#fid, 0 },\
        0x0,\
        ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,\
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
	CHASSIS_REAR_FAN_INFO(7, 1),
	CHASSIS_REAR_FAN_INFO(8, 2),
	CHASSIS_REAR_FAN_INFO(9, 3),
	CHASSIS_REAR_FAN_INFO(10, 4),
	CHASSIS_REAR_FAN_INFO(11, 5),
	CHASSIS_REAR_FAN_INFO(12, 6),
	PSU_FAN_INFO(1, 1),
	PSU_FAN_INFO(2, 1)
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
	int   value, ret;

	/* get fan present status
	 */
    ret = onlp_file_read_int(&value, "%s""fan%d_present", FAN_BOARD_PATH, fid);
    if (ret < 0) {
        AIM_LOG_ERROR("Unable to read status from (%s)\r\n", FAN_BOARD_PATH);
        return ONLP_STATUS_E_INTERNAL;
    }

	if (value == 0) {
		return ONLP_STATUS_OK; /* fan is not present */
	}
	info->status |= ONLP_FAN_STATUS_PRESENT;


    /* get fan direction
     */
	info->status |= ONLP_FAN_STATUS_F2B;


    /* get fan speed
     */
    ret = onlp_file_read_int(&value, "%s""fan%d_input", FAN_BOARD_PATH, fid);
    if (ret < 0) {
        AIM_LOG_ERROR("Unable to read status from (%s)\r\n", FAN_BOARD_PATH);
        return ONLP_STATUS_E_INTERNAL;
    }
	info->rpm = value;
    info->percentage = (info->rpm * 100)/MAX_FAN_SPEED;


    /* get fan fault status
     */
    if (!info->rpm) {
        info->status |= ONLP_FAN_STATUS_FAILED;
    }

	return ONLP_STATUS_OK;
}

static int
_onlp_fani_info_get_fan_on_psu(int pid, onlp_fan_info_t* info)
{
	int   value, ret;

	info->status |= ONLP_FAN_STATUS_PRESENT;

    /* get fan direction
     */
    info->status |= ONLP_FAN_STATUS_F2B;

    /* get fan speed
     */
    ret = onlp_file_read_int(&value, "%s""psu%d_fan1_input", PSU_SYSFS_PATH, pid);
    if (ret < 0) {
        AIM_LOG_ERROR("Unable to read status from (%s)\r\n", PSU_SYSFS_PATH);
        return ONLP_STATUS_E_INTERNAL;
    }

	info->rpm = value;
    info->percentage = (info->rpm * 100)/MAX_PSU_FAN_SPEED;

    /* get fan fault status
     */
    if (!info->rpm) {
        info->status |= ONLP_FAN_STATUS_FAILED;
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
        case FAN_7_ON_FAN_BOARD:
        case FAN_8_ON_FAN_BOARD:
        case FAN_9_ON_FAN_BOARD:
        case FAN_10_ON_FAN_BOARD:
        case FAN_11_ON_FAN_BOARD:
        case FAN_12_ON_FAN_BOARD:
            rc =_onlp_fani_info_get_fan(fid, info);						
            break;
        default:
            rc = ONLP_STATUS_E_INVALID;
            break;
    }	
    
    return rc;
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

    VALIDATE(id);

    fid = ONLP_OID_ID_GET(id);

    /* reject p=0 (p=0, stop fan) */
    if (p == 0){
        return ONLP_STATUS_E_INVALID;
    }

    if (fid < FAN_1_ON_FAN_BOARD || fid > FAN_12_ON_FAN_BOARD) {
        return ONLP_STATUS_E_INVALID;
    }

    if (onlp_file_write_int(p, "%s""fan%d_pwm", FAN_BOARD_PATH, fid) < 0) {
        AIM_LOG_ERROR("Unable to write data to file %s""fan%d_pwm", FAN_BOARD_PATH, fid);
        return ONLP_STATUS_E_INTERNAL;
    }

	return ONLP_STATUS_OK;
}

