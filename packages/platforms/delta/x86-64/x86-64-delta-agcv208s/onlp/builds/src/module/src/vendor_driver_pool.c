/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright (C) 2019 Delta Electronics, Inc.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 * </bsn.cl>
 ************************************************************/
#include <onlp/onlp.h>
#include <onlplib/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <AIM/aim.h>
#include "vendor_driver_pool.h"
#include "vendor_i2c_device_list.h"

#define CYPRESS 1

#ifdef CYPRESS
#include "CyUSBSerial.h"
#endif

static int vendor_driver_add(vendor_driver_t *driver);

/*
    Two I2C bus driver here:
        SMBUS: using onlp i2c driver
        IPMB: using raw command

    I2C BUS DRIVER START:
*/

/* smbus_access read or write markers */
#define I2C_SMBUS_READ 1
#define I2C_SMBUS_WRITE 0

/* To determine what functionality is present */
#define I2C_FUNC_I2C 0x00000001
#define I2C_FUNC_10BIT_ADDR 0x00000002
#define I2C_FUNC_PROTOCOL_MANGLING 0x00000004 /* I2C_M_{REV_DIR_ADDR,NOSTART,..} */
#define I2C_FUNC_SMBUS_PEC 0x00000008
#define I2C_FUNC_SMBUS_BLOCK_PROC_CALL 0x00008000 /* SMBus 2.0 */
#define I2C_FUNC_SMBUS_QUICK 0x00010000
#define I2C_FUNC_SMBUS_READ_BYTE 0x00020000
#define I2C_FUNC_SMBUS_WRITE_BYTE 0x00040000
#define I2C_FUNC_SMBUS_READ_BYTE_DATA 0x00080000
#define I2C_FUNC_SMBUS_WRITE_BYTE_DATA 0x00100000
#define I2C_FUNC_SMBUS_READ_WORD_DATA 0x00200000
#define I2C_FUNC_SMBUS_WRITE_WORD_DATA 0x00400000
#define I2C_FUNC_SMBUS_PROC_CALL 0x00800000
#define I2C_FUNC_SMBUS_READ_BLOCK_DATA 0x01000000
#define I2C_FUNC_SMBUS_WRITE_BLOCK_DATA 0x02000000
#define I2C_FUNC_SMBUS_READ_I2C_BLOCK 0x04000000  /* I2C-like block xfer  */
#define I2C_FUNC_SMBUS_WRITE_I2C_BLOCK 0x08000000 /* w/ 1-byte reg. addr. */

#define I2C_FUNC_SMBUS_BYTE (I2C_FUNC_SMBUS_READ_BYTE | \
                             I2C_FUNC_SMBUS_WRITE_BYTE)
#define I2C_FUNC_SMBUS_BYTE_DATA (I2C_FUNC_SMBUS_READ_BYTE_DATA | \
                                  I2C_FUNC_SMBUS_WRITE_BYTE_DATA)
#define I2C_FUNC_SMBUS_WORD_DATA (I2C_FUNC_SMBUS_READ_WORD_DATA | \
                                  I2C_FUNC_SMBUS_WRITE_WORD_DATA)
#define I2C_FUNC_SMBUS_BLOCK_DATA (I2C_FUNC_SMBUS_READ_BLOCK_DATA | \
                                   I2C_FUNC_SMBUS_WRITE_BLOCK_DATA)
#define I2C_FUNC_SMBUS_I2C_BLOCK (I2C_FUNC_SMBUS_READ_I2C_BLOCK | \
                                  I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)

/* Old name, for compatibility */
#define BF_I2C_FUNC_SMBUS_HWPEC_CALC I2C_FUNC_SMBUS_PEC

/* SMBus transaction types (size parameter in the above functions) 
   Note: these no longer correspond to the (arbitrary) PIIX4 internal codes! */
#define I2C_SMBUS_QUICK 0
#define I2C_SMBUS_BYTE 1
#define I2C_SMBUS_BYTE_DATA 2
#define I2C_SMBUS_WORD_DATA 3
#define I2C_SMBUS_PROC_CALL 4
#define I2C_SMBUS_BLOCK_DATA 5
#define I2C_SMBUS_I2C_BLOCK_BROKEN 6
#define I2C_SMBUS_BLOCK_PROC_CALL 7 /* SMBus 2.0 */
#define I2C_SMBUS_I2C_BLOCK_DATA 8

#define I2C_RETRIES 0x0701     /* number of times a device address      */
#define I2C_TIMEOUT 0x0702     /* set timeout - call with int 		*/
#define I2C_SLAVE 0x0703       /* Change slave address			*/
#define I2C_SLAVE_FORCE 0x0706 /* Change slave address			*/
#define I2C_TENBIT 0x0704      /* 0 for 7 bit addrs, != 0 for 10 bit	*/
#define I2C_FUNCS 0x0705       /* Get the adapter functionality */
#define I2C_RDWR 0x0707        /* Combined R/W transfer (one stop only)*/
#define I2C_PEC 0x0708         /* != 0 for SMBus PEC                   */
#define I2C_SMBUS 0x0720       /* SMBus-level access */

int VENDOR_DRV_SMBUS_Set(int bus, uint8_t dev, uint16_t daddr, uint8_t alen, uint16_t data, uint8_t dlen);
int VENDOR_DRV_SMBUS_Get(int bus, uint8_t dev, uint16_t daddr, uint8_t alen, uint16_t *data, uint8_t dlen);
int VENDOR_DRV_SMBUS_Set_Byte(int bus, uint8_t dev, uint16_t daddr, uint8_t alen, uint16_t data, uint8_t dlen);
int VENDOR_DRV_SMBUS_Get_Byte(int bus, uint8_t dev, uint16_t daddr, uint8_t alen, uint16_t *data, uint8_t dlen);
int VENDOR_DRV_SMBUS_Probe(uint8_t ucBus, uint8_t dev);
int VENDOR_DRV_SMBUS_Read_Block(int bus, uint8_t dev, uint16_t daddr, uint8_t *ReplyBuf, uint16_t BufSize);
int VENDOR_DRV_SMBUS_Write_Block(int bus, uint8_t dev, uint16_t daddr, uint8_t *value, uint16_t BufSize);
int VENDOR_DRV_SMBUS_Read_I2C_Block(int bus, uint8_t dev, uint16_t daddr, uint8_t *ReplyBuf, uint16_t BufSize);
int VENDOR_DRV_SMBUS_Write_I2C_Block(int bus, uint8_t dev, uint16_t daddr, uint8_t *value, uint16_t BufSize);

int VENDOR_DRV_I2C_SMBUS_Access(int dev_fd, char read_write, uint8_t command, int size, union i2c_smbus_data *data);
int VENDOR_DRV_I2C_SMBUS_Write_Quick(int dev_fd, uint8_t value);
int VENDOR_DRV_I2C_SMBUS_Read_Byte(int dev_fd);
int VENDOR_DRV_I2C_SMBUS_Write_Byte(int dev_fd, uint8_t value);
int VENDOR_DRV_I2C_SMBUS_Read_Byte_Data(int dev_fd, uint8_t command);
int VENDOR_DRV_I2C_SMBUS_Write_Byte_Data(int dev_fd, uint8_t command, uint8_t value);
int VENDOR_DRV_I2C_SMBUS_Read_Word_Data(int dev_fd, uint8_t command);
int VENDOR_DRV_I2C_SMBUS_Write_Word_Data(int dev_fd, uint8_t command, __u16 value);
int VENDOR_DRV_I2C_SMBUS_Process_Call(int dev_fd, uint8_t command, __u16 value);
int VENDOR_DRV_I2C_SMBUS_Read_Block_Data(int dev_fd, uint8_t command, uint8_t *values);
int VENDOR_DRV_I2C_SMBUS_Write_Block_Data(int dev_fd, uint8_t command, uint8_t length, uint8_t *values);
int VENDOR_DRV_I2C_SMBUS_Read_I2C_Block_Data(int file, uint8_t command, uint16_t length, uint8_t *values);
int VENDOR_DRV_I2C_SMBUS_Write_I2C_Block_Data(int file, uint8_t command, uint8_t length, const uint8_t *values);
int VENDOR_DRV_I2C_SMBUS_Block_Process_Call(int dev_fd, uint8_t command, uint8_t length, uint8_t *values);
int VENDOR_DRV_I2C_SMBUS_Probe(uint8_t bus, uint8_t dev);

static i2c_bus_driver_t smbus_functions = {
    VENDOR_DRV_SMBUS_Get,
    VENDOR_DRV_SMBUS_Set,
    VENDOR_DRV_SMBUS_Get_Byte,
    VENDOR_DRV_SMBUS_Set_Byte,
    VENDOR_DRV_SMBUS_Read_I2C_Block,
    VENDOR_DRV_SMBUS_Write_Block,
    VENDOR_DRV_SMBUS_Read_I2C_Block,
    VENDOR_DRV_SMBUS_Write_I2C_Block,
    VENDOR_DRV_SMBUS_Probe};

static int smbus_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "I2C", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &smbus_functions;

    return vendor_driver_add(driver);
}

/*================ internal function ================*/

