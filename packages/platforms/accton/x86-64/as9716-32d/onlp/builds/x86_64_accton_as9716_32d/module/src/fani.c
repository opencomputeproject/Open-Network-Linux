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
 ***********************************************************/
#include <onlp/platformi/base.h>
#include <onlp/platformi/fani.h>
#include "platform_lib.h"
#include <unistd.h>
#include <onlplib/file.h>
#include <fcntl.h>
#include "x86_64_accton_as9716_32d/x86_64_accton_as9716_32d_config.h"

#define SYS_I2C_DEVICES  "/sys/bus/i2c/devices/"
#define SYSFS_FAN_DIR     "/sys/bus/i2c/devices/17-0066"

#define MAX_FAN_SPEED     25500
#define MAX_PSU_FAN_SPEED 25500

enum fan_id {
    FAN_1_ON_FAN_BOARD = 1,
    FAN_2_ON_FAN_BOARD,
    FAN_3_ON_FAN_BOARD,
    FAN_4_ON_FAN_BOARD,
    FAN_5_ON_FAN_BOARD,
    FAN_6_ON_FAN_BOARD,
    FAN_1_ON_PSU1,
    FAN_1_ON_PSU2,
};

#define MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(id) \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##id##_ON_FAN_BOARD), "Fan "#id, ONLP_OID_CHASSIS }, \
        0x0,\
        ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE,\
        0,\
        0,\
    }

#define MAKE_FAN_INFO_NODE_ON_PSU(psu_id, fan_id) \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##fan_id##_ON_PSU##psu_id), "PSU-"#psu_id " Fan "#fan_id, ONLP_PSU_ID_CREATE(psu_id) }, \
        0x0,\
        0,\
        0,\
        0, \
    }

/* Static fan information */
onlp_fan_info_t fan_info[] = {
    { }, /* Not used */
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(1),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(2),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(3),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(4),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(5),
    MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(6),
    MAKE_FAN_INFO_NODE_ON_PSU(1,1),
    MAKE_FAN_INFO_NODE_ON_PSU(2,1),
};


/* PSU relative marco */
#define SET_PSU_TYPE_AC_F2B_FAN(info)                                   \
    do {                                                                \
        ONLP_OID_STATUS_FLAG_SET(info, PRESENT);                        \
        info->dir = ONLP_FAN_DIR_F2B;                                   \
        info->caps  |=  ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE; \
    } while(0)

#define SET_PSU_TYPE_AC_B2F_FAN(info)                                   \
    do {                                                                \
        ONLP_OID_STATUS_FLAG_SET(info, PRESENT);                        \
        info->dir = ONLP_FAN_DIR_B2F;                                   \
        info->caps |=  ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE; \
    } while(0)

static int
chassis_fani_hdr_get__(int id, onlp_oid_hdr_t* hdr)
{
    int v;

    *hdr = fan_info[id].hdr;
    ONLP_TRY(onlp_file_read_int(&v, "%s/fan%d_present",
                                SYSFS_FAN_DIR, id));
    if(v) {
        /* Fan is present */
        ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
        ONLP_TRY(onlp_file_read_int(&v, "%s/fan%d_fault",
                                    SYSFS_FAN_DIR, id));
        if(v) {
            /* Fan has failed. */
            ONLP_OID_STATUS_FLAG_SET(hdr, FAILED);
        }
    }
    return 0;
}

static int
psu_fani_hdr_get__(int id, const char* device, onlp_oid_hdr_t* hdr)
{
    int v;
    int rv;
    *hdr = fan_info[id].hdr;
   
    if(ONLP_SUCCESS(rv = onlp_file_read_int(&v, "%s/psu_fan1_fault", device))) {
        ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
        if(v) {
            ONLP_OID_STATUS_FLAG_SET(hdr, FAILED);
        }
    }
    return rv;
}

int
onlp_fani_hdr_get(onlp_oid_t oid, onlp_oid_hdr_t* hdr)
{
    int id = ONLP_OID_ID_GET(oid);
    
    switch(id)
    {
        case FAN_1_ON_PSU1:
            return psu_fani_hdr_get__(id, PSU1_AC_PMBUS_PREFIX, hdr);
        case FAN_1_ON_PSU2:
            return psu_fani_hdr_get__(id, PSU2_AC_PMBUS_PREFIX, hdr);
        case FAN_1_ON_FAN_BOARD:
        case FAN_2_ON_FAN_BOARD:
        case FAN_3_ON_FAN_BOARD:
        case FAN_4_ON_FAN_BOARD:
        case FAN_5_ON_FAN_BOARD:
        case FAN_6_ON_FAN_BOARD:
            return chassis_fani_hdr_get__(id, hdr);
        default:
            return ONLP_STATUS_E_PARAM;
    }
}

