/*
 * Cypress USB Serial Library header file (CyUSBSerial.h)
 * Copyright (C) 2012-2013  Cypress Semiconductor
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

/** \file CyUSBSerial.h
 *  \brief Cypress USB Serial Library header file
 */

#ifndef _INCLUDED_CYUSBSERIAL_H_
#define _INCLUDED_CYUSBSERIAL_H_

#ifdef __cplusplus
    #define CppCALLCONVEN extern "C"
#else
    #define CppCALLCONVEN
#endif

/* This is to export Windows API */
#ifdef WIN32
    #ifdef CYUSBSERIAL_EXPORTS
        #define CYWINEXPORT  CppCALLCONVEN __declspec(dllexport)
        #define WINCALLCONVEN
        #define LINUXCALLCONVEN
    #else
        #define CYWINEXPORT CppCALLCONVEN __declspec(dllimport)
        #define WINCALLCONVEN
        #define LINUXCALLCONVEN
    #endif
#else /* Linux and MAC */
    #define CYWINEXPORT CppCALLCONVEN
    #define WINCALLCONVEN
    #define LINUXCALLCONVEN
    #ifndef BOOL
        typedef bool BOOL;
    #endif
#endif
/*************************************************************************************/
/*******************************Constants*********************************************/
/*************************************************************************************/

/*@@Constants
This section contains details of the all the constants
that are part of Cypress USB Serial driver library.
*/
#define CY_STRING_DESCRIPTOR_SIZE 256                   /**< String descriptor size */
#define CY_MAX_DEVICE_INTERFACE 5                       /**< Maximum number of interfaces */

/** This section contains USB Serial library version information. */

/** Major version number for library. */
#define CY_US_VERSION_MAJOR             (1)

/** Minor version number for library. */
#define CY_US_VERSION_MINOR             (0)

/** Patch version number for library. */
#define CY_US_VERSION_PATCH             (0)

/** Version number for the device. */
#define CY_US_VERSION                   ((CY_US_VERSION_MAJOR)       | \
                                         (CY_US_VERSION_MINOR << 8)  | \
                                         (CY_US_VERSION_PATCH << 16))
/** Library build number. */
#define CY_US_VERSION_BUILD             (0)


/*************************************************************************************/
/****************************Data Type Definitions************************************/
/*************************************************************************************/

/*@@Data Types
This section defines the data types that are used by
Cypress USB Serial driver library.
*/
#ifndef UINT32
    typedef unsigned int UINT32;
#endif
#ifndef UINT8
    typedef unsigned char UINT8;
#endif
#ifndef UINT16
    typedef unsigned short UINT16;
#endif
#ifndef CHAR
    typedef char CHAR;
#endif
#ifndef UCHAR
    typedef unsigned char UCHAR;
#endif

/**
 *  \brief CyUSB Device Handle.
 *
 *  The handle is used by application to communicate with USB serial device.
 *  The handle is obtained by calling CyOpen.
 *
 *  \sa CyOpen
 */
typedef void* CY_HANDLE;

/**
 *  \brief Function pointer for getting async error/success notification on UART/SPI
 *
 *  This function pointer that will be passed to CySetEventNotification and get
 *  a callback with a 2 byte value bit map that reports error/events triggered during UART/SPI transaction.
 *  The bit map is defined in CY_CALLBACK_EVENTS.
 *
 *  \sa CY_CALLBACK_EVENTS
 */
typedef void (*CY_EVENT_NOTIFICATION_CB_FN)(UINT16 eventsNotified);

/**
 *  \brief This structure is used to hold VID and PID of USB device
 *
 *  This Strucuture holds the VID and PID of a USB device.
 *
 *  \sa CY_DEVICE_INFO
 *  \sa CyGetDeviceInfoVidPid
 */
typedef struct _CY_VID_PID {

    UINT16 vid;         /**< Holds the VID of the device */
    UINT16 pid;         /**< Holds the PID of the device */

} CY_VID_PID, *PCY_VID_PID;

/**
 *  \brief This structure is used to hold version information of the library.
 *
 *  This structure can be used to retrive the version information of the library.
 *
 *  \sa CyGetLibraryVersion
 */
typedef struct _CY_LIBRARY_VERSION {

    UINT8 majorVersion;     /**< The major version of the library */
    UINT8 minorVersion;     /**< The minor version of the library */
    UINT16 patch;           /**< The patch number of the library */
    UINT8 buildNumber;      /**< The build number of the library */

} CY_LIBRARY_VERSION, *PCY_LIBRARY_VERSION;

/**
 *  \brief This structure is used to hold firmware version of the USB Serial device.
 *
 *  This structure holds the version information of the USB serial device.
 *  It has major version, minor version, patch number and build number.
 *
 *  \sa CyGetFirmwareVersion
 */
typedef struct _CY_FIRMWARE_VERSION {

    UINT8 majorVersion;                 /**< Major version of the Firmware */
    UINT8 minorVersion;                 /**< Minor version of the Firmware */
    UINT16 patchNumber;                 /**< Patch Number of the Firmware */
    UINT32 buildNumber;                 /**< Build Number of the Firmware */

} CY_FIRMWARE_VERSION, *PCY_FIRMWARE_VERSION;

/**
 *  \brief Enumeration defining list of USB device classes supported by USB Serial device.
 *
 *  This is the list of USB device classes supported by USB Serial device.
 *
 *  \sa CY_DEVICE_INFO
 *  \sa CyGetDeviceInfo
 *  \sa CyGetDeviceInfoVidPid
 */
typedef enum _CY_DEVICE_CLASS{

    CY_CLASS_DISABLED = 0,              /**< None or the interface is disabled */
    CY_CLASS_CDC = 0x02,                /**< CDC ACM class */
    CY_CLASS_PHDC = 0x0F,               /**< PHDC class */
    CY_CLASS_VENDOR = 0xFF              /**< VENDOR specific class */

} CY_DEVICE_CLASS;

/**
 *  \brief Enumeration defining list of device types supported by USB Serial device in each interface.
 *
 *  This is the list of device types supported by USB Serial device when the interface type is
 *  configured as CY_CLASS_VENDOR. The interface type can be queried from the device by using CyGetDeviceInfo
 *  and CyGetDeviceInfoVidPid APIs.
 *
 *  The member of CY_DEVICE_INFO structure contains the interface type.
 *
 *  \sa CY_DEVICE_INFO
 *  \sa CyGetDeviceInfo
 *  \sa CyGetDeviceInfoVidPid
 */
typedef enum _CY_DEVICE_TYPE {

    CY_TYPE_DISABLED = 0,               /**< Invalid device type or interface is not CY_CLASS_VENDOR */
    CY_TYPE_UART,                       /**< Interface of device is of type UART */
    CY_TYPE_SPI,                        /**< Interface of device is of type SPI */
    CY_TYPE_I2C,                        /**< Interface of device is of type I2C */
    CY_TYPE_JTAG,                       /**< Interface of device is of type JTAG */
    CY_TYPE_MFG                         /**< Interface of device is in Manufacturing mode */

} CY_DEVICE_TYPE;

/**
 *  \brief This enumeration type defines the available device serial blocks.
 *
 *  USB Serial device has up to two configurable serial blocks. UART, SPI, I2C or JTAG functionality can be
 *  configured and used in these serial block. Windows driver binds to a serial block rather than the entire device.
 *  So, it is essential to find out which serial block to which current communications are directed. These enumeration
 *  structure provides the possible SERIAL BLOCK Options.
 *
 *  This enumration data type is a member of CY_DEVICE_INFO structure.
 *
 *  This data type information doesn't apply for non-windows operating system.
 *
 *  \sa CY_DEVICE_INFO
 *  \sa CyGetDeviceInfo
 *  \sa CyGetDeviceInfoVidPid
 */