int VENDOR_DRV_SMBUS_Set_Byte(int bus, uint8_t dev, uint16_t daddr, uint8_t alen, uint16_t data, uint8_t dlen)
{
    char dev_path[20];
    int fd = -1;
    int res = 0;

    sprintf(dev_path, "/dev/i2c-%d", bus);

    /* Open the SMBus device */
    if ((fd = open(dev_path, O_RDWR | O_SYNC)) < 0)
    {
        AIM_LOG_ERROR("[SET] Can not open %s\n", dev_path);
        return -1;
    }

    /* set slave address */
    if (ioctl(fd, I2C_SLAVE, dev) < 0)
    {
        if (errno != EBUSY)
        {
            AIM_LOG_ERROR("[SET] Can not set device address 0x%02X,shift=0\n", dev);
        }
        close(fd);
        return -1;
    }

    res = VENDOR_DRV_I2C_SMBUS_Write_Byte(fd, (uint8_t)data);

    if (res < 0)
    {
        AIM_LOG_ERROR("[SET] I2C Set Byte Failed,res = %d", res);
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}

int VENDOR_DRV_SMBUS_Get_Byte(int bus, uint8_t dev, uint16_t daddr, uint8_t alen, uint16_t *data, uint8_t dlen)
{
    char dev_path[20];
    int fd = -1;

    sprintf(dev_path, "/dev/i2c-%d", bus);

    /* Open the SMBus device */
    if ((fd = open(dev_path, O_RDWR | O_SYNC)) < 0)
    {
        AIM_LOG_ERROR("[GET] Can not open %s\n", dev_path);
        return -1;
    }

    /* set slave address */
    if (ioctl(fd, I2C_SLAVE, dev) < 0)
    {
        if (errno != EBUSY)
            AIM_LOG_ERROR("[GET] Can not set device address 0x%02X\n", dev);

        close(fd);
        return -1;
    }

    *data = VENDOR_DRV_I2C_SMBUS_Read_Byte(fd);
    close(fd);
    return 0;
}

int VENDOR_DRV_SMBUS_Set(int bus, uint8_t dev, uint16_t daddr, uint8_t alen, uint16_t data, uint8_t dlen)
{
    char dev_path[20];
    uint8_t upr_addr = 0, lwr_addr = 0;
    int fd = -1;
    int res = 0;

    sprintf(dev_path, "/dev/i2c-%d", bus);

    /* Open the SMBus device */
    if ((fd = open(dev_path, O_RDWR | O_SYNC)) < 0)
    {
        AIM_LOG_ERROR("[SET] Can not open %s\n", dev_path);
        return -1;
    }

    /* set slave address */
    if (ioctl(fd, I2C_SLAVE, dev) < 0)
    {
        if (errno != EBUSY)
        {
            AIM_LOG_ERROR("[SET] Can not set device address 0x%02X,shift=0\n", dev);
        }
        close(fd);
        return -1;
    }

    switch (alen)
    {
    case 2: //word address
        upr_addr = (daddr >> 8) & 0x00ff;
        lwr_addr = daddr & 0x00ff;
        break;
    }

    switch (dlen)
    {
    case 1: //byte
        res = VENDOR_DRV_I2C_SMBUS_Write_Byte_Data(fd, daddr, (uint8_t)data);
        if (alen == 2) //word address
        {
            res = VENDOR_DRV_I2C_SMBUS_Write_Word_Data(fd, upr_addr, data << 8 | lwr_addr);
        }
        break;
    case 2: //word
        res = VENDOR_DRV_I2C_SMBUS_Write_Word_Data(fd, daddr, (uint16_t)data);
        break;
    }

    if (res < 0)
    {
        AIM_LOG_ERROR("[SET] I2C Set Failed");
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

int VENDOR_DRV_SMBUS_Get(int bus, uint8_t dev, uint16_t daddr, uint8_t alen, uint16_t *data, uint8_t dlen)
{
    char dev_path[20];
    int fd = -1;
    int res = -1;

    sprintf(dev_path, "/dev/i2c-%d", bus);

    /* Open the SMBus device */
    if ((fd = open(dev_path, O_RDWR | O_SYNC)) < 0)
    {
        AIM_LOG_ERROR("[GET] Can not open %s\n", dev_path);
        return -1;
    }

    /* set slave address */
    if (ioctl(fd, I2C_SLAVE, dev) < 0)
    {
        if (errno != EBUSY)
            AIM_LOG_ERROR("[GET] Can not set device address 0x%02X\n", dev);

        close(fd);
        return -1;
    }

    switch (alen)
    {
        uint8_t upr_addr, lwr_addr;

    case 2: //word address
        upr_addr = (daddr >> 8) & 0x0ff;
        lwr_addr = daddr & 0x0ff;
        res = VENDOR_DRV_I2C_SMBUS_Write_Byte_Data(fd, upr_addr, lwr_addr);
        res = VENDOR_DRV_I2C_SMBUS_Read_Byte(fd);
        dlen = 0;
        break;
    }

    switch (dlen)
    {
    case 1: //byte
        res = VENDOR_DRV_I2C_SMBUS_Read_Byte_Data(fd, daddr);
        break;
    case 2: //word
        res = VENDOR_DRV_I2C_SMBUS_Read_Word_Data(fd, daddr);
        break;
    }

    if (res < 0)
    {
        AIM_LOG_ERROR("[GET] I2C Get Failed");
        close(fd);
        return -1;
    }
    else
        *data = (uint16_t)res;

    close(fd);
    return 0;
}

int VENDOR_DRV_SMBUS_Probe(uint8_t ucBus, uint8_t dev)
{
    int rv;
    rv = VENDOR_DRV_I2C_SMBUS_Probe(ucBus, dev);

    return rv;
}

int VENDOR_DRV_SMBUS_Write_Block(int bus, uint8_t dev, uint16_t daddr, uint8_t *value, uint16_t BufSize)
{
    char dev_path[20];
    int fd = -1;
    int res;

    sprintf(dev_path, "/dev/i2c-%d", bus);

    /* Open the SMBus device */
    if ((fd = open(dev_path, O_RDWR)) < 0)
    {
        AIM_LOG_ERROR("[BLOCK-WRITE] Can not open %s\n", dev_path);
        return -1;
    }

    /* set slave address */
    if (ioctl(fd, I2C_SLAVE, dev) < 0)
    {
        if (errno != EBUSY)
        {
            AIM_LOG_ERROR("[BLOCK-WRITE] Can not set device address 0x%02X", dev);
        }
        close(fd);
        return -1;
    }

    res = VENDOR_DRV_I2C_SMBUS_Write_Block_Data(fd, daddr, BufSize, value);
    usleep(30000);

    if (res < 0)
    {
        AIM_LOG_ERROR("[BLOCK-WRITE] I2C WRITE-BLOCK Failed");
        perror("perror");
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

int VENDOR_DRV_SMBUS_Read_Block(int bus, uint8_t dev, uint16_t daddr, uint8_t *ReplyBuf, uint16_t BufSize)
{
    char dev_path[20];
    int fd = -1;
    int res;

    unsigned long funcs;

    sprintf(dev_path, "/dev/i2c-%d", bus);

    /* Open the SMBus device */
    if ((fd = open(dev_path, O_RDWR)) < 0)
    {
        AIM_LOG_ERROR("[BLOCK-READ] Can not open %s\n", dev_path);
        return -1;
    }

    /* set slave address */
    if (ioctl(fd, I2C_SLAVE_FORCE, dev) < 0)
    {
        if (errno != EBUSY)
        {
            AIM_LOG_ERROR("[BLOCK-READ] Can not set device address 0x%02X", dev);
        }
        close(fd);
        return -1;
    }

#if 1
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0)
    {
        /* Some kind of error handling */
    }
    if (!(funcs & I2C_FUNC_SMBUS_READ_BLOCK_DATA))
    {
        /* Oops, the needed functionality (SMBus write_quick function) is not
       available! */
    }
#endif

    res = VENDOR_DRV_I2C_SMBUS_Read_Block_Data(fd, daddr, ReplyBuf);
    usleep(3000);

    if (res < 0)
    {
        AIM_LOG_ERROR("[BLOCK-READ] I2C READ-BLOCK Failed");
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

int VENDOR_DRV_SMBUS_Read_I2C_Block(int bus, uint8_t dev, uint16_t daddr, uint8_t *ReplyBuf, uint16_t BufSize)
{
    char dev_path[20];
    int fd = -1;
    int res;

    unsigned long funcs;
    sprintf(dev_path, "/dev/i2c-%d", bus);

    /* Open the SMBus device */
    if ((fd = open(dev_path, O_RDWR)) < 0)
    {
        AIM_LOG_ERROR("[I2C-BLOCK-READ] Can not open %s\n", dev_path);
        return -1;
    }
    /* set slave address */
    if (ioctl(fd, I2C_SLAVE, dev) < 0)
    {
        if (errno != EBUSY)
        {
            AIM_LOG_ERROR("[I2C-BLOCK-READ] Can not set device address 0x%02X", dev);
        }
        close(fd);
        return -1;
    }

#if 1
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0)
    {
        /* Some kind of error handling */
    }
    if (!(funcs & I2C_FUNC_SMBUS_READ_BLOCK_DATA))
    {
        /* Oops, the needed functionality (SMBus write_quick function) is not
       available! */
    }
#endif
    res = VENDOR_DRV_I2C_SMBUS_Read_I2C_Block_Data(fd, daddr, BufSize, ReplyBuf);

    if (res < 0)
    {
        AIM_LOG_ERROR("[I2C-BLOCK-READ] I2C I2C-BLOCK-READ Failed");
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

int VENDOR_DRV_SMBUS_Write_I2C_Block(int bus, uint8_t dev, uint16_t daddr, uint8_t *value, uint16_t BufSize)
{
    char dev_path[20];
    int fd = -1;
    int res = -1;

    sprintf(dev_path, "/dev/i2c-%d", bus);

    /* Open the SMBus device */
    if ((fd = open(dev_path, O_RDWR)) < 0)
    {
        AIM_LOG_ERROR("[I2C-BLOCK-WRITE] Can not open %s\n", dev_path);
        return -1;
    }

    /* set slave address */
    if (ioctl(fd, I2C_SLAVE, dev) < 0)
    {
        if (errno != EBUSY)
        {
            AIM_LOG_ERROR("[I2C-BLOCK-WRITE] Can not set device address 0x%02X", dev);
        }
        close(fd);
        return -1;
    }

    res = VENDOR_DRV_I2C_SMBUS_Write_I2C_Block_Data(fd, daddr, BufSize, value);
    usleep(30000);

    if (res < 0)
    {
        AIM_LOG_ERROR("[I2C-BLOCK-WRITE] I2C I2C-BLOCK-WRITE Failed");
        perror("perror");
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

int VENDOR_DRV_I2C_SMBUS_Access(int dev_fd, char read_write, uint8_t command, int size, union i2c_smbus_data *data)
{
    struct i2c_smbus_ioctl_data args;
    int res;

    args.read_write = read_write;
    args.command = command;
    args.size = size;
    args.data = data;

    res = ioctl(dev_fd, I2C_SMBUS, &args);

    return res;
}

int VENDOR_DRV_I2C_SMBUS_Write_Quick(int dev_fd, uint8_t value)
{
    return VENDOR_DRV_I2C_SMBUS_Access(dev_fd, value, 0, I2C_SMBUS_QUICK, NULL);
}

int VENDOR_DRV_I2C_SMBUS_Read_Byte(int dev_fd)
{
    union i2c_smbus_data data;

    if (VENDOR_DRV_I2C_SMBUS_Access(dev_fd, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data))
        return -1;
    else
        return 0x0FF & data.byte;
}

int VENDOR_DRV_I2C_SMBUS_Write_Byte(int dev_fd, uint8_t value)
{
    return VENDOR_DRV_I2C_SMBUS_Access(dev_fd, I2C_SMBUS_WRITE, value,
                                       I2C_SMBUS_BYTE, NULL);
}

int VENDOR_DRV_I2C_SMBUS_Read_Byte_Data(int dev_fd, uint8_t command)
{
    union i2c_smbus_data data;

    if (VENDOR_DRV_I2C_SMBUS_Access(dev_fd, I2C_SMBUS_READ, command, I2C_SMBUS_BYTE_DATA, &data))
        return -1;
    else
        return (0x0FF & data.byte);
}

int VENDOR_DRV_I2C_SMBUS_Write_Byte_Data(int dev_fd, uint8_t command, uint8_t value)
{
    union i2c_smbus_data data;

    data.byte = value;

    return VENDOR_DRV_I2C_SMBUS_Access(dev_fd, I2C_SMBUS_WRITE, command, I2C_SMBUS_BYTE_DATA, &data);
}

int VENDOR_DRV_I2C_SMBUS_Read_Word_Data(int dev_fd, uint8_t command)
{
    union i2c_smbus_data data;

    if (VENDOR_DRV_I2C_SMBUS_Access(dev_fd, I2C_SMBUS_READ, command, I2C_SMBUS_WORD_DATA, &data))
        return -1;
    else
        return (0xFFFF & data.word);
}

int VENDOR_DRV_I2C_SMBUS_Write_Word_Data(int dev_fd, uint8_t command, __u16 value)
{
    union i2c_smbus_data data;

    data.word = value;

    return VENDOR_DRV_I2C_SMBUS_Access(dev_fd, I2C_SMBUS_WRITE, command, I2C_SMBUS_WORD_DATA, &data);
}

int VENDOR_DRV_I2C_SMBUS_Process_Call(int dev_fd, uint8_t command, __u16 value)
{
    union i2c_smbus_data data;

    data.word = value;

    if (VENDOR_DRV_I2C_SMBUS_Access(dev_fd, I2C_SMBUS_WRITE, command, I2C_SMBUS_PROC_CALL, &data))
        return -1;
    else
        return (0xFFFF & data.word);
}

int VENDOR_DRV_I2C_SMBUS_Read_Block_Data(int dev_fd, uint8_t command, uint8_t *values)
{
    union i2c_smbus_data data;
    int i;

    if (VENDOR_DRV_I2C_SMBUS_Access(dev_fd, I2C_SMBUS_READ, command, I2C_SMBUS_BLOCK_DATA, &data))
        return -1;
    else
    {
        for (i = 1; i <= data.block[0]; i++)
            values[i - 1] = data.block[i];

        return data.block[0];
    }
}

int VENDOR_DRV_I2C_SMBUS_Write_Block_Data(int dev_fd, uint8_t command, uint8_t length, uint8_t *values)
{
    union i2c_smbus_data data;
    int i;

    if (length > I2C_SMBUS_BLOCK_MAX)
        length = I2C_SMBUS_BLOCK_MAX;

    for (i = 1; i <= length; i++)
        data.block[i] = values[i - 1];
    data.block[0] = length;

    return VENDOR_DRV_I2C_SMBUS_Access(dev_fd, I2C_SMBUS_WRITE, command, I2C_SMBUS_BLOCK_DATA, &data);
}

int VENDOR_DRV_I2C_SMBUS_Read_I2C_Block_Data(int file, uint8_t command,
                                             uint16_t length, uint8_t *values)
{
    union i2c_smbus_data data;
    int i = 0, j = 0;

    int count = length;

    while (count > 0)
    {
        int rsize = (count >= 32) ? 32 : count;
        data.block[0] = rsize;
        if (VENDOR_DRV_I2C_SMBUS_Access(file, I2C_SMBUS_READ, command + (i), I2C_SMBUS_I2C_BLOCK_DATA, &data))
            return -1;

        for (j = 1; j <= data.block[0]; i++, j++)
        {
            values[i] = data.block[j];
        }
        usleep(200);

        count -= rsize;
    }

    return 0;
}

int VENDOR_DRV_I2C_SMBUS_Write_I2C_Block_Data(int file, uint8_t command,
                                              uint8_t length,
                                              const uint8_t *values)
{
    union i2c_smbus_data data;
    int i;
    if (length > 32)
        length = 32;
    for (i = 1; i <= length; i++)
        data.block[i] = values[i - 1];
    data.block[0] = length;
    return VENDOR_DRV_I2C_SMBUS_Access(file, I2C_SMBUS_WRITE, command,
                                       I2C_SMBUS_I2C_BLOCK_BROKEN, &data);
}

int VENDOR_DRV_I2C_SMBUS_Block_Process_Call(int dev_fd, uint8_t command, uint8_t length, uint8_t *values)
{
    union i2c_smbus_data data;
    int i;

    if (length > I2C_SMBUS_BLOCK_MAX)
        length = I2C_SMBUS_BLOCK_MAX;

    for (i = 1; i <= length; i++)
        data.block[i] = values[i - 1];

    data.block[0] = length;

    if (VENDOR_DRV_I2C_SMBUS_Access(dev_fd, I2C_SMBUS_WRITE, command, I2C_SMBUS_BLOCK_PROC_CALL, &data))
    {
        AIM_LOG_ERROR("[BLOCK-PROCESS] VENDOR_DRV_I2C_SMBUS_Access failed, command:0x%02x\n", command);
        return -1;
    }
    else
    {
        for (i = 1; i <= data.block[0]; i++)
            values[i - 1] = data.block[i];

        return data.block[0];
    }
}

int VENDOR_DRV_I2C_SMBUS_Probe(uint8_t bus, uint8_t dev)
{
    char dev_path[20];
    int dev_fd = -1;
    int size = sizeof(dev_path);
    dev_path[size - 1] = '\0';

    sprintf(dev_path, "/dev/i2c-%d", bus);
    /* open the I2c device */
    if ((dev_fd = open(dev_path, O_RDWR | O_SYNC)) < 0)
    {
        AIM_LOG_ERROR("[PROBE] Can not open %s\n", dev_path);
        return -1;
    }

    /* set slave address */
    if (ioctl(dev_fd, I2C_SLAVE, dev) < 0)
    {
        if (errno != EBUSY)
        {
            AIM_LOG_ERROR("[PROBE] Can not set device address 0x%02X,shift=0\n", dev);
        }
        close(dev_fd);
        return -1;
    }

    if ((dev >= 0x30 && dev <= 0x37) || (dev >= 0x50 && dev <= 0x57))
    {
        if (VENDOR_DRV_I2C_SMBUS_Read_Byte(dev_fd) < 0)
        {
            return -1;
        }
    }
    else
    {
        if (VENDOR_DRV_I2C_SMBUS_Write_Quick(dev_fd, I2C_SMBUS_WRITE) < 0)
        {
            close(dev_fd);
            return -1;
        }
    }
    close(dev_fd);
    return 0;
}

static int ipmb_readb(int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint16_t *data, uint8_t dlen)
{
    int rv = 0, idx = 0;
    char ipmi_cmd[80], rv_char[256], *delim = " ", *tmp;
    uint16_t rv_data[32] = {0};

    sprintf(ipmi_cmd, "ipmitool raw 0x3c 0x01 %d %d %d %d", bus, dev, addr, dlen);
    if (vendor_system_call_get(ipmi_cmd, rv_char) != 0)
    {
        AIM_LOG_ERROR("IPMITOOL command: \"%s\": Get Data Failed (ret: %d)", ipmi_cmd);
        return ONLP_STATUS_E_INTERNAL;
    }

    tmp = strtok(rv_char, delim);

    while (tmp != NULL)
    {
        rv_data[idx] = strtol(tmp, NULL, 16);
        tmp = strtok(NULL, delim);
        idx++;
    }

    switch (dlen)
    {
    case 1:
        *data = rv_data[0] & 0xff;
        break;
    case 2:
        *data = (rv_data[0] & 0xff) + ((rv_data[1] & 0xff) << 8);
        break;
    default:
        AIM_LOG_ERROR("[INTERNAL-IPMB][GET] Dlan limitation is 1 to 2");
        break;
    }
    /*
    AIM_LOG_ERROR("IPMITOOL readb: bus: %02x, dev: %02x, addr: %02x, value: %x",
                bus, dev, addr, rv);
    */
    return rv;
}

static int ipmb_writeb(int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint16_t data, uint8_t dlen)
{
    int rv = 0;
    char ipmi_cmd[80];

    sprintf(ipmi_cmd, "ipmitool raw 0x3c 0x02 %d %d %d %d %d > /dev/null", bus, dev, addr, dlen, data);
    if (vendor_system_call_set(ipmi_cmd) != 0)
    {
        AIM_LOG_ERROR("IPMITOOL command: \"%s\": Set Data Failed.", ipmi_cmd);
        return ONLP_STATUS_E_INTERNAL;
    }
    /*else
    {
        AIM_LOG_ERROR("IPMITOOL writeb: bus: %02x, dev: %02x, addr: %02x, data: %02x",
                bus, dev, addr, byte);
    }*/

    return rv;
}

static int ipmb_block_read(int bus, uint8_t dev, uint16_t addr, uint8_t *rdata, uint16_t size)
{
    int rv = 0, idx = 0;
    char ipmi_cmd[80], rv_char[256], *delim = " ", *tmp;
    uint8_t rv_data[32] = {0};

    if (size > 64)
    {
        AIM_LOG_ERROR("The limitation of size is 64.");
        return ONLP_STATUS_E_INTERNAL;
    }

    sprintf(ipmi_cmd, "ipmitool raw 0x3c 0x01 %d %d %d %d", bus, dev, addr, size + 1);
    if (vendor_system_call_get(ipmi_cmd, rv_char) != 0)
    {
        AIM_LOG_ERROR("IPMITOOL command: \"%s\": Block read Failed.", ipmi_cmd);
        return ONLP_STATUS_E_INTERNAL;
    }

    tmp = strtok(rv_char, delim);

    while (tmp != NULL)
    {
        rv_data[idx] = strtol(tmp, NULL, 16);
        tmp = strtok(NULL, delim);
        idx++;
    }

    for (idx = 0; idx < size; idx++)
        rdata[idx] = rv_data[idx + 1] & 0xff;

    return rv;
}

static i2c_bus_driver_t ipmb_functions = {
    ipmb_readb,
    ipmb_writeb,
    ipmb_readb,
    ipmb_writeb,
    ipmb_block_read};

static int ipmb_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "IPMB", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &ipmb_functions;

    return vendor_driver_add(driver);
}

#ifdef CYPRESS
// CYPRESS I2C DRIVER START
static int cypressRealBus = 0;
static int cypressBusIsGood = 0;

static uint8_t cypress_find_real_bus(int bus)
{
    if (cypressBusIsGood)
        return 0;

    CY_DEVICE_INFO deviceInfo;
    CY_RETURN_STATUS rStatus;
    uint8_t dev = 0, numDevices = 0;

    CyGetListofDevices(&numDevices);

    for (dev = 0; dev < numDevices; dev++)
    {
        rStatus = CyGetDeviceInfo(dev, &deviceInfo);

        if (!rStatus && deviceInfo.vidPid.pid == bus)
        {
            cypressRealBus = dev;
            cypressBusIsGood = 1;
            return 0;
        }
    }

    AIM_LOG_ERROR("DRIVER: CYPRESS Cannot find bus.");
    return 1;
}

CY_RETURN_STATUS CyOpen_retry (unsigned char deviceNumber, unsigned char interfaceNum, CY_HANDLE *handle)
{
    int s32_loop = 0;
    CY_RETURN_STATUS rStatus;

    for (s32_loop = 0; s32_loop < 500; s32_loop++)
    {
        rStatus = CyOpen(deviceNumber, interfaceNum, handle);
        if (rStatus == CY_SUCCESS)
        {
            break;
        }

        usleep(10*1000);
    }

    if (rStatus != CY_SUCCESS)
    {
        AIM_LOG_ERROR("%s:%d: Error: retry CyOpen(): %d times = [%d].\n", __func__, __LINE__, s32_loop, rStatus);
    }

    return (rStatus);
}

static int cypress_I2C_master_write(
    int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint8_t *lpu8_data, uint16_t dlen)
{
    cypress_find_real_bus(bus);
    CY_HANDLE handle;
    CY_DATA_BUFFER dataBufferWrite;
    unsigned char wbuffer[256 + 4];
    CY_RETURN_STATUS rStatus;
    CY_I2C_DATA_CONFIG i2cDataConfig;
    uint8_t devNum = cypressRealBus;
    uint16_t u16_data_len = ((uint16_t)dlen) + ((uint16_t)alen);

    if (dlen > 256)
    {
        AIM_LOG_ERROR("%s:%d: Error: Address[0x%X], offset[0x%X], alen[%u], invalid size[0x%X].\n", 
               __func__, __LINE__, dev, addr, alen, dlen);
	return (CY_ERROR_INVALID_PARAMETER);
    }

    i2cDataConfig.isNakBit = true;
    i2cDataConfig.isStopBit = true;
    i2cDataConfig.slaveAddress = dev;

    memset(wbuffer, 0x00, sizeof(wbuffer));
    dataBufferWrite.buffer = wbuffer;

    if (2 == alen)
    {
        wbuffer[0] = (uint8_t) (addr >> 8);
        wbuffer[1] = (uint8_t) (addr & 0x00FF);
        memcpy(&(wbuffer[2]), lpu8_data, dlen);
        dataBufferWrite.length = u16_data_len;
    }
    else if (1 == alen)
    {
        wbuffer[0] = (uint8_t) (addr);
        memcpy(&(wbuffer[1]), lpu8_data, dlen);
        dataBufferWrite.length = u16_data_len;
    }
    else if (0 == alen)
    {
        memcpy(&(wbuffer[0]), lpu8_data, dlen);
        dataBufferWrite.length = u16_data_len;
    }
    else
    {
        AIM_LOG_ERROR("%s:%d: Error: Address[0x%X], offset[0x%X], invalid alen[%u], size[0x%X].\n", 
               __func__, __LINE__, dev, addr, alen, dlen);
	return (CY_ERROR_INVALID_PARAMETER);
    }


    rStatus = CyOpen_retry(devNum, 0, &handle);
    if (rStatus != CY_SUCCESS)
    {
        AIM_LOG_ERROR("%s:%d: DRIVER: Device Open Failed[%d].\n", __func__, __LINE__, rStatus);
        return CY_ERROR_DRIVER_OPEN_FAILED;
    }

    //AIM_LOG_ERROR("%s:%d: I2C address[0x%X], Wdata[0x%X 0x%X], Wl[%d].\n", __func__, __LINE__, i2cDataConfig.slaveAddress, wbuffer[0], wbuffer[1], dataBufferWrite.length);
    rStatus = CyI2cWrite(handle, &i2cDataConfig, &dataBufferWrite, 5000);
    CyClose(handle);

    if (rStatus != CY_SUCCESS)
    {
        AIM_LOG_ERROR("%s:%d: Error[%d]: Failed to write: address[0x%X], offset[0x%X], alen[%u], dlen[%u].\n", 
                      __func__, __LINE__, rStatus, dev, addr, alen, dlen);
        return CY_ERROR_REQUEST_FAILED;
    }

    if (dataBufferWrite.length != dataBufferWrite.transferCount)
    {
        AIM_LOG_ERROR("%s:%d: Error[%d]: Failed to write: address[0x%X], offset[0x%X], alen[%u], dlen[%u]: expected[%u], written[%u].\n", 
                      __func__, __LINE__, rStatus, dev, addr, alen, dlen, 
                      dataBufferWrite.length, dataBufferWrite.transferCount);
        return CY_ERROR_REQUEST_FAILED;
    }

    return CY_SUCCESS;
}

static int cypress_I2C_write_read(
    int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint8_t *lpu8_data, uint16_t dlen)
{
    cypress_find_real_bus(bus);
    CY_HANDLE handle;
    CY_DATA_BUFFER dataBufferRead;
    CY_DATA_BUFFER dataBufferWrite;
    unsigned char wbuffer[3] = {0, 0, 0};
    CY_RETURN_STATUS rStatus;
    CY_I2C_DATA_CONFIG i2cDataConfig;
    uint8_t devNum = cypressRealBus;

    if (dlen > 256)
    {
        AIM_LOG_ERROR("%s:%d: Error: Address[0x%X], offset[0x%X], alen[%u], invalid size[0x%X].\n", 
               __func__, __LINE__, dev, addr, alen, dlen);
	return (CY_ERROR_INVALID_PARAMETER);
    }

 
    i2cDataConfig.isNakBit = true;
    i2cDataConfig.isStopBit = true;
    i2cDataConfig.slaveAddress = dev;

    dataBufferWrite.buffer = wbuffer;

    if (0 != alen)
    {
        if (2 == alen)
        {
            wbuffer[0] = (uint8_t) (addr >> 8);
            wbuffer[1] = (uint8_t) (addr & 0x00FF);
            dataBufferWrite.length = 2;
        }
        else if (1 == alen)
        {
            wbuffer[0] = (uint8_t)addr;
            dataBufferWrite.length = 1;
        }
        else
        {
            AIM_LOG_ERROR("%s:%d: Error: Address[0x%X], offset[0x%X], invalid alen[%u], size[0x%X].\n", 
                          __func__, __LINE__, dev, addr, alen, dlen);
	    return (CY_ERROR_INVALID_PARAMETER);
        }
    }


    rStatus = CyOpen_retry(devNum, 0, &handle);
    if (rStatus != CY_SUCCESS)
    {
        AIM_LOG_ERROR("%s:%d: DRIVER: Device Open Failed[%d].\n", __func__, __LINE__, rStatus);
        return CY_ERROR_DRIVER_OPEN_FAILED;
    }

    if (0 != alen)
    {
        //AIM_LOG_ERROR("%s:%d: I2C address[0x%X], Wdata[0x%X 0x%X], Wl[%d].\n", __func__, __LINE__, i2cDataConfig.slaveAddress, wbuffer[0], wbuffer[1], dataBufferWrite.length);
        rStatus = CyI2cWrite(handle, &i2cDataConfig, &dataBufferWrite, 5000);
        if (rStatus != CY_SUCCESS)
        {
            CyClose(handle);

            AIM_LOG_ERROR("%s:%d: Error[%d]: Failed to write: address[0x%X], offset[0x%X], alen[%u], dlen[%u].\n",
                          __func__, __LINE__, rStatus, dev, addr, alen, dlen);
            return CY_ERROR_REQUEST_FAILED;
        }

        if (dataBufferWrite.length != dataBufferWrite.transferCount)
        {
            CyClose(handle);

            AIM_LOG_ERROR("%s:%d: Error[%d]: Failed to write: address[0x%X], offset[0x%X], alen[%u], dlen[%u]: expected[%u], written[%u].\n", 
                      __func__, __LINE__, rStatus, dev, addr, alen, dlen, 
                      dataBufferWrite.length, dataBufferWrite.transferCount);
            return CY_ERROR_REQUEST_FAILED;
        }
    }

    if (0 != dlen)
    {
        dataBufferRead.buffer = lpu8_data;
        dataBufferRead.buffer[0] = addr;
        dataBufferRead.length = dlen;

        rStatus = CyI2cRead(handle, &i2cDataConfig, &dataBufferRead, 5000);
        if (rStatus != CY_SUCCESS)
        {
            CyClose(handle);

            AIM_LOG_ERROR("%s:%d: Error[%d]: Failed to read: address[0x%X], offset[0x%X], alen[%u], dlen[%u].\n",
                          __func__, __LINE__, rStatus, dev, addr, alen, dlen);
            return CY_ERROR_REQUEST_FAILED;
        }

        if (dataBufferRead.length != dataBufferRead.transferCount)
        {
            CyClose(handle);

            AIM_LOG_ERROR("%s:%d: Error[%d]: Failed to read: address[0x%X], offset[0x%X], alen[%u], dlen[%u]: expected[%u], written[%u].\n", 
                      __func__, __LINE__, rStatus, dev, addr, alen, dlen, 
                      dataBufferRead.length, dataBufferRead.transferCount);
            return CY_ERROR_REQUEST_FAILED;
        }
    }

    CyClose(handle);

    return CY_SUCCESS;
}

static int cypress_i2c_get(
    int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint16_t *data, uint8_t dlen)
{
    return (cypress_I2C_write_read(bus, dev, addr, alen, (uint8_t *)data, dlen));
}

static int cypress_i2c_set(
    int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint16_t data, uint8_t dlen)
{
    return (cypress_I2C_master_write(bus, dev, addr, alen, (uint8_t *)&data, dlen));
}

static int cypress_i2c_set_by_byte(
    int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint16_t data, uint8_t dlen)
{
    return (cypress_I2C_master_write(bus, dev, addr, alen, (uint8_t *)&data, dlen));
}

static int cypress_i2c_get_by_byte(
    int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint16_t *data, uint8_t dlen)
{
    return (cypress_I2C_write_read(bus, dev, addr, alen, (uint8_t *)data, dlen));
}

static int cypress_i2c_set_by_block_write(
    int bus, uint8_t dev, uint16_t addr, uint8_t *value, uint16_t bufSize)
{
    AIM_LOG_ERROR("DRIVER: cypress_i2c_set_by_block_write NOT SUPPORT\n");
    return -1;
}

static int cypress_i2c_get_by_block_read(
    int bus, uint8_t dev, uint16_t addr, uint8_t *replyBuf, uint16_t bufSize)
{
    uint8_t alen = (dev == 0x56) ? 2 : 1;
    return (cypress_I2C_write_read(bus, dev, addr, alen, replyBuf, bufSize));
}

static i2c_bus_driver_t cypress_i2c_functions = {
    cypress_i2c_get,
    cypress_i2c_set,
    cypress_i2c_get_by_byte,
    cypress_i2c_set_by_byte,
    cypress_i2c_get_by_block_read,
    cypress_i2c_set_by_block_write,
    cypress_i2c_get_by_block_read,
    cypress_i2c_set_by_block_write};

static int cypress_i2c_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "CYPRESSI2C", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &cypress_i2c_functions;

    return vendor_driver_add(driver);
}
// CYPRESS I2C DRIVER END
#endif

/*
    I2C BUS DRIVER END:
*/

/*
    IPMI BUS DRIVER START:
*/
static int ipmi_get(int bus, char *cmd, char *filter, char *data, uint32_t dlen)
{
    char sys_cmd[128];
    char rv_char[256];

    if (bus)
    {
    }

    if (!cmd || !filter)
        return 0;

    sprintf(sys_cmd, "ipmitool %s %s", cmd, filter);
    if (vendor_system_call_get(sys_cmd, rv_char) != 0)
        return 0;

    memcpy(data, rv_char, (dlen - 1));
    data[dlen - 1] = '\0';

    return 0;
}

static int ipmi_set(int bus, char *cmd, char *filter)
{
    char sys_cmd[128];

    if (bus)
    {
    }

    if (!cmd || !filter)
        return 0;

    sprintf(sys_cmd, "ipmitool %s %s > /dev/null", cmd, filter);

    return vendor_system_call_set(sys_cmd);
}

ipmi_bus_driver_t ipmi_functions = {
    ipmi_get,
    ipmi_set};

static int ipmi_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "IPMI", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &ipmi_functions;

    return vendor_driver_add(driver);
}

/*
    IPMI BUS DRIVER END:
*/

/*
    DEVICE DRIVER START:
*/

/* CPLD DEVICE START*/
static int cpld_read(void *busDrvPtr, int bus, uint8_t dev, uint8_t addr, uint8_t *value)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;
    uint16_t data;

    rv = i2c->get(bus, dev, addr, 1, &data, 1);
    if (rv < 0)
    {
        return ONLP_STATUS_E_INTERNAL;
    }
    *value = (uint8_t)data;

    return rv;
}

static int cpld_write(void *busDrvPtr, int bus, uint8_t dev, uint8_t addr, uint8_t value)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    rv = i2c->set(bus, dev, addr, 1, value, 1);

    return rv;
}

static cpld_dev_driver_t cpld_functions = {
    cpld_read,
    cpld_write};

static int cpld_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "CPLD", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &cpld_functions;

    return vendor_driver_add(driver);
}

/* CPLD DEVICE END*/

/* EEPROM DEVICE START*/
static int eeprom_readb(
    void *busDrvPtr, int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint8_t *buf, uint16_t len)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0, index = 0;
    uint16_t data = 0;

    for (index = 0; index < len; index++)
    {
        rv = i2c->get(bus, dev, addr + index, alen, &data, 1);
        if (rv < 0)
        {
            return ONLP_STATUS_E_INTERNAL;
        }
        *(buf + index) = (uint8_t)(data & 0xff);
        data = 0;
    }

    return 0;
}

static int eeprom_writeb(
    void *busDrvPtr, int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint8_t *buf, uint16_t len)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0, index = 0;

    for (index = 0; index < len; index++)
    {
        rv = i2c->set(bus, dev, addr + index, alen, *(buf + index), 1);
        usleep(5000);
        if (rv < 0)
        {
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    return 0;
}

static int eeprom_readw(
    void *busDrvPtr, int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint16_t *buf, uint16_t len)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0, index = 0;

    for (index = 0; index < len; index++)
    {
        rv = i2c->get(bus, dev, addr + index, alen, (buf + index), 2);
        if (rv < 0)
        {
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    return 0;
}

static int eeprom_writew(
    void *busDrvPtr, int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint16_t *buf, uint16_t len)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0, index = 0;

    for (index = 0; index < len; index++)
    {
        rv = i2c->set(bus, dev, addr + index, alen, *(buf + index), 2);
        usleep(5000);
        if (rv < 0)
        {
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    return 0;
}


#define M_ONIE_EEPROM_I2C_ADDRESS  0x56

#define M_ONIE_DATA_SIZE  (eeprom_dev_data_list[0].len)

#define M_ONIE_FILE_NAME  "/run/ONIE_EEPROM_file.bin"

static int API_ONIE_file_is_valid(void)
{
    int s32_return = 0;
    struct stat st_stat;

    s32_return = stat(M_ONIE_FILE_NAME, &st_stat);
    if (0 != s32_return)
    {
        if (ENOENT != errno)
        {
            AIM_LOG_ERROR("%s: Error[%d]: failed to get status of file[%s].\n", __func__, errno, M_ONIE_FILE_NAME);
        }
        return (-1);
    }
    else if (M_ONIE_DATA_SIZE != st_stat.st_size)
    {
        //AIM_LOG_ERROR("%s: Error: file[%s] size[%ld] != %u.\n", __func__, M_ONIE_FILE_NAME, st_stat.st_size, M_ONIE_DATA_SIZE);
        return (-2);
    }
    else
    {
        return (0);
    }
}


static int API_ONIE_file_delete(void)
{
    int status = remove(M_ONIE_FILE_NAME);
    
    if (0 != status)
    {
        AIM_LOG_ERROR("%s: Error[%d]: failed to delete the file[%s].\n", __func__, errno, M_ONIE_FILE_NAME);
        return (-1);
    }

    return (0);
}


static int API_ONIE_file_read_all(uint8_t *lpsz_data, uint16_t u16_size)
{
    FILE *fp = NULL;
    size_t s32_len = 0;

    fp = fopen(M_ONIE_FILE_NAME, "rb");
    if (NULL == fp)
    {
        AIM_LOG_ERROR("%s: Error: failed to open file [%s].\n", __func__, M_ONIE_FILE_NAME);
        return (-1);
    }

    s32_len = fread(lpsz_data, 1, u16_size , fp);

    fclose(fp);

    if (u16_size != s32_len)
    {
        AIM_LOG_ERROR("%s:%d: Error: file[%s]: to read[%u], bytes read[%lu].\n", __func__, __LINE__, M_ONIE_FILE_NAME, u16_size, s32_len);
        return (-2);
    }
    else
    {
        return (0);
    }
}


static int API_ONIE_file_write_all(uint8_t *lpsz_data, uint16_t u16_size)
{
    FILE *fp = NULL;
    size_t s32_len = 0;

    fp = fopen(M_ONIE_FILE_NAME, "wb");
    if (NULL == fp)
    {
        AIM_LOG_ERROR("%s: Error: failed to open file [%s].\n", __func__, M_ONIE_FILE_NAME);
        return (-1);
    }

    s32_len = fwrite(lpsz_data, 1, u16_size , fp);

    fclose(fp);

    if (u16_size != s32_len)
    {
        AIM_LOG_ERROR("%s:%d: Error: file[%s]: to write[%u], bytes writen[%lu].\n", __func__, __LINE__, M_ONIE_FILE_NAME, u16_size, s32_len);
        return (-2);
    }
    else
    {
        return (0);
    }
}

static int eeprom_load(
    void *busDrvPtr, int bus, uint8_t dev, uint16_t start_addr, uint8_t alen, uint8_t *buf)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0, index = 0;
    uint16_t data;

#if 1  /* For the ONIE file feature. */
    if (M_ONIE_EEPROM_I2C_ADDRESS == dev)
    {
        if (0 == API_ONIE_file_is_valid())
        {
            return (API_ONIE_file_read_all(buf, M_ONIE_DATA_SIZE));
        }
        else
        {
            for (index = 0; index < M_ONIE_DATA_SIZE; index++)
            {
                rv = i2c->get(bus, dev, start_addr + index, alen, &data, 1);
                if (rv)
                {
                    AIM_LOG_ERROR("%s: Error: failed to read ONIE EEPROM: offset[%d]\n", __func__, start_addr + index);
                    break;
                }

                *(buf + index) = (uint8_t)data;
            }

            if (0 == rv)
            {
                if (0 != API_ONIE_file_write_all(buf, M_ONIE_DATA_SIZE))
                {
                    API_ONIE_file_delete();
                }
                return (0);
            }
            else
            {
                return (-1);
            }
        }
    }
#endif

    if (alen == 1)
    {
        i2c->i2c_block_read(bus, dev, start_addr, buf, 256);
    }
    else
    {
        for (index = 0; index < 256; index++)
        {
            //AIM_LOG_ERROR("BLOCK READ(%d) : 0x%x", index, *(buf + index));
            rv = i2c->get(bus, dev, start_addr + index, alen, &data, 1);
            *(buf + index) = (uint8_t)data;
            //AIM_LOG_ERROR("READ      (%d) : 0x%x", index, *(buf + index));
        }
    }

    return rv;
}

static eeprom_dev_driver_t eeprom_functions = {
    eeprom_readb,
    eeprom_writeb,
    eeprom_readw,
    eeprom_writew,
    eeprom_load,
};

static int eeprom_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "EEPROM", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &eeprom_functions;

    return vendor_driver_add(driver);
}
/* EEPROM DEVICE END*/

/* FAN DEVICE EMC2305 START*/
static const uint8_t emc2305_fan_config1_reg[] = {
    0x32, 0x42, 0x52, 0x62, 0x72};

static const uint8_t emc2305_fan_tach_reg[][2] = {
    {0x3e, 0x3f},
    {0x4e, 0x4f},
    {0x5e, 0x5f},
    {0x6e, 0x6f},
    {0x7e, 0x7f},
};

static const uint8_t emc2305_fan_tach_target_reg[][2] = {
    {0x3d, 0x3c},
    {0x4d, 0x4c},
    {0x5d, 0x5c},
    {0x6d, 0x6c},
    {0x7d, 0x7c},
};

static int emc2305_rpm_get(void *busDrvPtr, int bus, uint8_t dev, int id, int *rpm)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;
    uint16_t tach_count = 0, htach = 0, ltach = 0;

    rv = i2c->get(bus, dev, emc2305_fan_tach_reg[id][0], 1, &htach, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    rv = i2c->get(bus, dev, emc2305_fan_tach_reg[id][1], 1, &ltach, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    tach_count = (((htach << 8) | ltach) >> 0x3);

    if (tach_count == 0)
    {
        *rpm = 0;
        return 0;
    }

    *rpm = 7864320 / tach_count;

    return 0;
}

static int emc2305_rpm_set(void *busDrvPtr, int bus, uint8_t dev, int id, int rpm)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;
    uint16_t tach_count = 0, htach = 0, ltach = 0;

    tach_count = ((7864320 / rpm) << 0x3);
    htach = (tach_count & 0xff00) >> 8;
    ltach = tach_count & 0xff;

    rv = i2c->set(bus, dev, emc2305_fan_tach_target_reg[id][0], 1, htach, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    rv = i2c->set(bus, dev, emc2305_fan_tach_target_reg[id][1], 1, ltach, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    /* Set to RPM Mode */
    rv = i2c->set(bus, dev, emc2305_fan_config1_reg[id], 1, 0xab, 1);
    return 0;
}

static fan_dev_driver_t emc2305_functions = {
    emc2305_rpm_get,
    emc2305_rpm_set};

static int emc2305_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "EMC2305", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &emc2305_functions;

    return vendor_driver_add(driver);
}
/* FAN DEVICE EMC2305 END*/

/* FAN DEVICE PFM0812 START*/
/** Max number of fans */
#define PFM0812_MAX_FANS 4

typedef enum
{
    PFM0812_FAN_TACH_TARG_REG = 0x0A,
    PFM0812_FAN0_TACH_REG = 0x0B,
    PFM0812_FAN1_TACH_REG = 0x0C,
    PFM0812_FAN2_TACH_REG = 0x0D,
    PFM0812_FAN3_TACH_REG = 0x0E,

} pfm0812_registers;

/* Fan Tach reading register for individual fans */
static const uint8_t pfm0812_fan_tach_reg[4] = {
    PFM0812_FAN0_TACH_REG,
    PFM0812_FAN1_TACH_REG,
    PFM0812_FAN2_TACH_REG,
    PFM0812_FAN3_TACH_REG,
};

static const uint8_t pfm0812_fan_tach_target_reg = PFM0812_FAN_TACH_TARG_REG;

static int pfm0812_rpm_get(void *busDrvPtr, int bus, uint8_t dev, int id, int *rpm)
{
    int rv = 0;
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    uint16_t tach_count = 0;
    uint16_t htach = 0;

    rv = i2c->get(bus, dev, pfm0812_fan_tach_reg[id], 1, &htach, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    tach_count = htach;

    /* if tach_count = 0, system will exception.*/
    if (tach_count == 0)
    {
        *rpm = 0;
        return 0;
    }

    *rpm = 60 / (0.0000392 * (float)tach_count * 2);

    return rv;
}

static int pfm0812_rpm_set(void *busDrvPtr, int bus, uint8_t dev, int id, int rpm)
{
    int rv = 0;
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    uint16_t tach_count = 0;
    uint8_t ltach = 0;

    if (rpm < 3000)
        rpm = 3000;

    tach_count = (rpm - 3000) / 52;
    ltach = tach_count & 0x00ff;

    rv = i2c->set(bus, dev, pfm0812_fan_tach_target_reg, 1, ltach, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    return rv;
}

static fan_dev_driver_t pfm0812_functions = {
    pfm0812_rpm_get,
    pfm0812_rpm_set};

static int pfm0812_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "PFM0812", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &pfm0812_functions;

    return vendor_driver_add(driver);
}

/* FAN DEVICE PFM0812 END*/

/*THERMAL DEVICE TMP75 START*/

#define TMP75_TEMP_REG 0x00
#define TMP75_CONFIG_REG 0x01
#define TMP75_TLOW_REG 0x02
#define TMP75_THIGH_REG 0x03

static int tmp75_temp_get(void *busDrvPtr, int bus, uint8_t dev, int id, int *temperature)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0, is_negative = 0, data = 0;

    rv = i2c->get(bus, dev, TMP75_TEMP_REG, 1, (uint16_t *)&data, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    if (data & 0x800)
    {
        is_negative = 1;
        data = ~(data) + 1; /* 2's complement */
        data &= 0x7ff;
    }

    data = (((data << 8) | (data >> 8)) >> 4);
    *temperature = data * 62.5;
    if (is_negative)
        *temperature *= -1;

    return 0;
}

static int tmp75_limit_get(void *busDrvPtr, int bus, uint8_t dev, int id, int type, int *temperature)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0, data = 0;
    uint8_t limitAddr = 0;

    switch (type)
    {
    case VENDOR_THERMAL_LOW_THRESHOLD:
        limitAddr = TMP75_TLOW_REG;
        break;
    case VENDOR_THERMAL_HIGH_THRESHOLD:
        limitAddr = TMP75_THIGH_REG;
        break;
    default:
        AIM_LOG_ERROR("VENDOR: TMP75 unknow limit type!");
        break;
    }

    rv = i2c->get(bus, dev, limitAddr, 1, (uint16_t *)&data, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    *temperature = data * 1000;

    return 0;
}

static int tmp75_limit_set(void *busDrvPtr, int bus, uint8_t dev, int id, int type, int temperature)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;
    uint8_t limitAddr = 0;

    switch (type)
    {
    case VENDOR_THERMAL_LOW_THRESHOLD:
        limitAddr = TMP75_TLOW_REG;
        break;
    case VENDOR_THERMAL_HIGH_THRESHOLD:
        limitAddr = TMP75_THIGH_REG;
        break;
    default:
        AIM_LOG_ERROR("VENDOR: TMP75 unknow limit type!");
        break;
    }

    rv = i2c->set(bus, dev, limitAddr, 1, (uint16_t)temperature, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    return 0;
}

static thermal_dev_driver_t tmp75_functions = {
    tmp75_temp_get,
    tmp75_limit_get,
    tmp75_limit_set};

static int tmp75_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "TMP75", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &tmp75_functions;

    return vendor_driver_add(driver);
}
/*THERMAL DEVICE TMP75 END*/

/*THERMAL DEVICE TMP461 START*/

#define TMP461_HIGH_ADDRESS 0
#define TMP461_LOW_ADDRESS 1

static const uint8_t tmp461_thermal_reg[2] = {0x1, 0x10};
static const uint8_t tmp461_limit_reg_read_high[2] = {0x7, 0x8};
static const uint8_t tmp461_limit_reg_read_low[2] = {0x13, 0x14};
static const uint8_t tmp461_limit_reg_write_high[2] = {0xd, 0xe};
static const uint8_t tmp461_limit_reg_write_low[2] = {0x13, 0x14};

static int tmp461_temp_get(void *busDrvPtr, int bus, uint8_t dev, int id, int *temperature)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;
    uint16_t temp_high = 0, temp_low = 0, is_negative = 0;
    rv = i2c->get(bus, dev, tmp461_thermal_reg[TMP461_HIGH_ADDRESS], 1, &temp_high, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    rv = i2c->get(bus, dev, tmp461_thermal_reg[TMP461_LOW_ADDRESS], 1, &temp_low, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    if (temp_high & 0x80)
    {
        is_negative = 1;
        temp_high = ~temp_high + 1; /* 2's complement */
        temp_high &= 0xff;
    }

    *temperature = temp_high * 1000 + (temp_low >> 4) * 62.5;
    if (is_negative)
        *temperature *= -1;

    return 0;
}

static int tmp461_limit_get(void *busDrvPtr, int bus, uint8_t dev, int id, int type, int *temperature)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;
    uint8_t limitAddr_high = 0, limitAddr_low = 0;
    uint16_t temp_high = 0, temp_low = 0, is_negative = 0;

    switch (type)
    {
    case VENDOR_THERMAL_LOW_THRESHOLD:
        limitAddr_high = tmp461_limit_reg_read_high[TMP461_LOW_ADDRESS];
        limitAddr_low = tmp461_limit_reg_read_low[TMP461_LOW_ADDRESS];
        break;
    case VENDOR_THERMAL_HIGH_THRESHOLD:
        limitAddr_high = tmp461_limit_reg_read_high[TMP461_HIGH_ADDRESS];
        limitAddr_low = tmp461_limit_reg_read_low[TMP461_HIGH_ADDRESS];
        break;
    default:
        AIM_LOG_ERROR("VENDOR: TMP461 unknow limit type!");
        break;
    }

    rv = i2c->get(bus, dev, limitAddr_high, 1, &temp_high, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    rv = i2c->get(bus, dev, limitAddr_low, 1, &temp_low, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    if (temp_high & 0x80)
    {
        is_negative = 1;
        temp_high = ~temp_high + 1; /* 2's complement */
        temp_high &= 0xff;
    }

    *temperature = temp_high * 1000 + (temp_low >> 4) * 62.5;
    if (is_negative)
        *temperature *= -1;

    return 0;
}

static int tmp461_limit_set(void *busDrvPtr, int bus, uint8_t dev, int id, int type, int temperature)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;
    uint8_t limitAddr_high = 0, limitAddr_low = 0;

    switch (type)
    {
    case VENDOR_THERMAL_LOW_THRESHOLD:
        limitAddr_high = tmp461_limit_reg_write_high[TMP461_LOW_ADDRESS];
        limitAddr_low = tmp461_limit_reg_write_low[TMP461_LOW_ADDRESS];
        break;
    case VENDOR_THERMAL_HIGH_THRESHOLD:
        limitAddr_high = tmp461_limit_reg_write_high[TMP461_HIGH_ADDRESS];
        limitAddr_low = tmp461_limit_reg_write_low[TMP461_HIGH_ADDRESS];
        break;
    default:
        AIM_LOG_ERROR("VENDOR: TMP461 unknow limit type!");
        break;
    }

    rv = i2c->set(bus, dev, limitAddr_high, 1, (uint8_t)temperature, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    rv = i2c->set(bus, dev, limitAddr_low, 1, 0, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    return 0;
}

static thermal_dev_driver_t tmp461_functions = {
    tmp461_temp_get,
    tmp461_limit_get,
    tmp461_limit_set};

static int tmp461_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "TMP461", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &tmp461_functions;

    return vendor_driver_add(driver);
}
/*THERMAL DEVICE TMP461 END*/

/*THERMAL DEVICE LM75 START*/
#define LM75_TEMP_REG 0x00
#define LM75_CONFIG_REG 0x01
#define LM75_TLOW_REG 0x02
#define LM75_THIGH_REG 0x03

#define LM75_DEFAULT_THIGH 80
#define LM75_DEFAULT_TLOW 70

static int lm75_temp_get(
    void *busDrvPtr, int bus, uint8_t dev, int id, int *temperature)
{
    int rv = 0;
    uint16_t value = 0, is_negative = 0;
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;

    rv = i2c->get(bus, dev, LM75_TEMP_REG, 1, &value, 2);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    if (value & 0x800)
    {
        is_negative = 1;
        value = ~(value) + 1; /* 2's complement */
        value &= 0x7ff;
    }

    value = (((value << 8) | (value >> 8)) >> 4);
    *temperature = value * 62.5;
    if (is_negative)
        *temperature *= -1;

    return rv;
}

static int lm75_limit_get(
    void *busDrvPtr, int bus, uint8_t dev, int id, int type, int *temperature)
{
    int rv = 0;
    uint16_t value = 0;
    uint8_t limitAddr = 0;
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;

    switch (type)
    {
    case VENDOR_THERMAL_LOW_THRESHOLD:
        limitAddr = LM75_TLOW_REG;
        break;
    case VENDOR_THERMAL_HIGH_THRESHOLD:
        limitAddr = LM75_THIGH_REG;
        break;
    default:
        AIM_LOG_ERROR("VENDOR: LM75 unknow limit type!");
        break;
    }

    rv = i2c->get(bus, dev, limitAddr, 1, &value, 2);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    *temperature = value;
    *temperature *= 1000;

    return rv;
}

static int lm75_limit_set(
    void *busDrvPtr, int bus, uint8_t dev, int id, int type, int temperature)
{
    int rv = 0;
    uint8_t limitAddr = 0;
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;

    switch (type)
    {
    case VENDOR_THERMAL_LOW_THRESHOLD:
        limitAddr = LM75_TLOW_REG;
        break;
    case VENDOR_THERMAL_HIGH_THRESHOLD:
        limitAddr = LM75_THIGH_REG;
        break;
    default:
        AIM_LOG_ERROR("VENDOR: LM75 unknow limit type!");
        break;
    }

    rv = i2c->set(bus, dev, limitAddr, 1, (uint16_t)temperature, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    return rv;
}

static thermal_dev_driver_t lm75_functions = {
    lm75_temp_get,
    lm75_limit_get,
    lm75_limit_set};

static int lm75_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "LM75", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &lm75_functions;

    return vendor_driver_add(driver);
}
/*THERMAL DEVICE LM75 END*/

/*THERMAL DEVICE ADM1032 START*/
#define ADM1032_LOCAL 0
#define ADM1032_REMOTE 1

#define ADM1032_HIGH_ADDRESS 0
#define ADM1032_LOW_ADDRESS 1

/**
 *  LOCAL_TEMP only has one address, so there is no decimal in local temp
 */
#define ADM1032_LOCAL_TEMP_R 0x0
#define ADM1032_REMOTE_TEMP_HIGH_R 0x1
#define ADM1032_REMOTE_TEMP_LOW_R 0x10

#define ADM1032_LOCAL_TEMP_THIGH_R 0x5
#define ADM1032_LOCAL_TEMP_TLOW_R 0x6
#define ADM1032_REMOTE_TEMP_THIGH_R 0x7
#define ADM1032_REMOTE_TEMP_TLOW_R 0x8

#define ADM1032_LOCAL_TEMP_THIGH_W 0xb
#define ADM1032_LOCAL_TEMP_TLOW_W 0xc
#define ADM1032_REMOTE_TEMP_THIGH_W 0xd
#define ADM1032_REMOTE_TEMP_TLOW_W 0xe

/**
 *The default threshold values for the chip
 */

#define AMD1032_DEFAULT_THIGH 85
#define AMD1032_DEFAULT_TLOW 0

/*
 *  LOCAL_TEMP only has one address
 */
static const uint8_t adm1032_thermal_reg[][2] = {
    {ADM1032_LOCAL_TEMP_R, 0},
    {ADM1032_REMOTE_TEMP_HIGH_R, ADM1032_REMOTE_TEMP_LOW_R},
};

static const uint8_t adm1032_limit_reg_read[][2] = {
    {ADM1032_LOCAL_TEMP_THIGH_R, ADM1032_LOCAL_TEMP_TLOW_R},
    {ADM1032_REMOTE_TEMP_THIGH_R, ADM1032_REMOTE_TEMP_TLOW_R},
};

static const uint8_t adm1032_limit_reg_write[][2] = {
    {ADM1032_LOCAL_TEMP_THIGH_W, ADM1032_LOCAL_TEMP_TLOW_W},
    {ADM1032_REMOTE_TEMP_THIGH_W, ADM1032_REMOTE_TEMP_TLOW_W},
};

static int adm1032_temp_get(
    void *busDrvPtr, int bus, uint8_t dev, int id, int *temperature)
{
    int rv = 0;
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;

    uint16_t temp_high = 0, temp_low = 0;
    uint16_t is_negative = 0;

    rv = i2c->get(bus, dev, adm1032_thermal_reg[id][ADM1032_HIGH_ADDRESS], 1, &temp_high, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    rv = i2c->get(bus, dev, adm1032_thermal_reg[id][ADM1032_LOW_ADDRESS], 1, &temp_low, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    /*Example
    * 50.125 C
    * temp_high = 0011 0010 = 50 C
    * temp_low  = 0010 0000 = 0.125 C
    * 
    * -0.125 C
    * temp_low  = 1110 0000 = -0.125 C
    */

    if (temp_high & 0x80)
    {
        is_negative = 1;
        temp_high = ~temp_high + 1; /* 2's complement */
        temp_high &= 0xff;

        temp_low = ~temp_low + 1;
        temp_low &= 0xff;
    }

    *temperature = (int)((((temp_high << 8) | temp_low) >> 5) * 125);
    if (is_negative)
        *temperature *= -1;

    return rv;
}

static int adm1032_limit_get(
    void *busDrvPtr, int bus, uint8_t dev, int id, int type, int *temperature)
{
    int rv = 0;
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;

    uint8_t limitAddr = 0;
    uint16_t value;

    switch (type)
    {
    case VENDOR_THERMAL_LOW_THRESHOLD:
        limitAddr = adm1032_limit_reg_read[id][ADM1032_LOW_ADDRESS];
        break;
    case VENDOR_THERMAL_HIGH_THRESHOLD:
        limitAddr = adm1032_limit_reg_read[id][ADM1032_HIGH_ADDRESS];
        break;
    default:
        AIM_LOG_ERROR("VENDOR: ADM1032 unknow limit type!");
        break;
    }

    rv = i2c->get(bus, dev, limitAddr, 1, &value, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    *temperature = value;
    *temperature *= 1000;

    return rv;
}

static int adm1032_limit_set(
    void *busDrvPtr, int bus, uint8_t dev, int id, int type, int temperature)
{
    int rv = 0;
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;

    uint8_t limitAddr = 0;

    switch (type)
    {
    case VENDOR_THERMAL_LOW_THRESHOLD:
        limitAddr = adm1032_limit_reg_write[id][ADM1032_LOW_ADDRESS];
        break;
    case VENDOR_THERMAL_HIGH_THRESHOLD:
        limitAddr = adm1032_limit_reg_write[id][ADM1032_HIGH_ADDRESS];
        break;
    default:
        AIM_LOG_ERROR("VENDOR: ADM1032 unknow limit type!");
        break;
    }

    rv = i2c->set(bus, dev, limitAddr, 1, (uint8_t)temperature, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    return rv;
}

static thermal_dev_driver_t adm1032_functions = {
    adm1032_temp_get,
    adm1032_limit_get,
    adm1032_limit_set};

static int adm1032_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "ADM1032", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &adm1032_functions;

    return vendor_driver_add(driver);
}
/*THERMAL DEVICE ADM1032 START*/

/*PSU DEVICE PMBUS START*/
typedef enum pmbus_page_e
{
    PMBUS_PAGE_ALL = 0,
    PMBUS_PAGE_0 = PMBUS_PAGE_ALL,
    PMBUS_PAGE_1,
    PMBUS_PAGE_2,
    PMBUS_PAGE_3
} pmbus_page_t;

typedef enum
{
    PMBUS_CMD_MIN,
    PMBUS_CMD_PAGE = 0x0,

    PMBUS_CMD_VOUT_MODE = 0x20,

    PMBUS_CMD_FAN_CONFIG_1_2 = 0x3A,
    PMBUS_CMD_FAN_COMMAND_1 = 0x3B,
    PMBUS_CMD_FAN_COMMAND_2 = 0x3C,
    PMBUS_CMD_FAN_CONFIG_3_4 = 0x3D,
    PMBUS_CMD_FAN_COMMAND_3 = 0x3E,
    PMBUS_CMD_FAN_COMMAND_4 = 0x3F,

    PMBUS_CMD_STATUS_WORD = 0x79,

    PMBUS_CMD_AC_VOLTAGE_IN = 0x88,
    PMBUS_CMD_AC_AMPERE_IN = 0x89,
    PMBUS_CMD_DC_VOLTAGE_OUT = 0x8B,
    PMBUS_CMD_DC_AMPERE_OUT = 0x8C,

    PMBUS_CMD_READ_TEMPERATURE_1 = 0x8D,
    PMBUS_CMD_READ_TEMPERATURE_2 = 0x8E,
    PMBUS_CMD_READ_TEMPERATURE_3 = 0x8F,

    PMBUS_CMD_READ_FAN_SPEED_1 = 0x90,
    PMBUS_CMD_READ_FAN_SPEED_2 = 0x91,
    PMBUS_CMD_READ_FAN_SPEED_3 = 0x92,
    PMBUS_CMD_READ_FAN_SPEED_4 = 0x93,
    PMBUS_CMD_DC_WATT_OUT = 0x96,
    PMBUS_CMD_AC_WATT_IN = 0x97,

    PMBUS_CMD_PMBUS_REVISION = 0x98,
    PMBUS_CMD_ID = 0x99,
    PMBUS_CMD_MODEL_NAME = 0x9A,
    PMBUS_CMD_VERSION = 0x9B,
    PMBUS_CMD_SERIAL_NUMBER = 0x9E,

    PMBUS_CMD_VIN_MIN = 0xA0,
    PMBUS_CMD_VIN_MAX = 0xA1,
    PMBUS_CMD_IIN_MAX = 0xA2,
    PMBUS_CMD_PIN_MAX = 0xA3,
    PMBUS_CMD_VOUT_MIN = 0xA4,
    PMBUS_CMD_VOUT_MAX = 0xA5,
    PMBUS_CMD_IOUT_MAX = 0xA6,
    PMBUS_CMD_POUT_MAX = 0xA7,
    PMBUS_CMD_MAX = 0xFF,
    PMBUS_CMD_UNKNOW
} pmbus_command_t;

typedef enum
{
    BIT_FLAG_FORMAT,
    STRING_FORMAT,
    LINEAR_DATA_FORMAT,
    VOUT_DATA_FORMAT
} pmbus_algorithm_t;

typedef struct
{
    pmbus_page_t page;
    pmbus_command_t command;
    pmbus_algorithm_t algorithm;
    uint16_t dataLen;
} pmbus_command_info_t;

static pmbus_command_info_t pmbus_command_info[] = {

    {PMBUS_PAGE_0, PMBUS_CMD_VOUT_MODE, VOUT_DATA_FORMAT, 1},

    {PMBUS_PAGE_ALL, PMBUS_CMD_FAN_CONFIG_1_2, LINEAR_DATA_FORMAT, 1},
    {PMBUS_PAGE_ALL, PMBUS_CMD_FAN_COMMAND_1, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_ALL, PMBUS_CMD_FAN_COMMAND_2, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_ALL, PMBUS_CMD_FAN_CONFIG_3_4, LINEAR_DATA_FORMAT, 1},
    {PMBUS_PAGE_ALL, PMBUS_CMD_FAN_COMMAND_3, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_ALL, PMBUS_CMD_FAN_COMMAND_4, LINEAR_DATA_FORMAT, 2},

    {PMBUS_PAGE_ALL, PMBUS_CMD_STATUS_WORD, BIT_FLAG_FORMAT, 2},

    {PMBUS_PAGE_ALL, PMBUS_CMD_AC_VOLTAGE_IN, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_ALL, PMBUS_CMD_AC_AMPERE_IN, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_0, PMBUS_CMD_DC_VOLTAGE_OUT, VOUT_DATA_FORMAT, 2},
    {PMBUS_PAGE_0, PMBUS_CMD_DC_AMPERE_OUT, LINEAR_DATA_FORMAT, 2},

    {PMBUS_PAGE_0, PMBUS_CMD_READ_TEMPERATURE_1, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_0, PMBUS_CMD_READ_TEMPERATURE_2, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_0, PMBUS_CMD_READ_TEMPERATURE_3, LINEAR_DATA_FORMAT, 2},

    {PMBUS_PAGE_0, PMBUS_CMD_READ_FAN_SPEED_1, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_0, PMBUS_CMD_READ_FAN_SPEED_2, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_0, PMBUS_CMD_READ_FAN_SPEED_3, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_0, PMBUS_CMD_READ_FAN_SPEED_4, LINEAR_DATA_FORMAT, 2},

    {PMBUS_PAGE_0, PMBUS_CMD_DC_WATT_OUT, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_0, PMBUS_CMD_AC_WATT_IN, LINEAR_DATA_FORMAT, 2},

    {PMBUS_PAGE_0, PMBUS_CMD_PMBUS_REVISION, BIT_FLAG_FORMAT, 1},
    {PMBUS_PAGE_0, PMBUS_CMD_ID, STRING_FORMAT, 3},
    {PMBUS_PAGE_0, PMBUS_CMD_MODEL_NAME, STRING_FORMAT, 3},
    {PMBUS_PAGE_0, PMBUS_CMD_VERSION, STRING_FORMAT, 3},
    {PMBUS_PAGE_0, PMBUS_CMD_SERIAL_NUMBER, STRING_FORMAT, 3},

    {PMBUS_PAGE_ALL, PMBUS_CMD_VIN_MIN, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_ALL, PMBUS_CMD_VIN_MAX, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_ALL, PMBUS_CMD_IIN_MAX, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_ALL, PMBUS_CMD_PIN_MAX, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_ALL, PMBUS_CMD_VOUT_MIN, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_ALL, PMBUS_CMD_VOUT_MAX, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_ALL, PMBUS_CMD_IOUT_MAX, LINEAR_DATA_FORMAT, 2},
    {PMBUS_PAGE_ALL, PMBUS_CMD_POUT_MAX, LINEAR_DATA_FORMAT, 2}};

static int get_pmbus_algorithm_result(pmbus_algorithm_t algorithm, uint16_t *data)
{
    switch (algorithm)
    {
    case LINEAR_DATA_FORMAT:
        if (*data & 0x8000 && *data & 0x400)
        {
            return ((~(*data) + 1) & 0x7ff) * 1000 / pow(2, (~(*data >> 11) + 1) & 0x1f);
        }
        else if (*data & 0x8000)
        {
            return (*data & 0x7ff) * 1000 / pow(2, (~(*data >> 11) + 1) & 0x1f);
        }
        else if (*data & 0x400)
        {
            return ((~(*data) + 1) & 0x7ff) * 1000 * pow(2, ((*data >> 11)) & 0x1f);
        }
        else
        {
            return (*data & 0x7ff) * 1000 * pow(2, (*data >> 11) & 0x1f);
        }

    case VOUT_DATA_FORMAT:
        if (data[1] & 0x10)
        {
            return data[0] * 1000 / pow(2, (~data[1] + 1) & 0x1f);
        }
        else
        {
            return data[0] * 1000 * pow(2, data[1] & 0x1f);
        }

    default:
        AIM_LOG_ERROR("VENDOR: PMBUS Cannot find algorithm!");
        break;
    }

    return 0;
}

static pmbus_command_t get_pmbus_command_parm_type(vendor_psu_parm_type_t parmType)
{
    switch (parmType)
    {
    case PSU_PARM_VIN:
        return PMBUS_CMD_AC_VOLTAGE_IN;
    case PSU_PARM_IIN:
        return PMBUS_CMD_AC_AMPERE_IN;
    case PSU_PARM_VOUT:
        return PMBUS_CMD_DC_VOLTAGE_OUT;
    case PSU_PARM_IOUT:
        return PMBUS_CMD_DC_AMPERE_OUT;
    case PSU_PARM_POUT:
        return PMBUS_CMD_DC_WATT_OUT;
    case PSU_PARM_PIN:
        return PMBUS_CMD_AC_WATT_IN;

    default:
        AIM_LOG_ERROR("VENDOR: Wrong parm type in PMBUS driver.");
        return PMBUS_CMD_UNKNOW;
    }
}

static pmbus_command_info_t *get_pmbus_command_info(pmbus_command_t command)
{
    int i, cnt = sizeof(pmbus_command_info) / sizeof(pmbus_command_info_t);

    for (i = 0; i < cnt; i++)
    {
        if (pmbus_command_info[i].command == command)
        {
            return &pmbus_command_info[i];
        }
    }

    return NULL;
}

static int pmbus_command_data_get(
    i2c_bus_driver_t *i2c,
    int bus,
    uint8_t dev,
    pmbus_command_info_t *info,
    uint16_t *pmbusData)
{
    int rv = 0;

    if (info == NULL)
        return ONLP_STATUS_E_INTERNAL;

    rv = i2c->get(bus, dev, info->command, 1, pmbusData, info->dataLen);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    return 0;
}

static int pmbus_command_string_get(
    i2c_bus_driver_t *i2c,
    int bus,
    uint8_t dev,
    pmbus_command_info_t *info,
    char *string)
{
    int rv = 0;
    uint16_t len;

    if (info == NULL)
        return ONLP_STATUS_E_INTERNAL;

    rv = i2c->get(bus, dev, info->command, 1, &len, 1);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    rv = i2c->block_read(bus, dev, info->command, (uint8_t *)string, len);

    return rv;
}

static int pmbus_model_get(void *busDrvPtr, int bus, uint8_t dev, char *model)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;
    pmbus_command_info_t *info = get_pmbus_command_info(PMBUS_CMD_MODEL_NAME);
    rv = pmbus_command_string_get(i2c, bus, dev, info, model);
    return rv;
}

static int pmbus_serial_get(void *busDrvPtr, int bus, uint8_t dev, char *serial)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;
    pmbus_command_info_t *info = get_pmbus_command_info(PMBUS_CMD_SERIAL_NUMBER);
    rv = pmbus_command_string_get(i2c, bus, dev, info, serial);
    return rv;
}

static int pmbus_volt_get(void *busDrvPtr, int bus, uint8_t dev, int *volt)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    uint16_t pmbus_data[2] = {0};
    pmbus_command_info_t *info = NULL;
    info = get_pmbus_command_info(PMBUS_CMD_DC_VOLTAGE_OUT);
    rv = pmbus_command_data_get(i2c, bus, dev, info, &pmbus_data[0]);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    info = get_pmbus_command_info(PMBUS_CMD_VOUT_MODE);
    rv = pmbus_command_data_get(i2c, bus, dev, info, &pmbus_data[1]);
    if (rv < 0)
        return ONLP_STATUS_E_INTERNAL;

    *volt = get_pmbus_algorithm_result(info->algorithm, pmbus_data);

    return rv;
}

static int pmbus_amp_get(void *busDrvPtr, int bus, uint8_t dev, int *amp)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    uint16_t pmbus_data = 0;
    pmbus_command_info_t *info = get_pmbus_command_info(PMBUS_CMD_DC_AMPERE_OUT);
    rv = pmbus_command_data_get(i2c, bus, dev, info, &pmbus_data);
    if (rv != 0)
        return ONLP_STATUS_E_INTERNAL;

    *amp = get_pmbus_algorithm_result(info->algorithm, &pmbus_data);

    return rv;
}

static int pmbus_watt_get(void *busDrvPtr, int bus, uint8_t dev, int *watt)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    uint16_t pmbus_data = 0;
    pmbus_command_info_t *info = get_pmbus_command_info(PMBUS_CMD_DC_WATT_OUT);
    rv = pmbus_command_data_get(i2c, bus, dev, info, &pmbus_data);
    if (rv != 0)
        return ONLP_STATUS_E_INTERNAL;

    *watt = get_pmbus_algorithm_result(info->algorithm, &pmbus_data);

    return rv;
}

static int psu_runtime_parm_get(
    void *busDrvPtr, int bus, uint8_t dev,
    vendor_psu_parm_type_t parmType,
    int *parameter)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    uint16_t pmbusData = 0;
    pmbus_command_t command = get_pmbus_command_parm_type(parmType);

    if (command == PMBUS_CMD_UNKNOW)
    {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (command == PMBUS_CMD_DC_VOLTAGE_OUT)
    {
        rv = pmbus_volt_get(busDrvPtr, bus, dev, parameter);
        return rv;
    }

    pmbus_command_info_t *info = get_pmbus_command_info(command);
    rv = pmbus_command_data_get(i2c, bus, dev, info, &pmbusData);
    if (rv != 0)
        return ONLP_STATUS_E_INTERNAL;

    *parameter = get_pmbus_algorithm_result(info->algorithm, &pmbusData);

    return rv;
}

static int pmbus_runtime_info_get(
    void *busDrvPtr, int bus, uint8_t dev,
    vendor_psu_runtime_info_t *runtimeInfo)
{
    int rv = 0;
    int parameter = 0;
    vendor_psu_parm_type_t parmType = PSU_PARM_MIN;

    for (parmType = PSU_PARM_MIN; parmType < PSU_PARM_MAX; parmType++)
    {
        rv = psu_runtime_parm_get(busDrvPtr, bus, dev, parmType, &parameter);
        if (rv != 0)
            return ONLP_STATUS_E_INTERNAL;

        switch (parmType)
        {
        case PSU_PARM_VIN:
            runtimeInfo->vin = parameter;
            break;
        case PSU_PARM_IIN:
            runtimeInfo->iin = parameter;
            break;
        case PSU_PARM_VOUT:
            runtimeInfo->vout = parameter;
            break;
        case PSU_PARM_IOUT:
            runtimeInfo->iout = parameter;
            break;
        case PSU_PARM_POUT:
            runtimeInfo->pout = parameter;
            break;
        case PSU_PARM_PIN:
            runtimeInfo->pin = parameter;
            break;

        default:
            AIM_LOG_ERROR("VENDOR: Wrong parm type in PMBUS driver!");
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    return rv;
}

static int pmbus_fan_rpm_get(void *busDrvPtr, int bus, uint8_t dev, int id, int *rpm)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    uint16_t pmbus_data = 0;
    pmbus_command_info_t *info = get_pmbus_command_info(PMBUS_CMD_READ_FAN_SPEED_1);
    rv = pmbus_command_data_get(i2c, bus, dev, info, &pmbus_data);
    if (rv != 0)
        return ONLP_STATUS_E_INTERNAL;

    *rpm = get_pmbus_algorithm_result(info->algorithm, &pmbus_data);
    *rpm /= 1000;

    return rv;
}

static int pmbus_fan_rpm_set(void *busDrvPtr, int bus, uint8_t dev, int id, int rpm)
{
    AIM_LOG_ERROR("VENDOR: It is not supported in PMBUS.");
    return ONLP_STATUS_E_INTERNAL;
}

static psu_dev_driver_t pmbus_psu_functions = {
    pmbus_model_get,
    pmbus_serial_get,
    pmbus_volt_get,
    pmbus_amp_get,
    pmbus_watt_get,
    pmbus_runtime_info_get};

static fan_dev_driver_t pmbus_fan_functions = {
    pmbus_fan_rpm_get,
    pmbus_fan_rpm_set,
};

static int pmbus_psu_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "PMBUS_PSU", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &pmbus_psu_functions;

    return vendor_driver_add(driver);
}

static int pmbus_fan_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "PMBUS_FAN", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &pmbus_fan_functions;

    return vendor_driver_add(driver);
}

/*PSU DEVICE PMBUS END*/

/*SFP and QSFP CONTROL DEFINE*/

typedef enum
{
    /* Shared QSFP and SFP fields: */
    SFF_FIELD_MIN,
    IDENTIFIER,            /* Type of Transceiver */
    STATUS,                /* Support flags for upper pages */
    CHANNEL_LOS_INDICATOR, /* TX and RX LOS */
    CHANNEL_TX_FAULT,
    TEMPERATURE_ALARMS,
    VCC_ALARMS, /* Voltage */
    CHANNEL_RX_PWR_ALARMS,
    CHANNEL_TX_BIAS_ALARMS,
    TEMPERATURE,
    VCC, /* Voltage */
    CHANNEL_RX_PWR,
    CHANNEL_TX_PWR,
    CHANNEL_TX_BIAS,
    CHANNEL_TX_DISABLE,
    POWER_CONTROL,
    PAGE_SELECT_BYTE, /*SFF8436/SFF8636*/

    DIAGNOSTIC_MONITORING_TYPE, /* SFF8472 Diagnostic monitoring implemented */
    EXTERNAL_CALIBRATION,       /* SFF8472 diagnostic calibration constants */
    SFF_FIELD_MAX               /* keep this the last */

} sff_field_t;

/*SFP DEVICE SFF8636 START*/

#define SFF8636_PAGE_CONTROL 127
#define SFF8636_UPPER_PAGE_START_OFFSET 128
#define SFP_EEPROM_SIZE 256
#define SFP_EEPROM_PAGE_SIZE 128
#define CONTROL_IS_SUPPORT 1
#define CONTROL_IS_NOT_SUPPORT 0
/*
typedef enum
{
    SFF_FIELD_MIN,
    IDENTIFIER,
    STATUS,
    CHANNEL_LOS_INDICATOR,
    CHANNEL_TX_FAULT,
    VCC_ALARMS,
    CHANNEL_RX_PWR_ALARMS,
    CHANNEL_TX_BIAS_ALARMS,
    TEMPERATURE,
    VCC,
    CHANNEL_RX_PWR,
    CHANNEL_TX_PWR,
    CHANNEL_TX_BIAS,
    CHANNEL_TX_DISABLE,
    POWER_CONTROL,
    PAGE_SELECT_BYTE,
    SFF_FIELD_MAX

} sff_field_t;
*/
typedef enum
{
    SFF8636_EEPROM_BASE_PAGE,
    SFF8636_EEPROM_PAGE0 = 0,
} sff8636_page_t;

typedef struct
{
    sff_field_t field;
    uint8_t page;
    uint16_t offset;
    uint16_t length;
} sff8636_field_map_t;

static uint8_t get_sfp_flat_mem(i2c_bus_driver_t *i2c, int bus, uint8_t addr);
static uint8_t filter_control_value_for_get(int control, uint8_t *eeprom_data, int *combined_data);
static uint8_t filter_control_value_for_set(int control, uint8_t curr_status, int *write_status);
static sff_field_t get_control_field(int control);
static int eeprom_uppermap_change_page(i2c_bus_driver_t *i2c, int bus, uint8_t dev, uint8_t page);
static int get_sff8636_field(sff_field_t field, sff8636_field_map_t **info);
static int get_sff8636_value_from_eeprom(i2c_bus_driver_t *i2c, int bus, uint8_t dev, sff8636_field_map_t *info, uint8_t *value);
static int set_sff8636_value_to_eeprom(i2c_bus_driver_t *i2c, int bus, uint8_t dev, sff8636_field_map_t *info, uint8_t *value);

static sff8636_field_map_t sff8636_field_map[] = {
    {IDENTIFIER, SFF8636_EEPROM_BASE_PAGE, 0, 1},
    {STATUS, SFF8636_EEPROM_BASE_PAGE, 1, 2},
    {CHANNEL_LOS_INDICATOR, SFF8636_EEPROM_BASE_PAGE, 3, 1},
    {CHANNEL_TX_FAULT, SFF8636_EEPROM_BASE_PAGE, 4, 1},
    {VCC_ALARMS, SFF8636_EEPROM_BASE_PAGE, 7, 1},
    {CHANNEL_RX_PWR_ALARMS, SFF8636_EEPROM_BASE_PAGE, 9, 2},
    {CHANNEL_TX_BIAS_ALARMS, SFF8636_EEPROM_BASE_PAGE, 11, 2},
    {TEMPERATURE, SFF8636_EEPROM_BASE_PAGE, 22, 2},
    {VCC, SFF8636_EEPROM_BASE_PAGE, 26, 2},
    {CHANNEL_RX_PWR, SFF8636_EEPROM_BASE_PAGE, 34, 8},
    {CHANNEL_TX_BIAS, SFF8636_EEPROM_BASE_PAGE, 42, 8},
    {CHANNEL_TX_PWR, SFF8636_EEPROM_BASE_PAGE, 50, 8},
    {CHANNEL_TX_DISABLE, SFF8636_EEPROM_BASE_PAGE, 86, 1},
    {POWER_CONTROL, SFF8636_EEPROM_BASE_PAGE, 93, 1},
    {PAGE_SELECT_BYTE, SFF8636_EEPROM_BASE_PAGE, 127, 1},
};

static int sff8636_eeprom_load(void *busDrvPtr, int bus, uint8_t dev, uint8_t *data)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    rv = i2c->i2c_block_read(bus, dev, 0, data, SFP_EEPROM_PAGE_SIZE);
    if (0 == rv)
    {
        rv = i2c->i2c_block_read(bus, dev, SFP_EEPROM_PAGE_SIZE, data+SFP_EEPROM_PAGE_SIZE, SFP_EEPROM_PAGE_SIZE);
    }

    return rv;
}

static int sff8636_eeprom_dom_load(void *busDrvPtr, int bus, uint8_t dev, uint8_t *data)
{
    AIM_LOG_ERROR("VENDOR: DOM EEPROM is not supported on SFF8636");
    return -1;
}

static int sff8636_eeprom_read(void *busDrvPtr, int bus, uint8_t dev, uint8_t offset, uint16_t len, uint8_t *data)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    if ((offset < SFP_EEPROM_PAGE_SIZE) && ((offset + len) > SFP_EEPROM_PAGE_SIZE))
    {
        uint16_t u16_1st_len = SFP_EEPROM_PAGE_SIZE - offset;
        uint16_t u16_2nd_len = (offset + len) - SFP_EEPROM_PAGE_SIZE;

        rv = i2c->i2c_block_read(bus, dev, offset, data, u16_1st_len);
        if (0 == rv)
        {
            rv = i2c->i2c_block_read(bus, dev, SFP_EEPROM_PAGE_SIZE, data + u16_1st_len, u16_2nd_len);
        }
    }
    else
    {
        rv = i2c->i2c_block_read(bus, dev, offset, data, len);
    }

    return rv;
}

static int sff8636_eeprom_write(void *busDrvPtr, int bus, uint8_t dev, uint8_t offset, uint16_t len, uint8_t *data)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    if ((offset < SFP_EEPROM_PAGE_SIZE) && ((offset + len) > SFP_EEPROM_PAGE_SIZE))
    {
        uint16_t u16_1st_len = SFP_EEPROM_PAGE_SIZE - offset;
        uint16_t u16_2nd_len = (offset + len) - SFP_EEPROM_PAGE_SIZE;

        rv = eeprom_writeb(i2c, bus, dev, offset, 1, data, u16_1st_len);
        if (0 == rv)
        {
            rv = eeprom_writeb(i2c, bus, dev, SFP_EEPROM_PAGE_SIZE, 1, data + u16_1st_len, u16_2nd_len);
        }
    }
    else
    {
        rv = eeprom_writeb(i2c, bus, dev, offset, 1, data, len);
    }

    return rv;
}

