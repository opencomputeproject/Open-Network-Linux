/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2020 Alpha Networks Incorporation.
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
 ************************************************************
 *
 *
 *
 ***********************************************************/
#include <onlp/platformi/sfpi.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <onlplib/i2c.h>
#include "platform_lib.h"

#define DEBUG                           0

#define PCA9539_NUM1_I2C_BUS_ID         21
#define PCA9539_NUM3_I2C_BUS_ID         11
#define PCA9539_NUM4_I2C_BUS_ID         16

#define PCA9539_NUM1_I2C_ADDR           0x76  /* PCA9539#1 is used for XFP 1~4 ports */
#define PCA9539_NUM3_I2C_ADDR           0x77  /* PCA9539#3 is used for QSFP28 ports */
#define PCA9539_NUM4_I2C_ADDR           0x74  /* PCA9539#4 is used for XFP 5~8 ports */

#define PCA9539_IO_INPUT    1
#define PCA9539_IO_OUTPUT   0

#define PCA9539_IO0_INPUT_OFFSET       0x00
#define PCA9539_IO1_INPUT_OFFSET       0x01
#define PCA9539_IO0_OUTPUT_OFFSET      0x02
#define PCA9539_IO1_OUTPUT_OFFSET      0x03


/* configures the directions of the I/O pin. 1:input, 0:output */
#define PCA9539_IO0_DIRECTION_OFFSET   0x06 
#define PCA9539_IO1_DIRECTION_OFFSET   0x07

#define PCA9539_XFP_1_3_5_7_TXDIS_BIT      0  /* TX_DISB bit for XFP1, XFP3, XFP5 and XFP7 on PCA9539#1 or #4 */
#define PCA9539_XFP_1_3_5_7_TXFAULT_BIT    1
#define PCA9539_XFP_1_3_5_7_PRESENT_BIT    2

#define PCA9539_XFP_2_4_6_8_TXDIS_BIT      4  /* TX_DISB bit for XFP2, XFP4, XFP6 and XFP8 on PCA9539#1 or #4 */
#define PCA9539_XFP_2_4_6_8_TXFAULT_BIT    5
#define PCA9539_XFP_2_4_6_8_PRESENT_BIT    6

#define PCA9539_QSFP_LPMODE_BIT            0  /* PCA9539#3 */
#define PCA9539_QSFP_INT_BIT               1
#define PCA9539_QSFP_MODPRS_BIT            2 
#define PCA9539_QSFP_RESET_BIT             3

enum sfp_port
{
	SFP_PORT_10G_XFP_1 = 0,
	SFP_PORT_10G_XFP_2 = 1,
	SFP_PORT_10G_XFP_3 = 2,
	SFP_PORT_10G_XFP_4 = 3,
	SFP_PORT_10G_XFP_5 = 4,
	SFP_PORT_10G_XFP_6 = 5,
	SFP_PORT_10G_XFP_7 = 6,
	SFP_PORT_10G_XFP_8 = 7,		
	SFP_PORT_100G_QSFP28_1 = 8,
	SFP_PORT_100G_QSFP28_2 = 9
};

static int
port_to_pca9539_busid(int port)
{
    int ret = 0;

    if ((port < SFP_PORT_10G_XFP_1) || (port > SFP_PORT_100G_QSFP28_2))
    {
        AIM_LOG_INFO("%s:%d port[%d] is out of range\n", __FUNCTION__, __LINE__, port);
        return ret;
    }

    switch (port)
    {
        case SFP_PORT_10G_XFP_1:
        case SFP_PORT_10G_XFP_2:
        case SFP_PORT_10G_XFP_3:
        case SFP_PORT_10G_XFP_4:
            ret = PCA9539_NUM1_I2C_BUS_ID;
            break;

        case SFP_PORT_10G_XFP_5:
        case SFP_PORT_10G_XFP_6:
        case SFP_PORT_10G_XFP_7:
        case SFP_PORT_10G_XFP_8:
            ret = PCA9539_NUM4_I2C_BUS_ID;
            break;

        case SFP_PORT_100G_QSFP28_1:
        case SFP_PORT_100G_QSFP28_2:
            ret = PCA9539_NUM3_I2C_BUS_ID;
            break;
        
        default:
            break;        
    }
    
    DIAG_PRINT("%s, port:%d, busid:%d ", __FUNCTION__, port, ret);
    return ret;
}

