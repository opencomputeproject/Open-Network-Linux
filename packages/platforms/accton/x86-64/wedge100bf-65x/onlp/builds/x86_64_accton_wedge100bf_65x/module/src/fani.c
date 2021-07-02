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

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_FAN(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

#define MAX_FAN_SPEED    15400
#define BIT(i)            (1 << (i))

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
};

#define FAN_BOARD_PATH    "/sys/bus/i2c/devices/8-0033/"

#define FAN_BOARD_PATH_2  "/sys/bus/i2c/devices/9-0033/"

#define FETCH_FAN_PATH(x) (x <= 5) ? FAN_BOARD_PATH : FAN_BOARD_PATH_2

#define FETCH_FAN_ID(x)   ((x <= 5) ? x : (x-5) )

#define CHASSIS_FAN_INFO(fid)        \
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
    CHASSIS_FAN_INFO(5),
    CHASSIS_FAN_INFO(6),
    CHASSIS_FAN_INFO(7),
    CHASSIS_FAN_INFO(8),
    CHASSIS_FAN_INFO(9),
    CHASSIS_FAN_INFO(10)
};

/*
 * This function will be called prior to all of onlp_fani_* functions.
 */
int
onlp_fani_init(void)
{
    return bmc_tty_init();
}

int
onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* info)
{
    int  value = 0, fid;
    char path[64] = {0};
    VALIDATE(id);

    fid = ONLP_OID_ID_GET(id);
    *info = finfo[fid];

    /* get fan present status
     */
    sprintf(path, "%s""fantray_present", FETCH_FAN_PATH(fid));

    if (bmc_file_read_int(&value, path, 16) < 0) {
        AIM_LOG_ERROR("Unable to read status from file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (value & BIT(fid-1)) {
        return ONLP_STATUS_OK;
    }
    info->status |= ONLP_FAN_STATUS_PRESENT;


    /* get front fan rpm
     */
    sprintf(path, "%s""fan%d_input", FETCH_FAN_PATH(fid), FETCH_FAN_ID(fid)*2 - 1);

    if (bmc_file_read_int(&value, path, 10) < 0) {
        AIM_LOG_ERROR("Unable to read status from file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }    
    info->rpm = value;

    /* get rear fan rpm
     */
    sprintf(path, "%s""fan%d_input", FETCH_FAN_PATH(fid),  FETCH_FAN_ID(fid)*2);

    if (bmc_file_read_int(&value, path, 10) < 0) {
        AIM_LOG_ERROR("Unable to read status from file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

    /* take the min value from front/rear fan speed
     */
    if (info->rpm > value) {
        info->rpm = value;
    }


    /* set fan status based on rpm
     */
    if (!info->rpm) {
        info->status |= ONLP_FAN_STATUS_FAILED;
        return ONLP_STATUS_OK;
    }


    /* get speed percentage from rpm 
     */
    info->percentage = (info->rpm * 100)/MAX_FAN_SPEED;

    /* set fan direction
     */
    info->status |= ONLP_FAN_STATUS_F2B;

    return ONLP_STATUS_OK;
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
    char cmd[32] = {0};

    sprintf(cmd, "set_fan_speed.sh %d", p);

    if (bmc_send_command(cmd) < 0) {
        AIM_LOG_ERROR("Unable to send command to bmc(%s)\r\n", cmd);
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


