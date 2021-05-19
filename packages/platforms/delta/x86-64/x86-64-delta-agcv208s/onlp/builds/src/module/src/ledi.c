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
#include <onlp/platformi/ledi.h>
#include "vendor_driver_pool.h"
#include "vendor_i2c_device_list.h"

int onlp_ledi_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the information for the given LED
 * @param id The LED OID
 * @param rv [out] Receives the LED information.
 */
int onlp_ledi_info_get(onlp_oid_t oid, onlp_led_info_t *info)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, ONLP_OID_ID_GET(oid) - 1);
    int rv = 0, id = ONLP_OID_ID_GET(oid) - 1, cpld_idx = 0;
    vendor_dev_led_pin_t *curr_mode;
    uint8_t mode = 0;

    if (id < 0 || id > led_list_size)
    {
        AIM_LOG_ERROR("size=%d id=%d", led_list_size, id + 1);
        return ONLP_STATUS_E_PARAM;
    }
    *info = onlp_led_info[id];

    void *busDrv = (void *)vendor_find_driver_by_name(led_color_list[id]->bus_drv_name);
    cpld_dev_driver_t *cpld =
        (cpld_dev_driver_t *)vendor_find_driver_by_name(led_dev_list[id].dev_drv_name);

    cpld_idx = vendor_find_cpld_idx_by_name(led_color_list[id]->name);
    if (cpld_idx < 0)
        return ONLP_STATUS_E_INTERNAL;

    vendor_dev_do_oc(cpld_o_list[cpld_idx]);
    rv = cpld->readb(
        busDrv,
        led_color_list[id]->bus,
        led_color_list[id]->dev,
        led_color_list[id]->addr,
        &mode);
    vendor_dev_do_oc(cpld_c_list[cpld_idx]);

    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;
    curr_mode = led_color_list[id];

    while (curr_mode->mode != -1)
    {
        if (((mode & curr_mode->mask) == curr_mode->match) ? 1 : 0)
        {
            info->mode = curr_mode->mode;
            return ONLP_STATUS_OK;
        }
        curr_mode++;
    }

    AIM_LOG_ERROR("Function: %s, Unknown LED mode.", __FUNCTION__);
    return ONLP_STATUS_E_INTERNAL;
}

/**
 * @brief Get the LED operational status.
 * @param id The LED OID
 * @param status [out] Receives the operational status.
 */
int onlp_ledi_status_get(onlp_oid_t id, uint32_t *status)
{
    *status = ONLP_LED_STATUS_PRESENT;

    return ONLP_STATUS_OK;
}

/**
 * @brief Get the LED header.
 * @param id The LED OID
 * @param rv [out] Receives the header.
 */
int onlp_ledi_hdr_get(onlp_oid_t oid, onlp_oid_hdr_t *hdr)
{
    int id = ONLP_OID_ID_GET(oid) - 1;
    *hdr = onlp_led_info[id].hdr;
    return ONLP_STATUS_OK;
}

/**
 * @brief Turn an LED on or off
 * @param id The LED OID
 * @param on_or_off (boolean) on if 1 off if 0
 * @param This function is only relevant if the ONOFF capability is set.
 * @notes See onlp_led_set() for a description of the default behavior.
 */
int onlp_ledi_set(onlp_oid_t id, int on_or_off)
{
    if (!on_or_off)
    {
        return onlp_ledi_mode_set(id, ONLP_LED_MODE_OFF);
    }
    return ONLP_STATUS_OK;
}

/**
 * @brief Set the LED mode.
 * @param id The LED OID
 * @param mode The new mode.
 * @notes Only called if the mode is advertised in the LED capabilities.
 */
int onlp_ledi_mode_set(onlp_oid_t oid, onlp_led_mode_t mode)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, ONLP_OID_ID_GET(oid) - 1);
    int rv = 0, id = ONLP_OID_ID_GET(oid) - 1, cpld_idx = 0;
    uint8_t curr_data = 0;
    vendor_dev_led_pin_t *led_node;

    void *busDrv = (void *)vendor_find_driver_by_name(led_color_list[id]->bus_drv_name);
    cpld_dev_driver_t *cpld =
        (cpld_dev_driver_t *)vendor_find_driver_by_name(led_dev_list[id].dev_drv_name);

    led_node = led_color_list[id];

    while (led_node->mode != -1)
    {
        if (mode == led_node->mode)
        {
            cpld_idx = vendor_find_cpld_idx_by_name(led_node->name);
            if (cpld_idx < 0)
                return ONLP_STATUS_E_INTERNAL;

            vendor_dev_do_oc(cpld_o_list[cpld_idx]);
            rv = cpld->readb(
                busDrv,
                led_node->bus,
                led_node->dev,
                led_node->addr,
                &curr_data);

            curr_data &= ~led_node->mask;
            curr_data |= (led_node->match & led_node->mask);

            rv = cpld->writeb(
                busDrv,
                led_node->bus,
                led_node->dev,
                led_node->addr,
                curr_data);
            vendor_dev_do_oc(cpld_c_list[cpld_idx]);
        }
        led_node++;
    }

    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    return ONLP_STATUS_OK;
}

/**
 * @brief Set the LED character.
 * @param id The LED OID
 * @param c The character..
 * @notes Only called if the char capability is set.
 */
int onlp_ledi_char_set(onlp_oid_t id, char c)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief LED ioctl
 * @param id The LED OID
 * @param vargs The variable argument list for the ioctl call.
 */
int onlp_ledi_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
