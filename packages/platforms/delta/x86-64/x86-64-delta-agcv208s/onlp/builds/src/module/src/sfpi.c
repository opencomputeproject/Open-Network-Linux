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
#include <onlp/platformi/sfpi.h>
#include "vendor_driver_pool.h"
#include "vendor_i2c_device_list.h"

/**
 * @brief Initialize the SFPI subsystem.
 */
int onlp_sfpi_init(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Get the bitmap of SFP-capable port numbers.
 * @param bmap [out] Receives the bitmap.
 */
int onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t *bmap)
{
    //AIM_LOG_ERROR("Function: %s \n", __FUNCTION__);

    uint8_t index = 0;

    AIM_BITMAP_CLR_ALL(bmap);

    for (index = 0; index < sfp_list_size; index++)
    {
        if (strncmp(sfp_dev_list[index].dev_name, "copper", VENDOR_MAX_NAME_SIZE))
        {
            AIM_BITMAP_SET(bmap, index);
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Determine if an SFP is present.
 * @param port The port number.
 * @returns 1 if present
 * @returns 0 if absent
 * @returns An error condition.
 */
int onlp_sfpi_is_present(int port)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, port);

    int rv = 0, id = port;
    int present = 0;

    rv = vendor_get_status(&sfp_present_list[id], &present);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    return present;
}

/**
 * @brief Return the presence bitmap for all SFP ports.
 * @param dst Receives the presence bitmap.
 */
int onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t *dst)
{
    //AIM_LOG_ERROR("Function: %s \n", __FUNCTION__);

    int rv = 0, index = 0;

    for (index = 0; index < sfp_list_size; index++)
    {
        rv = onlp_sfpi_is_present(index);
        if (rv < 0)
            return ONLP_STATUS_E_INTERNAL;

        AIM_BITMAP_MOD(dst, index, rv);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Return the RX_LOS bitmap for all SFP ports.
 * @param dst Receives the RX_LOS bitmap.
 */
int onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t *dst)
{
    //AIM_LOG_ERROR("Function: %s\n", __FUNCTION__);

    uint8_t index = 0;
    int rv = 0, rx_los = 0, present = 0;
    onlp_sfp_bitmap_t bmap;
    AIM_BITMAP_INIT(&bmap, 255);
    AIM_BITMAP_CLR_ALL(&bmap);

    onlp_sfpi_bitmap_get(&bmap);
    AIM_BITMAP_ITER(&bmap, index)
    {
        present = onlp_sfpi_is_present(index);
        if (present)
        {
            rv = onlp_sfpi_control_get(index, ONLP_SFP_CONTROL_RX_LOS, &rx_los);
            if (rv < 0)
                return ONLP_STATUS_E_INTERNAL;
            AIM_BITMAP_MOD(dst, index, rx_los);
        }
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Read from an address on the given SFP port's bus.
 * @param port The port number.
 * @param devaddr The device address.
 * @param addr The address.
 * @returns The data if successful, error otherwise.
 */
int onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, port);

    int id = port, fail = 0;
    void *busDrv = (void *)vendor_find_driver_by_name(sfp_dev_list[id].bus_drv_name);
    sfp_dev_driver_t *sfp =
        (sfp_dev_driver_t *)vendor_find_driver_by_name(sfp_dev_list[id].dev_drv_name);

    vendor_dev_do_oc(sfp_o_list[id]);
    if (sfp->eeprom_load(
            busDrv,
            sfp_dev_list[id].bus,
            sfp_dev_list[id].dev,
            data) != ONLP_STATUS_OK)
    {
        AIM_LOG_ERROR("sfp->eeprom_load failed.");
        fail = 1;
    }
    vendor_dev_do_oc(sfp_c_list[id]);

    if (fail == 1)
        return ONLP_STATUS_E_INTERNAL;

    return ONLP_STATUS_OK;
}

/**
 * @brief Read a byte from an address on the given SFP port's bus.
 * @param port The port number.
 * @param devaddr The device address.
 * @param addr The address.
 */
int onlp_sfpi_dev_readb(int port, uint8_t devaddr, uint8_t addr)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, port);

    int id = port, fail = 0;
    ;
    uint8_t data = 0;
    void *busDrv = (void *)vendor_find_driver_by_name(sfp_dev_list[id].bus_drv_name);
    sfp_dev_driver_t *sfp =
        (sfp_dev_driver_t *)vendor_find_driver_by_name(sfp_dev_list[id].dev_drv_name);

    vendor_dev_do_oc(sfp_o_list[id]);
    if (sfp->eeprom_readb(
            busDrv,
            sfp_dev_list[id].bus,
            devaddr,
            addr,
            &data) != ONLP_STATUS_OK)
    {
        AIM_LOG_ERROR("sfp->eeprom_readb failed.");
        fail = 1;
    }
    vendor_dev_do_oc(sfp_c_list[id]);

    if (fail == 1)
        return ONLP_STATUS_E_INTERNAL;

    return data;
}