static int sff8636_eeprom_readb(void *busDrvPtr, int bus, uint8_t dev, uint16_t offset, uint8_t *data)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    rv = eeprom_readb(i2c, bus, dev, offset, 1, data, 1);

    return rv;
}

static int sff8636_eeprom_writeb(void *busDrvPtr, int bus, uint8_t dev, uint16_t offset, uint8_t data)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    rv = eeprom_writeb(i2c, bus, dev, offset, 1, &data, 1);

    return rv;
}

static int sff8636_eeprom_readw(void *busDrvPtr, int bus, uint8_t dev, uint16_t offset, uint16_t *data)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    i2c = i2c;
    rv = sff8636_eeprom_read(busDrvPtr, bus, dev, offset, 2, (uint8_t *)data);

    return rv;
}

static int sff8636_eeprom_writew(void *busDrvPtr, int bus, uint8_t dev, uint16_t offset, uint16_t data)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    i2c = i2c;
    rv = sff8636_eeprom_write(busDrvPtr, bus, dev, offset, 2, (uint8_t *)&data);

    return rv;
}

static int sff8636_control_is_support(int control, uint8_t *is_support)
{
    switch (control)
    {
    //case SFP_CONTROL_RESET:
    case SFP_CONTROL_RESET_STATE:
    case SFP_CONTROL_LP_MODE:
    case SFP_CONTROL_RX_LOS:
    case SFP_CONTROL_TX_FAULT:
    case SFP_CONTROL_TX_DISABLE:
        //case SFP_CONTROL_TX_DISABLE_CHANNEL:
        //case SFP_CONTROL_POWER_OVERRIDE:
        *is_support = CONTROL_IS_SUPPORT;
        break;
    default:
        *is_support = CONTROL_IS_NOT_SUPPORT;
        break;
    }

    return 0;
}