typedef enum _CY_DEVICE_SERIAL_BLOCK
{
    SerialBlock_SCB0 = 0,               /**< Serial Block Number 0 */
    SerialBlock_SCB1,                   /**< Serial Block Number 1 */
    SerialBlock_MFG                     /**< Serial Block Manufacturing Interface. */

} CY_DEVICE_SERIAL_BLOCK;

/**
 *  \brief Structure to hold information of the device connected to host.
 *
 *  The structure holds the information about device currently connected to host. The information
 *  can be obtained by using CyGetDeviceInfo and CyGetDeviceInfoVidPid APIs.
 *
 *  The information includes VID, PID, number of interfaces, string descriptors, device type
 *  and device class supported by each interface. Device type is valid only if the interface is CY_CLASS_VENDOR.
 *
 *  \sa CY_VID_PID
 *  \sa CY_DEVICE_CLASS
 *  \sa CY_DEVICE_TYPE
 *  \sa CyGetDeviceInfo
 *  \sa CyGetDeviceInfoVidPid
 */
typedef struct _CY_DEVICE_INFO {

    CY_VID_PID vidPid;                                      /**< VID and PID */
    UCHAR numInterfaces;                                    /**< Number of interfaces supported */
    UCHAR manufacturerName [CY_STRING_DESCRIPTOR_SIZE];     /**< Manufacturer name */
    UCHAR productName [CY_STRING_DESCRIPTOR_SIZE];          /**< Product name */
    UCHAR serialNum [CY_STRING_DESCRIPTOR_SIZE];            /**< Serial number */
    UCHAR deviceFriendlyName [CY_STRING_DESCRIPTOR_SIZE];   /**< Device friendly name : Windows only */
    CY_DEVICE_TYPE deviceType [CY_MAX_DEVICE_INTERFACE];    /**< Type of the device each interface has(Valid only
                                                                 for USB Serial Device) and interface in vendor class */
    CY_DEVICE_CLASS deviceClass [CY_MAX_DEVICE_INTERFACE];  /**< Interface class of each interface */

#ifdef WIN32
    CY_DEVICE_SERIAL_BLOCK  deviceBlock; /**< On Windows, each USB Serial device interface is associated with a
                                          *   separate driver instance. This variable represents the present driver
                                          *   interface instance that is associated with a serial block.
                                          */
#endif

} CY_DEVICE_INFO,*PCY_DEVICE_INFO;

/**
 *  \brief This structure is used to hold data buffer information.
 *
 *  This strucuture is used by all the data transaction APIs in the library to perform read, write
 *  operations.
 *  Before using a variable of this strucutre users need to initialize various members appropriately.
 *
 *  \sa CyUartRead
 *  \sa CyUartWrite
 *  \sa CyI2cRead
 *  \sa CyI2cWrite
 *  \sa CySpiReadWrite
 *  \sa CyJtagWrite
 *  \sa CyJtagRead
 */
typedef struct _CY_DATA_BUFFER {

    UCHAR *buffer;                      /**< Pointer to the buffer from where the data is read/written */
    UINT32 length;                      /**< Length of the buffer */
    UINT32 transferCount;               /**< Number of bytes actually read/written */

} CY_DATA_BUFFER,*PCY_DATA_BUFFER;

/**
 *  \brief Enumeration defining return status of  USB serial library APIs
 *
 *  The enumeration CY_RETURN_STATUS holds the different return status of all the
 *  APIs supported by USB Serial library.
 */
typedef enum _CY_RETURN_STATUS{

    CY_SUCCESS = 0,                         /**< API returned successfully without any errors. */
    CY_ERROR_ACCESS_DENIED,                 /**< Access of the API is denied for the application */
    CY_ERROR_DRIVER_INIT_FAILED,            /**< Driver initialisation failed */
    CY_ERROR_DEVICE_INFO_FETCH_FAILED,      /**< Device information fetch failed */
    CY_ERROR_DRIVER_OPEN_FAILED,            /**< Failed to open a device in the library */
    CY_ERROR_INVALID_PARAMETER,             /**< One or more parameters sent to the API was invalid */
    CY_ERROR_REQUEST_FAILED,                /**< Request sent to USB Serial device failed */
    CY_ERROR_DOWNLOAD_FAILED,               /**< Firmware download to the device failed */
    CY_ERROR_FIRMWARE_INVALID_SIGNATURE,    /**< Invalid Firmware signature in firmware file */
    CY_ERROR_INVALID_FIRMWARE,              /**< Invalid firmware */
    CY_ERROR_DEVICE_NOT_FOUND,              /**< Device disconnected */
    CY_ERROR_IO_TIMEOUT,                    /**< Timed out while processing a user request */
    CY_ERROR_PIPE_HALTED,                   /**< Pipe halted while trying to transfer data */
    CY_ERROR_BUFFER_OVERFLOW,               /**< OverFlow of buffer while trying to read/write data */
    CY_ERROR_INVALID_HANDLE,                /**< Device handle is invalid */
    CY_ERROR_ALLOCATION_FAILED,             /**< Error in Allocation of the resource inside the library */
    CY_ERROR_I2C_DEVICE_BUSY,               /**< I2C device busy */
    CY_ERROR_I2C_NAK_ERROR,                 /**< I2C device NAK */
    CY_ERROR_I2C_ARBITRATION_ERROR,         /**< I2C bus arbitration error */
    CY_ERROR_I2C_BUS_ERROR,                 /**< I2C bus error */
    CY_ERROR_I2C_BUS_BUSY,                  /**< I2C bus is busy */
    CY_ERROR_I2C_STOP_BIT_SET,              /**< I2C master has sent a stop bit during a transaction */
    CY_ERROR_STATUS_MONITOR_EXIST           /**< API Failed because the SPI/UART status monitor thread already exists */
} CY_RETURN_STATUS;

/**
 *  \brief This structure is used to store configuration of I2C module.
 *
 *  The structure contains parameters that are used in configuring I2C module of
 *  Cypress USB Serial device. CyGetI2cConfig and CySetI2cConfig APIs can be used to
 *  retrieve and configure I2C module respectively.
 *
 *  \sa CyGetI2cConfig
 *  \sa CySetI2cConfig
 */
typedef struct _CY_I2C_CONFIG{

    UINT32 frequency;               /**< I2C clock frequency 1KHz to 400KHz */
    UINT8 slaveAddress;             /**< Slave address of the I2C module, when it is configured as slave */
    BOOL isMaster;                  /**< true- Master , false- slave */
    BOOL isClockStretch;            /**< true- Stretch clock in case of no data availability
                                         (Valid only for slave mode)
                                         false- Do not Stretch clock */
} CY_I2C_CONFIG,*PCY_I2C_CONFIG;

/**
 *  \brief This structure is used to configure each I2C data transaction.
 *
 *  This structure defines parameters that are used for configuring
 *  I2C module during each data transaction. Which includes setting slave address
 *  (when device is in I2C slave mode), stopbit (to enable or disable) and
 *  Nak bit (to enable or disable).
 *
 *  \sa CyI2cWrite
 *  \sa CyI2cRead
 */
typedef struct _CY_I2C_DATA_CONFIG
{
    UCHAR slaveAddress;     /**< Slave address the master will communicate with */
    BOOL isStopBit;         /**< Set when stop bit is used */
    BOOL isNakBit;          /**< Set when I2C master wants to NAK the slave after read
                                 Applicable only when doing I2C read */
} CY_I2C_DATA_CONFIG, *PCY_I2C_DATA_CONFIG;

/**
 *  \brief Enumeration defining SPI protocol types supported by USB Serial SPI module.
 *
 *  These are the different protocols supported by USB-Serial SPI module.
 *
 *  \sa CY_SPI_CONFIG
 *  \sa CyGetSpiConfig
 *  \sa CySetSpiConfig
 */
