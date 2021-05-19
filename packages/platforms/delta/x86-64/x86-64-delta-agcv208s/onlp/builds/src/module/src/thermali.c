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
#include <onlp/platformi/thermali.h>
#include "vendor_driver_pool.h"
#include "vendor_i2c_device_list.h"

/**
 * @brief Initialize the thermal subsystem.
 */
int onlp_thermali_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the thermal's oid header.
 * @param id The thermal oid.
 * @param rv [out] Receives the header.
 */
int onlp_thermali_hdr_get(onlp_oid_t oid, onlp_oid_hdr_t *hdr)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, ONLP_OID_ID_GET(oid) - 1);
    int id = ONLP_OID_ID_GET(oid) - 1;

    *hdr = onlp_thermal_info[id].hdr;
    return ONLP_STATUS_OK;
}

/**
 * @brief Retrieve the thermal's operational status.
 * @param status The thermal oid.
 * @param status [out] Receives the operational status.
 */
int onlp_thermali_status_get(onlp_oid_t oid, uint32_t *status)
{
    *status = ONLP_THERMAL_STATUS_PRESENT;
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information for the given thermal OID.
 * @param id The Thermal OID
 * @param rv [out] Receives the thermal information.
 */
int onlp_thermali_info_get(onlp_oid_t oid, onlp_thermal_info_t *info)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, ONLP_OID_ID_GET(oid) - 1);
    int id = ONLP_OID_ID_GET(oid) - 1, fail = 0;

    if (id < 0 || id > thermal_list_size)
    {
        AIM_LOG_ERROR("size=%d id=%d", thermal_list_size, id + 1);
        return ONLP_STATUS_E_PARAM;
    }
    *info = onlp_thermal_info[id];

    void *busDrv = (void *)vendor_find_driver_by_name(thermal_dev_list[id].bus_drv_name);
    thermal_dev_driver_t *thermal =
        (thermal_dev_driver_t *)vendor_find_driver_by_name(thermal_dev_list[id].dev_drv_name);

    vendor_dev_do_oc(thermal_o_list[id]);
    if (thermal->temp_get(
            busDrv,
            thermal_dev_list[id].bus,
            thermal_dev_list[id].dev,
            thermal_dev_list[id].id,
            &info->mcelsius) != ONLP_STATUS_OK)
    {
        info->mcelsius = 0;
        fail = 1;
    }

    if (thermal->limit_get(
            busDrv,
            thermal_dev_list[id].bus,
            thermal_dev_list[id].dev,
            thermal_dev_list[id].id,
            VENDOR_THERMAL_LOW_THRESHOLD,
            &info->thresholds.warning) != ONLP_STATUS_OK)
    {
        info->thresholds.warning = 0;
        fail = 1;
    }

    if (thermal->limit_get(
            busDrv,
            thermal_dev_list[id].bus,
            thermal_dev_list[id].dev,
            thermal_dev_list[id].id,
            VENDOR_THERMAL_HIGH_THRESHOLD,
            &info->thresholds.shutdown) != ONLP_STATUS_OK)
    {
        info->thresholds.shutdown = 0;
        fail = 1;
    }
    vendor_dev_do_oc(thermal_c_list[id]);

    if (fail == 1)
        return ONLP_STATUS_E_INVALID;

    return ONLP_STATUS_OK;
}

/**
 * @brief Generic ioctl.
 */
int onlp_thermali_ioctl(int id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
