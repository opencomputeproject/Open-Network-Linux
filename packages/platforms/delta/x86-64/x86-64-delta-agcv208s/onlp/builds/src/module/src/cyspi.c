/*
 * SPI routines of Cypress USB Serial
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
#include <signal.h>
#include <sys/time.h>
#pragma pack (1)
typedef struct args {
    CY_HANDLE handle;
    UCHAR *readBuffer;
    UINT32 length;
    UINT32 ioTimeout;
    CY_RETURN_STATUS rStatus;
    UINT32 transferCount;
}args;

typedef struct
{
    UINT32 frequency;
    UINT8 dataWidth;
    UCHAR mode;
    UCHAR xferMode;
    BOOL isMsbFirst;
    BOOL isMaster;
    BOOL isContinuous;
    BOOL isSelectPrecede;
    BOOL cpha;
    BOOL cpol;
    BOOL isLoopback;
    UCHAR reserved[2];
} CyUsSpiConfig_t;
#pragma pack()

struct timeval startSpiTimeWrite, endSpiTimeWrite, startSpiTimeRead, endSpiTimeRead;
//Timer helper functions for proper timing
void startSpiTick (bool isWrite) {
    if (isWrite)
        gettimeofday (&startSpiTimeWrite, NULL);
    else
        gettimeofday (&startSpiTimeRead, NULL);
}

UINT32 getSpiLapsedTime (bool isWrite){

    signed int currentTime_sec, currentTime_usec, currentTime;
    if (isWrite){
        gettimeofday (&endSpiTimeWrite, NULL);
        currentTime_sec = (endSpiTimeWrite.tv_sec - startSpiTimeWrite.tv_sec) * 1000;
        currentTime_usec = ((endSpiTimeWrite.tv_usec - startSpiTimeWrite.tv_usec)) / 1000;
        currentTime = currentTime_sec + currentTime_usec;
        return (unsigned int)currentTime;
    }
    else{
        gettimeofday (&endSpiTimeRead, NULL);
        currentTime_sec = (endSpiTimeRead.tv_sec - startSpiTimeRead.tv_sec) * 1000;
        currentTime_usec = ((endSpiTimeRead.tv_usec - startSpiTimeRead.tv_usec)) / 1000;
        currentTime = currentTime_sec + currentTime_usec;
        return (unsigned int)currentTime;
    }
}
/*
   This API gets the current SPI config
   for the particluar interface of the device
 */
