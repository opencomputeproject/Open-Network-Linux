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
 *
 *
 ***********************************************************/
#include <onlp/platformi/psui.h>
#include <onlplib/file.h>
#include "platform_lib.h"



//#define PSU_EEPROM_PATH  "/sys/bus/i2c/devices/%d-00%02x/"

#define PSU_STATUS_PRESENT     1
#define PSU_STATUS_POWER_GOOD  1

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_PSU(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

static int eeprom_cfg[CHASSIS_PSU_COUNT][2] = {{13, 0x53}, {12, 0x50}};
static int pmbus_cfg[CHASSIS_PSU_COUNT][2] = {{13, 0x5b}, {12, 0x58}};

int
onlp_psui_init(void)
{
    return ONLP_STATUS_OK;
}

/*
 * Get all information about the given PSU oid.
 */
static onlp_psu_info_t pinfo[] =
{
    { }, /* Not used */
    {
        { ONLP_PSU_ID_CREATE(PSU1_ID), "PSU-1", 0 },
    },
    {
        { ONLP_PSU_ID_CREATE(PSU2_ID), "PSU-2", 0 },
    }
};

static int
get_DCorAC_cap(char *model)
{
    const char *dc_models[] = {"YM-2401J", "YM-1401A", NULL };
    int i;

    i = 0;
    while(dc_models[i]) {
        if (!strncasecmp(model, dc_models[i], strlen(dc_models[i]))) {
            return ONLP_PSU_CAPS_DC12;
        }
        i++;
    }
    return ONLP_PSU_CAPS_AC;
}

int
onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* info)
{
    int bus, offset, len;
    int val   = 0;
    int ret   = ONLP_STATUS_OK;
    int pid = ONLP_OID_ID_GET(id);
    int zid = pid - 1 ;   /*Turn to 0-base*/
    char *string;

    VALIDATE(id);
    if (pid >= AIM_ARRAYSIZE(pinfo) || pid == 0) {
        return ONLP_STATUS_E_INVALID;
    }

    memset(info, 0, sizeof(onlp_psu_info_t));
    *info = pinfo[pid]; /* Set the onlp_oid_hdr_t */

    bus = eeprom_cfg[zid][0];
    offset = eeprom_cfg[zid][1];
    /* Get the present state */
    ret = onlp_file_read_int(&val, PSU_SYSFS_PATH"psu_present", bus, offset);
    if (ret < 0) {
        AIM_LOG_ERROR("Unable to read status from (%s""psu%d_present)\r\n", PSU_SYSFS_PATH, pid);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (val != PSU_STATUS_PRESENT) {
        info->status &= ~ONLP_PSU_STATUS_PRESENT;
        return ONLP_STATUS_OK;
    }
    info->status |= ONLP_PSU_STATUS_PRESENT;

    /* Get power good status */
    ret = onlp_file_read_int(&val, PSU_SYSFS_PATH"psu_power_good", bus, offset);
    if (ret < 0) {
        AIM_LOG_ERROR("Unable to read status from (%s""psu%d_power_good)\r\n", PSU_SYSFS_PATH, pid);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (val != PSU_STATUS_POWER_GOOD) {
        info->status |=  ONLP_PSU_STATUS_FAILED;
        return ONLP_STATUS_OK;
    }

    if (info->status & ONLP_PSU_STATUS_FAILED) {
        return ONLP_STATUS_OK;
    }

    /* Read serial */
    string = NULL;
    len = onlp_file_read_str(&string, PSU_SYSFS_PATH"psu_serial", bus, offset);
    if (string && len) {
        strncpy(info->serial, string, len);
    }

    if (string) {
        aim_free(string);
        string = NULL;
    }
    bus = pmbus_cfg[zid][0];
    offset = pmbus_cfg[zid][1];

    /* Read voltage, current and power */
    val = 0;
    if (onlp_file_read_int(&val, PSU_SYSFS_PATH"psu_v_out", bus, offset) == 0 && val) {
        info->mvout = val;
        info->caps |= ONLP_PSU_CAPS_VOUT;
    }

    val = 0;
    if (onlp_file_read_int(&val, PSU_SYSFS_PATH"psu_i_out", bus, offset) == 0 && val) {
        info->miout = val;
        info->caps |= ONLP_PSU_CAPS_IOUT;
    }

    val = 0;
    if (onlp_file_read_int(&val, PSU_SYSFS_PATH"psu_p_out", bus, offset) == 0 && val) {
        info->mpout = val;
        info->caps |= ONLP_PSU_CAPS_POUT;
    }

    /* Read model */
    string = NULL;
    len = onlp_file_read_str(&string, PSU_SYSFS_PATH"psu_mfr_model", bus, offset);
    if (string && len) {
        strncpy(info->model, string, len);
        info->caps |= get_DCorAC_cap (info->model);
    }

    if (string) {
        aim_free(string);
        string = NULL;
    }

    /* Set the associated oid_table */
    val = 0;
    if (onlp_file_read_int(&val, PSU_SYSFS_PATH"psu_temp1_input", bus, offset) == 0 && val) {
        info->hdr.coids[0] = ONLP_THERMAL_ID_CREATE(pid + CHASSIS_THERMAL_COUNT);
    }

    return ret;
}