static int
port_to_pca9539_addr(int port)
{
    int ret = 0;

    if ((port < SFP_PORT_10G_XFP_1) || (port > SFP_PORT_100G_QSFP28_2))
    {
        AIM_LOG_INFO("%s:%d port[%d] is out of range\n", __FUNCTION__, __LINE__, port);
        return ret;
    }

    switch (port)
    {
        case SFP_PORT_10G_XFP_1:
        case SFP_PORT_10G_XFP_2:
        case SFP_PORT_10G_XFP_3:
        case SFP_PORT_10G_XFP_4:
            ret = PCA9539_NUM1_I2C_ADDR;
            break;

        case SFP_PORT_10G_XFP_5:
        case SFP_PORT_10G_XFP_6:
        case SFP_PORT_10G_XFP_7:
        case SFP_PORT_10G_XFP_8:
            ret = PCA9539_NUM4_I2C_ADDR;
            break;

        case SFP_PORT_100G_QSFP28_1:
        case SFP_PORT_100G_QSFP28_2:
            ret = PCA9539_NUM3_I2C_ADDR;
            break;
        
        default:
            break;        
    }
    
    DIAG_PRINT("%s, port:%d, pca9539_addr:%d ", __FUNCTION__, port, ret);
    return ret;
}

static int
port_to_pca9539_sfp_offset(int port, int direction)
{
    int ret = 0;

    if (direction == PCA9539_IO_INPUT)
    {
        switch (port)
        {
            case SFP_PORT_10G_XFP_1:
            case SFP_PORT_10G_XFP_2:
            case SFP_PORT_10G_XFP_5:
            case SFP_PORT_10G_XFP_6:
            case SFP_PORT_100G_QSFP28_1:
                ret = PCA9539_IO0_INPUT_OFFSET;
                break;

            case SFP_PORT_10G_XFP_3:
            case SFP_PORT_10G_XFP_4:
            case SFP_PORT_10G_XFP_7:
            case SFP_PORT_10G_XFP_8:
            case SFP_PORT_100G_QSFP28_2:
                ret = PCA9539_IO1_INPUT_OFFSET;
                break;
               
            default:
                break;        
        }
    }
    else if (direction == PCA9539_IO_OUTPUT)
    {
        switch (port)
        {
            case SFP_PORT_10G_XFP_1:
            case SFP_PORT_10G_XFP_2:
            case SFP_PORT_10G_XFP_5:
            case SFP_PORT_10G_XFP_6:
            case SFP_PORT_100G_QSFP28_1:
                ret = PCA9539_IO0_OUTPUT_OFFSET;
                break;

            case SFP_PORT_10G_XFP_3:
            case SFP_PORT_10G_XFP_4:
            case SFP_PORT_10G_XFP_7:
            case SFP_PORT_10G_XFP_8:
            case SFP_PORT_100G_QSFP28_2:
                ret = PCA9539_IO1_OUTPUT_OFFSET;
                break;
               
            default:
                break;        
        }
    }

    DIAG_PRINT("%s, port:%d, direction %d, offset:0x%X", __FUNCTION__, port, direction, ret);
    return ret;
}

static int
port_to_pca9539_txdis_bit(int port)
{
    int index = 0;

    switch (port)
    {
        case SFP_PORT_10G_XFP_1:
        case SFP_PORT_10G_XFP_3:
        case SFP_PORT_10G_XFP_5:
        case SFP_PORT_10G_XFP_7:
            index = PCA9539_XFP_1_3_5_7_TXDIS_BIT;
            break;

        case SFP_PORT_10G_XFP_2:
        case SFP_PORT_10G_XFP_4:
        case SFP_PORT_10G_XFP_6:
        case SFP_PORT_10G_XFP_8:
            index = PCA9539_XFP_2_4_6_8_TXDIS_BIT;
            break;
           
        default:
            break;        
    }
    
    DIAG_PRINT("%s, port:%d, index:%d ", __FUNCTION__, port, index);
    return index;
}