typedef enum _CY_SPI_PROTOCOL {

    CY_SPI_MOTOROLA = 0,  /**< In master mode, when not transmitting data (SELECT is inactive), SCLK is stable at CPOL.
                           *   In slave mode, when not selected, SCLK is ignored; i.e. it can be either stable or clocking.
                           *   In master mode, when there is no data to transmit (TX FIFO is empty), SELECT is inactive.
                           */
    CY_SPI_TI,            /**< In master mode, when not transmitting data, SCLK is stable at '0'.
                           *   In slave mode, when not selected, SCLK is ignored - i.e. it can be either stable or clocking.
                           *   In master mode, when there is no data to transmit (TX FIFO is empty), SELECT is inactive -
                           *   i.e. no pulse is generated.
                           *   *** It supports only mode 1 whose polarity values are
                           *   CPOL = 0
                           *   CPHA = 1
                           */
    CY_SPI_NS             /**< In master mode, when not transmitting data, SCLK is stable at '0'. In slave mode,
                           *   when not selected, SCLK is ignored; i.e. it can be either stable or clocking.
                           *   In master mode, when there is no data to transmit (TX FIFO is empty), SELECT is inactive.
                           *   *** It supports only mode 0 whose polarity values are
                           *   CPOL = 0
                           *   CPHA = 0
                           */
} CY_SPI_PROTOCOL;

/**
 *  \brief This structure is used to configure the SPI module of USB Serial device.
 *
 *  This structure defines configuration parameters that are used for configuring the SPI module .
 *
 *  \sa CY_SPI_PROTOCOL
 *  \sa CY_SPI_DATA_TRANSFER_MODE
 *  \sa CyGetSpiConfig
 *  \sa CySetSpiConfig
 */
typedef struct _CY_SPI_CONFIG
{

    UINT32 frequency;                               /**< SPI clock frequency.
                                                     *   ** IMPORTANT: The frequency range supported by SPI module is
                                                     *   1000(1KHz) to 3000000(3MHz)
                                                     */

    UCHAR dataWidth;                                /**< Data width in bits. The valid values are from 4 to 16. */

    CY_SPI_PROTOCOL protocol ;                      /**< SPI Protocols to be used as defined in CY_SPI_PROTOCOL */

    BOOL isMsbFirst;                                /**< false -> least significant bit is sent out first
                                                         true -> most significant bit is sent out first */

    BOOL isMaster;                                  /**< false --> Slave mode selected:
                                                         true --> Master mode selected*/

    BOOL isContinuousMode;                          /**< true - Slave select line is not asserted i.e
                                                     *   de-asserted for every word.
                                                     *   false- Slave select line is always asserted
                                                     */

    BOOL isSelectPrecede;                           /**< Valid only in TI mode.
                                                     *   true - The start pulse precedes the first data
                                                     *   false - The start pulse is in sync with first data.
                                                     */

    BOOL isCpha;                                    /**< false - Clock phase is 0; true - Clock phase is 1. */

    BOOL isCpol;                                    /**< false - Clock polarity is 0;true - Clock polarity is 1. */

}CY_SPI_CONFIG,*PCY_SPI_CONFIG;

/**
 *  \brief Enumeration defines UART baud rates supported by UART module of USB Serial device.
 *
 *  The enumeration lists the various baud rates supported by the UART when it is in UART
 *  vendor mode.
 *
 *  \sa CY_UART_CONFIG
 *  \sa CySetUartConfig
 *  \sa CyGetUartConfig
 */
typedef enum _CY_UART_BAUD_RATE
{
    CY_UART_BAUD_300 = 300,          /**< Baud rate of 300. */
    CY_UART_BAUD_600 = 600,          /**< Baud rate of 600. */
    CY_UART_BAUD_1200 = 1200,        /**< Baud rate of 1200. */
    CY_UART_BAUD_2400 = 2400,        /**< Baud rate of 2400. */
    CY_UART_BAUD_4800 = 4800,        /**< Baud rate of 4800. */
    CY_UART_BAUD_9600 = 9600,        /**< Baud rate of 9600. */
    CY_UART_BAUD_14400 = 14400,      /**< Baud rate of 14400. */
    CY_UART_BAUD_19200 = 19200,      /**< Baud rate of 19200. */
    CY_UART_BAUD_38400 = 38400,      /**< Baud rate of 38400. */
    CY_UART_BAUD_56000 = 56000,      /**< Baud rate of 56000. */
    CY_UART_BAUD_57600 = 57600,      /**< Baud rate of 57600. */
    CY_UART_BAUD_115200 = 115200,    /**< Baud rate of 115200. */
    CY_UART_BAUD_230400 = 230400,    /**< Baud rate of 230400. */
    CY_UART_BAUD_460800 = 460800,    /**< Baud rate of 460800. */
    CY_UART_BAUD_921600 = 921600,    /**< Baud rate of 921600. */
    CY_UART_BAUD_1000000 = 1000000,  /**< Baud rate of 1000000. */
    CY_UART_BAUD_3000000 = 3000000,  /**< Baud rate of 3000000. */

}CY_UART_BAUD_RATE;

/**
 *  \brief Enumeration defines the different parity modes supported by UART module of USB Serial device.
 *
 *  This enumeration defines the different parity modes of USB Serial UART module.
 *  It supports odd, even, mark and space parity modes.
 *
 *  \sa CY_UART_CONFIG
 *  \sa CySetUartConfig
 *  \sa CyGetUartConfig
 */
typedef enum _CY_UART_PARITY_MODE {

    CY_DATA_PARITY_DISABLE = 0,         /**< Data parity disabled */
    CY_DATA_PARITY_ODD,                 /**< Odd Parity */
    CY_DATA_PARITY_EVEN,                /**< Even Parity */
    CY_DATA_PARITY_MARK,                /**< Mark parity */
    CY_DATA_PARITY_SPACE                /**< Space parity */

} CY_UART_PARITY_MODE;

/**
 *  \brief Enumeration defines the different stop bit values supported by UART module of USB Serial device.
 *
 *  \sa CY_UART_CONFIG
 *  \sa CySetUartConfig
 *  \sa CyGetUartConfig
 */
typedef enum _CY_UART_STOP_BIT {

    CY_UART_ONE_STOP_BIT = 1,       /**< One stop bit */
    CY_UART_TWO_STOP_BIT            /**< Two stop bits */

} CY_UART_STOP_BIT;

/**
 *  \brief Enumeration defines flow control modes supported by UART module of USB Serial device.
 *
 *  The list provides the various flow control modes supported by USB Serial device.
 *
 *  \sa CyUartSetHwFlowControl
 *  \sa CyUartGetHwFlowControl
 */
typedef enum _CY_FLOW_CONTROL_MODES {

    CY_UART_FLOW_CONTROL_DISABLE = 0,       /**< Disable Flow control */
    CY_UART_FLOW_CONTROL_DSR,               /**< Enable DSR mode of flow control */
    CY_UART_FLOW_CONTROL_RTS_CTS,           /**< Enable RTS CTS mode of flow control */
    CY_UART_FLOW_CONTROL_ALL                /**< Enable RTS CTS and DSR flow control */

} CY_FLOW_CONTROL_MODES;

/**
 *  \brief Structure holds configuration of UART module of USB Serial device.
 *
 *  This structure defines parameters used for configuring the UART module.
 *  CySetUartConfig and CyGetUartConfig APIs are used to configure and retrieve
 *  the UART configuration information.
 *
 *  \sa CySetUartConfig
 *  \sa CyGetUartConfig
 */