static int sff8636_control_get(
    void *busDrvPtr, int bus, uint8_t dev,
    int control,
    int *status)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;
    uint8_t *eeprom_data;
    sff_field_t field = get_control_field(control);
    if (field == SFF_FIELD_MIN)
    {
        AIM_LOG_ERROR("VENDOR: The control type is not supported in SFF8636");
        return -1;
    }
    sff8636_field_map_t *info = (sff8636_field_map_t *)calloc(1, sizeof(sff8636_field_map_t));

    /* get the address of the control */
    rv = get_sff8636_field(field, &info);
    if (rv != 0)
        return rv;
    eeprom_data = (uint8_t *)calloc(1, sizeof(uint8_t) * info->length);

    /* get the control value from eeprom */
    rv = get_sff8636_value_from_eeprom(i2c, bus, dev, info, eeprom_data);
    if (rv != 0)
    {
        free(eeprom_data);
        return rv;
    }

    /* filter the right date which user need */
    if (filter_control_value_for_get(control, eeprom_data, status) != CONTROL_IS_SUPPORT)
    {
        AIM_LOG_ERROR("VENDOR: The control type get status error in SFF8636");
        rv = -1;
    }

    free(eeprom_data);
    eeprom_data = NULL;
    return rv;
}

static int sff8636_control_set(
    void *busDrvPtr, int bus, uint8_t dev,
    int control,
    int status)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;
    uint8_t *eeprom_data;
    uint8_t value;

    if (control == SFP_CONTROL_RX_LOS || control == SFP_CONTROL_TX_FAULT)
    {
        AIM_LOG_ERROR("VENDOR: This control type is not writable in SFF8636");
        return -1;
    }

    sff_field_t field = get_control_field(control);
    if (field == SFF_FIELD_MIN)
    {
        AIM_LOG_ERROR("VENDOR: This control type is not supported in SFF8636");
        return -1;
    }

    sff8636_field_map_t *info = (sff8636_field_map_t *)calloc(1, sizeof(sff8636_field_map_t));

    /* get the address of the control */
    rv = get_sff8636_field(field, &info);
    if (rv != 0)
        return rv;

    eeprom_data = (uint8_t *)calloc(1, sizeof(uint8_t) * info->length);
    /* get the control value from eeprom */
    rv = get_sff8636_value_from_eeprom(i2c, bus, dev, info, eeprom_data);
    if (rv != 0)
    {
        free(eeprom_data);
        return rv;
    }

    /* value check */
    rv = filter_control_value_for_set(control, *eeprom_data, &status);
    if (rv != 0)
    {
        free(eeprom_data);
        return rv;
    }

    /* set the control value to eeprom */
    value = (uint8_t)status;
    rv = set_sff8636_value_to_eeprom(i2c, bus, dev, info, &value);

    free(eeprom_data);
    eeprom_data = NULL;

    return rv;
}

