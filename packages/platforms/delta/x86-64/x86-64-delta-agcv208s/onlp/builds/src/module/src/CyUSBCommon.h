/*
 * Common header file of Cypress USB Serial
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

/** \file CyUSBCommon.h
 *  \brief Common header file of Cypress USB Serial
 */

#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <pthread.h>
#include "/usr/include/libusb-1.0/libusb.h"
#include "CyUSBSerial.h"

typedef struct CY_DEVICE
{
#pragma pack(1)
    unsigned char inEndpoint;
    unsigned char outEndpoint;
    unsigned char interruptEndpoint;
    unsigned char interfaceNum;
    bool i2cCancelEvent;
    bool spiCancelEvent;
    bool uartCancelEvent;
    bool rtsValue;
    bool dtrValue;
    unsigned short numEndpoints;
    CY_FLOW_CONTROL_MODES uartFlowControlMode;
    struct libusb_transfer *spiTransfer;
    struct libusb_transfer *uartTransfer;
    libusb_device_handle *devHandle;
    pthread_t spiThread;
    bool spiThreadRunning;
    pthread_t uartThread;
    bool uartThreadRunning;
#pragma pack()
    pthread_mutex_t readLock;
    pthread_mutex_t writeLock;
    pthread_mutex_t notificationLock;
#pragma pack(1)
    CY_DEVICE_TYPE deviceType;
#pragma pack()

} CY_DEVICE, *PCY_DEVICE;

CY_RETURN_STATUS CyResetPipe(CY_HANDLE handl, UINT8);

#define CY_DEBUG_PRINT_INFO(...)  //User need to enable this
#define CY_DEBUG_PRINT_ERROR(...) //printf

#define DUMP_DATA 1
#ifdef DUMP_DATA
#define CY_DUMP_DATA(INPUT, LEN)         \
    {                                    \
        int i = 0, len = LEN;            \
        while ((len))                    \
        {                                \
            printf("%x ", (INPUT)[i++]); \
            len--;                       \
        }                                \
        printf("\n");                    \
    }
#else
#define CY_DUMP_DATA   \
    (INPUT, LEN) do {} \
    while (0)          \
        ;
#endif

#define CY_USB_SERIAL_TIMEOUT 5000
#define CY_EVENT_NOTIFICATION_TIMEOUT 0 //This will make the transfer infinite

#define CY_VENDOR_REQUEST_DEVICE_TO_HOST 0xC0
#define CY_VENDOR_REQUEST_HOST_TO_DEVICE 0x40

#define CY_CLASS_INTERFACE_REQUEST_DEVICE_TO_HOST 0XA1
#define CY_CLASS_INTERFACE_REQUEST_HOST_TO_DEVICE 0x21

//I2C related macros
#define CY_SCB_INDEX_POS 15
#define CY_I2C_CONFIG_LENGTH 16
#define CY_I2C_WRITE_COMMAND_POS 3
#define CY_I2C_WRITE_COMMAND_LEN_POS 4
#define CY_I2C_GET_STATUS_LEN 3
#define CY_I2C_MODE_WRITE 1
#define CY_I2C_MODE_READ 0
#define CY_I2C_ERROR_BIT (1)
#define CY_I2C_ARBITRATION_ERROR_BIT (1 << 1)
#define CY_I2C_NAK_ERROR_BIT (1 << 2)
#define CY_I2C_BUS_ERROR_BIT (1 << 3)
#define CY_I2C_STOP_BIT_ERROR (1 << 4)
#define CY_I2C_BUS_BUSY_ERROR (1 << 5)
#define CY_I2C_ENABLE_PRECISE_TIMING 1
#define CY_I2C_EVENT_NOTIFICATION_LEN 3

