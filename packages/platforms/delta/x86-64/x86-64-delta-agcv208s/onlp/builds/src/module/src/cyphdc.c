/*
 * PHDC routines of Cypress USB Serial
 * Copyright (C) 2013  Cypress Semiconductor
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "CyUSBCommon.h"
/*
    PHDC clear feature
*/
CY_RETURN_STATUS CyPhdcClrFeature (CY_HANDLE handle)
{

    UINT16 wValue, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;

    bmRequestType = CY_CLASS_INTERFACE_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_PHDC_CLR_FEATURE;
    wValue = CY_PHDC_CLR_FEATURE_WVALUE;
    wIndex = device->interfaceNum;
    wLength = 0;

    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, NULL, wLength, ioTimeout);
    if (rStatus == 0)
        return CY_SUCCESS;
    else if (rStatus == LIBUSB_ERROR_TIMEOUT){
        CY_DEBUG_PRINT_ERROR ("CY:Time out error ..function is %s \n", __func__);
        return CY_ERROR_IO_TIMEOUT;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY: Error in function %s ...libusb error is %d!\n", __func__, rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
    return CY_SUCCESS;
}
/*
    PHDC set feature
*/
CY_RETURN_STATUS CyPhdcSetFeature (CY_HANDLE handle)
{

    UINT16 wValue, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus, ioTimeout = CY_USB_SERIAL_TIMEOUT ;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;

    bmRequestType = CY_CLASS_INTERFACE_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_PHDC_SET_FEATURE;
    wValue = CY_PHDC_SET_FEATURE_WVALUE;
    wIndex = device->interfaceNum;
    wLength = 0;

    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, NULL, wLength, ioTimeout);
    if (rStatus == 0)
        return CY_SUCCESS;
    else if (rStatus == LIBUSB_ERROR_TIMEOUT){
        CY_DEBUG_PRINT_ERROR ("CY:Time out error ..function is %s \n", __func__);
        return CY_ERROR_IO_TIMEOUT;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY: Error in function %s ...libusb error is %d!\n", __func__, rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
    PHDC clear feature
*/
CY_RETURN_STATUS CyPhdcGetStatus (CY_HANDLE handle, UINT16 *dataStatus)
{

    UINT16 wValue, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT ;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;

    bmRequestType = CY_CLASS_INTERFACE_REQUEST_DEVICE_TO_HOST;
    bmRequest = CY_PHDC_GET_DATA_STATUS;
    wValue = 0x00;
    wIndex = device->interfaceNum;
    wLength = CY_PHDC_GET_STATUS_LEN;

    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, (unsigned char*)dataStatus, wLength, ioTimeout);
    if (rStatus > 0)
        return CY_SUCCESS;
    else if (rStatus == LIBUSB_ERROR_TIMEOUT){
        CY_DEBUG_PRINT_ERROR ("CY:Time out error ..function is %s \n", __func__);
        return CY_ERROR_IO_TIMEOUT;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY: Error in function %s ...libusb error is %d!\n", __func__, rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
}