typedef struct _CY_UART_CONFIG {

    CY_UART_BAUD_RATE baudRate;             /**< Baud rate as defined in CY_UART_BAUD_RATE */
    UINT8 dataWidth;                        /**< Data width: valid values 7 or 8 */
    CY_UART_STOP_BIT stopBits;              /**< Number of stop bits to be used 1 or 2 */
    CY_UART_PARITY_MODE parityMode;         /**< UART parity mode as defined in CY_UART_PARITY_MODE */
    BOOL isDropOnRxErrors;                  /**< Whether to ignore framing as well as parity errors and receive data */

} CY_UART_CONFIG,*PCY_UART_CONFIG;

/**
 *  \brief Enumeration defining UART/SPI transfer error or status bit maps.
 *
 *  Enumeration lists the bit maps that are used to report error or status during
 *  UART/SPI transfer.
 *
 *  \sa CySetEventNotification
 */
typedef enum _CY_CALLBACK_EVENTS {

    CY_UART_CTS_BIT = 0x01,                         /**< CTS pin notification bit */
    CY_UART_DSR_BIT = 0x02,                         /**< State of transmission carrier. This signal
                                                         corresponds to V.24 signal 106 and RS-232 signal DSR. */
    CY_UART_BREAK_BIT = 0x04,                       /**< State of break detection mechanism of the device */
    CY_UART_RING_SIGNAL_BIT  = 0x08,                /**< State of ring signal detection of the device */
    CY_UART_FRAME_ERROR_BIT = 0x10,                 /**< A framing error has occurred */
    CY_UART_PARITY_ERROR_BIT = 0x20,                /**< A parity error has occured */
    CY_UART_DATA_OVERRUN_BIT = 0x40,                /**< Received data has been discarded due to overrun in
                                                     *   the device
                                                     */
    CY_UART_DCD_BIT = 0x100,                        /**< State of receiver carrier detection mechanism of
                                                     *   device. This signal corresponds to V.24 signal 109
                                                     *   and RS-232 signal DCD
                                                     */
    CY_SPI_TX_UNDERFLOW_BIT = 0x200,                /**< Notification sent when SPI fifo is empty */
    CY_SPI_BUS_ERROR_BIT  = 0x400,                  /**< Spi bus error has been detected */
    CY_ERROR_EVENT_FAILED_BIT = 0x800               /**< Event thread failed */

} CY_CALLBACK_EVENTS;

/*************************************************************************************/
/*********************USB Initialization APIs************************************/
/*************************************************************************************/

/*@@USB Initialization API
This section has all the APIs that handle device initialization and
fetching information about the device connected.
*/

/**
 *  \brief This API is used to initialize the library.
 *
 *  The API is used to initialize the underlying libusb library and
 *  is expected to be called when the application is being started.
 *
 *  Note: The API is used only in Linux and Mac OS.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_DRIVER_INIT_FAILED on failure
 *    (Failure could be because of not calling CyLibraryExit previously)
 *
 *  \sa CyOpen
 *  \sa CyLibraryExit
 */
CYWINEXPORT CY_RETURN_STATUS LINUXCALLCONVEN CyLibraryInit ();

/**
 *  \brief This API is used to free the library.
 *
 *  The API is used to free the library and should be called
 *  when exiting the application.
 *
 *  Note: This API is used only in Linux and Mac library.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_REQUEST_FAILED on failure
 *
 *  \sa CyOpen
 *  \sa CyClose
 *  \sa CyLibraryInit
 */
CYWINEXPORT CY_RETURN_STATUS LINUXCALLCONVEN CyLibraryExit ();

/**
 *  \brief This API retrieves number of USB devices connected to the host.
 *
 *  This API retrieves the number of devices connected to the host.
 *  In Windows family of operating systems the API retrieves only the number of devices that are attached
 *  to CyUSB3.SYS driver. For other operating systems, it retrieves the total number of USB devices present
 *  on the bus. It includes both USB Serial device as well as other devices.
 *
 *  Note: In case of Linux and Mac apart from providing number of devices connected, it builds the
 *  device list which is used for opening the device and obtaining device handle. Thus the API should be
 *  called during device discovery in the application.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_DEVICE_NOT_FOUND if there are no devices attached.
 *  \returns CY_ERROR_REQUEST_FAILED if library was not initialized.
 *
 *  \sa CyGetDeviceInfo
 *  \sa CyGetDeviceInfoVidPid
 *  \sa CyOpen
 *  \sa CyClose
 */
CYWINEXPORT CY_RETURN_STATUS CyGetListofDevices (
    UINT8* numDevices                            /**< Number of Devices connected */
    );

/**
 *  \brief This API retrieves the device information of a USB device.
 *
 *  This API retrieves information about a device connected to host. In order to
 *  get the device information on particular device user needs to provide the device number.
 *  To identify the device of interest, the application needs to loop through all devices connected
 *  and obtain the information.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_REQUEST_FAILED if library is not initialized (Only for Linux and Mac).
 *  \returns CY_ERROR_INVALID_PARAMETER if the input parameters are invalid.
 *  \returns CY_ERROR_DEVICE_INFO_FETCH_FAILED if failed to fetch device information.
 *  \returns CY_ERROR_ACCESS_DENIED if access is denied by operating system.
 *  \returns CY_ERROR_DEVICE_NOT_FOUND if specified device number is invalid.
 *
 *  \sa CY_DEVICE_INFO
 *  \sa CY_DEVICE_TYPE
 *  \sa CY_DEVICE_CLASS
 *  \sa CyGetListofDevices
 *  \sa CyGetDeviceInfoVidPid
 *  \sa CyOpen
 *  \sa CyClose
 */
CYWINEXPORT CY_RETURN_STATUS CyGetDeviceInfo(
    UINT8 deviceNumber,                         /**< Device number of the device of interest */
    CY_DEVICE_INFO *deviceInfo                  /**< Info of device returned */
    );

/**
 *  \brief This API is used to retrieve the information of all devices with specified Vendor ID and Product ID.
 *
 *  For a given VID and PID, the API returns deviceIdList and deviceInfoList.
 *  The deviceIdList contains the device numbers of all the devices with specified VID and PID.
 *  Using deviceInfoList application can identify the device of interest.
 *  Information that is provided includes interface number, string descriptor, deviceType and deviceClass.
 *
 *  \returns CY_SUCCESS on success else error codes as defined in the enumeration CY_RETURN_STATUS.
 *  \returns CY_ERROR_REQUEST_FAILED on if library is not initialized (Only for Linux and Mac)
 *  \returns CY_ERROR_INVALID_PARAMETER if the input parameters are invalid.
 *  \returns CY_ERROR_DEVICE_INFO_FETCH_FAILED if failed to fetch device information.
 *  \returns CY_ERROR_ACCESS_DENIED if access is denied by operating system.
 *  \returns CY_ERROR_DEVICE_NOT_FOUND if specified device number is invalid.
 *
 *  \sa CY_DEVICE_INFO
 *  \sa CY_DEVICE_CLASS
 *  \sa CY_DEVICE_TYPE
 *  \sa CyGetListofDevices
 *  \sa CyGetDeviceInfo
 *  \sa CyOpen
 *  \sa CyClose
 */
CYWINEXPORT CY_RETURN_STATUS CyGetDeviceInfoVidPid (
    CY_VID_PID vidPid,                          /**< VID and PID of device of interest */
    UINT8 *deviceIdList,                        /**< Array of device ID's returned */
    CY_DEVICE_INFO *deviceInfoList,             /**< Array of pointers to device info list */
    UINT8 *deviceCount,                         /**< Count of devices with specified VID PID */
    UINT8 infoListLength                        /**< Total length of the deviceInfoList allocated
                                                     (Size of deviceInfoList array) */
    );