//SPI related Macros
#define CY_SPI_CONFIG_LEN 16
#define CY_SPI_EVENT_NOTIFICATION_LEN 2
#define CY_SPI_READ_BIT (1)
#define CY_SPI_WRITE_BIT (1 << 1)
#define CY_SPI_SCB_INDEX_BIT (1 << 15)
#define CY_SPI_GET_STATUS_LEN 4
#define CY_SPI_UNDERFLOW_ERROR (1)
#define CY_SPI_BUS_ERROR (1 << 1)

//Vendor UART related macros
#define CY_UART_SET_FLOW_CONTROL_CMD 0x60
#define CY_UART_SEND_BREAK_CMD 0x17
#define CY_UART_CONFIG_LEN 16
#define CY_SET_LINE_CONTROL_STATE_CMD 0x22
#define CY_UART_EVENT_NOTIFICATION_LEN 10
#define CY_UART_SERIAL_STATE_CARRIER_DETECT 1
#define CY_UART_SERIAL_STATE_TRANSMISSION_CARRIER (1 << 1)
#define CY_UART_SERIAL_STATE_BREAK_DETECTION (1 << 2)
#define CY_UART_SERIAL_STATE_RING_SIGNAL_DETECTION (1 << 3)
#define CY_UART_SERIAL_STATE_FRAMING_ERROR (1 << 4)
#define CY_UART_SERIAL_STATE_PARITY_ERROR (1 << 5)
#define CY_UART_SERIAL_STATUE_OVERRUN (1 << 6)

//Bootloader related macros
#define CY_BOOT_CONFIG_SIZE 64
#define CY_DEVICE_CONFIG_SIZE 512
#define CY_FIRMWARE_BREAKUP_SIZE 4096
#define CY_GET_SILICON_ID_LEN 4
#define CY_GET_FIRMWARE_VERSION_LEN 8
#define CY_GET_SIGNATURE_LEN 4

//JTAG related Macros
#define CY_JTAG_OUT_EP 0x04
#define CY_JTAG_IN_EP 0x85

//GPIO related Macros
#define CY_GPIO_GET_LEN 2
#define CY_GPIO_SET_LEN 1

//PHDC related macros
#define CY_PHDC_SET_FEATURE 0X03
#define CY_PHDC_SET_FEATURE_WVALUE 0x0101
#define CY_PHDC_CLR_FEATURE 0X01
#define CY_PHDC_CLR_FEATURE_WVALUE 0x1
#define CY_PHDC_GET_DATA_STATUS 0x00
#define CY_PHDC_GET_STATUS_LEN 2