/*================ internal function ================*/

static int get_sff8636_field(sff_field_t field, sff8636_field_map_t **info)
{

    int i, cnt = sizeof(sff8636_field_map) / sizeof(sff8636_field_map_t);

    if (field >= SFF_FIELD_MAX)
    {
        return -1;
    }

    for (i = 0; i < cnt; i++)
    {
        if (sff8636_field_map[i].field == field)
        {
            *info = &(sff8636_field_map[i]);
        }
    }

    if (!info)
    {
        AIM_LOG_ERROR("VENDOR: Cannot find sff8636 field!");
        return -1;
    }

    return 0;
}

static uint8_t get_sfp_flat_mem(i2c_bus_driver_t *i2c, int bus, uint8_t dev)
{
    int rv = 0;
    uint8_t status[2];
    uint8_t flat_mem = 0;
    sff8636_field_map_t *info = (sff8636_field_map_t *)calloc(1, sizeof(sff8636_field_map_t));

    rv = get_sff8636_field(STATUS, &info);
    if (rv != 0)
        return rv;
    rv = eeprom_readb(i2c, bus, dev, info->offset, 1, status, info->length);
    if (rv != 0)
        return rv;

    flat_mem = status[1] & (1 << 2);

    return flat_mem;
}

static int eeprom_uppermap_change_page(i2c_bus_driver_t *i2c, int bus, uint8_t dev, uint8_t page)
{
    int rv = 0;
    if (!get_sfp_flat_mem(i2c, bus, dev))
    {
        rv = eeprom_writeb(i2c, bus, dev, SFF8636_PAGE_CONTROL, 1, &page, 1);
    }

    return rv;
}