/**
 *  \brief This API is used to open the USB Serial device.
 *
 *  This API is used to open USB Serial device based on the device number.
 *
 *  Note: The argument interfaceNum is used on Linux and Mac OS while obtaining handle for specific
 *  interface. In Windows family of operating systems, this argument should be set to zero.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_REQUEST_FAILED on if library is not initialized (Only for Linux and Mac)
 *  \returns CY_ERROR_INVALID_PARAMETER if the input parameters are invalid.
 *  \returns CY_ERROR_DRIVER_OPEN_FAILED if open was unsuccessful.
 *  \returns CY_ERROR_ACCESS_DENIED if access is denied by operating system.
 *  \returns CY_ERROR_ALLOCATION_FAILED if memory allocation was failed.
 *  \returns CY_ERROR_DEVICE_NOT_FOUND if specified device number is invalid.
 *
 *  \sa CyGetListofDevices
 *  \sa CyGetDeviceInfoVidPid
 *  \sa CyGetDeviceInfo
 *  \sa CyClose
 */
CYWINEXPORT CY_RETURN_STATUS CyOpen (
    UINT8 deviceNumber,                         /**< Device number of device that needs to be opened */
    UINT8 interfaceNum,                         /**< Interface Number */
    CY_HANDLE *handle                           /**< Handle returned by the API */
    );

/**
 *  \brief This API closes the specified device handle and releases all resources associated with it.
 *
 *  This API closes the device handle and releases all the resources allocated internally in the
 *  library. This API should be invoked using a valid device handle and upon successful return
 *  of CyOpen.
 *
 *  \returns CY_SUCCESS on success.
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid in case of Linux/Mac.
 *  \returns CY_ERROR_INVALID_PARAMETER if handle is invalid in case of Windows.
 *  \returns CY_ERROR_REQUEST_FAILED on error in case of library being not initialized (Only for Linux and Mac).
 *
 *  \sa CyOpen
 */
CYWINEXPORT CY_RETURN_STATUS CyClose (
    CY_HANDLE handle                                   /**< Handle of the device that needs to be closed */
    );

/**
 *  \brief This API is used to power cycle the host port.
 *
 *  This API will power cycle the upstream port. It will reenumerate the device after the power cycle.
 *
 *  Note: This API is not supported on Linux and Mac
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid in case of Linux/Mac.
 *  \returns CY_ERROR_INVALID_PARAMETER if handle is invalid in case of Windows.
 *  \returns CY_ERROR_REQUEST_FAILED on error if request was failed by driver.
 *
 *  \sa CyResetDevice
 */
CYWINEXPORT CY_RETURN_STATUS WINCALLCONVEN CyCyclePort (
    CY_HANDLE handle                                 /**< Valid device handle */
    );

/*************************************************************************************/
/********************************Common APIs*********************************************/
/*************************************************************************************/

/*@@Common APIs

  These APIs provide an interface for accessing GPIO pins, error status notification on UART/SPI,
  getting library and firmware version and signature.
*/

/**
 *  \brief This API sets the value of a GPIO.
 *
 *  This API is used to set the value of a GPIO. It can only set the value of a
 *  GPIO that is configured as an output.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED on error when request is failed by USB Serial device.
 *
 *  \sa CyGetGpioValue
 */
CYWINEXPORT CY_RETURN_STATUS CySetGpioValue (
    CY_HANDLE handle,                           /**< Valid device handle */
    UINT8 gpioNumber,                           /**< GPIO number */
    UINT8 value                                 /**< Value that needs to be set */
    );

/**
 *  \brief This API retrieves the value of a GPIO.
 *
 *  This API retrieves the value of a GPIO.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range and
 *           also when handle is invalid in case of Windows.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device.
 *
 *  \sa CySetGpioValue
 */
CYWINEXPORT CY_RETURN_STATUS CyGetGpioValue (
    CY_HANDLE handle,                           /**< Valid device handle */
    UINT8 gpioNumber,                           /**< GPIO number */
    UINT8 *value                                /**< Current state of the GPIO */
    );


/**
 *  \brief This API is used to register a callback for error/event notifications
 *         during UART/SPI data transfers.
 *
 *  The API is used to register a callback for error/event notifications while
 *  doing data transfer on UART or SPI. A callback will be issued based on the
 *  error/events sent by the device.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_STATUS_MONITOR_EXIST if notification callback is already registered.
 *
 *  \returns CY_CALLBACK_EVENTS
 *  \returns CY_EVENT_NOTIFICATION_CB_FN
 *  \sa CyAbortEventNotification
 */
CYWINEXPORT CY_RETURN_STATUS CySetEventNotification(
    CY_HANDLE handle,                                   /**< Valid device handle */
    CY_EVENT_NOTIFICATION_CB_FN notificationCbFn        /**< Callback function pointer */
    );

/**
 *  \brief The API is used to unregister the event callback.
 *
 *  The API is used to unregister the event callback.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_REQUEST_FAILED if API is called before registering callback.
 *
 *  \sa CySetEventNotification
 */
CYWINEXPORT CY_RETURN_STATUS CyAbortEventNotification(
    CY_HANDLE handle                               /**< Valid device handle */
    );

/**
 *  \brief This API retrieves the version of USB Serial library.
 *
 *  This API retrieves the version of USB Serial library.
 *
 *  \returns CY_SUCCESS
 *
 *  \sa CyGetFirmwareVersion
 */
CYWINEXPORT CY_RETURN_STATUS CyGetLibraryVersion (
    CY_HANDLE handle,                            /**< Valid device handle */
    PCY_LIBRARY_VERSION version                  /**< Library version of the current library */
    );

/**
 *  \brief This API retrieves the firmware version of the USB Serial device.
 *
 *  This API retrieves the firmware version of the USB Serial device.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device.
 *
 *  \sa CyGetLibraryVersion
 */
CYWINEXPORT CY_RETURN_STATUS CyGetFirmwareVersion (
    CY_HANDLE handle,                                   /**< Valid device handle */
    PCY_FIRMWARE_VERSION firmwareVersion                /**< Firmware version. */
    );

/**
 *  \brief This API resets the device by sending a vendor request.
 *
 *  The API will reset the device by sending a vendor request to the firmware. The device
 *  will be re-enumerated.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device.
 *
 *  \sa CyCyclePort
 */
CYWINEXPORT CY_RETURN_STATUS CyResetDevice (
    CY_HANDLE handle                                   /**< Valid device handle */
    );

/**
 *  \brief The API writes to the user flash area on the USB Serial device.
 *
 *  The API programs user flash area. The total space available is 512 bytes.
 *  The flash area address offset is from 0x0000 to 0x00200 and should be written
 *  page wise (page size is 128 bytes).
 *  On return, transferCount parameter in CY_DATA_BUFFER will specify the number of bytes actually
 *  programmed.
 *
 *  Note: Length and page address needs to be 128 bytes aligned.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device.
 *
 *  \sa CyReadUserFlash
 */
CYWINEXPORT CY_RETURN_STATUS CyProgUserFlash (
    CY_HANDLE handle,                       /**< Valid device handle */
    CY_DATA_BUFFER *progBuffer,             /**< Data buffer containing buffer address, length to write */
    UINT32 flashAddress,                    /**< Address to the data is written */
    UINT32 timeout                          /**< Timeout value of the API */
    );

/**
 *  \brief The API reads from the flash address specified.
 *
 *  Read from user flash area.The total space available is 512 bytes.
 *  The flash area address offset is from 0x0000 to 0x00200 and should be read
 *  page wise (page size is 128 bytes).
 *  On return transferCount parameter in CY_DATA_BUFFER will specify the number of bytes actually
 *  read.
 *
 *  Note: Length and page address needs to be 128 bytes aligned.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device.
 *
 *  \sa CyProgUserFlash
 */
CYWINEXPORT CY_RETURN_STATUS CyReadUserFlash (
    CY_HANDLE handle,                       /**< Valid device handle */
    CY_DATA_BUFFER *readBuffer,             /**< Data buffer containing buffer address, length to read */
    UINT32 flashAddress,                    /**< Address from which the data is read */
    UINT32 timeout                          /**< Timeout value of the API */
    );

