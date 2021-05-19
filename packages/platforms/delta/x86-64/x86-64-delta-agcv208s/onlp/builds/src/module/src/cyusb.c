/*
 * USB routines of Cypress USB Serial
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

libusb_context *glContext = NULL;
static bool glDriverInit = false;
static libusb_device **glDeviceList;
static int glNumDevices;
/*The API initializes the Libusb library
*/
pthread_mutex_t criticalSection;
CY_RETURN_STATUS CyLibraryInit ()
{
    int rStatus = LIBUSB_SUCCESS;

    if (!glContext)
        rStatus = libusb_init (&glContext);

    if (glDriverInit != true){
        if (rStatus != LIBUSB_SUCCESS){
            CY_DEBUG_PRINT_ERROR ("CY:Driver Init Failed ...\n");
            return CY_ERROR_DRIVER_INIT_FAILED;
        }
        glNumDevices = libusb_get_device_list (glContext, &glDeviceList);
        if (glNumDevices < 0){
            CY_DEBUG_PRINT_ERROR ("CY:Building device list Failed ...\n");
            glNumDevices = -1;
            return CY_ERROR_DRIVER_INIT_FAILED;
        }
        pthread_mutex_init (&criticalSection, NULL);
        glDriverInit = true;
        return CY_SUCCESS;
    }
    else{
        CY_DEBUG_PRINT_ERROR ("CY:Error ... library already initialized \n");
        return CY_ERROR_DRIVER_INIT_FAILED;
    }
}
/*
   This API needs to be called after Calling CyGetListofDevices.
 */
CY_RETURN_STATUS CyLibraryExit ()
{
    if (glDriverInit == true){
        if (glNumDevices >= 0)
            libusb_free_device_list (glDeviceList, 1);
        if (glContext) {
            libusb_exit (glContext);
            glContext = NULL;
        }
        glDriverInit = false;
        pthread_mutex_destroy (&criticalSection);
        return CY_SUCCESS;
    }
    CY_DEBUG_PRINT_ERROR ("CY:Error ... Library not initialized \n");
    return CY_ERROR_REQUEST_FAILED;
}
/*
 * This function Gets the number of all the devices currently
 * Connected to the host (It includes Cypress Device as well as
 * no Cypress Devices connected)
 */