/**
 * @brief Write a byte to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, port);

    int id = port, fail = 0;
    void *busDrv = (void *)vendor_find_driver_by_name(sfp_dev_list[id].bus_drv_name);
    sfp_dev_driver_t *sfp =
        (sfp_dev_driver_t *)vendor_find_driver_by_name(sfp_dev_list[id].dev_drv_name);

    vendor_dev_do_oc(sfp_o_list[id]);
    if (sfp->eeprom_writeb(
            busDrv,
            sfp_dev_list[id].bus,
            devaddr,
            addr,
            value) != ONLP_STATUS_OK)
    {
        AIM_LOG_ERROR("sfp->eeprom_writeb failed.");
        fail = 1;
    }
    vendor_dev_do_oc(sfp_c_list[id]);

    if (fail == 1)
        return ONLP_STATUS_E_INTERNAL;

    return ONLP_STATUS_OK;
}

/**
 * @brief Read a word from an address on the given SFP port's bus.
 * @param port The port number.
 * @param devaddr The device address.
 * @param addr The address.
 * @returns The word if successful, error otherwise.
 */
int onlp_sfpi_dev_readw(int port, uint8_t devaddr, uint8_t addr)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, port);

    int id = port, fail = 0;
    ;
    uint16_t data = 0;
    void *busDrv = (void *)vendor_find_driver_by_name(sfp_dev_list[id].bus_drv_name);
    sfp_dev_driver_t *sfp =
        (sfp_dev_driver_t *)vendor_find_driver_by_name(sfp_dev_list[id].dev_drv_name);

    vendor_dev_do_oc(sfp_o_list[id]);
    if (sfp->eeprom_readw(
            busDrv,
            sfp_dev_list[id].bus,
            devaddr,
            addr,
            &data) != ONLP_STATUS_OK)
    {
        AIM_LOG_ERROR("sfp->eeprom_readw failed.");
        fail = 1;
    }
    vendor_dev_do_oc(sfp_c_list[id]);

    if (fail == 1)
        return ONLP_STATUS_E_INTERNAL;

    return data;
}

/**
 * @brief Write a word to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, port);

    int id = port, fail = 0;
    void *busDrv = (void *)vendor_find_driver_by_name(sfp_dev_list[id].bus_drv_name);
    sfp_dev_driver_t *sfp =
        (sfp_dev_driver_t *)vendor_find_driver_by_name(sfp_dev_list[id].dev_drv_name);

    vendor_dev_do_oc(sfp_o_list[id]);
    if (sfp->eeprom_writew(
            busDrv,
            sfp_dev_list[id].bus,
            devaddr,
            addr,
            value) != ONLP_STATUS_OK)
    {
        AIM_LOG_ERROR("sfp->eeprom_writew failed.");
        fail = 1;
    }
    vendor_dev_do_oc(sfp_c_list[id]);

    if (fail == 1)
        return ONLP_STATUS_E_INTERNAL;

    return ONLP_STATUS_OK;
}

/**
 * @brief Read the SFP EEPROM.
 * @param port The port number.
 * @param data Receives the SFP data.
 */
int onlp_sfpi_dev_read(int port, uint8_t devaddr, uint8_t addr,
                       uint8_t *rdata, int size)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, port);

    int id = port, fail = 0;
    void *busDrv = (void *)vendor_find_driver_by_name(sfp_dev_list[id].bus_drv_name);
    sfp_dev_driver_t *sfp =
        (sfp_dev_driver_t *)vendor_find_driver_by_name(sfp_dev_list[id].dev_drv_name);

    if (size > 256)
    {
        AIM_LOG_ERROR("onlp_sfpi_dev_read: unsupport size, the size must be or less then 256.");
        return ONLP_STATUS_E_INTERNAL;
    }

    vendor_dev_do_oc(sfp_o_list[id]);
    if (sfp->eeprom_read(
            busDrv,
            sfp_dev_list[id].bus,
            devaddr,
            addr,
            size,
            rdata) != ONLP_STATUS_OK)
    {
        AIM_LOG_ERROR("sfp->eeprom_read failed.");
        fail = 1;
    }
    vendor_dev_do_oc(sfp_c_list[id]);

    if (fail == 1)
        return ONLP_STATUS_E_INTERNAL;

    return ONLP_STATUS_OK;
}

