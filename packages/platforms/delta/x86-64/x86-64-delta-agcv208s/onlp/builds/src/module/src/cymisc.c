/*
 * Miscellaneous routines of Cypress USB Serial
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

typedef struct NOTIFICATION_CB_PARAM{

    CY_HANDLE handle;
    CY_EVENT_NOTIFICATION_CB_FN notificationCbFn;

}NOTIFICATION_CB_PARAM;
/*
   This API is used to Read the Bootloder version
 */
CY_RETURN_STATUS CyGetFirmwareVersion(
        CY_HANDLE handle,
        CY_FIRMWARE_VERSION *firmwareVersion
        )
{
    UINT16 wValue, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus, ioTimeout = CY_USB_SERIAL_TIMEOUT;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;

    bmRequestType = CY_VENDOR_REQUEST_DEVICE_TO_HOST;
    bmRequest = CY_GET_VERSION_CMD;
    wValue = 0x00;
    wIndex = 0x00;
    wLength = CY_GET_FIRMWARE_VERSION_LEN;

    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, (unsigned char*)firmwareVersion, wLength, ioTimeout);

    if (rStatus > 0){
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
The API resets the device
*/
CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyResetDevice (
        CY_HANDLE handle                                   /*Valid device handle*/
        )
{
    UINT16 wValue, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle \n");
        return CY_ERROR_INVALID_HANDLE;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;

    bmRequestType = CY_VENDOR_REQUEST_DEVICE_TO_HOST;
    bmRequest = CY_DEVICE_RESET_CMD;
    wValue = 0xA6B6;
    wIndex = 0xADBA;
    wLength = 0;

    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, NULL, wLength, ioTimeout);
    //return buffer will tell the status of the command
    if (rStatus == LIBUSB_SUCCESS)
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

CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CySetGpioValue (
        CY_HANDLE handle,                               /*Valid device handle*/
        UINT8 gpioNumber,                         /*GPIO configuration value*/
        UINT8 value                              /*Value that needs to be set*/
        )
{
    UINT16 wValue, wIndex, wLength;
    UINT8 bmRequestType, bmRequest, buffer[CY_GPIO_SET_LEN];
    int rStatus;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (value)
        value = 1;
    bmRequestType = CY_VENDOR_REQUEST_DEVICE_TO_HOST;
    bmRequest = CY_GPIO_SET_VALUE_CMD;
    wValue = gpioNumber;
    wIndex = value;
    wLength = 0;

    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, buffer, wLength, ioTimeout);
    if (rStatus >= 0){
        CY_DEBUG_PRINT_INFO ("CY: Get Configuration of GPIO succedded...size is %d \n", rStatus);
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

CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyGetGpioValue (
        CY_HANDLE handle,                                 /*Valid device handle*/
        UINT8 gpioNumber,                           /*GPIO configuration value*/
        UINT8 *value                               /*Value that needs to be set*/
        )
{
    UINT16 wValue, wIndex, wLength;
    UINT8 bmRequestType, bmRequest, buffer[CY_GPIO_GET_LEN];
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;

    bmRequestType = CY_VENDOR_REQUEST_DEVICE_TO_HOST;
    bmRequest = CY_GPIO_GET_VALUE_CMD;
    wValue = gpioNumber;
    wIndex = 0x00;
    wLength = CY_GPIO_GET_LEN;

    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, buffer, wLength, ioTimeout);
    if (rStatus == CY_GPIO_GET_LEN){
        CY_DEBUG_PRINT_INFO ("CY: Get GPIO Configuration succedded...size is %d \n", rStatus);
        //return buffer will tell the status of the command
        if (buffer[0] == 0){
            (*value) = buffer[1];
            return CY_SUCCESS;
        }
        else
            return CY_ERROR_REQUEST_FAILED;
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
static void LIBUSB_CALL uart_notification_cb(struct libusb_transfer *transfer)
{
    UINT32 *completed = transfer->user_data;
    *completed = 1;
}

void* uartSetEventNotifcation (void *inputParameters)
{
    int rStatus, transferCompleted = 0, length = CY_UART_EVENT_NOTIFICATION_LEN;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    struct libusb_transfer *transfer;
    UINT16 errorStatus = 0;
    UCHAR uartStatus[CY_UART_EVENT_NOTIFICATION_LEN];
    struct timeval time;
    CY_EVENT_NOTIFICATION_CB_FN callbackFn;
    NOTIFICATION_CB_PARAM *cbParameters = (NOTIFICATION_CB_PARAM*)inputParameters;
    callbackFn = cbParameters->notificationCbFn;

    device = (CY_DEVICE *)cbParameters->handle;
    devHandle = device->devHandle;
    callbackFn = cbParameters->notificationCbFn;
    device->uartTransfer = transfer = libusb_alloc_transfer(0);
    if (transfer == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error in allocating trasnfer \n");
        errorStatus |= CY_ERROR_EVENT_FAILED_BIT;
        callbackFn(errorStatus);
        goto END;
    }
    while (device->uartCancelEvent == false){
        libusb_fill_interrupt_transfer (transfer, devHandle, device->interruptEndpoint, uartStatus, length,
            uart_notification_cb, &transferCompleted, CY_EVENT_NOTIFICATION_TIMEOUT);
        rStatus = libusb_submit_transfer (transfer);
        if (rStatus){
            CY_DEBUG_PRINT_ERROR ("CY:Error submitting uart interrupt token ... Libusb error is %d\n", rStatus);
            errorStatus |= CY_ERROR_EVENT_FAILED_BIT;
            callbackFn(errorStatus);
            break;
        }
        time.tv_sec = 0;
        time.tv_usec = 50;//polling timeout.
        while (transferCompleted == 0){
            libusb_handle_events_timeout (glContext, &time);
        }
        transferCompleted = 0;
        if (transfer->status == LIBUSB_TRANSFER_COMPLETED){
            CY_DEBUG_PRINT_INFO ("Successfully read and recieved data %d \n", transfer->actual_length);
            memcpy (&errorStatus, &uartStatus[8], 2);
            printf ("%x %x ", uartStatus[8], uartStatus[9]);
            callbackFn (errorStatus);
            errorStatus = 0;
        }
        else{
            errorStatus |= CY_ERROR_EVENT_FAILED_BIT;
            if (device->uartCancelEvent == false){
                CY_DEBUG_PRINT_ERROR ("CY:Error uart interrupt thread encountered error... Libusb transmission error is %d \n", transfer->status);
                pthread_join (device->uartThread, NULL);
                device->uartThreadRunning = false;
                callbackFn(errorStatus);
            }
            break;
        }
    }
    CY_DEBUG_PRINT_INFO ("Exiting notification thread \n");
    libusb_free_transfer (transfer);
END:
   free (inputParameters);
   return NULL;
}

static void LIBUSB_CALL spi_notification_cb(struct libusb_transfer *transfer)
{
    UINT32 *completed = transfer->user_data;
    *completed = 1;
}

void* spiSetEventNotifcation (void *inputParameters)
{
    int transferCompleted = 0, length = CY_SPI_EVENT_NOTIFICATION_LEN;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    struct libusb_transfer *transfer;
    UINT16 spiStatus = 0;
    unsigned char spiStatusBuffer[2] = {0, 0};
    UINT16 errorStatus = 0;
    struct timeval time;
    CY_EVENT_NOTIFICATION_CB_FN callbackFn;
    NOTIFICATION_CB_PARAM *cbParameters = (NOTIFICATION_CB_PARAM*)inputParameters;

    callbackFn = cbParameters->notificationCbFn;
    device = (CY_DEVICE *)cbParameters->handle;
    devHandle = device->devHandle;
    callbackFn = cbParameters->notificationCbFn;
    device->spiTransfer = transfer = libusb_alloc_transfer(0);
    if (transfer == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error in allocating trasnfer \n");
        errorStatus |= CY_ERROR_EVENT_FAILED_BIT;
        callbackFn (errorStatus);
        goto END;
    }
    libusb_fill_interrupt_transfer (transfer, devHandle, device->interruptEndpoint, spiStatusBuffer, length,
            spi_notification_cb, &transferCompleted, CY_EVENT_NOTIFICATION_TIMEOUT);
    while (device->spiCancelEvent == false){
        if (libusb_submit_transfer (transfer)){
            CY_DEBUG_PRINT_ERROR ("CY:Error submitting spi interrupt token ... \n");
            errorStatus |= CY_ERROR_EVENT_FAILED_BIT;
            callbackFn(errorStatus);
            break;
        }
        time.tv_sec = 0;
        time.tv_usec = 50;//polling timeout.
        while (transferCompleted == 0){
            libusb_handle_events_timeout (glContext, &time);
        }
        transferCompleted = 0;
        spiStatus = (spiStatusBuffer[1] << 8) | spiStatusBuffer[0];
        if (transfer->status == LIBUSB_TRANSFER_COMPLETED){
            CY_DEBUG_PRINT_INFO ("Successfully read and recieved data %d \n", transfer->actual_length);
            if (spiStatus & CY_SPI_UNDERFLOW_ERROR){
                errorStatus |= (CY_SPI_TX_UNDERFLOW_BIT);
            }
            if (spiStatus & CY_SPI_BUS_ERROR){
                errorStatus |= (CY_SPI_BUS_ERROR_BIT);
            }
            callbackFn (errorStatus);
            errorStatus = 0;
        }
        else{
            spiStatus |= CY_ERROR_EVENT_FAILED_BIT;
            if (device->spiCancelEvent == false){
                pthread_join (device->spiThread, NULL);
                device->spiThreadRunning = false;
                CY_DEBUG_PRINT_ERROR ("CY:Error spi interrupt thread was cancelled... Libusb transmission error is %d \n", transfer->status);
                callbackFn (spiStatus);
            }
            break;
        }
    }
    libusb_free_transfer (transfer);
END:
    free (inputParameters);
    pthread_exit (NULL);
    return NULL;
}

CYWINEXPORT CY_RETURN_STATUS WINCALLCONVEN CySetEventNotification(
    CY_HANDLE handle,                               /*Valid handle to communicate with device*/
    CY_EVENT_NOTIFICATION_CB_FN notificationCbFn   /*Call back function in case on error during Uart data transfers*/
    )
{
    CY_DEVICE *device;
    NOTIFICATION_CB_PARAM *args = NULL;
    int ret;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if (notificationCbFn == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid parameter.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    device = (CY_DEVICE*)handle;
    pthread_mutex_lock (&device->notificationLock);
    args = (NOTIFICATION_CB_PARAM *)malloc (sizeof (NOTIFICATION_CB_PARAM));
    args->handle = handle;
    args->notificationCbFn = notificationCbFn;
    if (device->deviceType == CY_TYPE_SPI){
        if (device->spiThreadRunning) {
            CY_DEBUG_PRINT_ERROR ("CY:Error already notification thread exists ... Function is %s \n", __func__);
            free (args);
            pthread_mutex_unlock (&device->notificationLock);
            return CY_ERROR_STATUS_MONITOR_EXIST;
        }
        ret = pthread_create (&device->spiThread, NULL, spiSetEventNotifcation, (void *) args);
        if (ret == 0){
            device->spiThreadRunning = true;
            pthread_mutex_unlock (&device->notificationLock);
            return CY_SUCCESS;
        }
        else {
            pthread_join (device->spiThread, NULL);
            device->spiThreadRunning = false;
            free (args);
            pthread_mutex_unlock (&device->notificationLock);
            CY_DEBUG_PRINT_ERROR ("CY:Error creating spi notification thread ... Function is %s \n", __func__);
            return CY_ERROR_REQUEST_FAILED;
        }
    }
    else if (device->deviceType == CY_TYPE_UART){
        if (device->uartThreadRunning) {
            CY_DEBUG_PRINT_ERROR ("CY:Error already notification thread exists ... Function is %s \n", __func__);
            free (args);
            pthread_mutex_unlock (&device->notificationLock);
            return CY_ERROR_STATUS_MONITOR_EXIST;
        }
        ret = pthread_create (&device->uartThread, NULL, uartSetEventNotifcation, (void *) args);
        if (ret == 0){
            device->uartThreadRunning = true;
            pthread_mutex_unlock (&device->notificationLock);
            return CY_SUCCESS;
        }
        else {
            pthread_join (device->uartThread, NULL);
            device->uartThreadRunning = false;
            free (args);
            pthread_mutex_unlock (&device->notificationLock);
            CY_DEBUG_PRINT_ERROR ("CY:Error creating uart notification thread ... Function is %s \n", __func__);
            return CY_ERROR_REQUEST_FAILED;
        }
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY:Error unknown device type ....Function is %s \n", __func__);
        free (args);
        pthread_mutex_unlock (&device->notificationLock);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*The API is used to cancel the uart Event notification*/
CYWINEXPORT CY_RETURN_STATUS WINCALLCONVEN CyAbortEventNotification(
    CY_HANDLE handle                               /*Valid handle to communicate with device*/
    )
{
    CY_DEVICE *device;
    device = (CY_DEVICE*)handle;
    pthread_mutex_lock (&device->notificationLock);
    if (device->deviceType == CY_TYPE_UART){
        if (!device->uartThreadRunning){
            CY_DEBUG_PRINT_ERROR ("CY:Error uart event notification not created ....function is %s \n", __func__);
            pthread_mutex_unlock (&device->notificationLock);
            return CY_ERROR_REQUEST_FAILED;
        }
        device->uartCancelEvent = true;
        libusb_cancel_transfer (device->uartTransfer);
        pthread_join (device->uartThread, NULL);
        device->uartThreadRunning = false;
        device->uartCancelEvent = false;
        pthread_mutex_unlock (&device->notificationLock);
        return CY_SUCCESS;
    }
    else if (device->deviceType == CY_TYPE_SPI){
        if (!device->spiThreadRunning){
            CY_DEBUG_PRINT_ERROR ("CY:Error spi event notification not created ....function is %s \n", __func__);
            pthread_mutex_unlock (&device->notificationLock);
            return CY_ERROR_REQUEST_FAILED;
        }
        device->spiCancelEvent = true;
        libusb_cancel_transfer (device->spiTransfer);
        pthread_join (device->spiThread, NULL);
        device->spiThreadRunning = false;
        device->spiCancelEvent = false;
        pthread_mutex_unlock (&device->notificationLock);
        return CY_SUCCESS;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY:Error.. unknown device type ....function is %s \n", __func__);
        pthread_mutex_unlock (&device->notificationLock);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
The API is used to programme user flash area
*/
CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyProgUserFlash (
    CY_HANDLE handle,                       /*Valid device handle*/
    CY_DATA_BUFFER *progBuffer,             /*data buffer containing buffer address, length to write*/
    UINT32 flashAddress,                    /*Address to the data is written*/
    UINT32 ioTimeout                          /*Timeout value of the API*/
    )
{
    UINT16 wValue, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    if ((progBuffer == NULL) || (progBuffer->buffer == NULL))
        return CY_ERROR_INVALID_PARAMETER;

    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;

    bmRequestType = CY_VENDOR_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_PROG_USER_FLASH_CMD;
    wValue = 0;
    wIndex = flashAddress;
    wLength = progBuffer->length;

    CY_DEBUG_PRINT_INFO ("CY:The Length is %d , Value is %d and index is %d\n", wLength, wValue, wIndex);
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, progBuffer->buffer, wLength, ioTimeout);
    if (rStatus > 0){
        (progBuffer->transferCount) = rStatus;
        return CY_SUCCESS;
    }
     else if (rStatus == LIBUSB_ERROR_TIMEOUT){
        CY_DEBUG_PRINT_ERROR ("CY:Time out error ..function is %s \n", __func__);
        (progBuffer->transferCount) = 0;
        return CY_ERROR_IO_TIMEOUT;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY: Error in function %s ...libusb error is %d!\n", __func__, rStatus);
        (progBuffer->transferCount) = 0;
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
The API is used to programme user flash area
*/
CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyReadUserFlash (
    CY_HANDLE handle,                       /*Valid device handle*/
    CY_DATA_BUFFER *readBuffer,             /*data buffer containing buffer address, length to write*/
    UINT32 flashAddress,                    /*Address to the data is written*/
    UINT32 ioTimeout                          /*Timeout value of the API*/
    )
{
    UINT16 wValue, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    if (readBuffer == NULL)
        return CY_ERROR_INVALID_PARAMETER;

    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;

    bmRequestType = CY_VENDOR_REQUEST_DEVICE_TO_HOST;
    bmRequest = CY_READ_USER_FLASH_CMD;
    wValue = 0;
    wIndex = flashAddress;
    wLength = readBuffer->length;

    CY_DEBUG_PRINT_INFO ("CY:The Length is %d , Value is %d and index is %d\n", wLength, wValue, wIndex);
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, readBuffer->buffer, wLength, ioTimeout);
    if (rStatus > 0){
        (readBuffer->transferCount) = rStatus;
        return CY_SUCCESS;
    }
    else if (rStatus == LIBUSB_ERROR_TIMEOUT){
        CY_DEBUG_PRINT_ERROR ("CY:Time out error ..function is %s \n", __func__);
        (readBuffer->transferCount) = 0;
        return CY_ERROR_IO_TIMEOUT;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY: Error in function %s ...libusb error is %d!\n", __func__, rStatus);
        (readBuffer->transferCount) = 0;
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
   This API is used to get the signature of the device. It would be CYUS when we are in actual device mode
   and CYBL when we are bootloader mode
 */
CY_RETURN_STATUS CyGetSignature (
        CY_HANDLE handle,
        UCHAR *signature
        )
{
    UINT16 wValue, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus, ioTimeout = CY_USB_SERIAL_TIMEOUT;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;

    bmRequestType = CY_VENDOR_REQUEST_DEVICE_TO_HOST;
    bmRequest = CY_GET_SIGNATURE_CMD;
    wValue = 0x00;
    wIndex = 0x00;
    wLength = CY_GET_SIGNATURE_LEN;

    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, (unsigned char*)signature, wLength, ioTimeout);
    if (rStatus > 0){
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