/**
 *  \brief This API retrieves the signature of the device firmware.
 *
 *  This API retrieves the signature of the device firmware.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device.
 */
CYWINEXPORT CY_RETURN_STATUS CyGetSignature (
    CY_HANDLE handle,                   /**< Valid device handle */
    UCHAR *pSignature                   /**< Signature returned */
    );

/****************************************************************************************/
/********************************UART API's**********************************************/
/****************************************************************************************/

/*@@UART API
  APIs used to communicate with UART module of the USB Serial device.
  These APIs provide support for configuration, data transfer and flow control.
*/

/**
 *  \brief This API retrieves the UART configuration from the USB Serial device.
 *
 *  This API retrieves the UART configuration from the USB Serial device.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device or when device type is
 *           not UART.
 *
 *  \sa CY_UART_CONFIG
 *  \sa CySetUartConfig
 */
CYWINEXPORT CY_RETURN_STATUS CyGetUartConfig (
    CY_HANDLE handle,                          /**< Valid device handle */
    CY_UART_CONFIG *uartConfig                 /**< UART configuration value read back */
    );

/**
 *  \brief This API sets the UART configuration of USB Serial device.
 *
 *  This API sets the UART configuration of USB Serial device.
 *
 *  Note: Using this API during an active transaction of UART may result in data loss.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device or when device type is
 *           not UART.
 *
 *  \sa CY_UART_CONFIG
 *  \sa CyGetUartConfig
 */
CYWINEXPORT CY_RETURN_STATUS CySetUartConfig (
    CY_HANDLE handle,                         /**< Valid device handle */
    CY_UART_CONFIG *uartConfig                /**< UART configuration value */
    );

/**
 *  \brief This API reads data from UART device.
 *
 *  This API is used to read data from UART device. User needs to initialize the readBuffer with buffer pointer,
 *  number of bytes to read before invoking this API.
 *  On return the transferCount parameter in CY_DATA_BUFFER will contain the number of bytes read.
 *
 *  \returns CY_SUCCESS on success.
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if input parameters were invalid.
 *  \returns CY_ERROR_REQUEST_FAILED if the device type is not UART.
 *  \returns CY_ERROR_IO_TIMEOUT if transfer was timed out.
 *  \returns CY_ERROR_PIPE_HALTED if pipe was stalled during data transfer.
 *  \returns CY_ERROR_DEVICE_NOT_FOUND if device was disconnected.
 *  \returns CY_ERROR_BUFFER_OVERFLOW if data received from USB Serial device is more than requested.
 *  \returns CY_ERROR_ALLOCATION_FAILED if transaction transmit buffer allocation was failed (Only in Windows).
 *
 *  \sa CY_DATA_BUFFER
 *  \sa CyUartWrite
 */
CYWINEXPORT CY_RETURN_STATUS CyUartRead (
    CY_HANDLE handle,                          /**< Valid device handle */
    CY_DATA_BUFFER* readBuffer,                /**< Read buffer details */
    UINT32 timeout                             /**< API timeout value */
    );

/**
 *  \brief This API writes the data to UART device.
 *
 *  This API writes the data to UART device. User need to initialize the
 *  writeBuffer with buffer pointer, number of bytes to write before invoking the API.
 *  On return the transferCount parameter in CY_DATA_BUFFER will contain the number
 *  of bytes written.
 *
 *  \returns CY_SUCCESS on success.
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if input parameters were invalid.
 *  \returns CY_ERROR_REQUEST_FAILED if the device type is not UART.
 *  \returns CY_ERROR_IO_TIMEOUT if transfer was timed out.
 *  \returns CY_ERROR_PIPE_HALTED if pipe was stalled during data transfer.
 *  \returns CY_ERROR_DEVICE_NOT_FOUND if device was disconnected.
 *  \returns CY_ERROR_BUFFER_OVERFLOW if data received from USB Serial device is more than requested.
 *  \returns CY_ERROR_ALLOCATION_FAILED if transaction transmit buffer allocation was failed (Only in Windows).
 *
 *  \sa CY_DATA_BUFFER
 *  \sa CyUartRead
 */
CYWINEXPORT CY_RETURN_STATUS CyUartWrite (
    CY_HANDLE handle,                      /**< Valid device handle */
    CY_DATA_BUFFER* writeBuffer,           /**< Write buffer details */
    UINT32 timeout                         /**< API timeout value */
    );

/**
 *  \brief This API enables hardware flow control.
 *
 *  This API enables hardware flow control.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE on error if handle is invalid in case of Linux/Mac.
 *  \returns CY_ERROR_INVALID_PARAMETER on error if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT on error if request was timed out.
 *  \returns CY_ERROR_REQUEST_FAILED on error if request was failed by device or if device type
 *           is not UART.
 *
 *  \sa CyUartGetHwFlowControl
 */
CYWINEXPORT CY_RETURN_STATUS CyUartSetHwFlowControl(
    CY_HANDLE handle,                       /**< Valid device handle */
    CY_FLOW_CONTROL_MODES mode              /**< Flow control mode */
    );

/**
 *  \brief This API retrieves the current hardware flow control status.
 *
 *  This API retrieves the current hardware flow control status.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device or when device type is
 *           not UART.
 *
 *  \sa CyUartSetHwFlowControl
 */
CYWINEXPORT CY_RETURN_STATUS CyUartGetHwFlowControl(
    CY_HANDLE handle,                     /**< Valid device handle */
    CY_FLOW_CONTROL_MODES *mode           /**< Flow control mode */
    );

/**
 *  \brief This API sets RTS signal in UART module.
 *
 *  This API is used to set the RTS pin to logical low..
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device or when device type is
 *           not UART.
 *
 *  \sa CyUartClearRts
 *  \sa CyUartSetDtr
 *  \sa CyUartClearDtr
 */
CYWINEXPORT CY_RETURN_STATUS CyUartSetRts(
    CY_HANDLE handle                /**< Valid device handle */
    );

/**
 *  \brief This API can be used to clear RTS signal in UART module.
 *
 *  This API used clear the RTS. It sets the RTS pin to logical high.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device or when device type is
 *           not UART.
 *
 *  \sa CyUartSetRts
 *  \sa CyUartSetDtr
 *  \sa CyUartClearDtr
 */
CYWINEXPORT CY_RETURN_STATUS CyUartClearRts(
    CY_HANDLE handle              /**< Valid device handle */
    );

/**
 *  \brief This API sets DTR signal in UART.
 *
 *  This API used set the DTR. It sets the DTR pin to logical low.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device or when device type is
 *           not UART.
 *
 *  \sa CyUartClearRts
 *  \sa CyUartSetRts
 *  \sa CyUartClearDtr
 */
CYWINEXPORT CY_RETURN_STATUS CyUartSetDtr(
    CY_HANDLE handle              /**< Valid device handle */
    );

/**
 *  \brief This API can be used to clear DTR signal in UART.
 *
 *  This API can be used clear the DTR. It sets the DTR pin to logical high.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device or when device type is
 *           not UART.
 *
 *  \sa CyUartSetRts
 *  \sa CyUartSetDtr
 *  \sa CyUartClearRts
 */
CYWINEXPORT CY_RETURN_STATUS CyUartClearDtr(
    CY_HANDLE handle              /**< Valid device handle */
    );

/**
 *  \brief This API can be used to set break timeout value .
 *
 *  This API can be used to set break timeout value .
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device or when device type is
 *           not UART.
 *
 *  \sa CyUartSetFlowControl
 */
CYWINEXPORT CY_RETURN_STATUS CyUartSetBreak(
    CY_HANDLE handle,                   /**< Valid device handle */
    UINT16 timeout                      /**< Break timeout value in milliseconds */
    );

/***********************************************************************************************/
/**********************************I2C API's****************************************************/
/***********************************************************************************************/

