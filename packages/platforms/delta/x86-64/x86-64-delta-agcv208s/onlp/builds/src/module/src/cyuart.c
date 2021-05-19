/*
 * UART routines of Cypress USB Serial
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
#pragma pack(1)
typedef struct {
    CY_UART_BAUD_RATE baudRate;
    UINT8 pinType;
    UINT8 dataWidth;
    UINT8 stopBits;
    UINT8 mode;
    UINT8 parity;
    UINT8 isMsbFirst;
    UINT8 txRetry;
    UINT8 rxInvertPolarity;
    UINT8 rxIgnoreError;
    UINT8 isFlowControl;
    UINT8 isLoopBack;
    UINT8 flags;
}CyUsUartConfig_t;
#pragma pack()
//Timer helper functions for proper timing
UINT32 getUartLapsedTime (struct timeval startTime){
    signed int currentTime_sec, currentTime_usec, currentTime;
    struct timeval endTime;
    gettimeofday (&endTime, NULL);
    currentTime_sec = (endTime.tv_sec - startTime.tv_sec) * 1000;
    currentTime_usec = ((endTime.tv_usec - startTime.tv_usec)) / 1000;
    currentTime = currentTime_sec + currentTime_usec;
    return (unsigned int)currentTime;
}
/*
   This API gets the current UART configuration of the
   device.Such as GPIO's assigned, flowcontrol, BaudRate
   etc.
 */