/**
 * @brief Write to an address on the given SFP port's bus.
 */
int onlp_sfpi_dev_write(int port, uint8_t devaddr, uint8_t addr, uint8_t *data, int size)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, port);

    int id = port, fail = 0;
    void *busDrv = (void *)vendor_find_driver_by_name(sfp_dev_list[id].bus_drv_name);
    sfp_dev_driver_t *sfp =
        (sfp_dev_driver_t *)vendor_find_driver_by_name(sfp_dev_list[id].dev_drv_name);

    if (size > 256)
    {
        AIM_LOG_ERROR("onlp_sfpi_dev_write: unsupport size, the size must be or less then 256.");
        return ONLP_STATUS_E_INTERNAL;
    }

    vendor_dev_do_oc(sfp_o_list[id]);
    if (sfp->eeprom_write(
            busDrv,
            sfp_dev_list[id].bus,
            devaddr,
            addr,
            size,
            data) != ONLP_STATUS_OK)
    {
        AIM_LOG_ERROR("sfp->eeprom_write failed.");
        fail = 1;
    }
    vendor_dev_do_oc(sfp_c_list[id]);

    if (fail == 1)
        return ONLP_STATUS_E_INTERNAL;

    return ONLP_STATUS_OK;
}

/**
 * @brief Read the SFP DOM EEPROM.
 * @param port The port number.
 * @param data Receives the SFP data.
 */
int onlp_sfpi_dom_read(int port, uint8_t data[256])
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, port);

    int id = port, fail = 0;
    void *busDrv = (void *)vendor_find_driver_by_name(sfp_dev_list[id].bus_drv_name);
    sfp_dev_driver_t *sfp =
        (sfp_dev_driver_t *)vendor_find_driver_by_name(sfp_dev_list[id].dev_drv_name);

    vendor_dev_do_oc(sfp_o_list[id]);
    if (sfp->eeprom_dom_load(
            busDrv,
            sfp_dev_list[id].bus,
            sfp_dev_list[id].dev,
            data) != ONLP_STATUS_OK)
    {
        AIM_LOG_ERROR("sfp->eeprom_dom_load failed.");
        fail = 1;
    }
    vendor_dev_do_oc(sfp_c_list[id]);

    if (fail == 1)
        return ONLP_STATUS_E_INTERNAL;

    return ONLP_STATUS_OK;
}

/**
 * @brief Perform any actions required after an SFP is inserted.
 * @param port The port number.
 * @param info The SFF Module information structure.
 * @notes Optional
 */
int onlp_sfpi_post_insert(int port, sff_info_t *info)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Returns whether or not the given control is suppport on the given port.
 * @param port The port number.
 * @param control The control.
 * @param rv [out] Receives 1 if supported, 0 if not supported.
 * @note This provided for convenience and is optional.
 * If you implement this function your control_set and control_get APIs
 * will not be called on unsupported ports.
 */
