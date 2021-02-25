/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2016 Accton Technology Corporation.
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
#include <unistd.h>
#include <onlplib/i2c.h>
#include <onlp/platformi/sfpi.h>
#include <onlplib/file.h>
#include "platform_lib.h"
#include "x86_64_accton_as9516_32d_log.h"

#define NUM_OF_QSFP_PORT 32
/* QSFP Port index */
#define QSFP_1          0
#define QSFP_2          1
#define QSFP_3          2
#define QSFP_4          3
#define QSFP_5          4
#define QSFP_6          5
#define QSFP_7          6
#define QSFP_8          7
#define QSFP_9          8
#define QSFP_10         9
#define QSFP_11         10
#define QSFP_12         11
#define QSFP_13         12
#define QSFP_14         13
#define QSFP_15         14
#define QSFP_16         15
#define QSFP_17         16
#define QSFP_18         17
#define QSFP_19         18
#define QSFP_20         19
#define QSFP_21         20
#define QSFP_22         21
#define QSFP_23         22
#define QSFP_24         23
#define QSFP_25         24
#define QSFP_26         25
#define QSFP_27         26
#define QSFP_28         27
#define QSFP_29         28
#define QSFP_30         29
#define QSFP_31         30
#define QSFP_32         31
/* Bit value */
#define BIT0            0x0001
#define BIT1            0x0002
#define BIT2            0x0004
#define BIT3            0x0008
#define BIT4            0x0010
#define BIT5            0x0020
#define BIT6            0x0040
#define BIT7            0x0080
#define BIT8            0x0100
#define BIT9            0x0200
#define BIT10           0x0400
#define BIT11           0x0800
#define BIT12           0x1000
#define BIT13           0x2000
#define BIT14           0x4000
#define BIT15           0x8000

typedef struct port_info_s
{
    int     port;
    uint8_t address;
    uint8_t chan;
    uint16_t bit;
} port_info_t;

typedef struct bitmap_info_s
{
    uint16_t bit;
    int     port;
} bitmap_info_t;

static port_info_t port_presence_info[] = 
{
    {QSFP_1,  0x22, 0x04, BIT1},
    {QSFP_2,  0x22, 0x04, BIT0},
    {QSFP_3,  0x22, 0x04, BIT3},
    {QSFP_4,  0x22, 0x04, BIT2},
    {QSFP_5,  0x22, 0x04, BIT5},
    {QSFP_6,  0x22, 0x04, BIT4},
    {QSFP_7,  0x22, 0x04, BIT7},
    {QSFP_8,  0x22, 0x04, BIT6},
    {QSFP_9,  0x22, 0x04, BIT9},
    {QSFP_10, 0x22, 0x04, BIT8},
    {QSFP_11, 0x22, 0x04, BIT11},
    {QSFP_12, 0x22, 0x04, BIT10},
    {QSFP_13, 0x22, 0x04, BIT13},
    {QSFP_14, 0x22, 0x04, BIT12},
    {QSFP_15, 0x22, 0x04, BIT15},
    {QSFP_16, 0x22, 0x04, BIT14},
    {QSFP_17, 0x23, 0x08, BIT1},
    {QSFP_18, 0x23, 0x08, BIT0},
    {QSFP_19, 0x23, 0x08, BIT3},
    {QSFP_20, 0x23, 0x08, BIT2},
    {QSFP_21, 0x23, 0x08, BIT5},
    {QSFP_22, 0x23, 0x08, BIT4},
    {QSFP_23, 0x23, 0x08, BIT7},
    {QSFP_24, 0x23, 0x08, BIT6},
    {QSFP_25, 0x23, 0x08, BIT9},
    {QSFP_26, 0x23, 0x08, BIT8},
    {QSFP_27, 0x23, 0x08, BIT11},
    {QSFP_28, 0x23, 0x08, BIT10},
    {QSFP_29, 0x23, 0x08, BIT13},
    {QSFP_30, 0x23, 0x08, BIT12},
    {QSFP_31, 0x23, 0x08, BIT15},
    {QSFP_32, 0x23, 0x08, BIT14},
};
/* bit-value port mapping 0-15 */
static bitmap_info_t bitmap_port_1_info[] = 
{
    {BIT0, QSFP_2},
    {BIT1, QSFP_1},
    {BIT2, QSFP_4},
    {BIT3, QSFP_3},
    {BIT4, QSFP_6},
    {BIT5, QSFP_5},
    {BIT6, QSFP_8},
    {BIT7, QSFP_7},
    {BIT8, QSFP_10},
    {BIT9, QSFP_9},
    {BIT10, QSFP_12},
    {BIT11, QSFP_11},
    {BIT12, QSFP_14},
    {BIT13, QSFP_13},
    {BIT14, QSFP_16},
    {BIT15, QSFP_15},

};
/* bit-value port mapping 16-31 */
static bitmap_info_t bitmap_port_2_info[] = 
{
    {BIT0, QSFP_18},
    {BIT1, QSFP_17},
    {BIT2, QSFP_20},
    {BIT3, QSFP_19},
    {BIT4, QSFP_22},
    {BIT5, QSFP_21},
    {BIT6, QSFP_24},
    {BIT7, QSFP_23},
    {BIT8, QSFP_26},
    {BIT9, QSFP_25},
    {BIT10, QSFP_28},
    {BIT11, QSFP_27},
    {BIT12, QSFP_30},
    {BIT13, QSFP_29},
    {BIT14, QSFP_32},
    {BIT15, QSFP_31},
};

uint64_t g_present_port_val;
/************************************************************
 *
 * SFPI Entry Points
 *
 ***********************************************************/