static int get_sff8636_value_from_eeprom(i2c_bus_driver_t *i2c, int bus, uint8_t dev, sff8636_field_map_t *info, uint8_t *value)
{
    int rv = 0;

    rv = eeprom_uppermap_change_page(i2c, bus, dev, info->page);
    if (rv != 0)
        return rv;

    rv = eeprom_readb(i2c, bus, dev, info->offset, 1, value, info->length);
    if (rv != 0)
        return rv;

    return rv;
}

static int set_sff8636_value_to_eeprom(i2c_bus_driver_t *i2c, int bus, uint8_t dev, sff8636_field_map_t *info, uint8_t *value)
{
    int rv = 0;

    rv = eeprom_uppermap_change_page(i2c, bus, dev, info->page);
    if (rv != 0)
        return rv;

    rv = eeprom_writeb(i2c, bus, dev, info->offset, 1, value, info->length);
    if (rv != 0)
        return rv;

    return rv;
}

static sff_field_t get_control_field(int control)
{
    switch (control)
    {
    case SFP_CONTROL_RX_LOS:
        return CHANNEL_LOS_INDICATOR;

    case SFP_CONTROL_TX_FAULT:
        return CHANNEL_TX_FAULT;

    case SFP_CONTROL_TX_DISABLE:
    case SFP_CONTROL_TX_DISABLE_CHANNEL:
        return CHANNEL_TX_DISABLE;

    case SFP_CONTROL_POWER_OVERRIDE:
        return POWER_CONTROL;

    case SFP_CONTROL_TEMPERATURE:
        return TEMPERATURE;

    case SFP_CONTROL_VOLTAGE:
        return VCC;

    case SFP_CONTROL_TX_BIAS:
        return CHANNEL_TX_BIAS;

    case SFP_CONTROL_RX_POWER:
        return CHANNEL_RX_PWR;

    case SFP_CONTROL_TX_POWER:
        return CHANNEL_TX_PWR;

    default:
        return SFF_FIELD_MIN;
    }
}

/* SFP_CONTROL_RESET */              /* SUPPORT_BY_CPLD */
/* SFP_CONTROL_RESET_STATE */        /* SUPPORT_BY_CPLD */
/* SFP_CONTROL_RX_LOS */             /* R 3.0 - 3.3 */
/* SFP_CONTROL_TX_FAULT */           /* R 4.0 - 4.3 */
/* SFP_CONTROL_TX_DISABLE */         /* RW 86.0 - 86.3 */
/* SFP_CONTROL_TX_DISABLE_CHANNEL */ /* RW 86.0 - 86.3 */
/* SFP_CONTROL_LP_MODE */            /* SUPPORT_BY_CPLD */
/* SFP_CONTROL_POWER_OVERRIDE */     /* RW 93.0 POWER_OVERRIDE 
                                         * RW 93.1 POWER_SET                                           
                                         */

static uint8_t filter_control_value_for_get(
    int control,
    uint8_t *eeprom_data, int *combined_data)
{
    int i = 0, j = 0;
    uint8_t channel = 4;

    switch (control)
    {
    case SFP_CONTROL_TX_DISABLE:
    case SFP_CONTROL_TX_DISABLE_CHANNEL:
        *combined_data = *eeprom_data & 0xf;
        break;

    case SFP_CONTROL_TX_FAULT:
    case SFP_CONTROL_RX_LOS:
        for (i = 0; i < channel; i++)
        {
            *(combined_data + i) = (*eeprom_data & (0x01 << i)) >> i;
        }
        break;

    case SFP_CONTROL_POWER_OVERRIDE:
        *combined_data = *eeprom_data & 0x3;
        break;

    case SFP_CONTROL_TEMPERATURE: /* Unit: Celsius */
        *combined_data = (eeprom_data[0] << 8 | eeprom_data[1]) / 256;
        break;

    case SFP_CONTROL_VOLTAGE: /* Unit: mV */
        *combined_data = (eeprom_data[0] << 8 | eeprom_data[1]) / 10;
        break;

    case SFP_CONTROL_TX_BIAS: /* Unit: uA */
        for (i = 0, j = 0; i < channel; i++, j = j + 2)
        {
            *(combined_data + i) = (eeprom_data[j] << 8 | eeprom_data[j + 1]) * 2;
        }
        break;

    case SFP_CONTROL_RX_POWER: /* Unit: uW */
        for (i = 0, j = 0; i < channel; i++, j = j + 2)
        {
            *(combined_data + i) = (eeprom_data[j] << 8 | eeprom_data[j + 1]) / 10;
        }
        break;

    case SFP_CONTROL_TX_POWER: /* Unit: uW */
        for (i = 0, j = 0; i < channel; i++, j = j + 2)
        {
            *(combined_data + i) = (eeprom_data[j] << 8 | eeprom_data[j + 1]) / 10;
        }
        break;

    default:
        return CONTROL_IS_NOT_SUPPORT;
    }

    return CONTROL_IS_SUPPORT;
}

#define SFP_CONTROL_POWER_OVERRIDE_MASK 0x3
#define SFP_CONTROL_TX_DISABLE_CHANNEL_MASK 0xF

static uint8_t filter_control_value_for_set(
    int control,
    uint8_t curr_status,
    int *write_status)
{
    int data;
    int status = *write_status;
    switch (control)
    {
    case SFP_CONTROL_TX_DISABLE:

        if (status == 0)
        {
            /*  clean mask bit value of current status */
            data = curr_status & (~SFP_CONTROL_TX_DISABLE_CHANNEL_MASK);
            *write_status = data;
            return 0;
        }
        else if (status == 1)
        {
            /*  clean mask bit of current status */
            data = curr_status & (~SFP_CONTROL_TX_DISABLE_CHANNEL_MASK);
            /*  calculate write data  */
            data |= SFP_CONTROL_TX_DISABLE_CHANNEL_MASK;
            *write_status = data;
            return 0;
        }
        else
        {
            AIM_LOG_ERROR("VENDOR:\t[SFF8636] Status of TX disable should be 0 or 1 ");
            return -1;
        }

    case SFP_CONTROL_TX_DISABLE_CHANNEL:
        if (status >= 0 && status <= SFP_CONTROL_TX_DISABLE_CHANNEL_MASK)
        {
            /*  clean mask bit of current status */
            data = curr_status & (~SFP_CONTROL_TX_DISABLE_CHANNEL_MASK);
            /*  calculate write data  */
            data |= (status & SFP_CONTROL_TX_DISABLE_CHANNEL_MASK);
            *write_status = data;
            return 0;
        }
        else
        {
            AIM_LOG_ERROR("VENDOR:\t[SFF8636] Status of TX disable channel should be 0 to 0xF ");
            return -1;
        }

    case SFP_CONTROL_POWER_OVERRIDE:
        if (status >= 0 && status <= SFP_CONTROL_POWER_OVERRIDE_MASK)
        {
            /*  clean mask bit of current status */
            data = curr_status & (~SFP_CONTROL_POWER_OVERRIDE_MASK);
            /*  calculate write data  */
            data |= (status & SFP_CONTROL_POWER_OVERRIDE_MASK);
            *write_status = data;
            return 0;
        }
        else
        {
            AIM_LOG_ERROR("VENDOR:\t[SFF8636] Status of TX disable channel should be 0 to 0x3 ");
            return -1;
        }
    default:
        return CONTROL_IS_NOT_SUPPORT;
    }
}

static sfp_dev_driver_t sff8636_functions =
    {
        sff8636_eeprom_load,
        sff8636_eeprom_dom_load,
        sff8636_eeprom_read,
        sff8636_eeprom_write,
        sff8636_eeprom_readb,
        sff8636_eeprom_writeb,
        sff8636_eeprom_readw,
        sff8636_eeprom_writew,
        sff8636_control_is_support,
        sff8636_control_get,
        sff8636_control_set};

static int sff8636_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "SFF8636", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &sff8636_functions;
    vendor_driver_add(driver);

    driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));
    strncpy(driver->name, "SFF8436", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &sff8636_functions;
    vendor_driver_add(driver);

    return 0;
}
/*SFP DEVICE SFF8636 END*/

/*SFP DEVICE SFF8472 START*/

typedef enum
{
    SFF8472_EEPROM_ADDRESS_A0H = 0x50,
    SFF8472_EEPROM_ADDRESS_A2H = 0x51,

} sff8472_address_t;

typedef struct sff8472_field_map
{
    sff_field_t field;
    uint8_t address;
    uint16_t offset;
    uint16_t length;
} sff8472_field_map_t;

static uint8_t filter_sff8472_control_value_for_get(int control, uint8_t *eeprom_data, int *combined_data);
static uint8_t filter_sff8472_control_value_for_set(int control, uint8_t *eeprom_data, int *write_status);
static sff_field_t get_sff8472_control_field(int control);
static int get_sff8472_field(sff_field_t field, sff8472_field_map_t **info);
static int get_sff8472_value_from_eeprom(i2c_bus_driver_t *i2c, int bus, uint8_t addr, sff8472_field_map_t *info, uint8_t *value);
static int set_sff8472_value_to_eeprom(i2c_bus_driver_t *i2c, int bus, uint8_t addr, sff8472_field_map_t *info, uint8_t *value);
static int sff8472_is_dom_supported(void *busDrvPtr, int bus, uint8_t addr);

static sff8472_field_map_t sff8472_field_map[] = {
    {IDENTIFIER, SFF8472_EEPROM_ADDRESS_A0H, 0, 1},
    {DIAGNOSTIC_MONITORING_TYPE, SFF8472_EEPROM_ADDRESS_A0H, 92, 1},
    {EXTERNAL_CALIBRATION, SFF8472_EEPROM_ADDRESS_A2H, 56, 36},
    {TEMPERATURE, SFF8472_EEPROM_ADDRESS_A2H, 96, 2},
    {VCC, SFF8472_EEPROM_ADDRESS_A2H, 98, 2},
    {CHANNEL_TX_BIAS, SFF8472_EEPROM_ADDRESS_A2H, 100, 2},
    {CHANNEL_TX_PWR, SFF8472_EEPROM_ADDRESS_A2H, 102, 2},
    {CHANNEL_RX_PWR, SFF8472_EEPROM_ADDRESS_A2H, 104, 2},
    {CHANNEL_LOS_INDICATOR, SFF8472_EEPROM_ADDRESS_A2H, 110, 1},
    {CHANNEL_TX_FAULT, SFF8472_EEPROM_ADDRESS_A2H, 110, 1},
    {CHANNEL_TX_DISABLE, SFF8472_EEPROM_ADDRESS_A2H, 110, 1},
};

static int sff8472_eeprom_load(void *busDrvPtr, int bus, uint8_t dev, uint8_t *data)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    rv = i2c->i2c_block_read(bus, dev, 0, data, SFP_EEPROM_SIZE);

    return rv;
}

static int sff8472_eeprom_dom_load(void *busDrvPtr, int bus, uint8_t dev, uint8_t *data)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    uint8_t domIsSupport = 0;
    rv = eeprom_readb(i2c, bus, dev, 92, 1, &domIsSupport, 1);

    if (domIsSupport & 0x40)
    {
        rv = i2c->i2c_block_read(bus, SFF8472_EEPROM_ADDRESS_A2H, 0, data, SFP_EEPROM_SIZE);
    }
    else
    {
        AIM_LOG_ERROR("VENDOR: This transiver does not support DOM information.");
        return -1;
    }

    return rv;
}

static int sff8472_eeprom_read(void *busDrvPtr, int bus, uint8_t dev, uint8_t offset, uint16_t len, uint8_t *data)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    rv = i2c->i2c_block_read(bus, dev, offset, data, len);

    return rv;
}

static int sff8472_eeprom_write(void *busDrvPtr, int bus, uint8_t dev, uint8_t offset, uint16_t len, uint8_t *data)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    rv = eeprom_writeb(i2c, bus, dev, offset, 1, data, len);

    return rv;
}

static int sff8472_eeprom_readb(void *busDrvPtr, int bus, uint8_t dev, uint16_t offset, uint8_t *data)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    rv = eeprom_readb(i2c, bus, dev, offset, 1, data, 1);

    return rv;
}

static int sff8472_eeprom_writeb(void *busDrvPtr, int bus, uint8_t dev, uint16_t offset, uint8_t data)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    rv = eeprom_writeb(i2c, bus, dev, offset, 1, &data, 1);

    return rv;
}

static int sff8472_eeprom_readw(void *busDrvPtr, int bus, uint8_t dev, uint16_t offset, uint16_t *data)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    rv = eeprom_readw(i2c, bus, dev, offset, 1, data, 1);

    return rv;
}

static int sff8472_eeprom_writew(void *busDrvPtr, int bus, uint8_t dev, uint16_t offset, uint16_t data)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;

    rv = eeprom_writew(i2c, bus, dev, offset, 1, &data, 1);

    return rv;
}

static int sff8472_control_is_support(int control, uint8_t *is_support)
{
    switch (control)
    {
    case SFP_CONTROL_RX_LOS:
    case SFP_CONTROL_TX_FAULT:
    case SFP_CONTROL_TX_DISABLE:
    case SFP_CONTROL_TEMPERATURE:
    case SFP_CONTROL_VOLTAGE:
    case SFP_CONTROL_TX_BIAS:
    case SFP_CONTROL_RX_POWER:
    case SFP_CONTROL_TX_POWER:
        *is_support = CONTROL_IS_SUPPORT;
        break;
    case SFP_CONTROL_RESET:
    case SFP_CONTROL_RESET_STATE:
    case SFP_CONTROL_LP_MODE:
    case SFP_CONTROL_TX_DISABLE_CHANNEL:
    case SFP_CONTROL_POWER_OVERRIDE:
        *is_support = CONTROL_IS_NOT_SUPPORT;
        break;
    default:
        *is_support = CONTROL_IS_NOT_SUPPORT;
        break;
    }

    return 0;
}

static int sff8472_is_dom_supported(void *busDrvPtr, int bus, uint8_t dev)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;
    uint8_t domIsSupport = 0;

    sff8472_field_map_t *info = (sff8472_field_map_t *)calloc(1, sizeof(sff8472_field_map_t));

    rv = get_sff8472_field(DIAGNOSTIC_MONITORING_TYPE, &info);
    if (rv != 0)
        return rv;

    rv = eeprom_readb(i2c, bus, dev, info->offset, 1, &domIsSupport, info->length);

    if (domIsSupport & 0x10)
    {
        /*TODO*/
        AIM_LOG_ERROR("VENDOR: External calibration not yet supported.");
        return -1;
    }

    if (!(domIsSupport & 0x40))
    {
        AIM_LOG_ERROR("VENDOR: This transiver does not support DOM information.");
        return -1;
    }

    return rv;
}

static int sff8472_control_get(
    void *busDrvPtr, int bus, uint8_t dev,
    int control,
    int *status)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;
    uint8_t *eeprom_data;
    sff_field_t field = get_sff8472_control_field(control);
    if (field == SFF_FIELD_MIN)
    {
        AIM_LOG_ERROR("VENDOR: The control type is not supported in SFF8472");
        return -1;
    }
    sff8472_field_map_t *info = (sff8472_field_map_t *)calloc(1, sizeof(sff8472_field_map_t));

    rv = get_sff8472_field(field, &info);
    if (rv != 0)
        return rv;

    eeprom_data = (uint8_t *)calloc(1, sizeof(uint8_t) * info->length);

    if (!sff8472_is_dom_supported(i2c, bus, dev))
    {

        /* get the control value from eeprom */
        rv = get_sff8472_value_from_eeprom(i2c, bus, dev, info, eeprom_data);
        if (rv != 0)
        {
            free(eeprom_data);
            return rv;
        }

        /* filter the right date which user need */
        if (filter_sff8472_control_value_for_get(control, eeprom_data, status) != CONTROL_IS_SUPPORT)
        {
            AIM_LOG_ERROR("VENDOR: The control type get status error in SFF8472");
            free(eeprom_data);
            return -1;
        }
    }

    free(eeprom_data);
    eeprom_data = NULL;

    return rv;
}

