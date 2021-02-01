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
};

#define MAX_PSU_FAN_SPEED 22000

#define CHASSIS_FAN_INFO(fid)		\
    { \
        { ONLP_FAN_ID_CREATE(FAN_##fid##_ON_FAN_BOARD), "Chassis Fan - "#fid, 0 },\
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
        return ONLP_STATUS_E_INTERNAL;
    }

	if (value == 0) {
		return ONLP_STATUS_OK;
	}
	info->status |= ONLP_FAN_STATUS_PRESENT;


    /* get fan fault status (turn on when any one fails)
     */
	ret = onlp_file_read_int(&value, "%s""fan%d_fault", FAN_BOARD_PATH, fid);
    if (ret < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (value > 0) {
        info->status |= ONLP_FAN_STATUS_FAILED;
    }


    /* get fan speed
     */
	ret = onlp_file_read_int(&value, "%s""fan%d_input", FAN_BOARD_PATH, fid);
    if (ret < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }
	info->rpm = value;


    /* get speed percentage from rpm
	 */
	ret = onlp_file_read_int(&value, "%s""fan_max_speed_rpm", FAN_BOARD_PATH);
    if (ret < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    info->percentage = (info->rpm * 100)/value;

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
        case FAN_1_ON_FAN_BOARD:
        case FAN_2_ON_FAN_BOARD:
        case FAN_3_ON_FAN_BOARD:
        case FAN_4_ON_FAN_BOARD:
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

    if (onlp_file_write_int(p, "%s""fan%d_duty_percentage", FAN_BOARD_PATH, fid) != 0) {
        AIM_LOG_ERROR("Unable to change duty cycle of fan (%d)\r\n", fid);
        return ONLP_STATUS_E_INTERNAL;
    }

	return ONLP_STATUS_OK;
}