static int
port_to_pca9539_txfault_bit(int port)
{
    int index = 0;

    switch (port)
    {
        case SFP_PORT_10G_XFP_1:
        case SFP_PORT_10G_XFP_3:
        case SFP_PORT_10G_XFP_5:
        case SFP_PORT_10G_XFP_7:
            index = PCA9539_XFP_1_3_5_7_TXFAULT_BIT;
            break;

        case SFP_PORT_10G_XFP_2:
        case SFP_PORT_10G_XFP_4:
        case SFP_PORT_10G_XFP_6:
        case SFP_PORT_10G_XFP_8:
            index = PCA9539_XFP_2_4_6_8_TXFAULT_BIT;
            break;
           
        default:
            break;        
    }
    
    DIAG_PRINT("%s, port:%d, index:%d ", __FUNCTION__, port, index);
    return index;
}

static int
port_to_pca9539_present_bit(int port)
{
    int index = 0;

    switch (port)
    {
        case SFP_PORT_10G_XFP_1:
        case SFP_PORT_10G_XFP_3:
        case SFP_PORT_10G_XFP_5:
        case SFP_PORT_10G_XFP_7:
            index = PCA9539_XFP_1_3_5_7_PRESENT_BIT;
            break;

        case SFP_PORT_10G_XFP_2:
        case SFP_PORT_10G_XFP_4:
        case SFP_PORT_10G_XFP_6:
        case SFP_PORT_10G_XFP_8:
            index = PCA9539_XFP_2_4_6_8_PRESENT_BIT;
            break;

        case SFP_PORT_100G_QSFP28_1:
        case SFP_PORT_100G_QSFP28_2:
            index = PCA9539_QSFP_MODPRS_BIT;
            break;
           
        default:
            break;        
    }
    
    DIAG_PRINT("%s, port:%d, index:%d ", __FUNCTION__, port, index);
    return index;
}

static int
port_to_pca9539_lpmode_bit(int port)
{
    int index = 0;

    switch (port)
    {
        case SFP_PORT_100G_QSFP28_1:
        case SFP_PORT_100G_QSFP28_2:
            index = PCA9539_QSFP_LPMODE_BIT;
            break;

        default:
            break;        
    }
    
    DIAG_PRINT("%s, port:%d, index:%d ", __FUNCTION__, port, index);
    return index;
}

static int
port_to_pca9539_reset_bit(int port)
{
    int index = 0;

    switch (port)
    {
        case SFP_PORT_100G_QSFP28_1:
        case SFP_PORT_100G_QSFP28_2:
            index = PCA9539_QSFP_RESET_BIT;
            break;

        default:
            break;        
    }
    
    DIAG_PRINT("%s, port:%d, index:%d ", __FUNCTION__, port, index);
    return index;
}

static int
spx70d0_sfp_present(int port)
{
    int data = 0;
    int pca9539_bus = 0;
    int pca9539_addr = 0;
    int offset = 0;

    DIAG_PRINT("%s, port:%d", __FUNCTION__, port);
    
    pca9539_bus = port_to_pca9539_busid(port);
    pca9539_addr = port_to_pca9539_addr(port);
    offset = port_to_pca9539_sfp_offset(port, PCA9539_IO_INPUT);

    data = onlp_i2c_readb(pca9539_bus, pca9539_addr, offset, ONLP_I2C_F_FORCE);
    if (data < 0)
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, data);
        return data;
    }

    if (DEBUG)
        AIM_LOG_INFO("%s:%d port[%d],busid[%d],addr[%02x],offset[%02x],read_byte[%02x]\n", __FUNCTION__, __LINE__,
                      port, pca9539_bus, pca9539_addr, offset, (unsigned char)data);

    /* "0" indicates Module Present. */
    if (!(data & (1 << port_to_pca9539_present_bit(port))))
    {
        /*
         * Return 1 if present.
         * Return 0 if not present.
         * Return < 0 if error.
         */
        return 1;
    }

    return 0;
}