static int sff8472_control_set(
    void *busDrvPtr, int bus, uint8_t dev,
    int control,
    int status)
{
    i2c_bus_driver_t *i2c = (i2c_bus_driver_t *)busDrvPtr;
    int rv = 0;
    uint8_t *eeprom_data;
    uint8_t value;

    if (control != SFP_CONTROL_TX_DISABLE)
    {
        AIM_LOG_ERROR("VENDOR: This control type is not writable in SFF8472");
        return -1;
    }

    sff_field_t field = get_sff8472_control_field(control);
    if (field == SFF_FIELD_MIN)
    {
        AIM_LOG_ERROR("VENDOR: This control type is not supported in SFF8472");
        return -1;
    }

    sff8472_field_map_t *info = (sff8472_field_map_t *)calloc(1, sizeof(sff8472_field_map_t));

    rv = get_sff8472_field(field, &info);
    if (rv != 0)
        return rv;

    eeprom_data = (uint8_t *)calloc(1, sizeof(uint8_t) * info->length);

    if (!sff8472_is_dom_supported(i2c, bus, dev))
    {
        /* get the control value from eeprom */
        rv = get_sff8472_value_from_eeprom(i2c, bus, dev, info, eeprom_data);

        /* value check */
        rv = filter_sff8472_control_value_for_set(control, eeprom_data, &status);
        if (rv != 0)
        {
            free(eeprom_data);
            return rv;
        }

        /* set the control value to eeprom */
        value = (uint8_t)status;
        rv = set_sff8472_value_to_eeprom(i2c, bus, dev, info, &value);
    }

    free(eeprom_data);
    eeprom_data = NULL;

    return rv;
}

/*================ internal function ================*/

static int get_sff8472_field(sff_field_t field, sff8472_field_map_t **info)
{

    int i, cnt = sizeof(sff8472_field_map) / sizeof(sff8472_field_map_t);

    if (field >= SFF_FIELD_MAX)
    {
        return -1;
    }

    for (i = 0; i < cnt; i++)
    {
        if (sff8472_field_map[i].field == field)
        {
            *info = &(sff8472_field_map[i]);
        }
    }

    if (!info)
    {
        AIM_LOG_ERROR("VENDOR: Cannot find sff8472 field!");
        return -1;
    }

    return 0;
}

static int get_sff8472_value_from_eeprom(i2c_bus_driver_t *i2c, int bus, uint8_t dev, sff8472_field_map_t *info, uint8_t *value)
{
    int rv = 0;

    rv = eeprom_readb(i2c, bus, info->address, info->offset, 1, value, info->length);

    return rv;
}

static int set_sff8472_value_to_eeprom(i2c_bus_driver_t *i2c, int bus, uint8_t dev, sff8472_field_map_t *info, uint8_t *value)
{
    int rv = 0;

    rv = eeprom_writeb(i2c, bus, info->address, info->offset, 1, value, info->length);

    return rv;
}

static sff_field_t get_sff8472_control_field(int control)
{
    switch (control)
    {
    case SFP_CONTROL_RX_LOS:
        return CHANNEL_LOS_INDICATOR;

    case SFP_CONTROL_TX_FAULT:
        return CHANNEL_TX_FAULT;

    case SFP_CONTROL_TX_DISABLE:
        return CHANNEL_TX_DISABLE;

    case SFP_CONTROL_TEMPERATURE:
        return TEMPERATURE;

    case SFP_CONTROL_VOLTAGE:
        return VCC;

    case SFP_CONTROL_TX_BIAS:
        return CHANNEL_TX_BIAS;

    case SFP_CONTROL_RX_POWER:
        return CHANNEL_RX_PWR;

    case SFP_CONTROL_TX_POWER:
        return CHANNEL_TX_PWR;

    default:
        return SFF_FIELD_MIN;
    }
}

/* SFP_CONTROL_RESET */       /* SUPPORT_BY_CPLD */
/* SFP_CONTROL_RESET_STATE */ /* SUPPORT_BY_CPLD */
/* SFP_CONTROL_RX_LOS */      /*R 3.0 - 3.3 */
/* SFP_CONTROL_TX_FAULT */    /*R 4.0 - 4.3 */
/* SFP_CONTROL_TX_DISABLE */  /*RW 86.0 - 86.3 */
/* SFP_CONTROL_TX_DISABLE_CHANNEL */
/* SFP_CONTROL_LP_MODE */         /* SUPPORT_BY_CPLD */
/* SFP_CONTROL_POWER_OVERRIDE, */ /*RW 93.0 */

static uint8_t filter_sff8472_control_value_for_get(
    int control,
    uint8_t *eeprom_data, int *combined_data)
{
    switch (control)
    {
    case SFP_CONTROL_TX_DISABLE:
        *combined_data = *eeprom_data & 0x40 ? 0x1 : 0x0;
        break;

    case SFP_CONTROL_RX_LOS:
        *combined_data = *eeprom_data & 0x02 ? 0x1 : 0x0;
        break;

    case SFP_CONTROL_TX_FAULT:
        *combined_data = *eeprom_data & 0x4 ? 0x1 : 0x0;
        break;

    case SFP_CONTROL_TEMPERATURE: /* Unit: Celsius */
        *combined_data = (eeprom_data[0] << 8 | eeprom_data[1]) / 256;
        break;

    case SFP_CONTROL_VOLTAGE: /* Unit: mV */
        *combined_data = (eeprom_data[0] << 8 | eeprom_data[1]) / 10;
        break;

    case SFP_CONTROL_TX_BIAS: /* Unit: uA */
        *combined_data = (eeprom_data[0] << 8 | eeprom_data[1]) * 2;
        break;

    case SFP_CONTROL_RX_POWER: /* Unit: uW */
        *combined_data = (eeprom_data[0] << 8 | eeprom_data[1]) / 10;
        break;

    case SFP_CONTROL_TX_POWER: /* Unit: uW */
        *combined_data = (eeprom_data[0] << 8 | eeprom_data[1]) / 10;
        break;

    default:
        return CONTROL_IS_NOT_SUPPORT;
    }

    return CONTROL_IS_SUPPORT;
}

static uint8_t filter_sff8472_control_value_for_set(
    int control,
    uint8_t *eeprom_data,
    int *write_status)
{
    int status = *write_status;
    uint8_t curr_status = *eeprom_data;
    switch (control)
    {
    case SFP_CONTROL_TX_DISABLE:
        if (status == 0)
        {
            *write_status = curr_status & ~0x40;
            return 0;
        }
        else if (status == 1)
        {
            *write_status = curr_status & ~0x40;
            *write_status |= 0x40;
            return 0;
        }
        else
        {
            AIM_LOG_ERROR("VENDOR:\t[SFF8472] Status of TX disable should be 0 or 1 ");
            return -1;
        }

    default:
        return CONTROL_IS_NOT_SUPPORT;
    }
}

static sfp_dev_driver_t sff8472_functions =
    {
        sff8472_eeprom_load,
        sff8472_eeprom_dom_load,
        sff8472_eeprom_read,
        sff8472_eeprom_write,
        sff8472_eeprom_readb,
        sff8472_eeprom_writeb,
        sff8472_eeprom_readw,
        sff8472_eeprom_writew,
        sff8472_control_is_support,
        sff8472_control_get,
        sff8472_control_set};

static int sff8472_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "SFF8472", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &sff8472_functions;
    vendor_driver_add(driver);

    return 0;
}
/*SFP DEVICE SFF8472 END*/

/*BMC DEVICE START*/

typedef enum bmc_command
{
    BMC_CMD_PSU_GET_PRESENT = 0,
    BMC_CMD_PSU_GET_POWER_GOOD,
    BMC_CMD_PSU_GET_MODEL_NAME,
    BMC_CMD_PSU_GET_SERIAL_NUM,
    BMC_CMD_PSU_GET_VOLTAGE,
    BMC_CMD_PSU_GET_CURRENT,
    BMC_CMD_FAN_GET_PRESENT,
    BMC_CMD_FAN_GET_RPM,
    BMC_CMD_FAN_PSU_GET_RPM,
    BMC_CMD_THERMAL_GET_TEMP,
    BMC_CMD_MAX
} bmc_command_t;

typedef enum bmc_command_type
{
    BMC_CMD_TYPE_NONE = 0,
    BMC_CMD_TYPE_STANDARD,
    BMC_CMD_TYPE_RAW,
    BMC_CMD_TYPE_RAW_PSU_STRING
} bmc_command_type_t;

typedef struct bmc_ipmi_command_info_s
{
    bmc_command_t command;
    bmc_command_type_t command_type;
    char *cmd;
    char *filter;
} bmc_ipmi_command_info_t;

/*
 *  The command array MUST 1-to-1 map to bmc_command_t
 */
static bmc_ipmi_command_info_t bmc_ipmi_command_info[] =
    {
        {BMC_CMD_PSU_GET_PRESENT,
         BMC_CMD_TYPE_NONE,
         NULL,
         NULL},
        {BMC_CMD_PSU_GET_POWER_GOOD,
         BMC_CMD_TYPE_NONE,
         NULL,
         NULL},
        {BMC_CMD_PSU_GET_MODEL_NAME,
         BMC_CMD_TYPE_RAW_PSU_STRING,
         "raw 0x3C 0xE0 0x11 %s 0x9A",
         ""},
        {BMC_CMD_PSU_GET_SERIAL_NUM,
         BMC_CMD_TYPE_RAW_PSU_STRING,
         "raw 0x3C 0xE0 0x11 %s 0x9E",
         ""},
        {BMC_CMD_PSU_GET_VOLTAGE,
         BMC_CMD_TYPE_RAW,
         "raw 0x3C 0xE0 0x11 %s 0x8B",
         ""},
        {BMC_CMD_PSU_GET_CURRENT,
         BMC_CMD_TYPE_RAW,
         "raw 0x3C 0xE0 0x11 %s 0x8C",
         ""},
        {BMC_CMD_FAN_GET_PRESENT,
         BMC_CMD_TYPE_RAW,
         "raw 0x3C 0xE0 0x05 %s 0x00",
         ""},
        {BMC_CMD_FAN_GET_RPM,
         BMC_CMD_TYPE_STANDARD,
         "sensor reading %s",
         "|cut -d '|' -f 2 |cut -d ' ' -f 2"},
        {BMC_CMD_FAN_PSU_GET_RPM,
         BMC_CMD_TYPE_RAW,
         "raw 0x3C 0xE0 0x11 %s 0x90",
         ""},
        {BMC_CMD_THERMAL_GET_TEMP,
         BMC_CMD_TYPE_STANDARD,
         "sensor reading %s",
         "|cut -d '|' -f 2 |cut -d ' ' -f 2"}};

static char *ipmi_psu_std_unit_id[] =
    {
        "1",
        "2"};
static char *ipmi_psu_raw_unit_id[] =
    {
        "0",
        "1"};
static char *ipmi_fan_std_unit_id[] =
    {
        "FanPWM_0",
        "FanPWM_1",
        "FanPWM_2",
        "FanPWM_3"};
static char *ipmi_fan_raw_unit_id[] =
    {
        "0",
        "1",
        "2",
        "3"};
static char *ipmi_thrml_unit_id[] =
    {
        "Temp_L0",
        "Temp_L1",
        "Temp_L2",
        "Temp_L3",
        "Temp_CPUB"};

static int bmc_addr_get_device_type(uint8_t addr)
{
    return addr >> 6;
}

static int bmc_addr_get_id(uint8_t addr)
{
    return addr & 0x3f;
}

static bmc_ipmi_command_info_t *get_bmc_command_info(bmc_command_t command)
{
    int i;

    for (i = 0; i < BMC_CMD_MAX; i++)
    {
        if (bmc_ipmi_command_info[i].command == command)
            return &bmc_ipmi_command_info[i];
    }

    return NULL;
}

//OK
static char *get_bmc_command_unit_id(int deviceType, int unit, bmc_ipmi_command_info_t *info)
{
    char *unit_id_str = NULL;

    if (deviceType == VENDOR_PSU)
    {
        if (info->command_type == BMC_CMD_TYPE_STANDARD)
            unit_id_str = ipmi_psu_std_unit_id[unit];
        else if ((info->command_type == BMC_CMD_TYPE_RAW) || (info->command_type == BMC_CMD_TYPE_RAW_PSU_STRING))
            unit_id_str = ipmi_psu_raw_unit_id[unit];
    }
    else if (deviceType == VENDOR_FAN)
    {
        if (info->command < BMC_CMD_FAN_PSU_GET_RPM)
        {
            if (info->command_type == BMC_CMD_TYPE_STANDARD)
                unit_id_str = ipmi_fan_std_unit_id[unit];
            else if ((info->command_type == BMC_CMD_TYPE_RAW) || (info->command_type == BMC_CMD_TYPE_RAW_PSU_STRING))
                unit_id_str = ipmi_fan_raw_unit_id[unit];
        }
        else
        {
            if (info->command_type == BMC_CMD_TYPE_STANDARD)
                unit_id_str = ipmi_psu_std_unit_id[unit];
            else if ((info->command_type == BMC_CMD_TYPE_RAW) || (info->command_type == BMC_CMD_TYPE_RAW_PSU_STRING))
                unit_id_str = ipmi_psu_raw_unit_id[unit];
        }
    }
    else if (deviceType == VENDOR_TEMPERATURE)
    {
        unit_id_str = ipmi_thrml_unit_id[unit];
    }
    else if (deviceType == VENDOR_PSU_PRESENT)
    {
        if (info->command_type == BMC_CMD_TYPE_STANDARD)
            unit_id_str = ipmi_psu_std_unit_id[unit];
        else if ((info->command_type == BMC_CMD_TYPE_RAW) || (info->command_type == BMC_CMD_TYPE_RAW_PSU_STRING))
            unit_id_str = ipmi_psu_raw_unit_id[unit];
    }
    else if (deviceType == VENDOR_FAN_PRESENT)
    {
        if (info->command_type == BMC_CMD_TYPE_STANDARD)
            unit_id_str = ipmi_fan_std_unit_id[unit];
        else if ((info->command_type == BMC_CMD_TYPE_RAW) || (info->command_type == BMC_CMD_TYPE_RAW_PSU_STRING))
            unit_id_str = ipmi_fan_raw_unit_id[unit];
    }

    return unit_id_str;
}

//OK
static int bmc_raw_data_process(char *bmcData)
{
    uint32_t result, value[4];

    if ((sscanf(bmcData, " %02X %02X %02X %02X %02X", &result,
                &value[0], &value[1], &value[2], &value[3]) != 5) ||
        (result != 0))
    {
        return 0;
    }
    else
    {
        result = (value[0] & 0xff);
        result |= ((value[1] & 0xff) << 8);
        result |= ((value[2] & 0xff) << 16);
        result |= ((value[3] & 0xff) << 24);
        sprintf(bmcData, "%u", result);

        return 0;
    }
}

static int bmc_raw_data_PSU_string(char *bmcData)
{
    uint32_t result, value[4];

    if ((sscanf(bmcData, " %02X %02X %02X %02X %02X", &result,
                &value[0], &value[1], &value[2], &value[3]) != 5) ||
        (result != 0))
    {
        ////AIM_LOG_ERROR("%s: Error: failed to convert first 5-byte of data[%s].", __func__, bmcData);
        return (-1);
    }


    result = (value[0] & 0xff);
    result |= ((value[1] & 0xff) << 8);
    result |= ((value[2] & 0xff) << 16);
    result |= ((value[3] & 0xff) << 24);

    if (result <= 1)
    {
        AIM_LOG_ERROR("%s: Error: length[%u] of string=[%s].", __func__, result, bmcData);
        return (-2);
	}


    uint32_t u32_loop = 0, u32_value[256];
    char *lpsz_ptr = bmcData+18;

    for (u32_loop = 0; u32_loop < (result - 1); u32_loop++)
    {
        if (sscanf(lpsz_ptr, " %02X ", &u32_value[u32_loop]) != 1)
        {
            AIM_LOG_ERROR("%s: Error: failed to convert string[%u]=[%s].\n", __func__, (result - 1), bmcData);
            return (-3);
        }

        bmcData[u32_loop] = (char)(u32_value[u32_loop]);
        lpsz_ptr += 3;

        if ('\n' == (*lpsz_ptr))
        {
            lpsz_ptr++;
        }
    }

    u32_value[u32_loop] = '\0';
    bmcData[u32_loop] = '\0';

    return 0;
}


#define M_BMC_SENSORS_FILE   "/run/BMC_sensors.txt"

#define FAN_TIME_THRESHOLD           (5)
#define THERMAL_TIME_THRESHOLD       (10)


#define M_TIME_2_SECONDS(timeval) ( (double)timeval.tv_sec + (double)(timeval.tv_usec / 1000.0 / 1000.0) )


void API_timer_gap(struct timeval *a, struct timeval *b, struct timeval *res)
{
    if (a->tv_sec > b->tv_sec)
    {
        timersub(a, b, res); 
    }
    else if (a->tv_sec < b->tv_sec)
    {
        timersub(b, a, res); 
    }
    else
    {
        if (a->tv_usec >= b->tv_usec) 
            timersub(a, b, res);
        else
            timersub(b, a, res);
    }
}


static int bmc_sensor_file_check(char *lpsz_filename, time_t time_threshold)
{
    struct stat st_stat;

    if (0 == stat(lpsz_filename, &st_stat))
    {
        if (0 == st_stat.st_size)
        {
            return (-1);
        }
        else
        {
            struct timeval st_current_time, st_file_time, st_time_gap;

            st_file_time.tv_sec = st_stat.st_mtim.tv_sec;
            st_file_time.tv_usec = (st_stat.st_mtim.tv_nsec / 1000);

            gettimeofday(&st_current_time, NULL);
            API_timer_gap(&st_current_time, &st_file_time, &st_time_gap);

            if (time_threshold < st_time_gap.tv_sec)
            {
                return (-2);
            }
            else
            {
                return (0);
            }
        }
    }
    else
    {
        return (-3);
    }
}


static int bmc_sensor_file_update(time_t time_threshold)
{
    int s32_return = 0;

    if (0 != bmc_sensor_file_check(M_BMC_SENSORS_FILE, time_threshold))
    {
        pid_t pid;
        char sz_cmd[256] = {0}, rv_char[256] = {0};

        pid = getpid();

        sprintf(sz_cmd, "ipmitool sdr > /run/BMC_s_%lu.txt", (unsigned long)pid);
        if (vendor_system_call_get(sz_cmd, rv_char) != 0)
        {
            AIM_LOG_ERROR("%s:%d: failed with command[%s].\n", __func__, __LINE__, sz_cmd);
            s32_return = -1;
        }
        else
        {
            sprintf(sz_cmd, "mv /run/BMC_s_%lu.txt %s", (unsigned long)pid, M_BMC_SENSORS_FILE);
            if (vendor_system_call_get(sz_cmd, rv_char) != 0)
            {
                AIM_LOG_ERROR("%s:%d: failed with command[%s].\n", __func__, __LINE__, sz_cmd);
                s32_return = -2;
            }
        }
    }


    return (s32_return);
}


static int bmc_sensor_get(char *lpsz_device_name, time_t time_threshold, char *lpsz_buf, int dlen)
{
    char sz_cmd[256] = {0}, sz_data[256] = {0};
    int rv = ONLP_STATUS_OK;

    if (0 != bmc_sensor_file_check(M_BMC_SENSORS_FILE, time_threshold))
    {
        bmc_sensor_file_update(time_threshold);
    }


    sprintf(sz_cmd, "cat %s | grep %s | awk -F'|' '{print $2}' | awk -F' ' '{ print $1}'", M_BMC_SENSORS_FILE, lpsz_device_name);
    if (vendor_system_call_get(sz_cmd, sz_data) != 0)
    {
        AIM_LOG_ERROR("%s:%d: failed with command[%s].\n", __func__, __LINE__, sz_cmd);
        return (-1);
    }
    else
    {
        memcpy(lpsz_buf, sz_data, (dlen-1));
        lpsz_buf[dlen-1] = '\0';
    }

    return rv;
}