static int
_onlp_fani_info_get_fan(int fid, onlp_fan_info_t* info)
{
    int   value;
    char  path[64] = {0};

    /* get fan present status
     */
    sprintf(path, "%s""fan%d_present", FAN_BOARD_PATH, fid);
    DEBUG_PRINT("Fan(%d), present path = (%s)", fid, path);
	
    ONLP_TRY(onlp_file_read_int(&value, path));
    if(value > 0)
        ONLP_OID_STATUS_FLAG_SET(info, PRESENT);
    else
        ONLP_OID_STATUS_FLAG_SET(info, FAILED);

    /* get fan fault status (turn on when any one fails)
     */
    sprintf(path, "%s""fan%d_fault", FAN_BOARD_PATH, fid);
    DEBUG_PRINT("Fan(%d), fault path = (%s)", fid, path);
	
    ONLP_TRY(onlp_file_read_int(&value, path));
    if (value > 0)
        ONLP_OID_STATUS_FLAG_SET(info, FAILED);

    /* get fan direction (both : the same)
     */
    sprintf(path, "%s""fan%d_direction", FAN_BOARD_PATH, fid);
    DEBUG_PRINT("Fan(%d), direction path = (%s)", fid, path);
	
    ONLP_TRY(onlp_file_read_int(&value, path));

    info->dir = value ? ONLP_FAN_DIR_F2B : ONLP_FAN_DIR_B2F;

    /* get front fan speed
     */
    sprintf(path, "%s""fan%d_front_speed_rpm", FAN_BOARD_PATH, fid);
    DEBUG_PRINT("Fan (%d), front speed path = (%s)", fid, path);

    ONLP_TRY(onlp_file_read_int(&value, path));
    info->rpm = value;

    /* get rear fan speed
     */
    sprintf(path, "%s""fan%d_rear_speed_rpm", FAN_BOARD_PATH, fid);
    DEBUG_PRINT("Fan (%d), rear speed path = (%s)", fid, path);

    ONLP_TRY(onlp_file_read_int(&value, path));
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
        return psu_type;
    }

    return 0;
}

static int
_onlp_fani_info_get_fan_on_psu(int pid, onlp_fan_info_t* info)
{
    int val = 0;
    psu_type_t psu_type;
    /* get fan direction
     */
    psu_type = _onlp_get_fan_direction_on_psu();
    switch (psu_type)
    {
        case PSU_TYPE_AC_F2B:
            SET_PSU_TYPE_AC_F2B_FAN(info);
            break;
        case PSU_TYPE_AC_B2F:
            SET_PSU_TYPE_AC_B2F_FAN(info);
            break;
        default:
            DEBUG_PRINT("[Debug][%s][%d][psu_type=%d]\n", __FUNCTION__, __LINE__, psu_type);
            break;
    }

    /* get fan fault status
     */
    ONLP_TRY(psu_ym2651y_pmbus_info_get(pid, "psu_fan1_fault", &val));
    if (val > 0)
        ONLP_OID_STATUS_FLAG_SET(info, FAILED);

    /* get fan speed
     */
    ONLP_TRY(psu_ym2651y_pmbus_info_get(pid, "psu_fan1_speed_rpm", &val));
    info->rpm = val;
    info->percentage = (info->rpm * 100) / MAX_PSU_FAN_SPEED;	    

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

    fid = ONLP_OID_ID_GET(id);
    *info = fan_info[fid];

    switch (fid)
    {
        case FAN_1_ON_PSU1:
            rc = _onlp_fani_info_get_fan_on_psu(PSU1_ID, info);
            break;
        case FAN_1_ON_PSU2:
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
onlp_fani_rpm_set(onlp_oid_id_t id, int rpm)
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
onlp_fani_percentage_set(onlp_oid_id_t id, int p)
{
    int  fid;
    char *path = NULL;

    fid = ONLP_OID_ID_GET(id);

    /* reject p=0 (p=0, stop fan) */
    if (p == 0){
        return ONLP_STATUS_E_INVALID;
    }

    switch (fid)
    {
        case FAN_1_ON_PSU1:
            return psu_ym2651y_pmbus_info_set(PSU1_ID, "psu_fan_duty_cycle_percentage", p);
        case FAN_1_ON_PSU2:
            return psu_ym2651y_pmbus_info_set(PSU2_ID, "psu_fan_duty_cycle_percentage", p);
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
 * This function sets the fan direction of the given OID.
 *
 * This function is only relevant if the fan OID supports both direction
 * capabilities.
 *
 * This function is optional unless the functionality is available.
 */
int
onlp_fani_dir_set(onlp_oid_id_t id, onlp_fan_dir_t dir)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * Generic fan ioctl. Optional.
 */
int
onlp_fani_ioctl(onlp_oid_id_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