static int
spx70d0_sfp_pca9539_direction_set(int IO_port)
{
    int offset = 0;
    char data = 0;
    int ret = 0;

    if (IO_port == 0)
        offset = PCA9539_IO0_DIRECTION_OFFSET;
    else
        offset = PCA9539_IO1_DIRECTION_OFFSET;

    /* 1. Configure the direction of PCA9539#1 */
    data = onlp_i2c_readb(PCA9539_NUM1_I2C_BUS_ID, PCA9539_NUM1_I2C_ADDR, offset, ONLP_I2C_F_FORCE);

    /* configuration direction to output, input:1, output:0.  */
    data &= ~(1 << PCA9539_XFP_1_3_5_7_TXDIS_BIT);
    data &= ~(1 << PCA9539_XFP_2_4_6_8_TXDIS_BIT);

    ret = onlp_i2c_writeb(PCA9539_NUM1_I2C_BUS_ID, PCA9539_NUM1_I2C_ADDR, offset, data, ONLP_I2C_F_FORCE);
    if (ret < 0)
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    /* 2. Configure the direction of PCA9539#4 */
    data = onlp_i2c_readb(PCA9539_NUM4_I2C_BUS_ID, PCA9539_NUM4_I2C_ADDR, offset, ONLP_I2C_F_FORCE);

    /* configuration direction to output, input:1, output:0.  */
    data &= ~(1 << PCA9539_XFP_1_3_5_7_TXDIS_BIT);
    data &= ~(1 << PCA9539_XFP_2_4_6_8_TXDIS_BIT);

    ret = onlp_i2c_writeb(PCA9539_NUM4_I2C_BUS_ID, PCA9539_NUM4_I2C_ADDR, offset, data, ONLP_I2C_F_FORCE);
    if (ret < 0)
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    /* 3. Configure the direction of PCA9539#3 */
    data = onlp_i2c_readb(PCA9539_NUM3_I2C_BUS_ID, PCA9539_NUM3_I2C_ADDR, offset, ONLP_I2C_F_FORCE);

    /* configuration direction to output, input:1, output:0.  */
    data &= ~(1 << PCA9539_QSFP_LPMODE_BIT);
    data &= ~(1 << PCA9539_QSFP_RESET_BIT);

    ret = onlp_i2c_writeb(PCA9539_NUM3_I2C_BUS_ID, PCA9539_NUM3_I2C_ADDR, offset, data, ONLP_I2C_F_FORCE);
    if (ret < 0)
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return 0;      
}

static int
spx70d0_sfp_pca9539_direction_setting_check(int IO_port)
{
    int offset = 0;
    int data = 0;
    //int ret = 0;

    if (IO_port == 0)
        offset = PCA9539_IO0_DIRECTION_OFFSET;
    else
        offset = PCA9539_IO1_DIRECTION_OFFSET;

    /* Get the I/O port direction setting of PCA9539#1 */
    data = onlp_i2c_readb(PCA9539_NUM1_I2C_BUS_ID, PCA9539_NUM1_I2C_ADDR, offset, ONLP_I2C_F_FORCE);

    DIAG_PRINT("[%s(%d)] pca9539_bus:%d, addr:0x%X, offset:0x%X, data:0x%2X \n", 
                __FUNCTION__, __LINE__,  PCA9539_NUM1_I2C_BUS_ID, PCA9539_NUM1_I2C_ADDR, offset, (unsigned char)data);

    /* Check PCA9539#2 I/O port0 direction has been initialized or not */
    if ((unsigned char)data != 0xEE)
        return 0; 

    /* Get the I/O port direction setting of PCA9539#4 */
    data = onlp_i2c_readb(PCA9539_NUM4_I2C_BUS_ID, PCA9539_NUM4_I2C_ADDR, offset, ONLP_I2C_F_FORCE);

    DIAG_PRINT("[%s(%d)] pca9539_bus:%d, addr:0x%X, offset:0x%X, data:0x%2X \n", 
                __FUNCTION__, __LINE__,  PCA9539_NUM4_I2C_BUS_ID, PCA9539_NUM4_I2C_ADDR, offset, (unsigned char)data);

    /* Check PCA9539#4 I/O port0 direction has been initialized or not */
    if ((unsigned char)data != 0xEE)
        return 0; 

    /* Get the I/O port direction setting of PCA9539#3 */
    data = onlp_i2c_readb(PCA9539_NUM3_I2C_BUS_ID, PCA9539_NUM3_I2C_ADDR, offset, ONLP_I2C_F_FORCE);

    DIAG_PRINT("[%s(%d)] pca9539_bus:%d, addr:0x%X, offset:0x%X, data:0x%2X \n", 
                __FUNCTION__, __LINE__,  PCA9539_NUM3_I2C_BUS_ID, PCA9539_NUM3_I2C_ADDR, offset, (unsigned char)data);

    /* Check PCA9539#4 I/O port0 direction has been initialized or not */
    if ((unsigned char)data != 0xF6)
        return 0;

    return 1;      
}