CY_RETURN_STATUS CyGetSpiConfig (
        CY_HANDLE handle,
        CY_SPI_CONFIG *spiConfig
        )
{
    UINT16 wValue, wIndex, wLength;
    UINT16 bmRequestType, bmRequest;
    CyUsSpiConfig_t localSpiConfig;
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;
    UINT8 scbIndex = 0;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if (spiConfig == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid parameter.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_SPI) {
        CY_DEBUG_PRINT_ERROR ("CY:Error opened device is not spi ..Function is %s \n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    if (device->interfaceNum > 0)
        scbIndex = 1;

    bmRequestType = CY_VENDOR_REQUEST_DEVICE_TO_HOST;
    bmRequest = CY_SPI_GET_CONFIG_CMD;
    wValue = (scbIndex << CY_SCB_INDEX_POS);
    wIndex = 0;
    wLength = CY_SPI_CONFIG_LEN;

    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, (unsigned char*)&localSpiConfig, wLength, ioTimeout);
    if (rStatus == CY_SPI_CONFIG_LEN){
        //CY_DUMP_DATA ((unsigned char*)&localSpiConfig, wLength);
        CY_DEBUG_PRINT_INFO ("CY: Read SPI config successfully %d\n", rStatus);
        spiConfig->frequency = localSpiConfig.frequency;
        spiConfig->dataWidth = localSpiConfig.dataWidth;
        spiConfig->protocol = localSpiConfig.mode;
        spiConfig->isMsbFirst = localSpiConfig.isMsbFirst;
        spiConfig->isMaster = localSpiConfig.isMaster;
        spiConfig->isContinuousMode = localSpiConfig.isContinuous;
        spiConfig->isSelectPrecede = localSpiConfig.isSelectPrecede;
        spiConfig->isCpha = localSpiConfig.cpha;
        spiConfig->isCpol = localSpiConfig.cpol;
        return CY_SUCCESS;
    }
    else if (rStatus == LIBUSB_ERROR_TIMEOUT){
        CY_DEBUG_PRINT_ERROR ("CY:Time out error ... Function is %s\n", __func__);
        return CY_ERROR_IO_TIMEOUT;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY: Error in function %s...libusb error is %d !\n", __func__, rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*
   This API sets SPI config of the device for that
   interface
 */
CY_RETURN_STATUS CySetSpiConfig (
        CY_HANDLE handle,
        CY_SPI_CONFIG *spiConfig
        )
{
    UINT16 wValue, wIndex, wLength;
    UINT8 bmRequestType, bmRequest;
    CyUsSpiConfig_t localSpiConfig;
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;
    UINT8 scbIndex = 0;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if (spiConfig == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid parameter.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }

    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_SPI) {
        CY_DEBUG_PRINT_ERROR ("CY:Error device type is not spi ... Function is %s \n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    if (spiConfig->frequency < 1000 || spiConfig->frequency > 3000000){
        CY_DEBUG_PRINT_ERROR ("CY:Error frequency trying to set in out of range ... Function is %s\n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    if (spiConfig->protocol == CY_SPI_TI){
        if (!(spiConfig->isCpol == false && spiConfig->isCpha == true && spiConfig->isContinuousMode == false)){
            CY_DEBUG_PRINT_ERROR ("CY:Error ... Wrong configuration for SPI TI mode \n");
            return CY_ERROR_REQUEST_FAILED;
        }
    }
    if (spiConfig->protocol == CY_SPI_NS){
        if (!(spiConfig->isCpol == false && spiConfig->isCpha == false && spiConfig->isSelectPrecede == false)){
            CY_DEBUG_PRINT_ERROR ("CY:Error ... Wrong configuration for SPI ti mode \n");
            return CY_ERROR_REQUEST_FAILED;
        }
    }
    else{
        if (spiConfig->isSelectPrecede != false){
            CY_DEBUG_PRINT_ERROR ("CY:Error ... Wrong configuration for SPI motorola mode \n");
            return CY_ERROR_REQUEST_FAILED;
        }
    }
    if (device->interfaceNum > 0)
        scbIndex = 1;
    bmRequestType = CY_VENDOR_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_SPI_SET_CONFIG_CMD;
    wValue = (scbIndex << CY_SCB_INDEX_POS);
    wIndex = 0;
    wLength = CY_SPI_CONFIG_LEN;
    //We will not expose all the spi config structure elements to user.
    //Fill in rest of the values.

    memset (&localSpiConfig, 0, CY_SPI_CONFIG_LEN);
    localSpiConfig.frequency = spiConfig->frequency;
    localSpiConfig.dataWidth = spiConfig->dataWidth;
    localSpiConfig.mode = spiConfig->protocol;
    localSpiConfig.isMsbFirst = spiConfig->isMsbFirst;
    localSpiConfig.isMaster = spiConfig->isMaster;
    localSpiConfig.isContinuous = spiConfig->isContinuousMode;
    localSpiConfig.isSelectPrecede = spiConfig->isSelectPrecede;
    localSpiConfig.cpha = spiConfig->isCpha;
    localSpiConfig.cpol = spiConfig->isCpol;
    //CY_DUMP_DATA ((unsigned char*)&localSpiConfig, wLength);
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
            wValue, wIndex, (unsigned char*)&localSpiConfig, wLength, ioTimeout);
    if (rStatus == CY_SPI_CONFIG_LEN){
        CY_DEBUG_PRINT_INFO ("CY: Setting SPI config success ...\n");
        return CY_SUCCESS;
    }
    else if (rStatus == LIBUSB_ERROR_TIMEOUT){
        CY_DEBUG_PRINT_ERROR ("CY:Time out error ..Function is %s\n", __func__);
        return CY_ERROR_IO_TIMEOUT;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY: Error in function %s ... !libusb error is %d\n", __func__, rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
}
/*Api will reset the spi module*/
CY_RETURN_STATUS CySpiReset (CY_HANDLE handle)
{
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT16 wValue, wIndex, wLength, bmRequestType, bmRequest;;
    UINT16 scbIndex = 0;
    UINT32 ioTimeout = CY_USB_SERIAL_TIMEOUT;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_SPI) {
        CY_DEBUG_PRINT_ERROR ("CY:Error device type is not spi ... Function is %s \n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    scbIndex = device->interfaceNum;
    if (scbIndex > 0)
        scbIndex = 1;
    bmRequestType = CY_VENDOR_REQUEST_DEVICE_TO_HOST;
    bmRequest = CY_SPI_RESET_CMD;
    wValue = ((scbIndex << CY_SCB_INDEX_POS));
    wIndex = 0;
    wLength = 0;
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,wValue, wIndex, NULL, wLength, ioTimeout);
    if (rStatus < 0){
        CY_DEBUG_PRINT_ERROR ("CY:Error in sending spi reset command...Libusb error is %d\n", rStatus);
        return rStatus;
    }
    return CY_SUCCESS;
}
/*
   This API reads SPI data from the specified interface of the device
   interface
 */

static void LIBUSB_CALL spi_read_cb(struct libusb_transfer *transfer)
{
	UINT32 *completed = transfer->user_data;
    *completed = 1;
}
//We adopted for async method here because there are 2 thread polling same fd
// i.e both read and write are polling same fd when one event triggers and other one is
//not completed then another thread will wait for more than 60sec.
CY_RETURN_STATUS CySpiRead (
        CY_HANDLE handle,
        CY_DATA_BUFFER *readBuffer,
        UINT32 ioTimeout
        )
{
    struct libusb_transfer *readTransfer;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    int readCompleted = 0;
    struct timeval time;
    int r;
    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    readBuffer->transferCount = 0;
    readTransfer = libusb_alloc_transfer(0);
    if (readTransfer == NULL){
        CY_DEBUG_PRINT_ERROR("CY:Error in allocating transfers \n");
        return CY_ERROR_ALLOCATION_FAILED;
    }
    libusb_fill_bulk_transfer(readTransfer, devHandle, device->inEndpoint, readBuffer->buffer, readBuffer->length,
            spi_read_cb, &readCompleted, ioTimeout);
    libusb_submit_transfer (readTransfer);
    time.tv_sec = (ioTimeout / 1000);
    time.tv_usec = ((ioTimeout % 1000) * 1000);//polling timeout.
    while (readCompleted == 0){
        r = libusb_handle_events_timeout_completed(glContext, &time, &readCompleted);
        if (r < 0) {
			if (r == LIBUSB_ERROR_INTERRUPTED)
				continue;
			libusb_cancel_transfer(readTransfer);
			while (!readCompleted)
				if (libusb_handle_events_completed(glContext, &readCompleted) < 0)
					break;
            readBuffer->transferCount = readTransfer->actual_length;
			libusb_free_transfer(readTransfer);
			return r;
		}
    }
    if (readTransfer->status == LIBUSB_TRANSFER_COMPLETED){
        readBuffer->transferCount = readTransfer->actual_length;
        libusb_free_transfer (readTransfer);
        return CY_SUCCESS;
    }
    else{
        if (readTransfer->status == LIBUSB_TRANSFER_TIMED_OUT){
            //We should not be hitting this case.. As the time out is infinite!!
            CY_DEBUG_PRINT_ERROR ("CY:Timeout error in doing SPI read/write .... %d Libusb errors %d\n",
                            readTransfer->actual_length,readTransfer->status);
            readBuffer->transferCount = readTransfer->actual_length;
            CySpiReset (handle);
            libusb_free_transfer (readTransfer);
            return CY_ERROR_IO_TIMEOUT;
        }
        if (readTransfer->status == LIBUSB_TRANSFER_OVERFLOW){
            //Need to handle this properly!
            CY_DEBUG_PRINT_ERROR ("CY:OverFlow error in doing SPI read/write .... Libusb errors %d %d \n",
                            readTransfer->status, readTransfer->actual_length);
            readBuffer->transferCount = readTransfer->actual_length;
            CySpiReset (handle);
            libusb_free_transfer (readTransfer);
            return CY_ERROR_BUFFER_OVERFLOW;
        }
        if (readTransfer->status != LIBUSB_TRANSFER_COMPLETED){
            CY_DEBUG_PRINT_ERROR ("CY:Error in doing SPI read/write .... Libusb errors are %d %d\n",
                            readTransfer->status, readTransfer->actual_length);
            readBuffer->transferCount = readTransfer->actual_length;
            CySpiReset (handle);
            libusb_free_transfer (readTransfer);
            //If timer is not completed then it implies we have timeout error
            return CY_ERROR_REQUEST_FAILED;
        }
    }
    return CY_ERROR_REQUEST_FAILED;
}
/*Internal SPI get status API for Write operation*/
CY_RETURN_STATUS CyGetSpiStatus (CY_HANDLE handle,
        int *spiStatus
        )
{
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    UINT16 wValue, wIndex, wLength, bmRequestType, bmRequest;;
    UINT16 scbIndex = 0;
    UINT32 ioTimeout = 0;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_SPI) {
        CY_DEBUG_PRINT_ERROR ("CY:Error device type is not spi ... Function is %s \n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    scbIndex = device->interfaceNum;
    if (scbIndex > 0)
        scbIndex = 1;
    bmRequestType = CY_VENDOR_REQUEST_DEVICE_TO_HOST;
    bmRequest = CY_SPI_GET_STATUS_CMD;
    wValue = ((scbIndex << CY_SCB_INDEX_POS));
    wIndex = 0;
    wLength = CY_SPI_GET_STATUS_LEN;
    rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,wValue, wIndex, (UCHAR*)spiStatus, wLength, ioTimeout);
    if (rStatus < CY_SPI_GET_STATUS_LEN){
        CY_DEBUG_PRINT_INFO ("CY:Error in sending spi Get Status command...Libusb error is %d\n", rStatus);
        return rStatus;
    }
    return CY_SUCCESS;
}
/* Function to write on to SPI alone*/
CY_RETURN_STATUS CySpiWrite (
        CY_HANDLE handle,
        CY_DATA_BUFFER *writeBuffer,
        UINT32 ioTimeout
        )
{
    int rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    int spiStatus = 1;
    UINT32 newIoTimeout = ioTimeout, elapsedTime = 0, loopCount = 1;
    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_SPI) {
        CY_DEBUG_PRINT_ERROR ("CY:Error device type is not spi ... Function is %s \n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    startSpiTick (true);
    rStatus = libusb_bulk_transfer (devHandle, device->outEndpoint, writeBuffer->buffer, writeBuffer->length,
            (int*)&(writeBuffer->transferCount), newIoTimeout);
    elapsedTime = getSpiLapsedTime(true);
    newIoTimeout = ioTimeout - elapsedTime;
    //because we have a sleep of 1 msec after every getstatus
    if (newIoTimeout)
        loopCount = (newIoTimeout);
    if (rStatus == LIBUSB_SUCCESS){
        CY_DEBUG_PRINT_INFO ("CY: Successfully written SPI data.. %d bytes Read ...\n", writeBuffer->transferCount);
        while (loopCount){
            usleep (1000);
            rStatus = CyGetSpiStatus (handle, &spiStatus);
            if (rStatus == CY_SUCCESS){
                if (spiStatus == 0){
                    return CY_SUCCESS;
                }
            }
            else {
                //Should never hit this case
                CY_DEBUG_PRINT_ERROR ("CY:Error in getting spi status \n");
                return CY_ERROR_REQUEST_FAILED;
            }
            if (ioTimeout)
                loopCount--;
        }
        if (loopCount == 0 && spiStatus > 0){
            writeBuffer->length = 0;
            CySpiReset (handle);
            return CY_ERROR_IO_TIMEOUT;
        }
    }
    else if (rStatus == LIBUSB_ERROR_TIMEOUT){
        CY_DEBUG_PRINT_ERROR ("CY:Error TimeOut ...function is %s\n", __func__);
        CySpiReset (handle);
        return CY_ERROR_IO_TIMEOUT;
    }
    else if (rStatus == LIBUSB_ERROR_PIPE){
        CY_DEBUG_PRINT_ERROR ("CY:Error Pipe error..function is %s\n", __func__);
        CySpiReset (handle);
        CyResetPipe (handle, device->outEndpoint);
        return CY_ERROR_PIPE_HALTED;
    }
    else if (rStatus == LIBUSB_ERROR_OVERFLOW){
        CY_DEBUG_PRINT_ERROR ("CY:Error Buffer Overflow...function is %s\n", __func__);
        return CY_ERROR_BUFFER_OVERFLOW;
    }
    else if (rStatus == LIBUSB_ERROR_NO_DEVICE) {
        CY_DEBUG_PRINT_ERROR ("CY:Error Device Disconnected ...function is %s\n", __func__);
        return CY_ERROR_DEVICE_NOT_FOUND;
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY:Error in writing SPI data ...Libusb Error is %d and bytes read is %d!\n", rStatus, writeBuffer->transferCount);
        return CY_ERROR_REQUEST_FAILED;
    }
    return CY_ERROR_REQUEST_FAILED;
}/*
    API to wrap up the data
  */
void spiCollectData (void *inputParameters) {

    UINT32 readLength = 0, length;
    CY_DATA_BUFFER readBuffer;
    args *inputData = (args *) inputParameters;
    UCHAR *buffer;
    CY_RETURN_STATUS rStatus = CY_SUCCESS;
    buffer = readBuffer.buffer = inputData->readBuffer;
    length = readBuffer.length = inputData->length;
    CY_HANDLE handle = inputData->handle;
    int newTimeout = inputData->ioTimeout, elapsedTime;
    while (readLength != length && newTimeout >= 0 && rStatus == CY_SUCCESS){
        //Get current time
        //Buffer is pointing to next address where we are suppose to fill the data
        readBuffer.buffer = &buffer[readLength];
        //Updated length which total length minus the total length of data read
        readBuffer.length = length - readLength;
        //Libusb fix for mac os!!
        //ISSUE:when api times out in MAC it comes back and say read length = 0!!
#ifdef __APPLE__
        if (readBuffer.length > 64)
            readBuffer.length = 64;
#endif
        startSpiTick (false);
        rStatus = CySpiRead (handle, &readBuffer, newTimeout);
        elapsedTime = getSpiLapsedTime (false);
        //Do this only when newTimeout is non zero
        if (newTimeout){
            newTimeout = newTimeout - elapsedTime;
            //If timeout is 0 then libusb considers that as infinite
            //So forcefully make the loop to comeout
            if (newTimeout <= 0)
                rStatus = CY_ERROR_IO_TIMEOUT;
        }
        if (rStatus != CY_SUCCESS){
            readLength += readBuffer.transferCount;
            break;
        }
        readLength += readBuffer.transferCount;
    }
    if (readLength != length && rStatus == CY_ERROR_IO_TIMEOUT){
        CySpiReset (handle);
    }
    inputData->transferCount = readLength;
    inputData->rStatus = rStatus;
}
/*
 * Api used to do read as well as write on spi
 */
CY_RETURN_STATUS CySpiReadWrite (CY_HANDLE handle,
        CY_DATA_BUFFER *readBuffer,
        CY_DATA_BUFFER *writeBuffer,
        UINT32 ioTimeout)
{
    struct args threadParameter;
    UINT32 ret;
    pthread_t readThreadID;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    CY_RETURN_STATUS rStatus;
    unsigned short spiTransferMode = 0, scbIndex = 0;
    UINT16 wValue, wIndex = 0, wLength;
    UINT16 bmRequestType, bmRequest;

    if (handle == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid handle.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_HANDLE;
    }
    if (readBuffer == NULL && writeBuffer == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid parameter.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType != CY_TYPE_SPI) {
        CY_DEBUG_PRINT_ERROR ("CY:Error opened device is not spi .. \n");
        return CY_ERROR_REQUEST_FAILED;
    }
    //Set both the bits and change it accordingly based on parameters parameters
    spiTransferMode |= ((CY_SPI_READ_BIT) | (CY_SPI_WRITE_BIT));
    if ((readBuffer == NULL || readBuffer->length == 0 || readBuffer->buffer == NULL))
        spiTransferMode &= ~(CY_SPI_READ_BIT);
    if ((writeBuffer == NULL || writeBuffer->length == 0 || writeBuffer->buffer == NULL))
        spiTransferMode &= ~(CY_SPI_WRITE_BIT);
    //if none of the bit is set it implies parameters sent is wrong
    if (spiTransferMode == 0){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid parameter.. Function is %s \n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    if (device->interfaceNum > 0)
        scbIndex = 1;
    //In read only case we take length to be equal to readBuffer length.
    //But in write or in write/read case we take length = writeBuffer length.
    if (spiTransferMode == 0x1)
        wIndex = readBuffer->length;
    else
        wIndex = writeBuffer->length;
    spiTransferMode |= (scbIndex << 15);
    bmRequestType = CY_VENDOR_REQUEST_HOST_TO_DEVICE;
    bmRequest = CY_SPI_READ_WRITE_CMD;
    wValue = (spiTransferMode);
    wLength = 0;
    if (pthread_mutex_trylock (&device->writeLock) == 0){
        rStatus = libusb_control_transfer (devHandle, bmRequestType, bmRequest,
                wValue, wIndex, NULL, wLength, 5000);
        if (rStatus){
            CY_DEBUG_PRINT_ERROR ("CY:Error Sending spi read write vendor command failed ... Libusb error is %d\n", rStatus);
            pthread_mutex_unlock (&device->writeLock);
            return CY_ERROR_REQUEST_FAILED;
        }
        //Read Bit is not set then write Only
        if (!(spiTransferMode & CY_SPI_READ_BIT)) {
            writeBuffer->transferCount = 0;
            if (readBuffer)
                readBuffer->transferCount = 0;
            rStatus = CySpiWrite (handle, writeBuffer, ioTimeout);
            pthread_mutex_unlock (&device->writeLock);
            return rStatus;
        }
        //Write Bit is not set then read only
        if (!(spiTransferMode & CY_SPI_WRITE_BIT)) {
            // We are starting a thread so that we can collect all the data
            // FIX for short length packet issue on SPI.
            readBuffer->transferCount = 0;
            if (writeBuffer)
                writeBuffer->transferCount = 0;
            threadParameter.handle = handle;
            threadParameter.readBuffer = readBuffer->buffer;
            threadParameter.length = readBuffer->length;
            threadParameter.ioTimeout = ioTimeout;
            ret = pthread_create (&readThreadID, NULL, (void *)spiCollectData, (void *)&threadParameter);
            if (ret){
                CY_DEBUG_PRINT_ERROR ("CY:Error in creating read thread ... Reading failed \n");
                pthread_mutex_unlock (&device->writeLock);
                readBuffer->transferCount = 0;
                return CY_ERROR_REQUEST_FAILED;
            }
            pthread_join (readThreadID, NULL);
            readBuffer->transferCount = threadParameter.transferCount;
            pthread_mutex_unlock (&device->writeLock);
            return threadParameter.rStatus;
        }
        writeBuffer->transferCount = 0;
        readBuffer->transferCount = 0;
        threadParameter.handle = handle;
        threadParameter.readBuffer = readBuffer->buffer;
        threadParameter.length = readBuffer->length;
        threadParameter.ioTimeout = ioTimeout;
        ret = pthread_create (&readThreadID, NULL, (void *)spiCollectData, (void *)&threadParameter);
        if (ret){
            CY_DEBUG_PRINT_ERROR ("CY:Error in creating read thread ... Reading failed \n");
            readBuffer->transferCount = 0;
            pthread_mutex_unlock (&device->writeLock);
            return CY_ERROR_REQUEST_FAILED;
        }
        rStatus = CySpiWrite (handle, writeBuffer, ioTimeout);
        if (rStatus == CY_SUCCESS) {
            pthread_join (readThreadID, NULL);
            rStatus = threadParameter.rStatus;
            readBuffer->transferCount = threadParameter.transferCount;
        }
        else {
            pthread_join (readThreadID, NULL);
            readBuffer->transferCount = threadParameter.transferCount;
        }
        pthread_mutex_unlock (&device->writeLock);
        return rStatus;
    }
    else{
        CY_DEBUG_PRINT_ERROR ("CY:Error API busy in service previous request ... Function is %s \n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    return rStatus;
}
