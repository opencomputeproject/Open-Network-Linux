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
#include <onlp/platformi/fani.h>
#include "platform_lib.h"

#define PSU_PREFIX_PATH  "/sys/bus/i2c/devices/"

#define MAX_FAN_SPEED     25500
#define MAX_PSU_FAN_SPEED 25500

enum fan_id {
	FAN_1_ON_FAN_BOARD = 1,
	FAN_2_ON_FAN_BOARD,
	FAN_3_ON_FAN_BOARD,
	FAN_4_ON_FAN_BOARD,
	FAN_5_ON_FAN_BOARD,
	FAN_6_ON_FAN_BOARD,
	FAN_1_ON_PSU_1,
	FAN_1_ON_PSU_2,
};

#define CHASSIS_FAN_INFO(fid) \
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
	PSU_FAN_INFO(1,1),
	PSU_FAN_INFO(2,1)
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
	char  path[64] = {0};

	/* get fan present status
	 */
	sprintf(path, "%s""fan%d_present", FAN_BOARD_PATH, fid);
	DEBUG_PRINT("Fan(%d), present path = (%s)", fid, path);
	
    if (onlp_file_read_int(&value, path) < 0) {
        AIM_LOG_ERROR("Unable to read status from file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

	if (value == 0) {
		return ONLP_STATUS_OK;
	}
	info->status |= ONLP_FAN_STATUS_PRESENT;


    /* get fan fault status (turn on when any one fails)
     */
	sprintf(path, "%s""fan%d_fault", FAN_BOARD_PATH, fid);
	DEBUG_PRINT("Fan(%d), fault path = (%s)", fid, path);
	
    if (onlp_file_read_int(&value, path) < 0) {
        AIM_LOG_ERROR("Unable to read status from file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (value > 0) {
        info->status |= ONLP_FAN_STATUS_FAILED;
    }


    /* get fan direction (both : the same)
     */
	sprintf(path, "%s""fan%d_direction", FAN_BOARD_PATH, fid);
	DEBUG_PRINT("Fan(%d), direction path = (%s)", fid, path);
	
    if (onlp_file_read_int(&value, path) < 0) {
        AIM_LOG_ERROR("Unable to read status from file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

    /* F2B has register value 0 as per x86-64-accton-as7326-56x-fan.c */
    info->status |= value ? ONLP_FAN_STATUS_B2F : ONLP_FAN_STATUS_F2B;


    /* get front fan speed
     */
	sprintf(path, "%s""fan%d_front_speed_rpm", FAN_BOARD_PATH, fid);
	DEBUG_PRINT("Fan (%d), front speed path = (%s)", fid, path);

    if (onlp_file_read_int(&value, path) < 0) {
        AIM_LOG_ERROR("Unable to read status from file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }
	info->rpm = value;

	/* get rear fan speed
	 */
	sprintf(path, "%s""fan%d_rear_speed_rpm", FAN_BOARD_PATH, fid);
	DEBUG_PRINT("Fan (%d), rear speed path = (%s)", fid, path);

    if (onlp_file_read_int(&value, path) < 0) {
        AIM_LOG_ERROR("Unable to read status from file (%s)\r\n", path);
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
    /* Try to read direction from PSU1.
     * If PSU1 is not valid, read from PSU2
     */
    int i = 0;

    for (i = PSU1_ID; i <= PSU2_ID; i++) {
        psu_type_t psu_type;
        psu_type = get_psu_type(i, NULL, 0);

        if (psu_type == PSU_TYPE_UNKNOWN) {
            continue;
        }

        switch (psu_type) {
            case PSU_TYPE_AC_F2B_3YPOWER:
            case PSU_TYPE_AC_F2B_ACBEL:
            case PSU_TYPE_DC_48V_F2B:
                return ONLP_FAN_STATUS_F2B;
            case PSU_TYPE_AC_B2F_3YPOWER:
            case PSU_TYPE_AC_B2F_ACBEL:
            case PSU_TYPE_DC_48V_B2F:
                return ONLP_FAN_STATUS_B2F;
            default:
                return 0;
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
    if (psu_pmbus_info_get(pid, "psu_fan1_fault", &val) == ONLP_STATUS_OK) {
        info->status |= (val > 0) ? ONLP_FAN_STATUS_FAILED : 0;
    }

    /* get fan speed
     */
    if (psu_pmbus_info_get(pid, "psu_fan1_speed_rpm", &val) == ONLP_STATUS_OK) {
        info->rpm = val;
	    info->percentage = (info->rpm * 100) / MAX_PSU_FAN_SPEED;	    
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
            rc =_onlp_fani_info_get_fan(fid, info);						
            break;
        default:
            rc = ONLP_STATUS_E_INVALID;
            break;
    }	
    
    return rc;
}

/*
 * This function sets the speed of the given fan in RPM.
 *
 * This function will only be called if the fan supprots the RPM_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
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
onlp_fani_percentage_set(onlp_oid_t id, int p)
{
    int  fid;
    char *path = NULL;

    VALIDATE(id);

    fid = ONLP_OID_ID_GET(id);

    /* reject p=0 (p=0, stop fan) */
    if (p == 0){
        return ONLP_STATUS_E_INVALID;
    }

    switch (fid)
	{
        case FAN_1_ON_PSU_1:
            return psu_pmbus_info_set(PSU1_ID, "psu_fan_duty_cycle_percentage", p);
        case FAN_1_ON_PSU_2:
            return psu_pmbus_info_set(PSU2_ID, "psu_fan_duty_cycle_percentage", p);
        case FAN_1_ON_FAN_BOARD:
        case FAN_2_ON_FAN_BOARD:
        case FAN_3_ON_FAN_BOARD:
        case FAN_4_ON_FAN_BOARD:
        case FAN_5_ON_FAN_BOARD:
        case FAN_6_ON_FAN_BOARD:
			path = FAN_NODE(fan_duty_cycle_percentage);
			break;
        default:
            return ONLP_STATUS_E_INVALID;
    }

	DEBUG_PRINT("Fan path = (%s)", path);
	
    if (onlp_file_write_integer(path, p) < 0) {
        AIM_LOG_ERROR("Unable to write data to file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

	return ONLP_STATUS_OK;
}


/*
 * This function sets the fan speed of the given OID as per
 * the predefined ONLP fan speed modes: off, slow, normal, fast, max.
 *
 * Interpretation of these modes is up to the platform.
 *
 */
int
onlp_fani_mode_set(onlp_oid_t id, onlp_fan_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function sets the fan direction of the given OID.
 *
 * This function is only relevant if the fan OID supports both direction
 * capabilities.
 *
 * This function is optional unless the functionality is available.
 */
int
onlp_fani_dir_set(onlp_oid_t id, onlp_fan_dir_t dir)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * Generic fan ioctl. Optional.
 */
int
onlp_fani_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

