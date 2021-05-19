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
#include <onlp/platformi/psui.h>
#include "vendor_driver_pool.h"
#include "vendor_i2c_device_list.h"

/**
 * @brief Initialize the PSU subsystem.
 */
int onlp_psui_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information structure for the given PSU
 * @param id The PSU OID
 * @param rv [out] Receives the PSU information.
 */
int onlp_psui_info_get(onlp_oid_t oid, onlp_psu_info_t *info)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, ONLP_OID_ID_GET(oid) - 1);
    int rv = 0, id = ONLP_OID_ID_GET(oid) - 1, fail = 0;
    vendor_psu_runtime_info_t runtimeInfo;

    if (id < 0 || id > psu_list_size)
    {
        AIM_LOG_ERROR("size=%d id=%d", psu_list_size, id + 1);
        return ONLP_STATUS_E_PARAM;
    }
    *info = onlp_psu_info[id];

    void *busDrv = (void *)vendor_find_driver_by_name(psu_dev_list[id].bus_drv_name);
    psu_dev_driver_t *psu =
        (psu_dev_driver_t *)vendor_find_driver_by_name(psu_dev_list[id].dev_drv_name);

    rv = onlp_psui_status_get(oid, &info->status);
    if (rv < 0)
        return ONLP_STATUS_E_INVALID;

    if (info->status == ONLP_PSU_STATUS_UNPLUGGED)
        return ONLP_STATUS_OK;

    vendor_dev_do_oc(psu_o_list[id]);

    if (psu->model_get(
            busDrv,
            psu_dev_list[id].bus,
            psu_dev_list[id].dev,
            (char *)&info->model) != ONLP_STATUS_OK)
    {
        AIM_LOG_ERROR("psu->model_get failed.");
        fail = 1;
    }

    if (psu->serial_get(
            busDrv,
            psu_dev_list[id].bus,
            psu_dev_list[id].dev,
            (char *)&info->serial) != ONLP_STATUS_OK)
    {
        AIM_LOG_ERROR("psu->serial_get failed.");
        fail = 1;
    }

    if (psu->runtime_info_get(
        busDrv, 
        psu_dev_list[id].bus, 
        psu_dev_list[id].dev, 
        &runtimeInfo))
    {
        AIM_LOG_ERROR("psu->runtime_info_get failed.");
        fail = 1;
    }

    /* millivolts */
    info->mvin = runtimeInfo.vin;
    info->mvout = runtimeInfo.vout;

    /* milliamps */
    info->miin = runtimeInfo.iin;
    info->miout = runtimeInfo.iout;

    /* milliwatts */
    info->mpin = runtimeInfo.pin;
    info->mpout = runtimeInfo.pout;

    vendor_dev_do_oc(psu_o_list[id]);

    if (fail == 1)
        return ONLP_STATUS_E_INVALID;

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's operational status.
 * @param id The PSU OID.
 * @param status [out] Receives the operational status.
 */
int onlp_psui_status_get(onlp_oid_t oid, uint32_t *status)
{
    int rv = 0, id = ONLP_OID_ID_GET(oid) - 1;
    int present = 0, power_good = 0;

    if (id < 0 || id > psu_list_size)
    {
        AIM_LOG_ERROR("size=%d id=%d", psu_list_size, id + 1);
        return ONLP_STATUS_E_PARAM;
    }

    rv = vendor_get_status(&psu_present_list[id], &present);
    if (rv < 0)
        return ONLP_STATUS_E_INVALID;

    if (psu_power_good_list[id].type != 0)
    {
        rv = vendor_get_status(&psu_power_good_list[id], &power_good);
    }
    else
    {
        /* Sometimes system cannot provide power good status */
        power_good = 1;
    }

    if (rv < 0)
        return ONLP_STATUS_E_INVALID;

    if (present && power_good)
    {
        *status = ONLP_PSU_STATUS_PRESENT;
    }
    else if (present == 1 && power_good == 0)
    {
        *status = ONLP_PSU_STATUS_FAILED;
    }
    else
    {
        *status = ONLP_PSU_STATUS_UNPLUGGED;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the PSU's oid header.
 * @param id The PSU OID.
 * @param rv [out] Receives the header.
 */
int onlp_psui_hdr_get(onlp_oid_t oid, onlp_oid_hdr_t *hdr)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, ONLP_OID_ID_GET(oid) - 1);
    int id = ONLP_OID_ID_GET(oid) - 1;

    *hdr = onlp_psu_info[id].hdr;

    return ONLP_STATUS_OK;
}

/**
 * @brief Generic PSU ioctl
 * @param id The PSU OID
 * @param vargs The variable argument list for the ioctl call.
 */
int onlp_psui_ioctl(onlp_oid_t pid, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