int onlp_sfpi_control_supported(int port, onlp_sfp_control_t control, int *rv)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d", __FUNCTION__, port);

    int id = port;
    sfp_dev_driver_t *sfp =
        (sfp_dev_driver_t *)vendor_find_driver_by_name(sfp_dev_list[id].dev_drv_name);

    if (sfp->control_is_support(control, (uint8_t *)rv) != ONLP_STATUS_OK)
    {
        AIM_LOG_ERROR("sfp->control_is_support failed.");
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Set an SFP control.
 * @param port The port.
 * @param control The control.
 * @param value The value.
 */
int onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d, control: %d\n", __FUNCTION__, port, control);

    int rv = 0, id = port, cpld_idx = 0, fail = 0;
    uint8_t curr_data = 0;
    void *busDrv = (void *)vendor_find_driver_by_name(sfp_dev_list[id].bus_drv_name);
    cpld_dev_driver_t *cpld =
        (cpld_dev_driver_t *)vendor_find_driver_by_name("CPLD");
    sfp_dev_driver_t *sfp =
        (sfp_dev_driver_t *)vendor_find_driver_by_name(sfp_dev_list[id].dev_drv_name);

    switch (control)
    {
    case ONLP_SFP_CONTROL_RESET:
    case ONLP_SFP_CONTROL_RESET_STATE:
        if (sfp_reset_list[id].type == 0)
        {
            /*NOT SUPPORT */
            return ONLP_STATUS_OK;
        }
        cpld_idx = vendor_find_cpld_idx_by_name(sfp_reset_list[id].name);
        if (cpld_idx < 0)
            return ONLP_STATUS_E_INTERNAL;

        vendor_dev_do_oc(cpld_o_list[cpld_idx]);
        rv = cpld->readb(
            busDrv,
            sfp_reset_list[id].bus,
            sfp_reset_list[id].dev,
            sfp_reset_list[id].addr,
            &curr_data);

        curr_data &= ~sfp_reset_list[id].mask;
        if (value == 1)
            curr_data |= (sfp_reset_list[id].match & sfp_reset_list[id].mask);
        else
            curr_data |= (~sfp_reset_list[id].match & sfp_reset_list[id].mask);

        curr_data &= 0xff;

        rv = cpld->writeb(
            busDrv,
            sfp_reset_list[id].bus,
            sfp_reset_list[id].dev,
            sfp_reset_list[id].addr,
            curr_data);
        vendor_dev_do_oc(cpld_c_list[cpld_idx]);

        if (rv < 0)
            return ONLP_STATUS_E_INTERNAL;

        return 0;

    case ONLP_SFP_CONTROL_LP_MODE:
        if (sfp_lpmode_list[id].type == 0)
        {
            /*NOT SUPPORT */
            return ONLP_STATUS_OK;
        }
        cpld_idx = vendor_find_cpld_idx_by_name(sfp_lpmode_list[id].name);
        if (cpld_idx < 0)
            return ONLP_STATUS_E_INTERNAL;

        vendor_dev_do_oc(cpld_o_list[cpld_idx]);
        rv = cpld->readb(
            busDrv,
            sfp_lpmode_list[id].bus,
            sfp_lpmode_list[id].dev,
            sfp_lpmode_list[id].addr,
            &curr_data);

        curr_data &= ~sfp_lpmode_list[id].mask;
        if (value == 1)
            curr_data |= (sfp_lpmode_list[id].match & sfp_lpmode_list[id].mask);
        else
            curr_data |= (~sfp_lpmode_list[id].match & sfp_lpmode_list[id].mask);

        curr_data &= 0xff;

        rv = cpld->writeb(
            busDrv,
            sfp_lpmode_list[id].bus,
            sfp_lpmode_list[id].dev,
            sfp_lpmode_list[id].addr,
            curr_data);
        vendor_dev_do_oc(cpld_c_list[cpld_idx]);

        if (rv < 0)
            return ONLP_STATUS_E_INTERNAL;

        return 0;

    case ONLP_SFP_CONTROL_TX_DISABLE:
    case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
    case ONLP_SFP_CONTROL_POWER_OVERRIDE:
        vendor_dev_do_oc(sfp_o_list[id]);
        if (sfp->control_set(
                busDrv,
                sfp_dev_list[id].bus,
                sfp_dev_list[id].dev,
                control,
                value) != ONLP_STATUS_OK)
        {
            AIM_LOG_ERROR("sfp->onlp_sfpi_control_set failed.");
            fail = 1;
        }
        vendor_dev_do_oc(sfp_c_list[id]);

        break;
    default:
        AIM_LOG_ERROR("Unsupported control.");
        return ONLP_STATUS_E_INTERNAL;
    }

    if (fail == 1)
        return ONLP_STATUS_E_INTERNAL;

    return ONLP_STATUS_OK;
}

/**
 * @brief Get an SFP control.
 * @param port The port.
 * @param control The control
 * @param [out] value Receives the current value.
 */
int onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int *value)
{
    //AIM_LOG_ERROR("Function: %s, instance: %d, control: %d\n", __FUNCTION__, port, control);

    int rv = 0, id = port, cpld_idx = 0, fail = 0;
    uint8_t curr_data = 0;
    int *eeprom_data = calloc(1, sizeof(int));

    void *busDrv = (void *)vendor_find_driver_by_name(sfp_dev_list[id].bus_drv_name);
    cpld_dev_driver_t *cpld =
        (cpld_dev_driver_t *)vendor_find_driver_by_name("CPLD");
    sfp_dev_driver_t *sfp =
        (sfp_dev_driver_t *)vendor_find_driver_by_name(sfp_dev_list[id].dev_drv_name);

    switch (control)
    {
    case ONLP_SFP_CONTROL_RESET:
    case ONLP_SFP_CONTROL_RESET_STATE:
        if (sfp_reset_list[id].type == 0)
        {
            /*NOT SUPPORT */
            return ONLP_STATUS_OK;
        }
        cpld_idx = vendor_find_cpld_idx_by_name(sfp_reset_list[id].name);
        if (cpld_idx < 0)
            return ONLP_STATUS_E_INTERNAL;

        vendor_dev_do_oc(cpld_o_list[cpld_idx]);
        rv = cpld->readb(
            busDrv,
            sfp_reset_list[id].bus,
            sfp_reset_list[id].dev,
            sfp_reset_list[id].addr,
            &curr_data);
        vendor_dev_do_oc(cpld_c_list[cpld_idx]);

        if (rv < 0)
            return ONLP_STATUS_E_INTERNAL;

        *value = ((curr_data & sfp_reset_list[id].mask) == sfp_reset_list[id].match) ? 1 : 0;
        return 0;

    case ONLP_SFP_CONTROL_LP_MODE:
        if (sfp_lpmode_list[id].type == 0)
        {
            /*NOT SUPPORT */
            return ONLP_STATUS_OK;
        }
        cpld_idx = vendor_find_cpld_idx_by_name(sfp_lpmode_list[id].name);
        if (cpld_idx < 0)
            return ONLP_STATUS_E_INTERNAL;

        vendor_dev_do_oc(cpld_o_list[cpld_idx]);
        rv = cpld->readb(
            busDrv,
            sfp_lpmode_list[id].bus,
            sfp_lpmode_list[id].dev,
            sfp_lpmode_list[id].addr,
            &curr_data);
        vendor_dev_do_oc(cpld_c_list[cpld_idx]);

        if (rv < 0)
            return ONLP_STATUS_E_INTERNAL;

        *value = ((curr_data & sfp_lpmode_list[id].mask) == sfp_lpmode_list[id].match) ? 1 : 0;
        return 0;

    case ONLP_SFP_CONTROL_RX_LOS:
        free(eeprom_data);
        eeprom_data = calloc(4, sizeof(int));
    case ONLP_SFP_CONTROL_TX_FAULT:
        free(eeprom_data);
        eeprom_data = calloc(4, sizeof(int));
    case ONLP_SFP_CONTROL_TX_DISABLE:
    case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
    case ONLP_SFP_CONTROL_POWER_OVERRIDE:
        vendor_dev_do_oc(sfp_o_list[id]);
        if (sfp->control_get(
                busDrv,
                sfp_dev_list[id].bus,
                sfp_dev_list[id].dev,
                control,
                eeprom_data) != ONLP_STATUS_OK)
        {
            AIM_LOG_ERROR("sfp->onlp_sfpi_control_get failed.");
            fail = 1;
        }
        vendor_dev_do_oc(sfp_c_list[id]);

        *value = eeprom_data[0];
        free(eeprom_data);

        break;
    default:
        AIM_LOG_ERROR("Unsupported control.");
        return ONLP_STATUS_E_INTERNAL;
    }

    if (fail == 1)
        return ONLP_STATUS_E_INTERNAL;

    return ONLP_STATUS_OK;
}

/**
 * @brief Remap SFP user SFP port numbers before calling the SFPI interface.
 * @param port The user SFP port number.
 * @param [out] rport Receives the new port.
 * @note This function will be called to remap the user SFP port number
 * to the number returned in rport before the SFPI functions are called.
 * This is an optional convenience for platforms with dynamic or
 * variant physical SFP numbering.
 */
int onlp_sfpi_port_map(int port, int *rport)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Deinitialize the SFP driver.
 */
int onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}

/**
 * @brief Generic debug status information.
 * @param port The port number.
 * @param pvs The output pvs.
 * @notes The purpose of this vector is to allow reporting of internal debug
 * status and information from the platform driver that might be used to debug
 * SFP runtime issues.
 * For example, internal equalizer settings, tuning status information, status
 * of additional signals useful for system debug but not exposed in this interface.
 *
 * @notes This is function is optional.
 */
void onlp_sfpi_debug(int port, aim_pvs_t *pvs)
{
}

/**
 * @brief Generic ioctl
 * @param port The port number
 * @param The variable argument list of parameters.
 *
 * @notes This generic ioctl interface can be used
 * for platform-specific or driver specific features
 * that cannot or have not yet been defined in this
 * interface. It is intended as a future feature expansion
 * support mechanism.
 *
 * @notes Optional
 */
int onlp_sfpi_ioctl(int port, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
