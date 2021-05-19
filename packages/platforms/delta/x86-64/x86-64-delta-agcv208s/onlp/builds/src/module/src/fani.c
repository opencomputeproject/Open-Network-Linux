/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright (C) 2019 Delta Electronics, Inc.
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
 ************************************************************/
#include <onlp/platformi/fani.h>
#include "vendor_driver_pool.h"
#include "vendor_i2c_device_list.h"

/**
 * @brief Initialize the fan platform subsystem.
 */
int onlp_fani_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information structure for the given fan OID.
 * @param id The fan OID
 * @param rv [out] Receives the fan information.
 */
int onlp_fani_info_get(onlp_oid_t oid, onlp_fan_info_t *info)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, ONLP_OID_ID_GET(oid) - 1);
    int rv = 0, id = ONLP_OID_ID_GET(oid) - 1, fail = 0;

    if (id < 0 || id > fan_list_size)
    {
        AIM_LOG_ERROR("size=%d id=%d", fan_list_size, id + 1);
        return ONLP_STATUS_E_PARAM;
    }
    *info = onlp_fan_info[id];

    void *busDrv = (void *)vendor_find_driver_by_name(fan_dev_list[id].bus_drv_name);
    fan_dev_driver_t *fan =
        (fan_dev_driver_t *)vendor_find_driver_by_name(fan_dev_list[id].dev_drv_name);

    rv = onlp_fani_status_get(oid, &info->status);
    if (rv < 0)
        return ONLP_STATUS_E_INVALID;

    if (info->status != ONLP_FAN_STATUS_PRESENT)
        return ONLP_STATUS_OK;

    vendor_dev_do_oc(fan_o_list[id]);
    if (fan->rpm_get(
            busDrv,
            fan_dev_list[id].bus,
            fan_dev_list[id].dev,
            fan_dev_list[id].id,
            &info->rpm) != ONLP_STATUS_OK)
    {
        info->rpm = 0;
        fail = 1;
    }
    vendor_dev_do_oc(fan_c_list[id]);

    if (fail == 1)
        return ONLP_STATUS_E_INVALID;

    if (fan_dev_data_list[id].fan_max_speed == 0)
    {
        info->percentage = 0;
        return ONLP_STATUS_OK;
    }

    info->percentage = (info->rpm * 100 / fan_dev_data_list[id].fan_max_speed);

    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the fan's operational status.
 * @param id The fan OID.
 * @param status [out] Receives the fan's operations status flags.
 * @notes Only operational state needs to be returned -
 *        PRESENT/FAILED
 */
int onlp_fani_status_get(onlp_oid_t oid, uint32_t *status)
{
    int rv = 0, id = ONLP_OID_ID_GET(oid) - 1;
    int present = 0;

    if (id < 0 || id > fan_list_size)
    {
        AIM_LOG_ERROR("size=%d id=%d", fan_list_size, id + 1);
        return ONLP_STATUS_E_PARAM;
    }

    rv = vendor_get_status(&fan_present_list[id], &present);

    if (present)
    {
        *status = ONLP_FAN_STATUS_PRESENT;
    }
    else
    {
        *status = ONLP_FAN_STATUS_FAILED;
    }

    if (rv < 0)
        return ONLP_STATUS_E_INVALID;

    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the fan's OID hdr.
 * @param id The fan OID.
 * @param rv [out] Receives the OID header.
 */
int onlp_fani_hdr_get(onlp_oid_t oid, onlp_oid_hdr_t *hdr)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, ONLP_OID_ID_GET(oid) - 1);
    int id = ONLP_OID_ID_GET(oid) - 1;

    *hdr = onlp_fan_info[id].hdr;

    return ONLP_STATUS_OK;
}

/**
 * @brief Set the fan speed in RPM.
 * @param id The fan OID
 * @param rpm The new RPM
 * @note This is only relevant if the RPM capability is set.
 */
int onlp_fani_rpm_set(onlp_oid_t oid, int rpm)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, ONLP_OID_ID_GET(oid) - 1);
    int id = ONLP_OID_ID_GET(oid) - 1, fail = 0;

    void *busDrv = (void *)vendor_find_driver_by_name(fan_dev_list[id].bus_drv_name);
    fan_dev_driver_t *fan =
        (fan_dev_driver_t *)vendor_find_driver_by_name(fan_dev_list[id].dev_drv_name);

    vendor_dev_do_oc(fan_o_list[id]);
    if (fan->rpm_set(
            busDrv,
            fan_dev_list[id].bus,
            fan_dev_list[id].dev,
            fan_dev_list[id].id,
            rpm) != ONLP_STATUS_OK)
    {
        fail = 1;
    }
    vendor_dev_do_oc(fan_c_list[id]);

    if (fail == 1)
        return ONLP_STATUS_E_INVALID;

    return ONLP_STATUS_OK;
}

/**
 * @brief Set the fan speed in percentage.
 * @param id The fan OID.
 * @param p The new fan speed percentage.
 * @note This is only relevant if the PERCENTAGE capability is set.
 */
int onlp_fani_percentage_set(onlp_oid_t oid, int p)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, ONLP_OID_ID_GET(oid) - 1);
    int id = ONLP_OID_ID_GET(oid) - 1, fail = 0, rpm = 0;

    void *busDrv = (void *)vendor_find_driver_by_name(fan_dev_list[id].bus_drv_name);
    fan_dev_driver_t *fan =
        (fan_dev_driver_t *)vendor_find_driver_by_name(fan_dev_list[id].dev_drv_name);

    rpm = (fan_dev_data_list[id].fan_max_speed / 100) * p;

    vendor_dev_do_oc(fan_o_list[id]);
    if (fan->rpm_set(
            busDrv,
            fan_dev_list[id].bus,
            fan_dev_list[id].dev,
            fan_dev_list[id].id,
            rpm) != ONLP_STATUS_OK)
    {
        fail = 1;
    }
    vendor_dev_do_oc(fan_c_list[id]);

    if (fail == 1)
        return ONLP_STATUS_E_INVALID;

    return ONLP_STATUS_OK;
}

/**
 * @brief Set the fan mode.
 * @param id The fan OID.
 * @param mode The new fan mode.
 */
int onlp_fani_mode_set(onlp_oid_t id, onlp_fan_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Set the fan direction (if supported).
 * @param id The fan OID
 * @param dir The direction.
 */
int onlp_fani_dir_set(onlp_oid_t id, onlp_fan_dir_t dir)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Generic fan ioctl
 * @param id The fan OID
 * @param vargs The variable argument list for the ioctl call.
 * @param Optional
 */
int onlp_fani_ioctl(onlp_oid_t fid, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