int
onlp_sfpi_init(void)
{
    /* Called at initialization time */    
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    int p;
    AIM_BITMAP_CLR_ALL(bmap);

    for(p = 0; p < NUM_OF_QSFP_PORT; p++)
    {
        AIM_BITMAP_SET(bmap, p);
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_reg_val_to_port_sequence(uint64_t *presence_bit_val, uint64_t presence_reg_val)
{

    int i;
    int reg_1_val, reg_2_val;
    uint64_t presence_port_bit_val = 0;
    /* port 0-15 */
    for (i = 0; i < 16; i++)
    {
        reg_1_val=(~presence_reg_val) & (bitmap_port_1_info[i].bit);
        if(reg_1_val != 0)
        {
            presence_port_bit_val=presence_port_bit_val | (1 << (bitmap_port_1_info[i].port));
        }
    }
     /* port 16-31 */
    presence_reg_val=(presence_reg_val >> 16);
    for (i = 0; i < 16; i++)
    {
        reg_2_val=(~presence_reg_val) & (bitmap_port_2_info[i].bit);
        if(reg_2_val != 0)
        {
            presence_port_bit_val=presence_port_bit_val | (1 << (bitmap_port_2_info[i].port));
        }
    }

    *presence_bit_val = presence_port_bit_val;

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
    int present=0;

    if(port < 16)
    {
        present=((g_present_port_val >> port) & 0x1);
    }

    if((port >= 16) && (port<32))
    {
        present=((g_present_port_val >> (port)) & 0x1);
    }

    return present;
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    uint64_t reg_presence_all = 0;
    uint64_t presence_all = 0;
    uint8_t bytes[4];
    int rd_size, i;
    uint8_t bus, i2c_addr, mux_i2c_addr, mux_chn, fpga_id, byte_buf[128];

    fpga_id=0;
    bus=32;
    mux_i2c_addr=0x74;
    rd_size=1;
    /* port 0-15 */
    mux_chn=port_presence_info[0].chan;
    i2c_addr=port_presence_info[0].address;
    for(i=0; i<2; i++)
    {
        if((fpga_proc_i2c_read(fpga_id, bus, mux_i2c_addr, mux_chn, i2c_addr
                                , rd_size, byte_buf)) != 0)
        {
            return ONLP_STATUS_E_INTERNAL;
        }
        else
        {
            bytes[i]=byte_buf[0];
        }
    }

    mux_chn=port_presence_info[16].chan;
    i2c_addr=port_presence_info[16].address;
    for(i=0; i<2; i++)
    {
        if((fpga_proc_i2c_read(fpga_id, bus, mux_i2c_addr, mux_chn, i2c_addr
                                , rd_size, byte_buf)) != 0)
        {
            return ONLP_STATUS_E_INTERNAL;
        }
        else
        {
            bytes[2+i]=byte_buf[0];
        }
    }
    /* Convert to 64 bit integer in port order */
    for(i = 3; i >= 0; i--) {
        reg_presence_all <<= 8;
        reg_presence_all |= bytes[i];
    }

    onlp_sfpi_reg_val_to_port_sequence(&presence_all, reg_presence_all);
    g_present_port_val=presence_all;

    /* Populate bitmap */
    for(i = 0; presence_all; i++) {
        AIM_BITMAP_MOD(dst, i, (presence_all & 1));
        presence_all >>= 1;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    /* Not defined*/
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    int rd_size, wr_size, i;
    uint8_t bus, i2c_addr, mux_i2c_addr, mux_chn, fpga_id, byte_buf[128];

    if(port <0 || port >= NUM_OF_QSFP_PORT)
        return ONLP_STATUS_E_INTERNAL;

    memset(data, 0, 256);
    fpga_id=0;
    bus=port;
    mux_i2c_addr=0x88;
    mux_chn=0;
    i2c_addr =0x50;
    /* set offset 0x0, get data from offset 0x0 */
    wr_size =1;
    byte_buf[0]=0;

    if (fpga_pltfm_init(fpga_id) != 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if((fpga_proc_i2c_write(fpga_id, bus, mux_i2c_addr, mux_chn, i2c_addr, wr_size, byte_buf)) != 0)
    {
        return ONLP_STATUS_E_INTERNAL;
    }
    /* get data of eeprom */
    rd_size = 1;
    for(i=0; i<256; i++)
    {
        if((fpga_proc_i2c_read(fpga_id, bus, mux_i2c_addr, mux_chn, i2c_addr, rd_size, byte_buf)) != 0)
        {
            return ONLP_STATUS_E_INTERNAL;
        }
        data[i]=byte_buf[0];
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_dev_readb(int port, uint8_t devaddr, uint8_t addr)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_sfpi_dev_readw(int port, uint8_t devaddr, uint8_t addr)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{    
    int rv;
    rv = ONLP_STATUS_E_UNSUPPORTED;

    switch(control)
        {
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                /*No defined */
                rv = ONLP_STATUS_OK;

                break;
            }
        case ONLP_SFP_CONTROL_RESET:
            {
                /*No defined */
                break;
            }

        default:
            rv = ONLP_STATUS_E_UNSUPPORTED;
            break;
        }

    return rv;
}

int
onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
{
    int rv;
    rv = ONLP_STATUS_E_UNSUPPORTED;

    switch(control)
        {
        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                /*No defined */
                rv = ONLP_STATUS_OK;

                break;
            }
        case ONLP_SFP_CONTROL_RESET:
            {
                /*No defined */
                break;
            }

        default:
            rv = ONLP_STATUS_E_UNSUPPORTED;
            break;
        }

    return rv;
}

int
onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}