CY_RETURN_STATUS CyGetUartConfig (
        CY_HANDLE handle,
        PCY_UART_CONFIG uartConfig
        )
{
    UINT16 wValue, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus;
    CY_DEVICE *device;
    CyUsUartConfig_t localUartConfig;
    libusb_device_handle *devHandle;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;
    UINT8 scbIndex = 0;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle..Function is %s\n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if (uartConfig == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid input parameter..Function is %s\n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_UART){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid device type needs to be uart..Function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    if (device->interfaceNum > 0)
        scbIndex = 1;
    bmRequestType = CY_VENDOR_REQUEST_DEVICE_TO_HOST;
    bmRequest = CY_UART_GET_CONFIG_CMD;
    wValue = (scbIndex << CY_SCB_INDEX_POS);
    wIndex = 0;
    wLength = CY_UART_CONFIG_LEN;
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, (unsigned char*)&localUartConfig, wLength, ioTimeout);
    //Since we are not exposing all the configuration elements
    //parse and fill only relevant elements.
    if (rStatus == CY_UART_CONFIG_LEN){
        uartConfig->dataWidth = localUartConfig.dataWidth;
        uartConfig->baudRate = localUartConfig.baudRate;
        uartConfig->stopBits = localUartConfig.stopBits;
        uartConfig->parityMode = (CY_UART_PARITY_MODE)localUartConfig.parity;;
        uartConfig->isDropOnRxErrors = localUartConfig.rxIgnoreError;
        //We are currently ignoring rest of the bits
        CY_DEBUG_PRINT_INFO ("CY:Successfully read UART Config\n");
        return CY_SUCCESS;
    }
    else{
        CY_DEBUG_PRINT_ERROR ("CY:Error in reading UART config ... Libusb Error is %d \n", rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
   This API sets the current UART configuration of the
   device.Such as GPIO's assigned, flowcontrol, BaudRate
   etc.
 */
CY_RETURN_STATUS CySetUartConfig (
        CY_HANDLE handle,
        CY_UART_CONFIG *uartConfig
        )
{
    UINT16 wValue, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus;
    CyUsUartConfig_t localUartConfig;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;
    UINT8 scbIndex = 0;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle..Function is %s\n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if (uartConfig == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid input parameter..Function is %s\n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    if (uartConfig->dataWidth < 7 || uartConfig->dataWidth > 8){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid input parameter..Function is %s\n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    if (uartConfig->stopBits < 1 || uartConfig->stopBits > 2){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid input parameter..Function is %s\n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->interfaceNum > 0)
        scbIndex = 1;
    if (device->deviceType != CY_TYPE_UART){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid device type needs to be uart..Function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    bmRequestType = CY_VENDOR_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_UART_SET_CONFIG_CMD;
    wValue = (scbIndex << CY_SCB_INDEX_POS);
    wIndex = 0;
    wLength = CY_UART_CONFIG_LEN;

    memset (&localUartConfig, 0, CY_UART_CONFIG_LEN);
    //Fill in rest of the UART config structure elements
    //that are not exposed in API with default values
    localUartConfig.baudRate = uartConfig->baudRate;
    localUartConfig.dataWidth = uartConfig->dataWidth;
    localUartConfig.stopBits = uartConfig->stopBits;
    localUartConfig.parity = (UCHAR) uartConfig->parityMode;
    localUartConfig.rxIgnoreError = uartConfig->isDropOnRxErrors;
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, (unsigned char*)&localUartConfig, wLength, ioTimeout);
    if (rStatus == CY_UART_CONFIG_LEN){
        CY_DEBUG_PRINT_INFO ("CY:Successfully Set UART Config \n");
        return CY_SUCCESS;
    }
    else{
        CY_DEBUG_PRINT_ERROR ("CY:Error in Setting UART config ... Libusb Error is %d \n", rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
   This Api writes the Data to UART block of the
   device.
 */
CY_RETURN_STATUS CyUartWrite (
        CY_HANDLE handle,
        CY_DATA_BUFFER* writeBuffer,
        unsigned int ioTimeOut
        )

{
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle..Function is %s\n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if ((writeBuffer == NULL) || (writeBuffer->buffer == NULL) || (writeBuffer->length == 0)){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid input parameters..Function is %s\n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    writeBuffer->transferCount = 0;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_UART){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid device type needs to be uart..Function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    rStatus = libusb_bulk_transfer (devHandle, device->outEndpoint, writeBuffer->buffer, writeBuffer->length,
            (int *)&((writeBuffer->transferCount)), ioTimeOut);
    if (rStatus == CY_SUCCESS) {
        CY_DEBUG_PRINT_INFO ("CY: SuccessFull in Wrting Data,%d bytes were transfered \n", (writeBuffer->transferCount));
        return CY_SUCCESS;
    }
    else if (rStatus == LIBUSB_ERROR_TIMEOUT){
        CY_DEBUG_PRINT_ERROR ("CY:TimeOut error ...Function is %s\n", __func__);
        return CY_ERROR_IO_TIMEOUT;
    }
    else if (rStatus == LIBUSB_ERROR_PIPE){
        CY_DEBUG_PRINT_ERROR ("CY:Pipe error endpoint Halted ...Function is %s\n", __func__);
        CyResetPipe (handle, device->outEndpoint);
        return CY_ERROR_PIPE_HALTED;
    }
    else if (rStatus == LIBUSB_ERROR_OVERFLOW){
        CY_DEBUG_PRINT_ERROR ("CY:Error Buffer Overflow occured ...Function is %s\n", __func__);
        return CY_ERROR_BUFFER_OVERFLOW;
    }
    else if (rStatus == LIBUSB_ERROR_NO_DEVICE) {
        CY_DEBUG_PRINT_ERROR ("CY: Device Disconnected .... Function is %s\n", __func__);
        return CY_ERROR_DEVICE_NOT_FOUND;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY: Unknown error ....Libusb error is %d Function is %s\n", rStatus, __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
   This Api Reads the Data from UART block of the
   device.
 */
CY_RETURN_STATUS CyUartRead (
        CY_HANDLE handle,
        CY_DATA_BUFFER* readBuffer,
        unsigned int ioTimeOut
        )

{
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT32 length, totalRead = 0, newIoTimeout = ioTimeOut, elapsedTime;
    int transferCount;
    UCHAR *buffer;
    struct timeval startTime;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle..Function is %s\n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if ((readBuffer == NULL) || (readBuffer->buffer == NULL) || (readBuffer->length == 0)){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid input parameters..Function is %s\n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    length = readBuffer->length;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    readBuffer->transferCount = 0;
    if (device->deviceType != CY_TYPE_UART){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid device type needs to be uart..Function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    //Collect all the data in low baud rate for uart. As we get data in short packet
    do {
        // buffer will be pointing to new pointer
        buffer = &(readBuffer->buffer[totalRead]);
        //Start the tick
        gettimeofday(&startTime, NULL);
        rStatus = libusb_bulk_transfer (devHandle, device->inEndpoint, buffer, length,
                &transferCount, newIoTimeout);
        elapsedTime = getUartLapsedTime(startTime);
        //Get the new timeout.
        newIoTimeout = newIoTimeout - elapsedTime;
        //Initialise totalRead to initially read + bytes returned now
        totalRead += transferCount;
        //length will initial length - transferCount
        length = (length - transferCount);

    }while ((rStatus == CY_SUCCESS) && (totalRead != readBuffer->length) && (newIoTimeout > 0));
    if (newIoTimeout <= 0 && totalRead != readBuffer->length)
        rStatus = LIBUSB_ERROR_TIMEOUT;
    if (rStatus == CY_SUCCESS){
        //CY_DUMP_DATA (readBuffer->buffer, readBuffer->transferCount);
        readBuffer->transferCount = totalRead;
        CY_DEBUG_PRINT_INFO ("CY: SuccessFull in Reading Data,%d bytes were transfered \n", (readBuffer->transferCount));
        return CY_SUCCESS;
    }
    else if (rStatus == LIBUSB_ERROR_TIMEOUT){
        readBuffer->transferCount = totalRead;
        CY_DEBUG_PRINT_ERROR ("CY:TimeOut error... Function is %s\n", __func__);
        return CY_ERROR_IO_TIMEOUT;
    }
    else if (rStatus == LIBUSB_ERROR_PIPE){
        readBuffer->transferCount = totalRead;
        CY_DEBUG_PRINT_ERROR ("CY:Pipe error endpoint Halted ...Function is %s\n", __func__);
        CyResetPipe (handle, device->inEndpoint);
        return CY_ERROR_PIPE_HALTED;
    }
    else if (rStatus == LIBUSB_ERROR_OVERFLOW){
        readBuffer->transferCount = totalRead;
        CY_DEBUG_PRINT_ERROR ("CY:Error Buffer Overflow occured ...Function is %s\n", __func__);
        return CY_ERROR_BUFFER_OVERFLOW;
    }
    else if (rStatus == LIBUSB_ERROR_NO_DEVICE) {
        readBuffer->transferCount = totalRead;
        CY_DEBUG_PRINT_ERROR ("CY: Device Disconnected ....Function is %s\n", __func__);
        return CY_ERROR_DEVICE_NOT_FOUND;
    }
    else {
        readBuffer->transferCount = totalRead;
        CY_DEBUG_PRINT_ERROR ("CY: Unknown error ....Libusb error is %d Function is %s\n", rStatus, __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
   This Api sets the hardware flow control
 */
CY_RETURN_STATUS CyUartSetHwFlowControl (
        CY_HANDLE handle,
        CY_FLOW_CONTROL_MODES mode
        )

{
    UINT16 wValue = 0, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus, ioTimeout = CY_USB_SERIAL_TIMEOUT ;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle..Function is %s\n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if (mode > 3){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid parameter..Function is %s\n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_UART){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid device type needs to be uart..Function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    bmRequestType = CY_CLASS_INTERFACE_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_UART_SET_FLOW_CONTROL_CMD;
    wValue |= mode;
    wIndex = device->interfaceNum;
    wLength = 0;
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, NULL, wLength, ioTimeout);
    if (rStatus < 0){
        CY_DEBUG_PRINT_ERROR ("CY:Error in setting uart flow control ... Libusb Error is %d \n", rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
    device->uartFlowControlMode = mode;
    return CY_SUCCESS;
}
/*
Api gets the current flow control mode
*/
CY_RETURN_STATUS CyUartGetHwFlowControl (
        CY_HANDLE handle,
        CY_FLOW_CONTROL_MODES *mode
        )
{
    CY_DEVICE *device;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle..Function is %s\n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if (mode == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid input parameters..Function is %s\n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    device = (CY_DEVICE *)handle;
    if (device->deviceType != CY_TYPE_UART){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid device type needs to be uart..Function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    (*mode) = device->uartFlowControlMode;
    return CY_SUCCESS;
}
/* The API is used to break
*/
CYWINEXPORT CY_RETURN_STATUS CyUartSetBreak(
    CY_HANDLE handle,                   /*Valid handle to communicate with device*/
    UINT16 timeout                      /*Break timeout value in milliseconds */
    )
{
    UINT16 wValue = 0, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus, ioTimeout = CY_USB_SERIAL_TIMEOUT ;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle..Function is %s\n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_UART){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid device type needs to be uart..Function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    bmRequestType = CY_CLASS_INTERFACE_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_UART_SEND_BREAK_CMD;
    wValue = timeout;
    wIndex = device->interfaceNum;
    wLength = 0;
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, NULL, wLength, ioTimeout);
    if (rStatus != LIBUSB_SUCCESS){
        CY_DEBUG_PRINT_ERROR ("CY:Error in setting break ... Libusb Error is %d \n", rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
    return CY_SUCCESS;
}
/*
   This Api sets the RTS UART pins High
 */
CY_RETURN_STATUS CyUartSetRts (
        CY_HANDLE handle
        )
{
    UINT16 wValue = 0, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle..Function is %s\n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_UART){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid device type needs to be uart..Function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    bmRequestType = CY_CLASS_INTERFACE_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_SET_LINE_CONTROL_STATE_CMD;
    wValue |= (1 << 1) | (device->dtrValue);
    wIndex = device->interfaceNum;
    wLength = 0;
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, NULL, wLength, ioTimeout);
    if (rStatus == CY_SUCCESS){
        device->rtsValue = 1;
        return CY_SUCCESS;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY:Error in setting RTS of UART ... Libusb Error is %d \n", rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
   This Api clears the RTS UART pin and makes it low
 */
CY_RETURN_STATUS CyUartClearRts (
        CY_HANDLE handle
        )
{
    UINT16 wValue = 0, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus, ioTimeout = CY_USB_SERIAL_TIMEOUT ;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle..Function is %s\n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_UART){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid device type needs to be uart..Function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    bmRequestType = CY_CLASS_INTERFACE_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_SET_LINE_CONTROL_STATE_CMD;
    wValue = (device->dtrValue);
    wIndex = device->interfaceNum;
    wLength = 0;

    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, NULL, wLength, ioTimeout);
    if (rStatus == CY_SUCCESS){
        device->rtsValue = 0;
        return CY_SUCCESS;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY:Error in clearing RTS of UART ... Libusb Error is %d \n", rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
   This Api sets the DTR UART pin High
 */
CY_RETURN_STATUS CyUartSetDtr (
        CY_HANDLE handle
        )

{
    UINT16 wValue = 0, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus, ioTimeout = CY_USB_SERIAL_TIMEOUT ;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle..Function is %s\n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_UART){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid device type needs to be uart..Function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    bmRequestType = CY_CLASS_INTERFACE_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_SET_LINE_CONTROL_STATE_CMD;
    wValue = ((device->rtsValue) << 1) | 1;
    wIndex = device->interfaceNum;
    wLength = 0;
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, NULL, wLength, ioTimeout);
    if (rStatus == CY_SUCCESS){
        device->dtrValue = 1;
        return CY_SUCCESS;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY:Error in setting DTR of UART ... Libusb Error is %d \n", rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
}

/*
   This Api clears the DTR UART pin and makes it low
 */

CY_RETURN_STATUS CyUartClearDtr (
        CY_HANDLE handle
        )
{
    UINT16 wValue = 0, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus, ioTimeout = CY_USB_SERIAL_TIMEOUT ;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle..Function is %s\n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;

    if (device->deviceType != CY_TYPE_UART){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid device type needs to be uart..Function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }

    bmRequestType = CY_CLASS_INTERFACE_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_SET_LINE_CONTROL_STATE_CMD;
    wValue = ((device->rtsValue) << 1);
    wIndex = device->interfaceNum;
    wLength = 0;

    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, NULL, wLength, ioTimeout);
    if (rStatus == CY_SUCCESS){
        device->dtrValue = 0;
        return CY_SUCCESS;
    }
    else{
        CY_DEBUG_PRINT_ERROR ("CY:Error in function %s... Libusb Error is %d \n",__func__, rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
}