static int
spx70d0_sfp_init(void)
{
    int ret = 0;

    DIAG_PRINT("%s", __FUNCTION__);

    /* Configure PCA9539#1,3,4 I/O port0 direction if not been initialized */
    if (spx70d0_sfp_pca9539_direction_setting_check(0) == 0)
    {
        /* config pca9539 IO port0 direction */
        ret = spx70d0_sfp_pca9539_direction_set(0);
        if (ret < 0)
        {
            AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ret);
            return ret;
        }

        /* config pca9539 IO port1 direction */
        ret = spx70d0_sfp_pca9539_direction_set(1);
        if (ret < 0)
        {
            AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ret);
            return ret;
        }

        onlp_sfpi_control_set(SFP_PORT_10G_XFP_1, ONLP_SFP_CONTROL_TX_DISABLE, 0);
        onlp_sfpi_control_set(SFP_PORT_10G_XFP_2, ONLP_SFP_CONTROL_TX_DISABLE, 0);
        onlp_sfpi_control_set(SFP_PORT_10G_XFP_3, ONLP_SFP_CONTROL_TX_DISABLE, 0);
        onlp_sfpi_control_set(SFP_PORT_10G_XFP_4, ONLP_SFP_CONTROL_TX_DISABLE, 0);
        onlp_sfpi_control_set(SFP_PORT_10G_XFP_5, ONLP_SFP_CONTROL_TX_DISABLE, 0);
        onlp_sfpi_control_set(SFP_PORT_10G_XFP_6, ONLP_SFP_CONTROL_TX_DISABLE, 0);
        onlp_sfpi_control_set(SFP_PORT_10G_XFP_7, ONLP_SFP_CONTROL_TX_DISABLE, 0);
        onlp_sfpi_control_set(SFP_PORT_10G_XFP_8, ONLP_SFP_CONTROL_TX_DISABLE, 0);
        onlp_sfpi_control_set(SFP_PORT_100G_QSFP28_1, ONLP_SFP_CONTROL_LP_MODE, 0);
        onlp_sfpi_control_set(SFP_PORT_100G_QSFP28_1, ONLP_SFP_CONTROL_RESET, 1);
        onlp_sfpi_control_set(SFP_PORT_100G_QSFP28_2, ONLP_SFP_CONTROL_LP_MODE, 0);
        onlp_sfpi_control_set(SFP_PORT_100G_QSFP28_2, ONLP_SFP_CONTROL_RESET, 1);
    }

    return 0;
}

/************************************************************
 *
 * SFPI Entry Points
 *
 ***********************************************************/
