/*
 * JTAG routines of Cypress USB Serial
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
 * This API enables the Jtag module
 */
CY_RETURN_STATUS CyJtagEnable (
        CY_HANDLE handle
        )
{
    UINT16 wValue, wIndex, wLength;
    UINT16 bmRequestType, bmRequest;
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_JTAG) {
        CY_DEBUG_PRINT_ERROR ("CY:Error device type is not jtag ... Function is %s \n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    bmRequestType = CY_VENDOR_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_JTAG_ENABLE_CMD;
    wValue = 0x00;
    wIndex = 0;
    wLength = 0;
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, NULL, wLength, ioTimeout);
    if (rStatus >= 0){
        CY_DEBUG_PRINT_INFO ("CY: JTAG enable successfully \n");
        return CY_SUCCESS;
    }
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
 * This API disables the Jtag module
 */
CY_RETURN_STATUS CyJtagDisable (
        CY_HANDLE handle
        )
{
    UINT16 wValue, wIndex, wLength;
    UINT16 bmRequestType, bmRequest;
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_JTAG) {
        CY_DEBUG_PRINT_ERROR ("CY:Error device type is not jtag ... Function is %s \n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    bmRequestType = CY_VENDOR_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_JTAG_DISABLE_CMD;
    wValue = 0x00;
    wIndex = 0;
    wLength = 0;
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, NULL, wLength, ioTimeout);
    if (rStatus >= 0){
        CY_DEBUG_PRINT_INFO ("CY: JTAG disable successfully \n");
        return CY_SUCCESS;
    }
    else if (rStatus == LIBUSB_ERROR_TIMEOUT){
        CY_DEBUG_PRINT_ERROR ("CY:Time out error while enabling JTAG  ..\n");
        return CY_ERROR_IO_TIMEOUT;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY:Error while enabling JTAG ...libusb error is %d function is %s!\n", rStatus, __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
 * This API is used to do jtag write
 */
CY_RETURN_STATUS CyJtagWrite (
        CY_HANDLE handle,
        CY_DATA_BUFFER *writeBuffer,
        UINT32 ioTimeout
        )
{
    int rStatus = 0;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT16 wValue, wIndex, wLength;
    UINT16 bmRequestType, bmRequest;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if ((writeBuffer == NULL) || (writeBuffer->buffer == NULL) || (writeBuffer->length == 0)){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid parameter.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_JTAG) {
        CY_DEBUG_PRINT_ERROR ("CY:Error device type is not jtag ... Function is %s \n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    bmRequestType = CY_VENDOR_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_JTAG_WRITE_CMD;
    wValue = writeBuffer->length;
    wIndex = 0;
    wLength = 0;
    writeBuffer->transferCount = 0;
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, NULL, wLength, ioTimeout);
    if (rStatus < 0){
        CY_DEBUG_PRINT_ERROR ("CY: JTAG Vendor command failed %d...function is %s \n", rStatus, __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    rStatus = libusb_bulk_transfer (devHandle, CY_JTAG_OUT_EP, writeBuffer->buffer, writeBuffer->length,
            (int*)&(writeBuffer->transferCount), ioTimeout);
    if (rStatus == CY_SUCCESS) {
        CY_DEBUG_PRINT_ERROR ("CY: Number of bytes written is .... %d \n", writeBuffer->transferCount);
        return CY_SUCCESS;
    }
    else if (rStatus == LIBUSB_ERROR_TIMEOUT){
        CY_DEBUG_PRINT_ERROR ("CY:TimeOut error ...Function is %s %d\n", __func__, writeBuffer->transferCount);
        return CY_ERROR_IO_TIMEOUT;
    }
    else if (rStatus == LIBUSB_ERROR_PIPE){
        CY_DEBUG_PRINT_ERROR ("CY:Pipe error Function is %s \n", __func__);
        CyResetPipe (handle, CY_JTAG_OUT_EP);
        return CY_ERROR_PIPE_HALTED;
    }
    else if (rStatus == LIBUSB_ERROR_OVERFLOW){
        CY_DEBUG_PRINT_ERROR ("CY:Error Buffer Overflow..Function is %s \n", __func__);
        return CY_ERROR_BUFFER_OVERFLOW;
    }
    else if (rStatus == LIBUSB_ERROR_NO_DEVICE) {
        CY_DEBUG_PRINT_ERROR ("CY: Device Disconnected ....Function is %s \n", __func__);
        return CY_ERROR_DEVICE_NOT_FOUND;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY: Error in Function %s...Libusb Error is %d !\n", __func__, rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
   This API is used to read JTAG data from device interface
 */
CY_RETURN_STATUS CyJtagRead (
        CY_HANDLE handle,
        CY_DATA_BUFFER *readBuffer,
        UINT32 ioTimeout
        )
{
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT16 wValue, wIndex, wLength;
    UINT16 bmRequestType, bmRequest;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if ((readBuffer == NULL) || (readBuffer->buffer == NULL) || (readBuffer->length == 0)){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid parameter.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_JTAG) {
        CY_DEBUG_PRINT_ERROR ("CY:Error device type is not jtag ... Function is %s \n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }

    bmRequestType = CY_VENDOR_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_JTAG_READ_CMD;
    wValue = readBuffer->length;
    wIndex = 0;
    wLength = 0;

    readBuffer->transferCount = 0;
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, NULL, wLength, ioTimeout);
    if (rStatus < 0){
        CY_DEBUG_PRINT_INFO ("CY: JTAG Vendor Command failed %d..  Function is %s \n", rStatus, __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    rStatus = libusb_bulk_transfer (devHandle, CY_JTAG_IN_EP, readBuffer->buffer, readBuffer->length,
            (int*)&(readBuffer->transferCount), ioTimeout);
     if (rStatus == CY_SUCCESS){
        CY_DEBUG_PRINT_ERROR ("CY: Number of bytes read is .... %d \n", readBuffer->transferCount);
        return CY_SUCCESS;
    }
    else if (rStatus == LIBUSB_ERROR_TIMEOUT){
        CY_DEBUG_PRINT_ERROR ("CY:TimeOut error ...Function is %s \n", __func__);
        return CY_ERROR_IO_TIMEOUT;
    }
    else if (rStatus == LIBUSB_ERROR_PIPE){
        CY_DEBUG_PRINT_ERROR ("CY:Pipe error Function is %s \n", __func__);
        CyResetPipe (handle, CY_JTAG_IN_EP);
        return CY_ERROR_PIPE_HALTED;
    }
    else if (rStatus == LIBUSB_ERROR_OVERFLOW){
        CY_DEBUG_PRINT_ERROR ("CY:Error Buffer Overflow..Function is %s \n", __func__);
        return CY_ERROR_BUFFER_OVERFLOW;
    }
    else if (rStatus == LIBUSB_ERROR_NO_DEVICE) {
        CY_DEBUG_PRINT_ERROR ("CY: Device Disconnected ....Function is %s \n", __func__);
        return CY_ERROR_DEVICE_NOT_FOUND;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY: Error in function is %s ...Libusb Error is %d!\n", __func__, rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
}
