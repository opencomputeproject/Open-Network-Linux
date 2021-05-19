/*
## Cypress USB Serial Library header file (CyUSBSerial.h)
## ===========================
##
##  Copyright Cypress Semiconductor Corporation, 2012-2013,
##  All Rights Reserved
##  UNPUBLISHED, LICENSED SOFTWARE.
##
##  CONFIDENTIAL AND PROPRIETARY INFORMATION
##  WHICH IS THE PROPERTY OF CYPRESS.
##
##  Use of this file is governed
##  by the license agreement included in the file
##
##     <install>/license/license.txt
##
##  where <install> is the Cypress software
##  installation root directory path.
##
## ===========================
*/

#ifndef _INCLUDED_CYUSBBOOTLOADER_H_
#define _INCLUDED_CYUSBBOOTLOADER_H_

/*This is to export windows API*/
#ifdef CYWINEXPORT
#undef CYWINEXPORT
#endif
#ifdef WIN32

#ifdef CYUSBSERIAL_EXPORTS
    #define CYWINEXPORT  extern "C" __declspec(dllexport)
    #define WINCALLCONVEN 
    #else
        #define CYWINEXPORT  extern "C" __declspec(dllimport)
        #define WINCALLCONVEN
    #endif
#else /*Linux and MAC*/
    #define CYWINEXPORT
    #define WINCALLCONVEN
    #ifndef BOOL
        typedef bool BOOL;
    #endif
#endif



#include "CyUSBSerial.h"

/* Summary
Enumeration defining return status of  USB serial library APIs

Description
The enumeration CY_RETURN_STATUS holds the different return status of all the
APIs supported by USB Serial library.
*/
   /*API Failed because the SPI/UART status monitor thread already exists*/
/*
Summary
Structure to hold Boot mode data buffer information. 

Description
This strucuture is used by boot mode data transaction API to perform read, write 
operations.

Before using a variable of this strucutre users need to initialize various members appropriately. 
      buffer - Need to be initialized to pre-allocated memory only, and user is 
               expected to deallocate the memory in his application.
      length - Contains total length the buffer to be used while performing 
               read/write operations.
      bytesReturned -  Specifies the number of bytes actually read/written.
      address - Address from where the data need to written/read

See Also
CyReadFlash
CyProgFlash
CyVerifyFlash
CyReadMemory
CyWriteMemory

CY_BOOTLOADER_VERSION
*/
typedef struct _CY_BOOTLD_BUFFER {

    UCHAR *buffer;                 /*Address of the buffer*/
    UINT32 length;                 /*total length to be read/written */
    UINT32 *bytesReturned;         /*Total length of bytes that was written/read*/
    UINT32 address;                /*Address from where data needs to be written/read in device*/

} CY_BOOTLD_BUFFER,*PCY_BOOTLD_BUFFER;

/*
Summary
This structure is used to hold Boot loader version. 

Description
This data type holds the information of version of the bootloader in device. It has major version, 
minor version and patch number.

See Also
CY_BOOTLD_BUFFER
*/
typedef struct _CY_BOOTLOADER_VERSION {

    UCHAR majorVersion;          /*Major version of BootLoader*/
    UCHAR minorVersion;          /*Minor version of the BootLoader*/
    UINT16 patchNumber;          /*Patch Number of the BootLoader*/  

} CY_BOOTLOADER_VERSION;


/*************************************************************************************/
/****************************BOOTLOADER API'S*****************************************/
/*************************************************************************************/
    
/*@@BOOTLoader API

These APIs provide an interface for configuring the device when it is in BOOT mode.

The API's under this group gives user to option to configure the device when the device is in BOOT 
mode. The APIs include support for device configuration, SCB level configuration,  USB interface 
configuration, checksum, firmware download.

*/


/*
Summary
This API retrieves the BootLoader version.

Description
This API gets the bootloader version of the USB Serial device.


Return Value
CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.

See Also
CyGetSiliconID 

CY_BOOTLOADER_VERSION
*/
CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyGetBootLoaderVersion (
    CY_HANDLE handle,                                   /*Valid device handle*/
    CY_BOOTLOADER_VERSION *bootLoaderVersion          /*Boot Loader version.*/
    );
/*
Summary
This API retrieves the silicon ID.

Description
This API gets the silicon ID of the USB Serial device into the argument siliconID.

Return Value
CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.

See Also
CyGetBootLoaderVersion

*/
CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyGetSiliconID (
    CY_HANDLE handle,                                /*Valid device handle*/
    UINT32 *siliconID                                /*Boot Loader version.*/
    );

/*
Summary
This API can be used to change the device operational mode from device firmware to bootloader.

Description
This API changes the device operational mode from device firmware to bootloader or 
Manufacturing mode. 

Call the API GetSignature to identify the current operational mode. This API should be called only 
when the device is in firmware mode.

Return Value
CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.

See Also
CyGetSignature

*/
CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyJumpToMfgMode (
    CY_HANDLE handle);

/*
Summary
This API can be used to read device configuration

Description
This API reads the device configuration from the device configuration table. It fills the device 
configuration as a series of bytes in the argument deviceConfig

Return Value
CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.

See Also
CyWriteDeviceConfig
*/

CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyReadDeviceConfig (
    CY_HANDLE handle,                                 /* Valid device handle*/
    UCHAR *deviceConfig                               /* Device configuration value.*/
    );

/*
Summary
This API can be used to update the device configuration table.

Description;
This API updates the device configuration in the configuration table of device. The device 
configuration must be supplied as an array of bytes.

Return Value
CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.

See Also
CyReadDeviceConfig

*/

CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyWriteDeviceConfig (
    CY_HANDLE handle,                                 /*Valid Device handle*/
    UCHAR *deviceConfig                               /*Device configuration value */
    );

/*
Summary
This API can be used to read the content of flash from specified address.

Description
The readBuffer structure must be filled and with address to be read from, appropriate buffer 
and number of bytes to be read.

The actual bytes read will be available in bytesReturned member of CY_BOOTLD_BUFFER. 

Return Value
CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.

See Also
CY_BOOTLD_BUFFER
CyProgFlash
CyVerifyFlash
*/

CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyReadFlash (
    CY_HANDLE handle,                              /*Valid device handle*/
    CY_BOOTLD_BUFFER *readBuffer,                  /*Buffer pointer containing buffer address, length and address of flash*/
    UINT32 timeout                                 /*API timeout value*/
    );

/*
Summary
This API can be used to program the flash at specified address.

Description
The writeBuffer structure must be filled and with address to be written to, appropriate buffer location
and number of bytes to be written.

The actual bytes written will be available in bytesReturned member of CY_BOOTLD_BUFFER. 

Return Value
CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.

See Also
CY_BOOTLD_BUFFER
CyReadFlash
CyVerifyFlash
*/

CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyProgFlash (
    CY_HANDLE handle,                           /*Valid device handle*/
    CY_BOOTLD_BUFFER *writeBuffer,              /*Buffer pointer containing the address of buffer pointer, length and address of flash*/
    UINT32 timeout                              /*API timeout value*/
    );
/*
Summary
This API can be used to read the memory content of SRAM from specified address.

Description
This API reads the content of flash in USB Serial device. The argument readBuffer need to be 
initialized with address, number of bytes to be read and buffer location before invoking 
this API.

Return Value
CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.

See Also
CY_BOOTLD_BUFFER
CyWriteMemory

*/

CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyReadMemory (
    CY_HANDLE handle,                           /*Valid handle to communicate with device*/    
    CY_BOOTLD_BUFFER *readBuffer,               /*Bootloader read buffer details*/
    UINT32 timeout                              /*API timeout value*/
    );

/*
Summary
This API can be used to write content to SRAM at specified address.

Description
This API writes the buffer content to SRAM. The argument writeBuffer need to be initialized with
target address, number of bytes to be written and buffer location before invoking this API.

Return Value
CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.

See Also
CY_BOOTLD_BUFFER
CyReadMemory
*/

CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyWriteMemory (
    CY_HANDLE handle,                           /*Valid handle to communicate with device*/
    CY_BOOTLD_BUFFER *writeBuffer,              /*Bootloader write buffer details*/
    UINT32 timeout                              /*API timeout value*/
    );

/*
Summary
This API can be used calculate the checksum of the firmware loaded and compares it with the checksum in
device configuration table.

Return Value
CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.

See Also
CyUpdateChecksum 
*/

CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN  CyValidateChecksum (
    CY_HANDLE handle                             /*Valid handle to communicate with device*/          
    );
/*
Summary
This API can be used to read boot configuration.

Description
This API reads the boot configuration from the boot configuration table of device. The bootConfig 
need to be parsed to obtain actual configuration values. 

Return Value
CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.

See Also
CyWriteBootConfig

*/
CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyReadBootConfig (
    CY_HANDLE handle,                      /*Valid handle to communicate with device*/              
    UCHAR *bootConfig                      /*Current Boot configuration value read back*/ 
    );
/*
Summary
This API updates the device boot configuration table.

Description;
This API updates the boot configuration in the boot configuration table of device.
The bootConfig is pointer to an array of bytes contain the configuration.

Return Value
CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.

See Also
CyReadBootConfig
*/

CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyWriteBootConfig (
    CY_HANDLE handle,                           /*Valid handle to communicate with device*/ 
    UCHAR *bootConfig                           /*Boot configuration value to be updated*/ 
    );
/*
Summary
This API can be used to download firmware on to USB Serial device.

Description;
This API downloads the firmware specified in filePath on to flash of the USB Serial device.

Return Value
CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.

See Also
CyReadBootConfig
CyWriteBootConfig
CyReadFlash
CyProgFlash
CyReadMemory
CyWriteMemory
*/
CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyDownloadFirmware (
    CY_HANDLE handle,                          /*Valid handle to communicate with device*/ 
    CHAR *filePath                             /*Path of Firmware file*/
    );

/*
Summary
This API can be used enable flash configuration on USB Serial device.

Description;
This API configures the the firmware and allows user to enable/diable flash changes.

Return Value
CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.

See Also
CyReadBootConfig
CyWriteBootConfig
CyReadFlash
CyProgFlash
CyReadMemory
CyWriteMemory
*/
CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyFlashConfigEnable (
    CY_HANDLE handle,                          /*Valid handle to communicate with device*/ 
    BOOL enable                                /*Set to TRUE to enable flash configuration
                                                        FALSE to disable flash configuration */
    );

/*
Summary
This API can be used to obtain the Silicon Serial No.

Description;
This API can be used to obtain the Silicon Serial No.

Return Value
CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.

See Also
CyReadBootConfig
CyWriteBootConfig
CyReadFlash
CyProgFlash
CyReadMemory
CyWriteMemory
*/
CYWINEXPORT CY_RETURN_STATUS CyGetSiliconSerialID (
    CY_HANDLE handle,                             /*Valid device handle*/
    UCHAR buffer[8]                               /*Buffer to contain 8 bytes of data.*/
    );

#endif /* _INCLUDED_CYUSBBOOTLOADER_H_ */