int
onlp_sfpi_init(void)
{
    DIAG_PRINT("%s", __FUNCTION__);
    /* Called at initialization time */
    spx70d0_sfp_init();

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t *bmap)
{
    /*
     * Ports {0, 9}
     */
    int p = 0;
    AIM_BITMAP_CLR_ALL(bmap);

    for (p = 0; p < NUM_OF_SFP_PORT; p++)
    {
        AIM_BITMAP_SET(bmap, p);
    }
    DIAG_PRINT("%s", __FUNCTION__);

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_is_present(int port)
{
    /*
     * Return 1 if present.
     * Return 0 if not present.
     * Return < 0 if error.
     */
    int present = 0;

    present = spx70d0_sfp_present(port);

    DIAG_PRINT("%s, port=%d, present:%d", __FUNCTION__, port, present);
    return present;
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t *dst)
{
    DIAG_PRINT("%s", __FUNCTION__);

    int i = 0;
    AIM_BITMAP_CLR_ALL(dst);
    for (i = 0; i < NUM_OF_SFP_PORT; i++)
    {
        if (onlp_sfpi_is_present(i))
        {
            AIM_BITMAP_SET(dst, i);
        }
    }

    return ONLP_STATUS_OK;
}

static int
port_to_dev_busid(int port)
{
    int busid = 0;

    switch (port)
    {
        case SFP_PORT_10G_XFP_1:
            busid = 17;
            break;
        case SFP_PORT_10G_XFP_2:
            busid = 18;
            break;
        case SFP_PORT_10G_XFP_3:
            busid = 19;
            break;
        case SFP_PORT_10G_XFP_4:
            busid = 20;
            break;
        case SFP_PORT_10G_XFP_5:
            busid = 12;
            break;
        case SFP_PORT_10G_XFP_6:
            busid = 13;
            break;
        case SFP_PORT_10G_XFP_7:
            busid = 14;
            break;
        case SFP_PORT_10G_XFP_8:
            busid = 15;
            break;
        case SFP_PORT_100G_QSFP28_1:
            busid = 9;
            break;
        case SFP_PORT_100G_QSFP28_2:
            busid = 10;
            break;
        default:
            break;        
    }

    return busid;
}

int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    int busid = port_to_dev_busid(port);

    DIAG_PRINT("%s, port:%d, busid:%d", __FUNCTION__, port, busid);

    /*
     * Read the SFP eeprom into data[]
     */
    memset(data, 0x0, 256);

    if (onlp_i2c_read(busid, SFP_PLUS_EEPROM_I2C_ADDR, 0x0, 256, data, ONLP_I2C_F_FORCE) != 0)
    {
        AIM_LOG_INFO("Unable to read eeprom from port(%d)\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int onlp_sfpi_dom_read(int port, uint8_t data[256])
{
    int busid = port_to_dev_busid(port);
    int dev_addr = 0;

    memset(data, 0x0, 256);
    DIAG_PRINT("%s, port:%d, busid:%d", __FUNCTION__, port, busid);

    if (port >= SFP_START_INDEX && port < (SFP_START_INDEX+NUM_OF_SFP_PORT))
    {
        dev_addr = SFP_PLUS_EEPROM_I2C_ADDR;
    }

    if (onlp_i2c_read(busid, dev_addr, 0x0, 256, data, ONLP_I2C_F_FORCE) != 0)
    {
        AIM_LOG_INFO("Unable to read eeprom from port(%d)\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_dev_readb(int port, uint8_t devaddr, uint8_t addr)
{
    int ret = 0;
    int bus = port_to_dev_busid(port);

    ret = onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE);
    DIAG_PRINT("%s, port:%d, devaddr:%d, addr:%d, ret:%d(0x%02X)", __FUNCTION__, port, devaddr, addr, ret, ret);

    return ret;
}

int
onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    int ret = 0;
    int bus = port_to_dev_busid(port);

    ret = onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
    DIAG_PRINT("%s, port:%d, devaddr:%d, addr:%d, value:%d(0x%02X), ret:%d", __FUNCTION__, port, devaddr, addr, value, value, ret);

    return ret;
}

int
onlp_sfpi_dev_readw(int port, uint8_t devaddr, uint8_t addr)
{
    int ret = 0;
    int bus = port_to_dev_busid(port);

    ret = onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE);
    DIAG_PRINT("%s, port:%d, devaddr:%d, addr:%d, ret:%d(0x%04X)", __FUNCTION__, port, devaddr, addr, ret, ret);

    return ret;
}

int
onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    int ret = 0;
    int bus = port_to_dev_busid(port);

    ret = onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
    DIAG_PRINT("%s, port:%d, devaddr:%d, addr:%d, value:%d(0x%04X), ret:%d", __FUNCTION__, port, devaddr, addr, value, value, ret);

    return ret;
}

/* the description is not correct, will remove later
  Reset and LP mode can control by CPLD so the setting will be keep in CPLD.
  For other options, control is get/set to QSFP28.
  Control options set to QSFP28 will be lost when the QSFP28 is removed.
  Upper layer software system should keep the configuration and set it again when detect a new sfp module insert. 
    [QSFP]
     function                            R/W  CPLD           EEPROM
    ------------------------------------ ---  -------------  -----------------
    ONLP_SFP_CONTROL_RESET                W   0x7
    ONLP_SFP_CONTROL_RESET_STATE         R/W  0x7  
    ONLP_SFP_CONTROL_RX_LOS               R   none           byte 4
    ONLP_SFP_CONTROL_TX_FAULT             R   none           byte 3
    ONLP_SFP_CONTROL_TX_DISABLE          R/W  none           byte 86
    ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL  R/W  none           byte 86
    ONLP_SFP_CONTROL_LP_MODE             R/W  0x9          
    ONLP_SFP_CONTROL_POWER_OVERRIDE      R/W  none           byte 93
    [SFP]
     function                            R/W  CPLD       
    ------------------------------------ ---  ---------------  
    ONLP_SFP_CONTROL_RESET               Not Support(There is no RESET pin in SFP module)
    ONLP_SFP_CONTROL_RESET_STATE         Not Support(There is no RESET pin in SFP module)
    ONLP_SFP_CONTROL_RX_LOS               R   0x13/0x14/0x15
    ONLP_SFP_CONTROL_TX_FAULT             R   0x10/0x11/0x12
    ONLP_SFP_CONTROL_TX_DISABLE          R/W  0x19/0x1A/0x1B
    ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL  Not Support(It is for QSFP)
    ONLP_SFP_CONTROL_LP_MODE             Not Support(It is for QSFP)
    ONLP_SFP_CONTROL_POWER_OVERRIDE      Not Support(It is for QSFP)

*/

int onlp_sfpi_control_supported(int port, onlp_sfp_control_t control, int *supported)
{
    if (supported == NULL)
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ONLP_STATUS_E_PARAM);
        return ONLP_STATUS_E_PARAM;
    }

    *supported = 0;

    if ((port >= SFP_PORT_10G_XFP_1) && (port <= SFP_PORT_10G_XFP_8))
    {
        switch (control)
        {
            case ONLP_SFP_CONTROL_TX_FAULT:
            case ONLP_SFP_CONTROL_TX_DISABLE:
              *supported = 1;
              break;
                
            default:
              *supported = 0;
              break;
        }
    }
    else if ((port >= SFP_PORT_100G_QSFP28_1) && (port <= SFP_PORT_100G_QSFP28_2))
    {
        switch (control)
        {
            case ONLP_SFP_CONTROL_LP_MODE:
            case ONLP_SFP_CONTROL_RESET:
              *supported = 1;
              break;
                
            default:
              *supported = 0;
              break;
        }
    }

    DIAG_PRINT("%s, port:%d, control:%d(%s), supported:%d", __FUNCTION__, port, control, sfp_control_to_str(control), *supported);
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{
    int rv = ONLP_STATUS_OK;
    int pca9539_bus = 0;
    int pca9539_addr = 0;
    int offset = 0;
    int port_bit = 0;
    int supported = 0;
    char optval = 0;
    
    if ((onlp_sfpi_control_supported(port, control, &supported) == ONLP_STATUS_OK) && (supported == 0))
        return ONLP_STATUS_E_UNSUPPORTED;

    DIAG_PRINT("%s, port:%d, control:%d(%s), value:0x%X", __FUNCTION__, port, control, sfp_control_to_str(control), value);

    switch (control)
    {
        /* TXDIS is Output direction!!!! */
        case ONLP_SFP_CONTROL_TX_DISABLE:
            offset = port_to_pca9539_sfp_offset(port, PCA9539_IO_OUTPUT);
            port_bit = port_to_pca9539_txdis_bit(port);
            break;

        case ONLP_SFP_CONTROL_LP_MODE:
            offset = port_to_pca9539_sfp_offset(port, PCA9539_IO_OUTPUT);
            port_bit = port_to_pca9539_lpmode_bit(port);
            break;

        case ONLP_SFP_CONTROL_RESET:
            offset = port_to_pca9539_sfp_offset(port, PCA9539_IO_OUTPUT);
            port_bit = port_to_pca9539_reset_bit(port);
            break;
            
        default:
            return ONLP_STATUS_E_UNSUPPORTED;
            break;
    }

    pca9539_bus = port_to_pca9539_busid(port);
    pca9539_addr = port_to_pca9539_addr(port);

    optval = onlp_i2c_readb(pca9539_bus, pca9539_addr, offset, ONLP_I2C_F_FORCE);

    if (value != 0)
    {
        optval |= (1 << port_bit);
    }
    else
    {
        optval &= ~(1 << port_bit);
    }

    rv = onlp_i2c_writeb(pca9539_bus, pca9539_addr, offset, optval, ONLP_I2C_F_FORCE);
    if (rv < 0)
    {
        dump_stack();
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, rv);
        return rv;
    }
    
    DIAG_PRINT("%s, port_bit:%d, pca9539_bus:%d, pca9539_addr:0x%X, offset:0x%X", __FUNCTION__, port_bit, pca9539_bus, pca9539_addr, offset);
    return rv;
}

