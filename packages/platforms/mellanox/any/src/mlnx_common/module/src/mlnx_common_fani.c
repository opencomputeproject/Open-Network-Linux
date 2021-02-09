/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
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
#include <fcntl.h>
#include <onlplib/file.h>
#include <onlplib/mmap.h>
#include <onlp/platformi/fani.h>
#include <onlp/fan.h>
#include <onlp/psu.h>
#include "mlnx_common/mlnx_common.h"
#include "mlnx_common_log.h"

#define FAN_STATUS_OK  1

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_FAN(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)


int onlp_fani_get_min_rpm(int id);

static int
_onlp_fani_read_fan_eeprom(int local_id, onlp_fan_info_t* info)
{
    const char sanity_checker[] = "MLNX";
    const uint8_t sanity_offset = 8;
    const uint8_t sanity_len    = 4;
    const uint8_t block1_start  = 12;
    const uint8_t block1_type   = 1;
    const uint8_t block2_start  = 14;
    const uint8_t block2_type   = 5;
    const uint8_t serial_offset = 8;
    const uint8_t serial_len    = 24;
    const uint8_t part_len      = 20;
    const uint8_t fan_offset    = 14;
    const uint8_t multiplier    = 16;
    uint8_t data[256] = {0};
    uint8_t offset = 0;
    uint8_t temp   = 0;
    int rv  = 0;
    int len = 0;

    /* We have 4 FRU with 2 fans(total 8 fans).
       Eeprom is per FRU but not per fan.
       So, need to convert fan ID to FRU ID.*/
    if (local_id % 2) {
        local_id = local_id / 2 + 1;
    } else {
        local_id /= 2;
    }

    rv = onlp_file_read(data, sizeof(data), &len,
                        IDPROM_PATH, "fan", local_id);
    if (rv < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Sanity checker */
    if (strncmp(sanity_checker, (char*)&data[sanity_offset], sanity_len)) {
        return ONLP_STATUS_E_INVALID;
    }

    /* Checking eeprom block type with S/N and P/N */
    if (data[block1_start + 1] != block1_type) {
        return ONLP_STATUS_E_INVALID;
    }

    /* Reading serial number */
    offset = data[block1_start] * multiplier + serial_offset;
    aim_strlcpy(info->serial, (char *)&data[offset], serial_len);

    /* Reading part number */
    offset += serial_len;
    aim_strlcpy(info->model, (char *)&data[offset], part_len);

    /* Reading fan direction */
    if (data[block2_start + 1] != block2_type) {
        return ONLP_STATUS_E_INVALID;
    }
    offset = data[block2_start] * multiplier + fan_offset;
    temp = data[offset];
    switch (temp) {
    case 1:
        info->caps |= ONLP_FAN_CAPS_F2B;
        break;
    case 2:
        info->caps |= ONLP_FAN_CAPS_B2F;
        break;
    default:
        break;
    }

    return ONLP_STATUS_OK;
}

static int
_onlp_fani_info_get_fan(int local_id, onlp_fan_info_t* info)
{
    int r_val, ret;
    float range = 0;
    float temp  = 0;
    int fru_index = 0;
    const char fan_model[]=FAN_MODEL;
    mlnx_platform_info_t* mlnx_platform_info = get_platform_info();
    if(mlnx_platform_info->fan_type == FAN_TYPE_NO_EEPROM)
        aim_strlcpy(info->model, fan_model, sizeof(info->model));

    if(!mlnx_platform_info->fan_fixed) {
      /* not fixed FAN's can have more than 1 FAN per FRU.
         EEPROM is per FRU but not per FAN.
         So, need to convert fan ID to FRU ID.*/
        if (mlnx_platform_info->fan_per_module == 0) {
            info->status |= ONLP_FAN_STATUS_FAILED;
            return ONLP_STATUS_OK;
        }

        /* get fan status */
        fru_index = (local_id + mlnx_platform_info->fan_per_module -1) / mlnx_platform_info->fan_per_module;

        ret = onlp_file_read_int(&r_val, "%s/%s", THERMAL_PATH, mlnx_platform_info->fan_fnames[fru_index].status);
        if (ret < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }
        if (r_val != FAN_STATUS_OK) {
            if(mlnx_platform_info->fan_type == FAN_TYPE_EEPROM)
                info->status &= ~ONLP_FAN_STATUS_PRESENT;
            return ONLP_STATUS_OK;
        }
    }
    /* Fixed system FAN is always present */
    info->status |= ONLP_FAN_STATUS_PRESENT;

    /* get fan speed */
    ret = onlp_file_read_int(&r_val, "%s/%s", THERMAL_PATH, mlnx_platform_info->fan_fnames[local_id].r_speed_get);
    if (ret < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }
    info->rpm = r_val;

    /* check failure */
    if (info->rpm <= 0) {
        info->status |= ONLP_FAN_STATUS_FAILED;
        return ONLP_STATUS_OK;
    }

    if (ONLP_FAN_CAPS_GET_PERCENTAGE & info->caps) {
        /* get fan min speed */
        ret = onlp_file_read_int(&r_val, "%s/%s", THERMAL_PATH, mlnx_platform_info->fan_fnames[local_id].min);
        if (ret < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }
        mlnx_platform_info->min_fan_speed[local_id] = r_val;

        /* get fan max speed */
        ret = onlp_file_read_int(&r_val, "%s/%s", THERMAL_PATH, mlnx_platform_info->fan_fnames[local_id].max);
        if (ret < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }
        mlnx_platform_info->max_fan_speed[local_id] = r_val;

        /* get speed percentage from rpm */
        range = mlnx_platform_info->max_fan_speed[local_id] - mlnx_platform_info->min_fan_speed[local_id];
        if (range > 0) {
            temp = ((float)info->rpm - (float)mlnx_platform_info->min_fan_speed[local_id]) / range * 40.0 + 60.0;
            if (temp < PERCENTAGE_MIN) {
                temp = PERCENTAGE_MIN;
            }
            info->percentage = (int)temp;
        } else {
            return ONLP_STATUS_E_INTERNAL;
        }
    }
    if(mlnx_platform_info->fan_type == FAN_TYPE_NO_EEPROM)
        return ONLP_STATUS_OK;
    else
        return _onlp_fani_read_fan_eeprom(local_id, info);

}


 static int
_onlp_fani_info_get_fan_on_psu(int local_id, int psu_id, onlp_fan_info_t* info)
{
    int   r_val, ret;
    float rpms_per_perc = 0.0;
    float temp = 0.0;
    mlnx_platform_info_t* mlnx_platform_info = get_platform_info();
    /* get fan status
    */
    ret = onlp_file_read_int(&r_val, "%s/%s", THERMAL_PATH, mlnx_platform_info->fan_fnames[local_id].status);
    if (ret < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (r_val != FAN_STATUS_OK) {
        if(mlnx_platform_info->fan_type == FAN_TYPE_EEPROM)
            info->status &= ~ONLP_FAN_STATUS_PRESENT;
        return ONLP_STATUS_OK;
    }
    info->status |= ONLP_FAN_STATUS_PRESENT;

    /* get fan speed
    */
    ret = onlp_file_read_int(&r_val, "%s/%s", THERMAL_PATH, mlnx_platform_info->fan_fnames[local_id].r_speed_get);
    if (ret < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }
    info->rpm = r_val;

    /* check failure */
    if (info->rpm <= 0) {
      info->status |= ONLP_FAN_STATUS_FAILED;
      return ONLP_STATUS_OK;
    }

    /* get speed percentage from rpm */
    rpms_per_perc = PSU_FAN_RPM_MIN / PERCENTAGE_MIN;
    temp = (float)info->rpm / rpms_per_perc;
    if (temp < PERCENTAGE_MIN) {
        temp = PERCENTAGE_MIN;
    }
    info->percentage = (int)temp;

    if (0 != psu_fan_read_eeprom_vpd((local_id-mlnx_platform_info->first_psu_fan_id)+1, info))
        return ONLP_STATUS_E_INTERNAL;

    return ONLP_STATUS_OK;
}


int
onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* info)
{
    int rc = 0;
    int local_id = 0;
    VALIDATE(id);
    mlnx_platform_info_t* mlnx_platform_info = get_platform_info();
    local_id = ONLP_OID_ID_GET(id);

    *info = mlnx_platform_info->finfo[local_id];

    if(local_id<mlnx_platform_info->first_psu_fan_id)
      rc =_onlp_fani_info_get_fan(local_id, info);
    else
      rc = _onlp_fani_info_get_fan_on_psu(local_id, (local_id-mlnx_platform_info->first_psu_fan_id)+1, info);
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
    float temp = 0.0;
    int   rv = 0, local_id = 0, nbytes = 10;
    char  r_data[10]   = {0};
    onlp_fan_info_t* info = NULL;

    mlnx_platform_info_t* mlnx_platform_info = get_platform_info();
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);
    info = &mlnx_platform_info->finfo[local_id];

    if (0 == (ONLP_FAN_CAPS_SET_RPM & info->caps)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    /* reject rpm=0% (rpm=0%, stop fan) */
    if (0 == rpm) {
        return ONLP_STATUS_E_INVALID;
    }

    /* Set fan speed
       Converting percent to driver value.
       Driver accept value in range between 153 and 255.
       Value 153 is minimum rpm.
       Value 255 is maximum rpm.
    */
    if (local_id > mlnx_platform_info->fan_num) {
        return ONLP_STATUS_E_INTERNAL;
    }
    if (mlnx_platform_info->max_fan_speed[local_id] - mlnx_platform_info->min_fan_speed[local_id] < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }
    if (rpm < mlnx_platform_info->min_fan_speed[local_id] || rpm > mlnx_platform_info->max_fan_speed[local_id]) {
        return ONLP_STATUS_E_PARAM;
    }

    temp = (rpm - mlnx_platform_info->min_fan_speed[local_id]) * (RPM_MAGIC_MAX - RPM_MAGIC_MIN) /
           (mlnx_platform_info->max_fan_speed[local_id] - mlnx_platform_info->min_fan_speed[local_id]) + RPM_MAGIC_MIN;

    snprintf(r_data, sizeof(r_data), "%d", (int)temp);
    nbytes = strnlen(r_data, sizeof(r_data));
    rv = onlp_file_write((uint8_t*)r_data, nbytes, "%s/%s", THERMAL_PATH,
                         mlnx_platform_info->fan_fnames[local_id].r_speed_set);
    if (rv < 0) {
        return ONLP_STATUS_E_INTERNAL;
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
    float temp = 0.0;
    int   rv = 0, local_id = 0, nbytes = 10;
    char  r_data[10]   = {0};
    onlp_fan_info_t* info = NULL;
    mlnx_platform_info_t* mlnx_platform_info = get_platform_info();
    VALIDATE(id);
    local_id = ONLP_OID_ID_GET(id);
    info = &mlnx_platform_info->finfo[local_id];

    if (0 == (ONLP_FAN_CAPS_SET_PERCENTAGE & info->caps)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    /* reject p=0% (p=0%, stop fan) */
    if (0 == p) {
        return ONLP_STATUS_E_INVALID;
    }

    if (p < PERCENTAGE_MIN || p > PERCENTAGE_MAX) {
        return ONLP_STATUS_E_PARAM;
    }

    /* Set fan speed
       Converting percent to driver value.
       Driver accept value in range between 153 and 255.
       Value 153 is 60%.
       Value 255 is 100%.
    */
    temp = (p - PERCENTAGE_MIN) * (RPM_MAGIC_MAX - RPM_MAGIC_MIN) /
           (PERCENTAGE_MAX - PERCENTAGE_MIN) + RPM_MAGIC_MIN;

    snprintf(r_data, sizeof(r_data), "%d", (int)temp);
    nbytes = strnlen(r_data, sizeof(r_data));
    rv = onlp_file_write((uint8_t*)r_data, nbytes, "%s/%s", THERMAL_PATH,
                         mlnx_platform_info->fan_fnames[local_id].r_speed_set);
    if (rv < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
onlp_fani_get_min_rpm(int id)
{
    int r_val;

    mlnx_platform_info_t* mlnx_platform_info = get_platform_info();

    if (onlp_file_read_int(&r_val, "%s/%s", THERMAL_PATH, mlnx_platform_info->fan_fnames[id].min) < 0)
        return ONLP_STATUS_E_INTERNAL;

    return r_val;
}