CY_RETURN_STATUS CyGetListofDevices (
        UINT8 *numDevices
        )
{
    // Use this variable to call libusb_close and exit of the application
    if (numDevices == NULL)
        return CY_ERROR_INVALID_PARAMETER;
    if (!glDriverInit){
        CY_DEBUG_PRINT_ERROR ("CY:Error Library not initialised ...function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    pthread_mutex_lock (&criticalSection);
    libusb_free_device_list (glDeviceList, 1);
    glNumDevices = (*numDevices) = libusb_get_device_list (glContext, &glDeviceList);
    pthread_mutex_unlock (&criticalSection);
    if (glNumDevices < 0){
        CY_DEBUG_PRINT_ERROR ("CY:Building device list Failed ...function is %s\n", __func__);
        glNumDevices = -1;
        (*numDevices) = -1;
        return CY_ERROR_REQUEST_FAILED;
    }
    return CY_SUCCESS;
}
/* This function gets all the neccessary info such as VID,PID,
   String Descriptors and if is a cypress serial device you will
   get the info on class and device type
 */
CY_RETURN_STATUS CyGetDeviceInfo (
        UINT8 deviceNumber,
        CY_DEVICE_INFO *deviceInfo
        )
{
    struct libusb_device_descriptor deviceDesc;
    int rStatus;
    UINT32 numInterfaces;
    UINT8 iManufacturer, iProduct, iSerial;
    libusb_device *usbDevice;;
    struct libusb_config_descriptor *configDesc;
    libusb_device_handle *devHandle;

    // Get the list of descriptor info for the device
    if (glDriverInit == false){
        CY_DEBUG_PRINT_ERROR ("CY:Error Library not initialised ...function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    if (deviceInfo == NULL)
        return CY_ERROR_INVALID_PARAMETER;
    pthread_mutex_lock (&criticalSection);
    if (deviceNumber >= glNumDevices){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid device number... \n");
        pthread_mutex_unlock (&criticalSection);
        return CY_ERROR_INVALID_PARAMETER;
    }
    usbDevice = glDeviceList[deviceNumber];
    rStatus = libusb_get_device_descriptor (usbDevice, &deviceDesc);
    if (rStatus != LIBUSB_SUCCESS){
        CY_DEBUG_PRINT_ERROR ("CY:Error ... unable to retrieve device descriptor \n");
        pthread_mutex_unlock (&criticalSection);
        return CY_ERROR_DEVICE_INFO_FETCH_FAILED;
    }

    deviceInfo->vidPid.vid = deviceDesc.idVendor;
    deviceInfo->vidPid.pid = deviceDesc.idProduct;
    // Get the all the index of the String descriptors so that it can be used
    // to retrieve the string descriptor info.
    iManufacturer = deviceDesc.iManufacturer;
    iProduct = deviceDesc.iProduct;
    iSerial = deviceDesc.iSerialNumber;
    //Get the Device handle so that we can communicate with the device retreiving
    // descriptor info
    deviceInfo->manufacturerName[0] = '\0';
    deviceInfo->productName[0] = '\0';
    deviceInfo->serialNum[0] = '\0';
    rStatus = libusb_open (usbDevice, &devHandle);
    if (rStatus == LIBUSB_ERROR_ACCESS){
        CY_DEBUG_PRINT_ERROR ("CY:Error ...Insufficient permission... Libusb error is %d \n", rStatus);
        pthread_mutex_unlock (&criticalSection);
        return CY_ERROR_ACCESS_DENIED;
    }
    else if (rStatus != CY_SUCCESS){
        CY_DEBUG_PRINT_ERROR ("CY:Error in opening the device... Libusb error is %d \n", rStatus);
        pthread_mutex_unlock (&criticalSection);
        return CY_ERROR_DEVICE_INFO_FETCH_FAILED;
    }
    if (iManufacturer > 0){
        rStatus = libusb_get_string_descriptor_ascii (devHandle, iManufacturer, deviceInfo->manufacturerName, 256);
        if (rStatus <= LIBUSB_SUCCESS){
            CY_DEBUG_PRINT_ERROR ("CY:Error in Getting Manufacturer name Error is <%x> \n", rStatus);
        }
    }
    if (iProduct > 0){
        rStatus = libusb_get_string_descriptor_ascii (devHandle, iProduct, deviceInfo->productName, 256);
        if (rStatus <= LIBUSB_SUCCESS){
            CY_DEBUG_PRINT_ERROR ("CY:Error in Getting product name Error is <%d> \n", rStatus);
        }
    }
    if (iSerial > 0){
        rStatus = libusb_get_string_descriptor_ascii (devHandle, iSerial, deviceInfo->serialNum, 256);
        if (rStatus <= LIBUSB_SUCCESS){
            CY_DEBUG_PRINT_ERROR ("CY:Error in Getting Serial name <%d>\n", rStatus);
        }
    }
    rStatus = libusb_get_config_descriptor (usbDevice, 0, &configDesc);
    if (rStatus == LIBUSB_SUCCESS){
        UINT32 index_i = 0;
        const struct libusb_interface *interface;
        numInterfaces = configDesc->bNumInterfaces;
        deviceInfo->numInterfaces = numInterfaces;
        interface = configDesc->interface;
        while ((numInterfaces) && (index_i < CY_MAX_DEVICE_INTERFACE)){
            deviceInfo->deviceClass[index_i] = (CY_DEVICE_CLASS)interface->altsetting->bInterfaceClass;
            if (deviceInfo->deviceClass[index_i] == CY_CLASS_VENDOR){
                deviceInfo->deviceType[index_i] = (CY_DEVICE_TYPE)interface->altsetting->bInterfaceSubClass;
            }
            else
                deviceInfo->deviceType[index_i] = CY_TYPE_DISABLED;
            index_i++;
            numInterfaces--;
            interface++;
        }
        libusb_free_config_descriptor(configDesc);
    }
    else {
        CY_DEBUG_PRINT_ERROR ("CY: Error in Getting config descriptor ...Libusb error is %d \n", rStatus);
        if (devHandle)
            libusb_close (devHandle);
        pthread_mutex_unlock (&criticalSection);
        return CY_ERROR_DEVICE_INFO_FETCH_FAILED;
    }
    if (devHandle)
        libusb_close (devHandle);
    pthread_mutex_unlock (&criticalSection);
    return CY_SUCCESS;
}
/* This function gets all the neccessary info such as VID,PID,
   String Descriptors and if is a cypress serial device you will
   get the info on class and device type
 */
CY_RETURN_STATUS CyGetDeviceInfoVidPid (
        CY_VID_PID vidPid,
        UINT8 *deviceNumber,
        PCY_DEVICE_INFO deviceInfoList,
        UINT8 *deviceCount,
        UINT8 infoListLength
        )
{
    struct libusb_device_descriptor deviceDesc;
    int rStatus = CY_ERROR_DRIVER_INIT_FAILED;
    UINT32 numInterfaces, index = 0;
    int devNum;
    UINT8 iManufacturer, iProduct, iSerial;
    libusb_device *usbDevice;
    struct libusb_config_descriptor *configDesc;
    libusb_device_handle *devHandle = NULL;
    PCY_DEVICE_INFO deviceInfo;

    if (glDriverInit == false){
        CY_DEBUG_PRINT_ERROR ("CY:Error Library not initialised ...function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    if ((infoListLength) < 1){
        CY_DEBUG_PRINT_ERROR ("CY:Error invalid device info list length specified should be > 0 .. function is %s\n", __func__);
        return CY_ERROR_INVALID_PARAMETER;
    }
    if (deviceNumber == NULL || deviceInfoList == NULL || deviceCount == NULL)
        return CY_ERROR_INVALID_PARAMETER;
    // Get the list of descriptor info for the device
    (*deviceCount) = 0;
    pthread_mutex_lock (&criticalSection);
    for (devNum = 0; devNum < glNumDevices; devNum++) {
        //We are making sure that we do not overrun
        //the list.
        deviceInfo = &(deviceInfoList [index]);
        usbDevice = glDeviceList[devNum];
        rStatus = libusb_get_device_descriptor (usbDevice, &deviceDesc);
        if (rStatus != LIBUSB_SUCCESS){
            CY_DEBUG_PRINT_ERROR ("CY:Error in getting device descriptor for device-%d... Libusb Error is %d \n", devNum, rStatus);
            pthread_mutex_unlock (&criticalSection);
            return CY_ERROR_DEVICE_INFO_FETCH_FAILED;
        }
        if ((deviceDesc.idVendor != vidPid.vid) || (deviceDesc.idProduct != vidPid.pid)){
            continue;
        }
        (*deviceCount)++;
        if (index > infoListLength){
        	continue;
        }
        rStatus = libusb_open (usbDevice, &devHandle);
        if (rStatus == LIBUSB_ERROR_ACCESS){
            CY_DEBUG_PRINT_ERROR ("CY:Insufficient permission ... Libusb error is %d \n", rStatus);
            pthread_mutex_unlock (&criticalSection);
            return CY_ERROR_ACCESS_DENIED;
        }
        else if (rStatus != LIBUSB_SUCCESS){
            CY_DEBUG_PRINT_ERROR ("CY:Error in Opening the Device ...Error is %d \n", rStatus);
            pthread_mutex_unlock (&criticalSection);
            return CY_ERROR_DEVICE_INFO_FETCH_FAILED;
        }
        deviceNumber[index] = devNum;
        index++;
        deviceInfo->vidPid.vid = deviceDesc.idVendor;
        deviceInfo->vidPid.pid = deviceDesc.idProduct;
        // Get all the index of the String descriptors so that it can be used
        // to retrieve the string descriptor info.
        iManufacturer = deviceDesc.iManufacturer;
        iProduct = deviceDesc.iProduct;
        iSerial = deviceDesc.iSerialNumber;
        //Get the Device handle so that we can communicate with the device retreiving
        // descriptor info
        //Initialise manufacturer, product and serial names
        deviceInfo->manufacturerName[0] = '\0';
        deviceInfo->productName[0] = '\0';
        deviceInfo->serialNum[0] = '\0';
        if (iManufacturer > 0) {
            rStatus = libusb_get_string_descriptor_ascii (devHandle, iManufacturer, deviceInfo->manufacturerName, 256);
            if (rStatus <= LIBUSB_SUCCESS){
                CY_DEBUG_PRINT_INFO ("CY:Error in Getting Manufacturer name Error is <%d> \n",rStatus);
            }
        }
        if (iProduct > 0){
            rStatus = libusb_get_string_descriptor_ascii (devHandle, iProduct, deviceInfo->productName, 256);
            if (rStatus <= LIBUSB_SUCCESS){
                CY_DEBUG_PRINT_INFO ("CY:Error in Getting product name Error is <%d> \n", rStatus);
            }
        }
        if (iSerial > 0){
            rStatus = libusb_get_string_descriptor_ascii (devHandle, iSerial, deviceInfo->serialNum, 256);
            if (rStatus <= LIBUSB_SUCCESS){
                CY_DEBUG_PRINT_INFO ("CY:Error in Getting Serial name <%d>\n", rStatus);
            }
        }
        CY_DEBUG_PRINT_INFO ("Manufacturer name <%s> \nProduct Name <%s> \nserial number <%s> \n",
                deviceInfo->manufacturerName,deviceInfo->productName,deviceInfo->serialNum);
        rStatus = libusb_get_config_descriptor (usbDevice, 0, &configDesc);
        if (rStatus == LIBUSB_SUCCESS){
            int index_i = 0;
            const struct libusb_interface *interfaceDesc;
            numInterfaces = configDesc->bNumInterfaces;
            deviceInfo->numInterfaces = numInterfaces;
            interfaceDesc = configDesc->interface;
            while ((numInterfaces) && (index_i < CY_MAX_DEVICE_INTERFACE)){
                deviceInfo->deviceClass[index_i] = (CY_DEVICE_CLASS)interfaceDesc->altsetting->bInterfaceClass;
                if (deviceInfo->deviceClass[index_i] == CY_CLASS_VENDOR)
                    deviceInfo->deviceType[index_i] = (CY_DEVICE_TYPE)interfaceDesc->altsetting->bInterfaceSubClass;
                else
                    deviceInfo->deviceType[index_i] = CY_TYPE_DISABLED;

                index_i++;
                numInterfaces--;
                interfaceDesc++;
            }
        }
        else {
            CY_DEBUG_PRINT_ERROR ("CY: Error in Getting config descriptor ... Libusb Error is %d\n", rStatus);
            pthread_mutex_unlock (&criticalSection);
            return CY_ERROR_DEVICE_INFO_FETCH_FAILED;
        }
        libusb_free_config_descriptor (configDesc);
        libusb_close (devHandle);
    }
    if ((*deviceCount) == 0)
        rStatus = CY_ERROR_DEVICE_NOT_FOUND;
    pthread_mutex_unlock (&criticalSection);
    return rStatus;
}
/*
   This API will claim the interface in the device
   To make sure only claimed application speaks to device.
 */
CY_RETURN_STATUS CySelectInterface (
        CY_HANDLE handle,
        UINT8 interfaceNum
        )
{
    UINT32 rStatus, numEP;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;
    libusb_device *usbDev;
    struct libusb_config_descriptor *configDesc;
    const struct libusb_interface *interfaceDesc;
    const struct libusb_endpoint_descriptor *epDesc;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;

    usbDev = libusb_get_device (devHandle);
    if (usbDev == NULL){
        CY_DEBUG_PRINT_ERROR ("CY:Error Invalide handle ..function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    //Get the config descriptor and parse it to get the
    //interface and endpoint descriptor
    rStatus = libusb_get_config_descriptor (usbDev, 0, &configDesc);
    if (rStatus != LIBUSB_SUCCESS){
        CY_DEBUG_PRINT_ERROR ("CY:Error in Getting Config Desc ...function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    interfaceDesc = configDesc->interface;
    //Interface Number should be a valid one and should not exceed
    // total number of interfaces
    if (interfaceNum >= configDesc->bNumInterfaces){
        CY_DEBUG_PRINT_ERROR ("CY:Interface Number not valid... \n");
        libusb_free_config_descriptor (configDesc);
        return CY_ERROR_REQUEST_FAILED;
    }
    if (libusb_kernel_driver_active (devHandle, interfaceNum)){
        CY_DEBUG_PRINT_ERROR ("CY:Kernel driver active on the interface number %d \n", interfaceNum);;
        //User can uncomment this section if needed.
#ifdef CY_DETACH_KERNEL_DRIVER
        if (!libusb_detach_kernel_driver (devHandle, interfaceNum)){
            CY_DEBUG_PRINT_ERROR ("CY:Kernel driver detach failed %d\n", interfaceNum);
            return CY_ERROR_REQUEST_FAILED;
        }
#else
        return CY_ERROR_REQUEST_FAILED;
#endif
    }
    rStatus = libusb_claim_interface (devHandle, interfaceNum);
    if (rStatus != LIBUSB_SUCCESS){
        CY_DEBUG_PRINT_ERROR ("CY:Error in claiming interface -interface num %d... Libusb error is %d \n", interfaceNum, rStatus);
        return CY_ERROR_REQUEST_FAILED;
    }
    device->interfaceNum = interfaceNum;
    while (interfaceNum--)
        interfaceDesc++;

    epDesc = interfaceDesc->altsetting->endpoint;
    numEP = interfaceDesc->altsetting->bNumEndpoints;
    device->numEndpoints = numEP;
    // Check the total number of endpoints interface has
    // and get all the endpoint add
    CY_DEBUG_PRINT_INFO ("CY:Info The total number of endpoints are %d \n", numEP);
    while (numEP){
        if (epDesc->bmAttributes == 0x2){ //Bulk EP checking
            if (epDesc->bEndpointAddress & 0x80)
                device->inEndpoint = epDesc->bEndpointAddress;
            else
                device->outEndpoint = epDesc->bEndpointAddress;
        }
        else if (epDesc->bmAttributes == 0x3) //Interrupt EP checking (We have only one interrupt EP)
            device->interruptEndpoint = epDesc->bEndpointAddress;
        epDesc++;
        numEP--;
    }
    CY_DEBUG_PRINT_INFO ("CY:Info The Endpoints are in %d and out %d and interrup %d\n",
            device->inEndpoint, device->outEndpoint, device->interruptEndpoint);
    libusb_free_config_descriptor (configDesc);
    return CY_SUCCESS;
}
/*
 * This API selects the type of the device
 */
void CySelectDeviceType (CY_DEVICE *device, libusb_device *libUsbdev, unsigned char interfaceNum)
{
    int rStatus, numInterfaces;
    struct libusb_config_descriptor *configDesc;
    int index = 0;
    const struct libusb_interface *interfaceDesc;
    device->deviceType = CY_TYPE_DISABLED;

    rStatus = libusb_get_config_descriptor (libUsbdev, 0, &configDesc);
    if (0 == rStatus){
        interfaceDesc = configDesc->interface;
        numInterfaces = configDesc->bNumInterfaces;
        if (interfaceNum >= numInterfaces)
            return;
        while (index != interfaceNum) {
            index++;
            interfaceDesc++;
        }
        if (interfaceDesc->altsetting->bInterfaceClass == CY_CLASS_VENDOR)
            device->deviceType = (CY_DEVICE_TYPE)interfaceDesc->altsetting->bInterfaceSubClass;
        libusb_free_config_descriptor (configDesc);
    }
    CY_DEBUG_PRINT_INFO ("CY:Info The device type is %d \n", device->deviceType);
}
/*
   The Api Gets the handle for the specified device number
   (refer to usage guide and example for usage)
   and this handle should be called for further communication
   with the device
 */
CY_RETURN_STATUS CyOpen (
        unsigned char deviceNumber,
        unsigned char interfaceNum,
        CY_HANDLE *handle
        )
{
    libusb_device_handle *devHandle;
    libusb_device *dev;
    CY_DEVICE *device;
    int rStatus;

    if (glDriverInit == false){
        CY_DEBUG_PRINT_ERROR ("CY:Error Library not initialised ...function is %s\n", __func__);
        return CY_ERROR_REQUEST_FAILED;
    }
    pthread_mutex_lock (&criticalSection);
    if (glDriverInit == true){
        if (deviceNumber >= glNumDevices){
            CY_DEBUG_PRINT_ERROR ("CY:Error ... Invalid device number ... \n");
	    pthread_mutex_unlock (&criticalSection);
            return CY_ERROR_INVALID_PARAMETER;
        }
        dev = glDeviceList [deviceNumber];
        rStatus = libusb_open (dev, &devHandle);
        if (rStatus == LIBUSB_ERROR_ACCESS){
            CY_DEBUG_PRINT_ERROR ("CY:Error in opening the device ..Access denied \n");
            handle = NULL;
            pthread_mutex_unlock (&criticalSection);
            return CY_ERROR_ACCESS_DENIED;
        }
        if (rStatus != LIBUSB_SUCCESS){
            CY_DEBUG_PRINT_ERROR ("CY:Error in Opening the Device ...Error is %d \n", rStatus);
            handle = NULL;
            pthread_mutex_unlock (&criticalSection);
            return CY_ERROR_DRIVER_OPEN_FAILED;
        }
        device = (CY_DEVICE *)malloc(sizeof (CY_DEVICE));
        if (device == NULL){
            pthread_mutex_unlock (&criticalSection);
            return CY_ERROR_ALLOCATION_FAILED;
        }
        device->devHandle = devHandle;
        (*handle) = device;
        rStatus = CySelectInterface (device, interfaceNum);
        if (rStatus != CY_SUCCESS){
            libusb_close (devHandle);
            free (device);
            pthread_mutex_unlock (&criticalSection);
            return CY_ERROR_DRIVER_OPEN_FAILED;
        }
        CySelectDeviceType (device, dev, interfaceNum);
        if (device->deviceType == CY_TYPE_UART) {
            CyUartSetRts (*handle);
            CyUartSetDtr (*handle);
            if (!CyUartSetHwFlowControl (*handle, CY_UART_FLOW_CONTROL_DISABLE))
                device->uartFlowControlMode = CY_UART_FLOW_CONTROL_DISABLE;
        }
        //initialising structure elements
        device->spiThreadRunning = false;
        device->uartThreadRunning = false;
        device->spiCancelEvent = false;
        device->uartCancelEvent = false;
        device->spiTransfer = NULL;
        device->uartTransfer = NULL;
        if (pthread_mutex_init (&device->readLock, NULL)){
            CY_DEBUG_PRINT_ERROR ("CY:Error initializing the read mutex .. Function is %s \n", __func__);
            libusb_close (devHandle);
            free (device);
            pthread_mutex_unlock (&criticalSection);
            return CY_ERROR_DRIVER_OPEN_FAILED;
        }
        if (pthread_mutex_init (&device->writeLock, NULL)){
            CY_DEBUG_PRINT_ERROR ("CY:Error initializing the write mutex .. Function is %s \n", __func__);
            libusb_close (devHandle);
            free (device);
            pthread_mutex_unlock (&criticalSection);
            return CY_ERROR_DRIVER_OPEN_FAILED;
        }
        if (pthread_mutex_init (&device->notificationLock, NULL)){
            CY_DEBUG_PRINT_ERROR ("CY:Error initializing the write mutex .. Function is %s \n", __func__);
            libusb_close (devHandle);
            free (device);
            pthread_mutex_unlock (&criticalSection);
            return CY_ERROR_DRIVER_OPEN_FAILED;
        }
        pthread_mutex_unlock (&criticalSection);
        return CY_SUCCESS;
    }
    else{
        CY_DEBUG_PRINT_ERROR ("CY:Error iniitalise library by calling CyLibraryInit()....function is %s\n", __func__);
        return CY_ERROR_DRIVER_OPEN_FAILED;
    }
}
/*
   The Api Closes the handle and needs to be called only if CyGetNumDevices
   or CyOpen is called
 */
CY_RETURN_STATUS CyClose (
        CY_HANDLE handle
        )
{
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;
    if (device->deviceType == CY_TYPE_UART) {
        CyUartClearRts (handle);
        CyUartClearDtr (handle);
        CyUartSetHwFlowControl (handle, CY_UART_FLOW_CONTROL_DISABLE);
    }
    if (glDriverInit == true){
        if (device->deviceType == CY_TYPE_SPI || device->deviceType == CY_TYPE_UART){
            if (device->spiThreadRunning || device->uartThreadRunning){
            	CyAbortEventNotification(handle);
            }
        }
        if (pthread_mutex_destroy (&device->readLock)){
            CY_DEBUG_PRINT_ERROR ("CY:Error de initializing the read mutex .. Function is %s \n", __func__);
            return CY_ERROR_REQUEST_FAILED;
        }
        if (pthread_mutex_destroy (&device->writeLock)){
            CY_DEBUG_PRINT_ERROR ("CY:Error de initializing the write mutex .. Function is %s \n", __func__);
            return CY_ERROR_REQUEST_FAILED;
        }
        if (pthread_mutex_destroy (&device->notificationLock)){
            CY_DEBUG_PRINT_ERROR ("CY:Error de initializing the write mutex .. Function is %s \n", __func__);
            return CY_ERROR_REQUEST_FAILED;
        }
        libusb_close ((libusb_device_handle*)devHandle);
        free (device);
    }
    return CY_SUCCESS;
}
/*
   This Api will reset the pipe and clears the endpoint
 */
CY_RETURN_STATUS CyResetPipe (
        CY_HANDLE handle,
        UINT8 endPointAddress
        )
{
    UINT32 rStatus;
    CY_DEVICE *device;
    libusb_device_handle *devHandle;

    if (handle == NULL)
        return CY_ERROR_INVALID_HANDLE;
    device = (CY_DEVICE *)handle;
    devHandle = device->devHandle;

    rStatus = libusb_clear_halt ((libusb_device_handle *)devHandle, endPointAddress);
    if (rStatus != LIBUSB_SUCCESS){
        CY_DEBUG_PRINT_ERROR ("CY:Error in resetting the pipe ... \n");
        return CY_ERROR_REQUEST_FAILED;
    }
    return CY_SUCCESS;
}
/*
   This Api will get the library version,patch
   and build number
 */
CY_RETURN_STATUS CyGetLibraryVersion (
        CY_HANDLE handle,
        PCY_LIBRARY_VERSION version
        )
{
    version->majorVersion = CY_US_VERSION_MAJOR;
    version->minorVersion = CY_US_VERSION_MINOR;
    version->patch = CY_US_VERSION_PATCH;
    version->buildNumber = CY_US_VERSION_BUILD;
    return CY_SUCCESS;
}
