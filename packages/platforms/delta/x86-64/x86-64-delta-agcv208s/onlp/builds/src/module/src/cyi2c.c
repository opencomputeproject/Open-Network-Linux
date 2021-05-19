/*
 * I2C routines of Cypress USB Serial
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
typedef struct
{
    UINT32 frequency;           /**< Frequency of operation. Only valid values are
                                     100KHz and 400KHz. */
    UINT8 sAddress;             /**< Slave address to be used when in slave mode. */
    BOOL isMsbFirst;            /**< Whether to transmit most significant bit first. */
    BOOL isMaster;              /**< Whether to block is to be configured as a master:
                                     CyTrue - The block functions as I2C master.
                                     CyFalse - The block functions as I2C slave. */
    BOOL sIgnore;               /**< Ignore general call in slave mode. */
    BOOL clockStretch;          /**< Wheteher to stretch clock in case of no FIFO availability. */
    BOOL isLoopback;            /**< Whether to loop back TX data to RX. Valid only
                                     for debug purposes. */
    UCHAR reserved[6];          /**< Reserved for future use */
} CyUsI2cConfig_t;
#pragma pack()
#ifdef CY_I2C_ENABLE_PRECISE_TIMING
struct timeval startTimeWrite, endTimeWrite, startTimeRead, endTimeRead;
//Timer helper functions for proper timing
void startI2cTick (bool isWrite) {
    if (isWrite)
        gettimeofday (&startTimeWrite, NULL);
    else
        gettimeofday (&startTimeRead, NULL);
}