/*@@I2C API

  These set of APIs provide an interface to configure I2C module and do
  read/write on the I2C device connected to USB Serial device.
*/

/**
 *  \brief This API retrieves the configuration of I2C module of USB Serial device.
 *
 *  This API retrieves the configuration of I2C module of USB Serial device.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device or when device type is
 *           not I2C.
 *
 *  \sa CY_I2C_CONFIG
 *  \sa CySetI2cConfig
 */
CYWINEXPORT CY_RETURN_STATUS CyGetI2cConfig (
    CY_HANDLE handle,                         /**< Valid device handle */
    CY_I2C_CONFIG *i2cConfig                  /**< I2C configuration value read back */
    );

/**
 *  \brief This API configures the I2C module of USB Serial device.
 *
 *  This API configures the I2C module of USB Serial device.
 *
 *  Note: Using this API during an active transaction of I2C may result in data loss.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device or when device type is
 *           not I2C.
 *
 *  \sa CY_I2C_CONFIG
 *  \sa CySetI2cConfig
 */
CYWINEXPORT CY_RETURN_STATUS  CySetI2cConfig (
    CY_HANDLE handle,                         /**< Valid device handle */
    CY_I2C_CONFIG *i2cConfig                  /**< I2C configuration value */
    );

/**
 *  \brief This API reads data from the USB Serial I2C module.
 *
 *  This API provides an interface to read data from the I2C device
 *  connected to USB Serial.
 *
 *  The readBuffer parameter needs to be initialized with buffer pointer, number of bytes to be read
 *  before invoking the API. On return, the transferCount field will contain the number of bytes
 *  read back from device.
 *  CY_I2C_DATA_CONFIG structure specifies parameters such as setting stop bit, NAK and
 *  slave address of the I2C device.
 *
 *  \returns CY_SUCCESS on success.
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if input parameters were invalid.
 *  \returns CY_ERROR_REQUEST_FAILED if the device type is not I2C
 *  \returns CY_ERROR_IO_TIMEOUT if transfer was timed out.
 *  \returns CY_ERROR_DEVICE_NOT_FOUND if device was disconnected.
 *  \returns CY_ERROR_BUFFER_OVERFLOW if data received from USB Serial device is more than requested.
 *  \returns CY_ERROR_ALLOCATION_FAILED if transaction transmit buffer allocation was failed (Only in Windows).
 *  \returns CY_ERROR_I2C_DEVICE_BUSY if I2C device was busy processing previous request.
 *  \returns CY_ERROR_I2C_NAK_ERROR if request was nacked by I2C device.
 *  \returns CY_ERROR_I2C_ARBITRATION_ERROR if a I2C bus arbitration error occured.
 *  \returns CY_ERROR_I2C_BUS_ERROR if there was any I2C bus error while an on going transaction.
 *  \returns CY_ERROR_I2C_STOP_BIT_SET if stop bit was set by I2C master.
 *
 *  \sa CY_DATA_BUFFER
 *  \sa CY_DATA_CONFIG
 *  \sa CyI2cCWrite
 */
CYWINEXPORT CY_RETURN_STATUS CyI2cRead (
    CY_HANDLE handle,                           /**< Valid device handle */
    CY_I2C_DATA_CONFIG *dataConfig,             /**< I2C data config */
    CY_DATA_BUFFER *readBuffer,                 /**< Read buffer details */
    UINT32 timeout                              /**< API timeout value */
    );

/**
 *  \brief This API writes data to USB Serial I2C module .
 *
 *  This API provides an interface to write data to the I2C device
 *  connected to USB Serial.
 *  The writeBuffer parameter needs to be initialized with buffer pointer, number of bytes to be written
 *  before invoking the API. On return, transferCount field contains number of bytes actually written to the device.
 *  CY_I2C_DATA_CONFIG structure specifies parameter such as setting stop bit, Nak and slave address
 *  of the I2C device being communicated when USB Serial is master.
 *
 *  \returns CY_SUCCESS on success.
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if input parameters were invalid.
 *  \returns CY_ERROR_REQUEST_FAILED if the device type is not I2C
 *  \returns CY_ERROR_IO_TIMEOUT if transfer was timed out.
 *  \returns CY_ERROR_PIPE_HALTED if pipe was stalled during data transfer.
 *  \returns CY_ERROR_DEVICE_NOT_FOUND if device was disconnected.
 *  \returns CY_ERROR_BUFFER_OVERFLOW if data received from USB Serial device is more than requested.
 *  \returns CY_ERROR_ALLOCATION_FAILED if transaction transmit buffer allocation was failed (Only in Windows).
 *  \returns CY_ERROR_I2C_DEVICE_BUSY if I2C device was busy processing previous request.
 *  \returns CY_ERROR_I2C_NAK_ERROR if request was nacked by I2C device.
 *  \returns CY_ERROR_I2C_ARBITRATION_ERROR if a I2C bus arbitration error occured.
 *  \returns CY_ERROR_I2C_BUS_ERROR if there was any I2C bus error while an on going transaction.
 *  \returns CY_ERROR_I2C_STOP_BIT_SET if stop bit was set by I2C master.
 *
 *  \sa CY_DATA_BUFFER
 *  \sa CY_DATA_CONFIG
 *  \sa CyI2cRead
 */
CYWINEXPORT CY_RETURN_STATUS  WINCALLCONVEN CyI2cWrite (
    CY_HANDLE handle,                           /**< Valid device handle */
    CY_I2C_DATA_CONFIG *dataConfig,             /**< I2C Slave address */
    CY_DATA_BUFFER *writeBuffer,                /**< Write buffer details */
    UINT32 timeout                              /**< API timeout value */
    );

/**
 *  \brief This API resets the I2C module in USB Serial device.
 *
 *  This API resets the I2C module whenever there is an error in data transaction.
 *
 *  If resetMode = 0 the I2C read module will be reset.
 *  If resetMode = 1 the I2C write module will be reset.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device or when device type is
 *           not I2C.
 *
 *  \sa CyI2CRead
 *  \sa CyI2CWrite
 */
CYWINEXPORT CY_RETURN_STATUS CyI2cReset(
                                        CY_HANDLE handle, /**< Valid device handle */
                                        BOOL resetMode    /**< Reset mode */
                                        );

/***********************************************************************************************/
/**********************************SPI APIs****************************************************/
/***********************************************************************************************/

/*@@SPI API
  These set of APIs provide an interface to configure SPI module and perform
  read/write operations with the SPI device connected to USB Serial device.
*/

/**
 *  \brief This API retrieves the configuration of SPI module of USB Serial device.
 *
 *  This API retrieves the configuration of SPI module of USB Serial device.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device or when device type is
 *           not SPI.
 *
 *  \sa CY_SPI_CONFIG
 *  \sa CySetSpiConfig
 */
CYWINEXPORT CY_RETURN_STATUS CyGetSpiConfig (
    CY_HANDLE handle,                         /**< Valid device handle */
    CY_SPI_CONFIG *spiConfig                  /**< SPI configuration structure value read back */
    );

/**
 *  \brief This API sets the configuration of the SPI module on USB Serial device.
 *
 *  This API sets the configuration of the SPI module in USB Serial device.
 *
 *  NOTE: Using this API during an active transaction of SPI may result in data loss.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if the request is timed out.
 *  \returns CY_ERROR_REQUEST_FAILED when request is failed by USB Serial device or when device type is
 *           not SPI.
 *
 *  \sa CY_SPI_CONFIG
 *  \sa CyGetSpiConfig
 */
CYWINEXPORT CY_RETURN_STATUS CySetSpiConfig (
    CY_HANDLE handle,                         /**< Valid device handle */
    CY_SPI_CONFIG *spiConfig                  /**< SPI configuration structure value */
    );