typedef enum CY_VENDOR_CMDS
{
    CY_GET_VERSION_CMD = 0xB0,     /**< Get the version of the boot-loader.
                                         *   value = 0, index = 0, length = 4;
                                         *   data_in = 32 bit version.
                                         */
    CY_GET_SIGNATURE_CMD = 0xBD,   /**< Get the signature of the firmware
                                         *   It is suppose to be 'CYUS' for normal firmware
                                         *   and 'CYBL' for Bootloader.
                                         */
    CY_UART_GET_CONFIG_CMD = 0xC0, /**< Retreive the 16 byte UART configuration information.
                                         *   MS bit of value indicates the SCB index.
                                         *   length = 16, data_in = 16 byte configuration.
                                         */
    CY_UART_SET_CONFIG_CMD,        /**< Update the 16 byte UART configuration information.
                                         *   MS bit of value indicates the SCB index.
                                         *   length = 16, data_out = 16 byte configuration information.
                                         */
    CY_SPI_GET_CONFIG_CMD,         /**< Retreive the 16 byte SPI configuration information.
                                         *   MS bit of value indicates the SCB index.
                                         *   length = 16, data_in = 16 byte configuration.
                                         */
    CY_SPI_SET_CONFIG_CMD,         /**< Update the 16 byte SPI configuration information.
                                         *   MS bit of value indicates the SCB index.
                                         *   length = 16, data_out = 16 byte configuration information.
                                         */
    CY_I2C_GET_CONFIG_CMD,         /**< Retreive the 16 byte I2C configuration information.
                                         *   MS bit of value indicates the SCB index.
                                         *   length = 16, data_in = 16 byte configuration.
                                         */
    CY_I2C_SET_CONFIG_CMD = 0xC5,  /**< Update the 16 byte I2C configuration information.
                                         *   MS bit of value indicates the SCB index.
                                         *   length = 16, data_out = 16 byte configuration information.
                                         */
    CY_I2C_WRITE_CMD,              /**< Perform I2C write operation.
                                         *   value = bit0 - start, bit1 - stop, bit3 - start on idle,
                                         *   bits[14:8] - slave address, bit15 - scbIndex. length = 0. The
                                         *   data is provided over the bulk endpoints.
                                         */
    CY_I2C_READ_CMD,               /**< Perform I2C read operation.
                                         *   value = bit0 - start, bit1 - stop, bit2 - Nak last byte,
                                         *   bit3 - start on idle, bits[14:8] - slave address, bit15 - scbIndex,
                                         *   length = 0.  The data is provided over the bulk endpoints.
                                         */
    CY_I2C_GET_STATUS_CMD,         /**< Retreive the I2C bus status.
                                         *   value = bit0 - 0: TX 1: RX, bit15 - scbIndex, length = 3,
                                         *   data_in = byte0: bit0 - flag, bit1 - bus_state, bit2 - SDA state,
                                         *   bit3 - TX underflow, bit4 - arbitration error, bit5 - NAK
                                         *   bit6 - bus error,
                                         *   byte[2:1] Data count remaining.
                                         */
    CY_I2C_RESET_CMD,              /**< The command cleans up the I2C state machine and frees the bus.
                                         *   value = bit0 - 0: TX path, 1: RX path; bit15 - scbIndex,
                                         *   length = 0.
                                         */
    CY_SPI_READ_WRITE_CMD = 0xCA,  /**< The command starts a read / write operation at SPI.
                                         *   value = bit 0 - RX enable, bit 1 - TX enable, bit 15 - scbIndex;
                                         *   index = length of transfer.
                                         */
    CY_SPI_RESET_CMD,              /**< The command resets the SPI pipes and allows it to receive new
                                         *   request.
                                         *   value = bit 15 - scbIndex
                                         */
    CY_SPI_GET_STATUS_CMD,         /**< The command returns the current transfer status. The count will match
                                         *   the TX pipe status at SPI end. For completion of read, read all data
                                         *   at the USB end signifies the end of transfer.
                                         *   value = bit 15 - scbIndex
                                         */
    CY_JTAG_ENABLE_CMD = 0xD0,     /**< Enable JTAG module */
    CY_JTAG_DISABLE_CMD,           /**< Disable JTAG module */
    CY_JTAG_READ_CMD,              /**< JTAG read vendor command */
    CY_JTAG_WRITE_CMD,             /**< JTAG write vendor command */
    CY_GPIO_GET_CONFIG_CMD = 0xD8, /**< Get the GPIO configuration */
    CY_GPIO_SET_CONFIG_CMD,        /**< Set the GPIO configuration */
    CY_GPIO_GET_VALUE_CMD,         /**< Get GPIO value */
    CY_GPIO_SET_VALUE_CMD,         /**< Set the GPIO value */
    CY_PROG_USER_FLASH_CMD = 0xE0, /**< Program user flash area. The total space available is 512 bytes.
                                         *   This can be accessed by the user from USB. The flash area
                                         *   address offset is from 0x0000 to 0x00200 and can be written to
                                         *   page wise (128 byte).
                                         */
    CY_READ_USER_FLASH_CMD,        /**< Read user flash area. The total space available is 512 bytes.
                                         *   This can be accessed by the user from USB. The flash area
                                         *   address offset is from 0x0000 to 0x00200 and can be written to
                                         *   page wise (128 byte).
                                         */
    CY_DEVICE_RESET_CMD = 0xE3,    /**< Performs a device reset from firmware */

} CY_VENDOR_CMDS;

extern libusb_context *glContext;