int
onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int *value)
{
    int rv = ONLP_STATUS_OK;
    int pca9539_bus = 0;
    int pca9539_addr = 0;
    int offset = 0;
    int port_bit = 0;
    int supported = 0;
    char optval = 0;

    if (value == NULL)
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ONLP_STATUS_E_PARAM);
        return ONLP_STATUS_E_PARAM;
    }

    if ((onlp_sfpi_control_supported(port, control, &supported) == ONLP_STATUS_OK) && (supported == 0))
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ONLP_STATUS_E_UNSUPPORTED);
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    *value = 0;

    switch (control)
    {
        case ONLP_SFP_CONTROL_TX_FAULT:
            offset = port_to_pca9539_sfp_offset(port, PCA9539_IO_INPUT);
            port_bit = port_to_pca9539_txfault_bit(port);
            break;
            
        case ONLP_SFP_CONTROL_TX_DISABLE:
            offset = port_to_pca9539_sfp_offset(port, PCA9539_IO_OUTPUT);
            port_bit = port_to_pca9539_txdis_bit(port);
            break;

        case ONLP_SFP_CONTROL_LP_MODE:
            offset = port_to_pca9539_sfp_offset(port, PCA9539_IO_OUTPUT);
            port_bit = port_to_pca9539_lpmode_bit(port);
            break;

        case ONLP_SFP_CONTROL_RESET:
            offset = port_to_pca9539_sfp_offset(port, PCA9539_IO_OUTPUT);
            port_bit = port_to_pca9539_reset_bit(port);
            break;

        default:
            return ONLP_STATUS_E_UNSUPPORTED;
            break;
    }

    pca9539_bus = port_to_pca9539_busid(port);
    pca9539_addr = port_to_pca9539_addr(port);
    
    optval = onlp_i2c_readb(pca9539_bus, pca9539_addr, offset, ONLP_I2C_F_FORCE);
    if ((optval & (1 << port_bit)) != 0) //1
    {
        *value = 1;
    }
    else
    {
        *value = 0;
    }
    
    DIAG_PRINT("%s, port_bit:%d, cpld_bus:%d, addr:0x%X, offset:0x%X", __FUNCTION__, port_bit, pca9539_bus, pca9539_addr, offset);
    DIAG_PRINT("%s, port:%d, control:%d(%s), value:0x%X", __FUNCTION__, port, control, sfp_control_to_str(control), *value);

    return rv;
}

int
onlp_sfpi_denit(void)
{
    DIAG_PRINT("%s", __FUNCTION__);
    return ONLP_STATUS_OK;
}