/**
 *  \brief This API reads and writes data to SPI device connected to USB Serial device.
 *
 *  This API provides an interface to do data transfer with the SPI slave/master
 *  connected to USB Serial device.
 *  To perform read only operation, pass NULL as argument for writeBuffer and to perform
 *  write only operation pass NULL as an argument for readBuffer.
 *  On return, the transferCount field will contain the number of bytes read and/or written.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid in case of Linux/Mac.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_REQUEST_FAILED if the device type is not SPI or when libusb reported
 *           unknown error in case of Linux/Mac.
 *  \returns CY_ERROR_IO_TIMEOUT if transfer was timed out.
 *  \returns CY_ERROR_PIPE_HALTED if pipe was stalled during data transfer.
 *  \returns CY_ERROR_DEVICE_NOT_FOUND if device was disconnected.
 *  \returns CY_ERROR_BUFFER_OVERFLOW if data received from USB Serial device is more than requested.
 *
 *  \sa CY_DATA_BUFFER
 *  \sa CyGetSpiConfig
 *  \sa CySetSpiConfig
 */
CYWINEXPORT CY_RETURN_STATUS CySpiReadWrite (
    CY_HANDLE handle,               /**< Valid device handle */
    CY_DATA_BUFFER* readBuffer,     /**< Read data buffer */
    CY_DATA_BUFFER* writeBuffer,    /**< Write data buffer */
    UINT32 timeout                  /**< Time out value of the API */
    );

/**************************************************************************************/
/*****************************************JTAG APIs***********************************/
/**************************************************************************************/

/*@@JTAG API
  These set of APIs can be used to enable or disable JTAG module on the USB Serial device.
  Once the JTAG is enabled, read and write operations can be performed.
  When JTAG is enabled other modules in the USB Serial device cannot be used.
*/

/**
 *  \brief This API enables JTAG module.
 *
 *  This API enables JTAG module in USB Serial device and the function disables all other functionality
 *  till CyJtagDisable is invoked.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid in case of Linux/Mac.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if request was timed out.
 *  \returns CY_ERROR_REQUEST_FAILED if request was failed by device or if device type
 *           is not JTAG.
 *
 *  \sa CyJtagDisable
 */
CYWINEXPORT CY_RETURN_STATUS CyJtagEnable (
    CY_HANDLE handle                          /**< Valid device handle */
    );

/**
 *  \brief This API disables JTAG module.
 *
 *  This API disables Jtag interface in USB Serial device. This API must be invoked before exiting the
 *  application if CyJtagEnable was previously invoked.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid in case of Linux/Mac.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_IO_TIMEOUT if request was timed out.
 *  \returns CY_ERROR_REQUEST_FAILED if request was failed by device or if device type
 *           is not JTAG.
 *
 *  \sa CyJtagEnable
 */
CYWINEXPORT CY_RETURN_STATUS CyJtagDisable (
    CY_HANDLE handle                          /**< Valid device handle */
    );

/**
 *  \brief This API can be used to write data to JTAG module.
 *
 *  This API provides an interface to write data to JTAG device connected to USB Serial device.
 *  The writeBuffer need to be initialized with buffer and length of data to be written before invoking
 *  the API. Upon return, transferCount field in CY_DATA_BUFFER is updated with actual number of bytes written.
 *
 *  Note: CyJtagEnable must be called before invoking this API.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid in case of Linux/Mac.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_REQUEST_FAILED if device type is not JTAG or when encountered
 *           unknown libusb errors in Linux/Mac.
 *  \returns CY_ERROR_PIPE_HALTED if there was any pipe error during transaction.
 *  \returns CY_ERROR_IO_TIMEOUT if transfer was timed out.
 *  \returns CY_ERROR_DEVICE_NOT_FOUND if device was disconnected.
 *
 *  \sa CY_DATA_BUFFER
 *  \sa CyJtagRead
 *  \sa CyJtagEnable
 */
CYWINEXPORT CY_RETURN_STATUS CyJtagWrite (
    CY_HANDLE handle,                         /**< Valid device handle */
    CY_DATA_BUFFER *writeBuffer,              /**< Write buffer details */
    UINT32 timeout                            /**< API timeout value */
    );

/**
 *  \brief This API reads data from JTAG device.
 *
 *  This API provides an interface to read data from JTAG device.
 *  The readBuffer need to be initialized with buffer and length of data to be written before invoking
 *  the API. Upon return, transferCount field in CY_DATA_BUFFER structure
 *  is updated with actual number of bytes read.
 *
 *  Note: CyJtagEnable must be called before invoking this API.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle is invalid in case of Linux/Mac.
 *  \returns CY_ERROR_INVALID_PARAMETER if specified parameters are invalid or out of range.
 *  \returns CY_ERROR_REQUEST_FAILED if device type is not JTAG or when encountered
 *           unknown libusb errors in Linux/Mac.
 *  \returns CY_ERROR_IO_TIMEOUT if transfer was timed out.
 *  \returns CY_ERROR_DEVICE_NOT_FOUND if device was disconnected.
 *  \returns CY_ERROR_BUFFER_OVERFLOW if data received from USB Serial device is more than requested.
 *
 *  \sa CY_DATA_BUFFER
 *  \sa CyJtagWrite
 *  \sa CyJtagEnable
 */
CYWINEXPORT CY_RETURN_STATUS CyJtagRead (
    CY_HANDLE handle,                         /**< Valid device handle */
    CY_DATA_BUFFER *readBuffer,               /**< Read buffer parameters */
    UINT32 timeout                            /**< API timeout value */
    );

/**************************************************************************************/
/*****************************************PHDC APIs***********************************/
/**************************************************************************************/

/*@@PHDC API
  Set of PHDC class request APIs. The PHDC class requests include set, clear feature and
  PHDC get status.
*/

/**
 *  \brief This API sends a PHDC clear feature command.
 *
 *  This API sends a PHDC clear feature command.
 *
 *  Note: Meta data feature is not supported by USB Serial device.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle was invalid.
 *  \returns CY_ERROR_IO_TIMEOUT if request timed out.
 *  \returns CY_ERROR_REQUEST_FAILED if request was failed by device.
 *
 *  \sa CyPhdcSetFeature
 *  \sa CyPhdcGetStatus
 */
CYWINEXPORT CY_RETURN_STATUS CyPhdcClrFeature (
        CY_HANDLE handle                /**< Valid device handle */
        );

/**
 *  \brief This API sends a PHDC set feature command.
 *
 *  This API sends a PHDC set feature command.
 *
 *  Note: Meta data feature is not supported by USB Serial device.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle was invalid.
 *  \returns CY_ERROR_IO_TIMEOUT if request timed out.
 *  \returns CY_ERROR_REQUEST_FAILED if request was failed by device.
 *
 *  \sa CyPhdcClrFeature
 *  \sa CyPhdcGetStatus
 */
CYWINEXPORT CY_RETURN_STATUS CyPhdcSetFeature (
        CY_HANDLE handle                /**< Valid device handle */
        );

/**
 *  \brief This API retrieves the endpoint status of PHDC transaction.
 *
 *  The API retrieves the status of PHDC transaction. It returns 2 bytes of data pending bit map
 *  which is defined as per PHDC spec.
 *
 *  \returns CY_SUCCESS on success
 *  \returns CY_ERROR_INVALID_HANDLE if handle was invalid.
 *  \returns CY_ERROR_IO_TIMEOUT if request timed out.
 *  \returns CY_ERROR_REQUEST_FAILED if request was failed by device.
 *
 *  \sa CyPhdcClrFeature
 *  \sa CyPhdcSetFeature
 */
CYWINEXPORT CY_RETURN_STATUS CyPhdcGetStatus (
        CY_HANDLE handle,               /**< Valid device handle */
        UINT16 *dataStatus              /**< Data pending status bit map */
        );

#endif /*_INCLUDED_Cypress USB Serial_H_*/