UINT32 getI2cLapsedTime (bool isWrite){

    signed int currentTime_sec, currentTime_usec, currentTime;
    if (isWrite){
        gettimeofday (&endTimeWrite, NULL);
        currentTime_sec = (endTimeWrite.tv_sec - startTimeWrite.tv_sec) * 1000;
        currentTime_usec = ((endTimeWrite.tv_usec - startTimeWrite.tv_usec)) / 1000;
        currentTime = currentTime_sec + currentTime_usec;
        return (unsigned int)currentTime;
    }
    else{
        gettimeofday (&endTimeRead, NULL);
        currentTime_sec = (endTimeRead.tv_sec - startTimeRead.tv_sec) * 1000;
        currentTime_usec = ((endTimeRead.tv_usec - startTimeRead.tv_usec)) / 1000;
        currentTime = currentTime_sec + currentTime_usec;
        return (unsigned int)currentTime;
    }
}
#endif
CY_RETURN_STATUS handleI2cError (UINT8 i2cStatus){

    if (i2cStatus & CY_I2C_NAK_ERROR_BIT){
        CY_DEBUG_PRINT_ERROR ("CY:Error Nacked by device ...Function is %s\n", __func__);
        return CY_ERROR_I2C_NAK_ERROR;
    }
    if (i2cStatus & CY_I2C_BUS_ERROR_BIT){
        CY_DEBUG_PRINT_ERROR ("CY:Error bus error occured... Function is %s\n", __func__);
        return CY_ERROR_I2C_BUS_ERROR;
    }
    if (i2cStatus & CY_I2C_ARBITRATION_ERROR_BIT){
        CY_DEBUG_PRINT_ERROR ("CY:Error Arbitration error occured.. Function is %s\n", __func__);
        return CY_ERROR_I2C_ARBITRATION_ERROR;
    }
    if (i2cStatus & CY_I2C_STOP_BIT_ERROR){
        CY_DEBUG_PRINT_ERROR ("CY:Error Stop bit set by master..Function is %s\n", __func__);
        return CY_ERROR_I2C_STOP_BIT_SET;
    }
    else {
        //We should never hit this case!!!!
        CY_DEBUG_PRINT_ERROR ("CY:Unknown error..Function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
}
//Used internally by read and write API to check if data is received at the I2C end.
CY_RETURN_STATUS CyI2cGetStatus (CY_HANDLE handle, bool mode, UCHAR *i2cStatus);
CY_RETURN_STATUS waitForNotification (CY_HANDLE handle, UINT16 *bytesPending, UINT32 ioTimeout);
/*
 *  This API gets the current I2C config
 *  for the particluar interface of the device
 */
CY_RETURN_STATUS CyGetI2cConfig (
        CY_HANDLE handle,
        CY_I2C_CONFIG *i2cConfig
        )
{
    UINT16 wValue, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    int rStatus;
    CyUsI2cConfig_t localI2cConfig;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT16 scbIndex = 0;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if (i2cConfig == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid parameter.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_I2C) {
        CY_DEBUG_PRINT_ERROR ("CY:Error opened device is not i2c ..Function is %s \n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    scbIndex = device->interfaceNum;
    if (scbIndex > 0)
        scbIndex = 1;
    bmRequestType = CY_VENDOR_REQUEST_DEVICE_TO_HOST;
    bmRequest = CY_I2C_GET_CONFIG_CMD;
    wValue = (scbIndex << CY_SCB_INDEX_POS);
    wIndex = 0x00;
    wLength = CY_I2C_CONFIG_LENGTH;

    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, (unsigned char*)&localI2cConfig, wLength, ioTimeout);
    if (rStatus == CY_I2C_CONFIG_LENGTH){
        CY_DEBUG_PRINT_INFO ("CY: Read I2C config ...size is %d \n", rStatus);
        i2cConfig->frequency = localI2cConfig.frequency;
        i2cConfig->slaveAddress = localI2cConfig.sAddress;
        i2cConfig->isMaster = localI2cConfig.isMaster;
        i2cConfig->isClockStretch = localI2cConfig.clockStretch;
        return CY_SUCCESS;
    }
    else if (rStatus == LIBUSB_ERROR_NO_DEVICE) {
        CY_DEBUG_PRINT_ERROR ("CY: Device Disconnected ....Function is %s\n", __func__);
        return CY_ERROR_DEVICE_NOT_FOUND;
    }
    else if (rStatus == LIBUSB_ERROR_TIMEOUT){
        CY_DEBUG_PRINT_ERROR ("CY:Error time out ....Function is %s\n", __func__);
        return CY_ERROR_IO_TIMEOUT;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY: Error in doing I2C read ...libusb error is %d function is %s!\n", rStatus, __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
 *  This API sets I2C config of the device for that
 *  interface
 */
CY_RETURN_STATUS CySetI2cConfig (
        CY_HANDLE handle,
        CY_I2C_CONFIG *i2cConfig
        )
{
    UINT16 wValue, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    CyUsI2cConfig_t localI2cConfig;
    int rStatus;
    CY_DEVICE *device = NULL;
    libusb_device_handle *devHandle;
    UINT16 scbIndex = 0;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if (i2cConfig == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid parameter.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    if (i2cConfig->frequency < 1000 || i2cConfig->frequency > 400000){
        CY_DEBUG_PRINT_ERROR ("CY:Error frequency trying to set in out of ..range Function is %s \n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    if ((i2cConfig->slaveAddress % 2) != 0){
        CY_DEBUG_PRINT_ERROR ("CY:Error slave address needs to even..Function is %s \n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    scbIndex = device->interfaceNum;
    if (device->deviceType != CY_TYPE_I2C) {
        CY_DEBUG_PRINT_ERROR ("CY:Error opened device is not i2c ..Function is %s \n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    if (scbIndex > 0)
        scbIndex = 1;
    bmRequestType = CY_VENDOR_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_I2C_SET_CONFIG_CMD;
    wValue = (scbIndex << CY_SCB_INDEX_POS);
    wIndex = 0x00;
    wLength = CY_I2C_CONFIG_LENGTH;
    //We need to pass entire 16 bytes config structure to firmware
    //but we will not expose all the structure elements to user.
    //so filling some of the values.
    memset (&localI2cConfig, 0, CY_I2C_CONFIG_LENGTH);
    localI2cConfig.frequency = i2cConfig->frequency;
    localI2cConfig.sAddress = i2cConfig->slaveAddress;
    localI2cConfig.isMaster = i2cConfig->isMaster;
    localI2cConfig.clockStretch = i2cConfig->isClockStretch;
    localI2cConfig.isMsbFirst = 1;
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, (unsigned char*)&localI2cConfig, wLength, ioTimeout);
    if (rStatus == CY_I2C_CONFIG_LENGTH){
        CY_DEBUG_PRINT_INFO ("CY: Setting I2C config successful ...\n");
        return CY_SUCCESS;
    }
    else if (rStatus == LIBUSB_ERROR_NO_DEVICE) {
        CY_DEBUG_PRINT_ERROR ("CY: Device Disconnected ....Function is %s\n", __func__);
        return CY_ERROR_DEVICE_NOT_FOUND;
    }
    else if (rStatus == LIBUSB_ERROR_TIMEOUT){
        CY_DEBUG_PRINT_ERROR ("CY:Error time out ....Function is %s\n", __func__);
        return CY_ERROR_IO_TIMEOUT;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY: Error in doing I2C read ...libusb error is %d function is %s!\n", rStatus, __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
 *  This API reads I2C data from the specified interface of the device
 *  interface
 */
CY_RETURN_STATUS CyI2cRead (
        CY_HANDLE handle,
        CY_I2C_DATA_CONFIG *i2cDataConfig,
        CY_DATA_BUFFER *readBuffer,
        UINT32 ioTimeout
        )
{
    int rStatus;
    CY_DEVICE *device = NULL;
    libusb_device_handle *devHandle;
    UINT16 wValue = 0, wIndex, wLength, bytesPending = 0;
    UINT8 bmRequestType, bmRequest;
    UCHAR i2cStatus[CY_I2C_GET_STATUS_LEN];
    UINT16 scbIndex = 0;
    bool mode = CY_I2C_MODE_READ;
    UINT32 elapsedTime;
    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if ((readBuffer == NULL) || (readBuffer->buffer == NULL) || (readBuffer->length == 0)){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid parameter.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    readBuffer->transferCount = 0;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_I2C) {
        CY_DEBUG_PRINT_ERROR ("CY:Error opened device is not i2c ..Function is %s \n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    if (pthread_mutex_trylock (&device->readLock) == 0){
        scbIndex = device->interfaceNum;
        if (scbIndex > 0)
            scbIndex = 1;
        i2cDataConfig->slaveAddress = ((i2cDataConfig->slaveAddress & 0x7F) | (scbIndex << 7));
        bmRequestType = CY_VENDOR_REQUEST_HOST_TO_DEVICE;
        bmRequest = CY_I2C_READ_CMD;
        wValue = ((i2cDataConfig->isStopBit) | (i2cDataConfig->isNakBit << 1));
        wValue |= (((i2cDataConfig->slaveAddress) << 8));
        wIndex = readBuffer->length;
        wLength = 0;
        rStatus = CyI2cGetStatus (handle, mode, (UCHAR *)i2cStatus);
        if (rStatus == CY_SUCCESS)
        {
            if ((i2cStatus[0] & CY_I2C_ERROR_BIT)){
                CY_DEBUG_PRINT_ERROR ("CY:Error device busy ... function is %s \n", __func__);
                pthread_mutex_unlock (&device->readLock);
                return CY_ERROR_I2C_DEVICE_BUSY;
            }
        }
        rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,wValue, wIndex, NULL, wLength, ioTimeout);
        if (rStatus == LIBUSB_ERROR_NO_DEVICE){
            CY_DEBUG_PRINT_ERROR ("CY:Error device disconnected ... \n");
            pthread_mutex_unlock (&device->readLock);
            return CY_ERROR_DEVICE_NOT_FOUND;
        }
        if (rStatus < 0){
            CY_DEBUG_PRINT_ERROR ("CY:Error in sending Read vendor command ... Libusb Error is %d .. Function is %s \n", rStatus, __func__);
            pthread_mutex_unlock (&device->readLock);
            return CY_ERROR_I2C_DEVICE_BUSY;
        }
        //Hoping that previous calls do not take much time!!
#ifdef CY_I2C_ENABLE_PRECISE_TIMING
        startI2cTick(false);
#endif
        rStatus = libusb_bulk_transfer (devHandle, device->inEndpoint, readBuffer->buffer, readBuffer->length,
                (int*)&readBuffer->transferCount, ioTimeout);
#ifdef CY_I2C_ENABLE_PRECISE_TIMING
        elapsedTime = getI2cLapsedTime(false);
        //Giving an extra 10 msec to notification to findout the status
        ioTimeout = (ioTimeout - elapsedTime);
        if (ioTimeout == 0)
            ioTimeout = 10;
#endif
        if (rStatus == LIBUSB_SUCCESS){
            CY_DEBUG_PRINT_INFO ("CY: Successfully read i2c data.. %d bytes Read ...\n", readBuffer->transferCount);
            bytesPending = readBuffer->length;
            rStatus = waitForNotification (handle, &bytesPending, ioTimeout);
            if (rStatus)
                readBuffer->transferCount = (readBuffer->length - bytesPending);
            else
                readBuffer->transferCount = readBuffer->length;
            pthread_mutex_unlock (&device->readLock);
            return rStatus;
        }
        else if (rStatus == LIBUSB_ERROR_TIMEOUT){
            CY_DEBUG_PRINT_ERROR ("CY:Timeout error ..Function is %s\n", __func__);
            pthread_mutex_unlock (&device->readLock);
            return CY_ERROR_IO_TIMEOUT;
        }
        else if (rStatus == LIBUSB_ERROR_PIPE){
            CY_DEBUG_PRINT_INFO ("Pipe Error \n");
            rStatus = CyResetPipe (handle, device->outEndpoint);
            if (rStatus != CY_SUCCESS){
                CY_DEBUG_PRINT_ERROR ("Error in reseting the pipe \n");
            }
            else {
                CY_DEBUG_PRINT_INFO ("Reset pipe succeded \n");
            }

            rStatus = CyI2cGetStatus (handle, mode, (UCHAR *)i2cStatus);
            if (rStatus == CY_SUCCESS)
            {
                CyI2cReset (handle, mode);
                rStatus = handleI2cError (i2cStatus[0]);
                pthread_mutex_unlock (&device->readLock);
                return rStatus;
            }
            else {
                pthread_mutex_unlock (&device->readLock);
                return CY_ERROR_I2C_DEVICE_BUSY;
            }
        }
        else if (rStatus == LIBUSB_ERROR_NO_DEVICE) {
            pthread_mutex_unlock (&device->readLock);
            CY_DEBUG_PRINT_ERROR ("CY: Device Disconnected ....Function is %s\n", __func__);
            return CY_ERROR_DEVICE_NOT_FOUND;
        }
        else {
            pthread_mutex_unlock (&device->readLock);
            CY_DEBUG_PRINT_ERROR ("CY: Error in doing I2C read ...libusb error is %d function is %s!\n", rStatus, __func__);
            return CY_ERROR_REQUEST_FAILED;
        }
    }
    else{
        CY_DEBUG_PRINT_ERROR ("CY: Error API busy in servicing previous request... function is %s!\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
 *  This API writes I2C data into the specified interface of the device
 */
CY_RETURN_STATUS CyI2cWrite (
        CY_HANDLE handle,
        CY_I2C_DATA_CONFIG *i2cDataConfig,
        CY_DATA_BUFFER *writeBuffer,
        UINT32 ioTimeout
        )
{
    int rStatus;
    UCHAR i2cStatus[CY_I2C_GET_STATUS_LEN];
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT16 wValue = 0, wIndex, wLength, bytesPending = 0;
    UINT8 bmRequestType, bmRequest;
    UINT16 scbIndex = 0;
    BOOL mode = CY_I2C_MODE_WRITE;
    UINT32 elapsedTime;
    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if ((writeBuffer == NULL) || (writeBuffer->buffer == NULL) || (writeBuffer->length == 0)){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid parameter.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    writeBuffer->transferCount = 0;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    scbIndex = device->interfaceNum;
    if (device->deviceType != CY_TYPE_I2C){
        CY_DEBUG_PRINT_ERROR ("CY:Error opened device is not i2c ..Function is %s \n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    if (pthread_mutex_trylock (&device->writeLock) == 0){
        if (scbIndex > 0)
            scbIndex = 1;
        bmRequestType = CY_VENDOR_REQUEST_HOST_TO_DEVICE;
        bmRequest = CY_I2C_WRITE_CMD;
        i2cDataConfig->slaveAddress = ((i2cDataConfig->slaveAddress & 0x7F) | (scbIndex << 7));
        wValue = ((i2cDataConfig->isStopBit));
        wValue |= (((i2cDataConfig->slaveAddress) << 8));
        wIndex = (UINT16)(writeBuffer->length);
        wLength = 0;
        CY_DEBUG_PRINT_INFO ("wValue is %x \n", wValue);
        //Send I2C write vendor command before actually sending the data over bulk ep
        rStatus = CyI2cGetStatus (handle, mode, (UCHAR *)i2cStatus);
        if (rStatus == CY_SUCCESS)
        {
            if ((i2cStatus[0] & CY_I2C_ERROR_BIT)){
                CY_DEBUG_PRINT_ERROR ("CY:Error ... Device busy ... function is %s \n", __func__);
                pthread_mutex_unlock (&device->writeLock);
                return CY_ERROR_I2C_DEVICE_BUSY;
            }
        }
        else if (rStatus == LIBUSB_ERROR_NO_DEVICE){
            CY_DEBUG_PRINT_ERROR ("CY:Error device not found \n");
            pthread_mutex_unlock (&device->writeLock);
            return CY_ERROR_DEVICE_NOT_FOUND;
        }
        rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,wValue, wIndex, NULL, wLength, ioTimeout);
        if (rStatus == LIBUSB_ERROR_NO_DEVICE){
            CY_DEBUG_PRINT_ERROR ("CY:Error device not found \n");
            pthread_mutex_unlock (&device->writeLock);
            return CY_ERROR_DEVICE_NOT_FOUND;
        }
        if (rStatus < 0){
            CY_DEBUG_PRINT_ERROR ("CY:Error in sending write vendor command ... Libusb Error is %d \n", rStatus);
            pthread_mutex_unlock (&device->writeLock);
            return CY_ERROR_I2C_DEVICE_BUSY;
        }
        //After vendor command is sent send the actual data to be sent to i2c devic
#ifdef CY_I2C_ENABLE_PRECISE_TIMING
        startI2cTick(true);
#endif
        rStatus = libusb_bulk_transfer (devHandle, device->outEndpoint, writeBuffer->buffer, writeBuffer->length,
                (int*)&(writeBuffer->transferCount), ioTimeout);
#ifdef CY_I2C_ENABLE_PRECISE_TIMING
        elapsedTime = getI2cLapsedTime(true);
        ioTimeout = (ioTimeout - elapsedTime);
        //Giving an extra 10 msec to notification to findout the status
        if (ioTimeout == 0)
            ioTimeout = 10;
#endif
        //Once the data is sent to usbserial, check if it was actually written to i2c device.
        if (rStatus == LIBUSB_SUCCESS){
            CY_DEBUG_PRINT_INFO ("CY: Successfully written i2c data.. %d bytes written ...\n", writeBuffer->transferCount);
            bytesPending = writeBuffer->length;
            rStatus = waitForNotification (handle, &bytesPending, ioTimeout);
            if (rStatus)
                writeBuffer->transferCount = (writeBuffer->length - bytesPending);
            else
                writeBuffer->transferCount = writeBuffer->length;
            pthread_mutex_unlock (&device->writeLock);
            return rStatus;
        }
        //Transaction is stallled when we hit some I2C error while the transfer
        //was going on. After we hit this error clear stall and check why we hit this by
        //CyGetStatus.
        else if (rStatus == LIBUSB_ERROR_PIPE){
            CY_DEBUG_PRINT_INFO ("CY:Pipe Error ... Function is %s\n", __func__);
            rStatus = CyResetPipe (handle, device->outEndpoint);
            if (rStatus != CY_SUCCESS){
                CY_DEBUG_PRINT_ERROR ("CY:Error in reseting the pipe ..Function is %s\n", __func__);
            }
            else {
                CY_DEBUG_PRINT_INFO ("Reset pipe succeded \n");
            }

            rStatus = CyI2cGetStatus (handle, mode, (UCHAR *)i2cStatus);
            if (rStatus == CY_SUCCESS)
            {
                CyI2cReset (handle, mode);
                rStatus = handleI2cError (i2cStatus[0]);
                pthread_mutex_unlock (&device->writeLock);
                return rStatus;
            }
        }
        else if (rStatus == LIBUSB_ERROR_NO_DEVICE) {
            CY_DEBUG_PRINT_ERROR ("CY: Device Disconnected ....Function is %s\n", __func__);
            pthread_mutex_unlock (&device->writeLock);
            return CY_ERROR_DEVICE_NOT_FOUND;
        }
        else if (rStatus == LIBUSB_ERROR_TIMEOUT){
            CY_DEBUG_PRINT_ERROR ("CY:Error time out ....Function is %s\n", __func__);
            pthread_mutex_unlock (&device->writeLock);
            return CY_ERROR_IO_TIMEOUT;
        }
        else{
            CY_DEBUG_PRINT_ERROR ("CY: Error in doing I2C read ...libusb error is %d function is %s!\n", rStatus, __func__);
            pthread_mutex_unlock (&device->writeLock);
            return CY_ERROR_REQUEST_FAILED;
        }
    }
    else{
        CY_DEBUG_PRINT_ERROR ("CY:API busy with servicing previous request... function is %s!\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    pthread_mutex_unlock (&device->writeLock);
    return CY_ERROR_REQUEST_FAILED;
}
/*
 *  This API gets the current status of the I2C data transaction
 */
CY_RETURN_STATUS CyI2cGetStatus (
        CY_HANDLE handle,
        bool mode,
        UCHAR *i2cStatus
        )
{
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT16 wValue, wIndex, wLength, bmRequestType, bmRequest;;
    UINT16 scbIndex = 0;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    if (i2cStatus == NULL)
        return CY_ERROR_INVALID_PARAMETER;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_I2C) {
        CY_DEBUG_PRINT_ERROR ("CY:Error opened device is not i2c .. \n");
        return CY_ERROR_REQUEST_FAILED;
    }
    scbIndex = device->interfaceNum;
    if (scbIndex > 0)
        scbIndex = 1;
    bmRequestType = CY_VENDOR_REQUEST_DEVICE_TO_HOST;
    bmRequest = CY_I2C_GET_STATUS_CMD;
    wValue = ((scbIndex << CY_SCB_INDEX_POS) | mode);
    wIndex = 0;
    wLength = CY_I2C_GET_STATUS_LEN;

    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,wValue, wIndex, (UCHAR*)i2cStatus, wLength, ioTimeout);
    if (rStatus < CY_I2C_GET_STATUS_LEN){
        CY_DEBUG_PRINT_INFO ("CY:Error in sending I2C Get Status command...Libusb error is %d\n", rStatus);
        return rStatus;
    }
    return CY_SUCCESS;
}
/*
 *  This API resets the I2C module
 */
CY_RETURN_STATUS CyI2cReset (
        CY_HANDLE handle,
        BOOL resetMode
        )
{
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT16 wValue, wIndex, wLength, bmRequestType, bmRequest;
    UINT16 scbIndex = 0;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_I2C) {
        CY_DEBUG_PRINT_ERROR ("CY:Error opened device is not i2c .. \n");
        return CY_ERROR_REQUEST_FAILED;
    }
    scbIndex = device->interfaceNum;
    if (scbIndex > 0)
        scbIndex = 1;
    bmRequestType = CY_VENDOR_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_I2C_RESET_CMD;
    wValue = ((scbIndex << CY_SCB_INDEX_POS) | resetMode );
    wIndex = 0;
    wLength = 0;
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,wValue, wIndex, NULL, wLength, ioTimeout);
    if (rStatus < 0){
        CY_DEBUG_PRINT_ERROR ("CY:Error in sending I2C Reset command ..libusb error is %d\n", rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
    return CY_SUCCESS;
}
static void LIBUSB_CALL i2c_notification_cb(struct libusb_transfer *transfer)
{
    UINT32 *completed = transfer->user_data;
    *completed = 1;
}

CY_RETURN_STATUS waitForNotification (CY_HANDLE handle, UINT16 *bytesPending, UINT32 ioTimeout){

    UINT32 transferCompleted = 0, length = CY_I2C_EVENT_NOTIFICATION_LEN;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    struct libusb_transfer *transfer;
    CY_RETURN_STATUS errorStatus, rStatus;
    UCHAR i2cStatus[CY_I2C_EVENT_NOTIFICATION_LEN];
    struct timeval time;

    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    transfer = libusb_alloc_transfer(0);
    if (transfer == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error in allocating trasnfer \n");
        errorStatus = CY_ERROR_ALLOCATION_FAILED;
        (*bytesPending) = 0;
        return errorStatus;
        //callbackFn (errorStatus, 0);
    }
    libusb_fill_interrupt_transfer (transfer, devHandle, device->interruptEndpoint, i2cStatus, length,
            i2c_notification_cb, &transferCompleted, ioTimeout);
    if (libusb_submit_transfer (transfer)){
        CY_DEBUG_PRINT_ERROR ("CY:Error in submitting interrupt transfer ...\n");
        libusb_cancel_transfer (transfer);
        libusb_free_transfer (transfer);
        (*bytesPending) = 0;
        //callbackFn (CY_ERROR_REQUEST_FAILED, 0);
        return CY_ERROR_REQUEST_FAILED;
    }
    time.tv_sec = 0;
    time.tv_usec = 50;//polling timeout.
    while (transferCompleted == 0){
        libusb_handle_events_timeout (glContext, &time);
    }
    transferCompleted = 0;
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED){
        CY_DEBUG_PRINT_INFO ("CY:Info successfully recieved data on interrupt pipe length is %d \n", transfer->actual_length);
        if (i2cStatus[0] & 0x80){ //Error notification is for write
            if ((i2cStatus[0] & CY_I2C_ERROR_BIT)){
                CY_DEBUG_PRINT_INFO ("Bytes pending is %x %x %x", i2cStatus[0], i2cStatus[1], i2cStatus[2]);
                //There was some error, so reset the i2c module and usb module
                //of the device, so branch out of the loop(Check below for the errors reported).
                rStatus = CyI2cReset (device, CY_I2C_MODE_WRITE);
                if (rStatus != CY_SUCCESS){
                    CY_DEBUG_PRINT_INFO ("CY:i2c reset failed \n");
                }
                //Report the amount of byte that were actually written
                memcpy(bytesPending, &i2cStatus[1], 2);
                errorStatus = handleI2cError (i2cStatus[0]);
            }
            else
                errorStatus = CY_SUCCESS;
        }
        else //Error notification is for read
        {
            if ((i2cStatus[0] & CY_I2C_ERROR_BIT)){
                CY_DEBUG_PRINT_INFO ("Bytes pending is %x %x %x", i2cStatus[0], i2cStatus[1], i2cStatus[2]);
                rStatus = CyI2cReset (device, CY_I2C_MODE_READ);
                if (rStatus != CY_SUCCESS){
                    CY_DEBUG_PRINT_INFO ("CY:i2c reset failed \n");
                }
                //Report the amount of byte that were actually written
                memcpy(bytesPending, &i2cStatus[1], 2);
                errorStatus = handleI2cError (i2cStatus[0]);
            }
            else
                errorStatus = CY_SUCCESS;
        }
        libusb_free_transfer (transfer);
        return errorStatus;
    }
    else{
        libusb_cancel_transfer (transfer);
        if (transfer->status == LIBUSB_TRANSFER_TIMED_OUT){
            CY_DEBUG_PRINT_ERROR ("CY:Error Timeout in getting i2c transfer status ....\n");
            CyI2cGetStatus (handle, 1, (UCHAR *)&errorStatus);
            errorStatus = CY_ERROR_IO_TIMEOUT;
        }
        if (transfer->status == LIBUSB_TRANSFER_OVERFLOW){
            CY_DEBUG_PRINT_ERROR ("CY:Error buffer overFlow in i2c transfer status ....\n");
            errorStatus = CY_ERROR_BUFFER_OVERFLOW;
        }
        if (transfer->status != LIBUSB_TRANSFER_COMPLETED){
            CY_DEBUG_PRINT_ERROR ("CY:Error in i2c transfer status ... Libusb transfer error is %d \n", transfer->status);
            errorStatus = CY_ERROR_REQUEST_FAILED;
        }
        libusb_free_transfer (transfer);
        return CY_ERROR_REQUEST_FAILED;
    }
}