static int _bmc_data_get(
    ipmi_bus_driver_t *ipmi,
    int deviceType,
    int id,
    int bus,
    void *command_info,
    char *bmcData,
    int dataLen)
{
    int rc = 0;

    if (!command_info || !bmcData)
        return 0;

    bmc_ipmi_command_info_t *info = NULL;
    char *unit_id_str = NULL;
    char cmd[32] = {0};

    info = (bmc_ipmi_command_info_t *)command_info;
    if (!info->cmd || !info->filter)
        return 0;

    if ((unit_id_str = get_bmc_command_unit_id(deviceType, id, info)) == NULL)
    {
        AIM_LOG_ERROR("Can't get unit id for deviceType %d!", deviceType);
        return 0;
    }

    if (BMC_CMD_FAN_GET_RPM == info->command)
    {
        rc = bmc_sensor_get(unit_id_str, FAN_TIME_THRESHOLD, bmcData, dataLen);
    }
    else if (BMC_CMD_THERMAL_GET_TEMP == info->command)
    {
        rc = bmc_sensor_get(unit_id_str, THERMAL_TIME_THRESHOLD, bmcData, dataLen);
    }
    else
    {
        sprintf(cmd, info->cmd, unit_id_str);

        rc = ipmi->get(bus,
                   cmd,
                   info->filter,
                   bmcData,
                   dataLen);
        //AIM_LOG_ERROR("Issue ipmitool %s %s => %s", cmd, info->filter, bmcData);
    }

    if ((rc == 0) && (info->command_type == BMC_CMD_TYPE_RAW))
        rc = bmc_raw_data_process(bmcData);

    if ((rc == 0) && (info->command_type == BMC_CMD_TYPE_RAW_PSU_STRING))
        rc = bmc_raw_data_PSU_string(bmcData);

    return rc;
}

int bmc_data_get(
    ipmi_bus_driver_t *ipmi,
    int deviceType,
    int id,
    int bus,
    bmc_command_t command,
    char *bmcData,
    int dataLen)
{
    void *info = NULL;

    info = get_bmc_command_info(command);
    return _bmc_data_get(ipmi, deviceType, id, bus, info, bmcData, dataLen);
}

static int bmc_psu_model_get(
    void *busDrvPtr,
    int bus,
    uint8_t addr,
    char *model)
{
    ipmi_bus_driver_t *ipmi = (ipmi_bus_driver_t *)busDrvPtr;
    int rc = 0;
    char buf[256] = {0};
    int len = 0;
    int deviceType = bmc_addr_get_device_type(addr);
    int id = bmc_addr_get_id(addr);

    if (!model)
        return 0;

    if ((rc = bmc_data_get(ipmi, deviceType, id, bus, BMC_CMD_PSU_GET_MODEL_NAME, buf, sizeof(buf))) == 0)
    {
        len = strlen(buf);
        if (buf[len - 1] == '\n')
        {
            buf[len - 1] = '\0';
            len--;
        }
        strcpy(model, buf);
    }

    return rc;
}

static int bmc_psu_serial_get(
    void *busDrvPtr,
    int bus,
    uint8_t addr,
    char *serial)
{
    ipmi_bus_driver_t *ipmi = (ipmi_bus_driver_t *)busDrvPtr;
    int rc = 0;
    char buf[256] = {0};
    int len = 0;
    int deviceType = bmc_addr_get_device_type(addr);
    int id = bmc_addr_get_id(addr);

    if (!serial)
        return 0;

    if ((rc = bmc_data_get(ipmi, deviceType, id, bus, BMC_CMD_PSU_GET_SERIAL_NUM, buf, sizeof(buf))) == 0)
    {
        len = strlen(buf);
        if (buf[len - 1] == '\n')
        {
            buf[len - 1] = '\0';
            len--;
        }
        strcpy(serial, buf);
    }

    return rc;
}

static int bmc_psu_volt_get(
    void *busDrvPtr,
    int bus,
    uint8_t addr,
    int *volt)
{
    ipmi_bus_driver_t *ipmi = (ipmi_bus_driver_t *)busDrvPtr;
    int rc = 0;
    char buf[256] = {0};
    int deviceType = bmc_addr_get_device_type(addr);
    int id = bmc_addr_get_id(addr);

    if (!volt)
        return 0;

    if ((rc = bmc_data_get(ipmi, deviceType, id, bus, BMC_CMD_PSU_GET_VOLTAGE, buf, sizeof(buf))) == 0)
    {
        *volt = atoi(buf);
    }

    return rc;
}

static int bmc_psu_amp_get(
    void *busDrvPtr,
    int bus,
    uint8_t addr,
    int *amp)
{
    ipmi_bus_driver_t *ipmi = (ipmi_bus_driver_t *)busDrvPtr;
    int rv = 0;
    char buf[256] = {0};
    int deviceType = bmc_addr_get_device_type(addr);
    int id = bmc_addr_get_id(addr);
    if (!amp)
        return 0;
    if ((rv = bmc_data_get(ipmi, deviceType, id, bus, BMC_CMD_PSU_GET_CURRENT, buf, sizeof(buf))) == 0)
    {
        *amp = atoi(buf);
    }
    return rv;
}

static int bmc_psu_watt_get(
    void *busDrvPtr,
    int bus,
    uint8_t addr,
    int *watt)
{
    ipmi_bus_driver_t *ipmi = (ipmi_bus_driver_t *)busDrvPtr;
    int rc = 0;
    char buf[256] = {0};
    int deviceType = bmc_addr_get_device_type(addr);
    int id = bmc_addr_get_id(addr);

    if (!watt)
        return 0;

    if ((rc = bmc_data_get(ipmi, deviceType, id, bus, BMC_CMD_PSU_GET_VOLTAGE, buf, sizeof(buf))) == 0)
    {
        *watt = atoi(buf);
    }

    if ((rc = bmc_data_get(ipmi, deviceType, id, bus, BMC_CMD_PSU_GET_CURRENT, buf, sizeof(buf))) == 0)
    {
        *watt *= atoi(buf);
        *watt /= 1000;
    }

    return rc;
}

static int bmc_psu_runtime_info_get(
    void *busDrvPtr,
    int bus,
    uint8_t addr,
    vendor_psu_runtime_info_t *runtimeInfo)
{
    int rv = 0;
    vendor_psu_parm_type_t parmType = PSU_PARM_MIN;

    for (parmType = PSU_PARM_MIN; parmType <= PSU_PARM_MAX; parmType++)
    {
        switch (parmType)
        {
        case PSU_PARM_VIN:
            runtimeInfo->vin = 0;
            break;
        case PSU_PARM_IIN:
            runtimeInfo->iin = 0;
            break;
        case PSU_PARM_VOUT:
            bmc_psu_volt_get(busDrvPtr, bus, addr, &runtimeInfo->vout);
            break;
        case PSU_PARM_IOUT:
            bmc_psu_amp_get(busDrvPtr, bus, addr, &runtimeInfo->iout);
            break;
        case PSU_PARM_POUT:
            bmc_psu_watt_get(busDrvPtr, bus, addr, &runtimeInfo->pout);
            break;
        case PSU_PARM_PIN:
            runtimeInfo->pin = 0;
            break;

        default:
            AIM_LOG_ERROR("VENDOR: Wrong parm type in PMBUS driver!");
            return ONLP_STATUS_E_INTERNAL;
        }
    }

    return rv;
}

static int bmc_fan_rpm_get(
    void *busDrvPtr,
    int bus,
    uint8_t addr,
    int noused,
    int *rpm)
{
    ipmi_bus_driver_t *ipmi = (ipmi_bus_driver_t *)busDrvPtr;
    int rc = 0;
    char buf[256] = {0};
    int deviceType = bmc_addr_get_device_type(addr);
    int id = bmc_addr_get_id(addr);

    if ((rc = bmc_data_get(ipmi, deviceType, id, bus, BMC_CMD_FAN_GET_RPM, buf, sizeof(buf))) == 0)
    {
        *rpm = (uint16_t)atoi(buf);
    }

    return rc;
}

static int bmc_fan_rpm_set(
    void *busDrvPtr,
    int bus,
    uint8_t addr,
    int noused,
    int rpm)
{
    int rc = 0;
    AIM_LOG_ERROR("It is not supported in BMC.");
    return rc;
}

static int fan_psu_rpm_get(
    void *busDrvPtr,
    int bus,
    uint8_t addr,
    int noused,
    int *rpm)
{
    ipmi_bus_driver_t *ipmi = (ipmi_bus_driver_t *)busDrvPtr;
    int rc = 0;
    char buf[256] = {0};
    int deviceType = bmc_addr_get_device_type(addr);
    int id = bmc_addr_get_id(addr);

    if ((rc = bmc_data_get(ipmi, deviceType, id, bus, BMC_CMD_FAN_PSU_GET_RPM, buf, sizeof(buf))) == 0)
    {
        *rpm = (uint16_t)atoi(buf);
    }

    return rc;
}

static int fan_psu_rpm_set(
    void *busDrvPtr,
    int bus,
    uint8_t addr,
    int noused,
    int rpm)
{
    AIM_LOG_ERROR("It is not supported in BMC.");
    return 0;
}

static int bmc_thrml_temp_get(
    void *busDrvPtr,
    int bus,
    uint8_t addr,
    int noused,
    int *temperature)
{
    ipmi_bus_driver_t *ipmi = (ipmi_bus_driver_t *)busDrvPtr;
    int rc = 0;
    char buf[256] = {0};
    int deviceType = bmc_addr_get_device_type(addr);
    int id = bmc_addr_get_id(addr);

    if (!temperature)
        return 0;

    if ((rc = bmc_data_get(ipmi, deviceType, id, bus, BMC_CMD_THERMAL_GET_TEMP, buf, sizeof(buf))) == 0)
    {
        *temperature = atoi(buf) * 1000;
    }

    return rc;
}

static int bmc_thrml_limit_get(
    void *busDrvPtr,
    int bus,
    uint8_t addr,
    int noused,
    int type,
    int *temperature)
{
    return 0;
}

static int bmc_thrml_limit_set(
    void *busDrvPtr,
    int bus,
    uint8_t addr,
    int noused,
    int type,
    int temperature)
{
    return 0;
}
static int bmc_present_get(
    void *busDrvPtr,
    int bus,
    uint8_t addr,
    uint8_t offset,
    uint8_t *present)
{
    ipmi_bus_driver_t *ipmi = (ipmi_bus_driver_t *)busDrvPtr;
    bmc_command_t command;
    int rc = 0;
    char buf[256] = {0};
    int deviceType = bmc_addr_get_device_type(addr);
    int id = bmc_addr_get_id(addr);

    if (!present)
        return 0;

    if (deviceType == VENDOR_PSU_PRESENT)
        command = BMC_CMD_PSU_GET_PRESENT;
    else if (deviceType == VENDOR_FAN_PRESENT)
        command = BMC_CMD_FAN_GET_PRESENT;
    else
    {
        AIM_LOG_ERROR("Unknown BMC deviceType: %d!", deviceType);
        return 0;
    }

    if ((rc = bmc_data_get(ipmi, deviceType, id, bus, command, buf, sizeof(buf))) == 0)
    {
        if (buf[0] == '\0')
            *present = 0;
        else
        {
            if ((command == BMC_CMD_FAN_GET_PRESENT) && !atoi(buf))
                *present = 0;
            else
                *present = 1;
        }
    }

    return rc;
}

static psu_dev_driver_t bmc_psu_functions = {
    bmc_psu_model_get,
    bmc_psu_serial_get,
    bmc_psu_volt_get,
    bmc_psu_amp_get,
    bmc_psu_watt_get,
    bmc_psu_runtime_info_get};

static fan_dev_driver_t bmc_fan_functions = {
    bmc_fan_rpm_get,
    bmc_fan_rpm_set,
};

static fan_dev_driver_t bmc_fan_psu_functions = {
    fan_psu_rpm_get,
    fan_psu_rpm_set,
};

static thermal_dev_driver_t bmc_thrml_functions = {
    bmc_thrml_temp_get,
    bmc_thrml_limit_get,
    bmc_thrml_limit_set};

static status_get_driver_t bmc_stat_functions = {
    bmc_present_get};

int bmc_psu_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "BMC_PSU", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &bmc_psu_functions;

    return vendor_driver_add(driver);
}

int bmc_fan_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "BMC_FAN", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &bmc_fan_functions;

    return vendor_driver_add(driver);
}

int bmc_psu_fan_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "BMC_PSU_FAN", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &bmc_fan_psu_functions;

    return vendor_driver_add(driver);
}

int bmc_thrml_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "BMC_TMP", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &bmc_thrml_functions;

    return vendor_driver_add(driver);
}

int bmc_present_get_driver_init()
{
    vendor_driver_t *driver = (vendor_driver_t *)calloc(1, sizeof(vendor_driver_t));

    strncpy(driver->name, "BMC_STAT", VENDOR_MAX_NAME_SIZE);
    driver->dev_driver = &bmc_stat_functions;

    return vendor_driver_add(driver);
}
/*BMC DEVICE END*/

/*
    DEVICE DRIVER END:
*/

/*
    VENDOR FUNCTION START
*/
int vendor_dev_do_oc(vendor_dev_oc_t *dev_oc)
{
    //AIM_LOG_ERROR("Function: %s", __FUNCTION__);

    i2c_bus_driver_t *i2c = NULL;

    int rv = 0;
    uint16_t data;

    if (dev_oc == NULL)
    {
        /*NO NEED */
        return rv;
    }

    while (dev_oc->type != 0)
    {
        i2c = (i2c_bus_driver_t *)vendor_find_driver_by_name(dev_oc->bus_drv_name);

        if (dev_oc->type == 1)
        {
            rv = i2c->set(dev_oc->bus, dev_oc->dev, dev_oc->addr, 1, dev_oc->value, 1);
        }
        else if (dev_oc->type == 2)
        {
            rv = i2c->get(dev_oc->bus, dev_oc->dev, dev_oc->addr, 1, &data, 1);
            if (rv < 0)
                return rv;

            data &= ~dev_oc->mask;
            data |= (dev_oc->match & dev_oc->mask);

            rv = i2c->set(dev_oc->bus, dev_oc->dev, dev_oc->addr, 1, data, 1);
        }
        else
        {
            AIM_LOG_ERROR("Function: %s, unknown oc type. \n", __FUNCTION__);
            rv = -1;
        }

        if (rv < 0)
            return rv;

        dev_oc++;
    }

    return rv;
}

int vendor_find_cpld_idx_by_name(char *name)
{
    int idx = 0;

    while (idx < cpld_list_size)
    {
        if (!strncmp(cpld_dev_list[idx].dev_name, name, VENDOR_MAX_NAME_SIZE))
        {
            return idx;
        }
        idx++;
    }

    AIM_LOG_ERROR("Function: %s, Cannot find CPLD index.", __FUNCTION__);
    return -1;
}

int vendor_get_status(vendor_dev_io_pin_t *io_pin, int *active)
{
    int rv = 0, cpld_idx = 0;
    void *busDrv = NULL;
    status_get_driver_t *pg = NULL;

    if (io_pin->type == CPLD_DEV)
    {
        if (io_pin->addr == 0)
        {
            *active = 1;
            return 0;
        }

        busDrv = (void *)vendor_find_driver_by_name(io_pin->bus_drv_name);
        pg = (status_get_driver_t *)vendor_find_driver_by_name("CPLD");

        cpld_idx = vendor_find_cpld_idx_by_name(io_pin->name);
        if (cpld_idx < 0)
            return ONLP_STATUS_E_INTERNAL;

        vendor_dev_do_oc(cpld_o_list[cpld_idx]);
    }
    else if (io_pin->type == BMC_DEV)
    {
        busDrv = (void *)vendor_find_driver_by_name(io_pin->bus_drv_name);
        pg = (status_get_driver_t *)vendor_find_driver_by_name("BMC_STAT");
    }
    else
    {
        /* 
            If there is no present status on fan, default to IS_PRESENT.
            Due to some platform has internal fan which you cannot pull out
            and it always present in system.
        */
        *active = 1;
        return 0;
    }

    rv = pg->status_get(
        busDrv,
        io_pin->bus,
        io_pin->dev,
        io_pin->addr,
        (uint8_t *)active);

    if (io_pin->type == CPLD_DEV)
    {
        vendor_dev_do_oc(cpld_c_list[cpld_idx]);
        *active = ((*active & io_pin->mask) == io_pin->match) ? 1 : 0;
    }

    return rv;
}

int vendor_system_call_get(char *cmd, char *data)
{
    FILE *fp;
    char buf[256] = {0};
    int c;

    fp = popen(cmd, "r");
    if (!fp)
        return -1;

    while (fread(buf, sizeof(buf), 1, fp))
        while ((c = getchar()) != EOF)
            printf("output = %s", buf);

    memcpy(data, buf, sizeof(buf));

    return WEXITSTATUS(pclose(fp));
}

int vendor_system_call_set(char *cmd)
{
    return system(cmd);
}

void vendor_remove_unbind_eeprom()
{
    char onie_addr[VENDOR_MAX_NAME_SIZE], cmd[256];

    if (eeprom_list_size != 0)
    {
        sprintf(onie_addr, "%d-00%x", eeprom_dev_list[0].bus, eeprom_dev_list[0].dev);
        sprintf(cmd, "ls /sys/bus/i2c/devices/%s/eeprom > /dev/null 2>&1", onie_addr);

        vendor_system_call_set(cmd);
        if (vendor_system_call_get("echo $?", cmd) == 0)
        {
            sprintf(cmd, "echo -n %s > /sys/bus/i2c/drivers/eeprom/unbind  > /dev/null 2>&1", onie_addr);
            vendor_system_call_set(cmd);
        }
    }
}

static vendor_driver_node_t *driver_list_head, *driver_list_curr;

static int vendor_driver_add(vendor_driver_t *driver)
{
    vendor_driver_node_t *newnode =
        (vendor_driver_node_t *)calloc(1, sizeof(vendor_driver_node_t));

    newnode->driver_hdl = driver;

    if (!driver_list_head)
    {
        driver_list_head = driver_list_curr = newnode;
    }
    else
    {
        driver_list_curr->node = newnode;
        driver_list_curr = newnode;
    }

    return 0;
}

void *vendor_find_driver_by_name(const char *driver_name)
{
    vendor_driver_node_t *driver_node = driver_list_head;

    while (driver_node)
    {
        if (strncmp(
                ((vendor_driver_t *)((vendor_driver_node_t *)driver_node->driver_hdl))->name,
                driver_name,
                VENDOR_MAX_NAME_SIZE) == 0)
        {
            return (void *)(driver_node->driver_hdl->dev_driver);
        }
        driver_node = driver_node->node;
    }

    AIM_LOG_ERROR("Function: %s, Cannot find driver %s.", __FUNCTION__, driver_name);

    return NULL;
}

int vendor_driver_init()
{
    smbus_driver_init();
    ipmi_driver_init();
    cpld_driver_init();
    eeprom_driver_init();
    emc2305_driver_init();
    pfm0812_driver_init();
    tmp75_driver_init();
    tmp461_driver_init();
    lm75_driver_init();
    adm1032_driver_init();
    pmbus_psu_driver_init();
    pmbus_fan_driver_init();
    sff8636_driver_init();
    sff8472_driver_init();
    ipmb_driver_init();
    bmc_psu_driver_init();
    bmc_fan_driver_init();
    bmc_psu_fan_driver_init();
    bmc_thrml_driver_init();
    bmc_present_get_driver_init();

    vendor_remove_unbind_eeprom();

#ifdef CYPRESS
    cypress_i2c_driver_init();
    CyLibraryInit();
#endif

    return 0;
}
/*
    VENDOR DRIVER FUNCTION END
*/
