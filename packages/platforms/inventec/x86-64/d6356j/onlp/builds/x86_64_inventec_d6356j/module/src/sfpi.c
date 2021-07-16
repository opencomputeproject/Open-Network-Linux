/************************************************************
 * sfpi.c
 *
 *           Copyright 2018 Inventec Technology Corporation.
 *
 ************************************************************
 *
 ***********************************************************/

#include <onlp/platformi/sfpi.h>
#include <fcntl.h> /* For O_RDWR && open */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <onlplib/i2c.h>
#include "platform_lib.h"
#include <dirent.h>
#include <onlplib/file.h>

#define MAX_SFP_PATH 128

#define SFP_MUX_START_INDEX 22
#define QSFP_MUX_START_INDEX 14
#define QSFP_DEV_ADDR 0x50
#define NUM_OF_SFP_PORT 48
#define NUM_OF_QSFP_PORT 8
#define NUM_OF_ALL_PORT (NUM_OF_SFP_PORT+NUM_OF_QSFP_PORT)
#define PORT_TO_PATH_ID(port)    (port+1)
static const int sfp_mux_index[NUM_OF_ALL_PORT] = {
    23, 22, 25, 24, 27, 26, 29, 28,
    31, 30, 33, 32, 35, 34, 37, 36,
    39, 38, 41, 40, 43, 43, 45, 44,
    47, 46, 49, 48, 51, 50, 53, 52,
    55, 54, 57, 56, 59, 58, 61, 60,
    63, 62, 65, 64, 67, 66, 69, 68,
    15, 14, 17, 16, 19, 18, 21, 20,
};

/************************************************************
 *
 * SFPI Entry Points
 *
 ***********************************************************/
enum onlp_sfp_port_type {
    ONLP_PORT_TYPE_SFP = 0,
    ONLP_PORT_TYPE_QSFP,
    ONLP_PORT_TYPE_MAX
};

static int
onlp_sfpi_port_type(int port)
{
    if(port >= 0 && port < NUM_OF_SFP_PORT) {
        return ONLP_PORT_TYPE_SFP;
    } else if(port >= NUM_OF_SFP_PORT && port < NUM_OF_ALL_PORT) {
        return ONLP_PORT_TYPE_QSFP;
    } else {
        AIM_LOG_ERROR("Invalid port(%d)\r\n", port);
        return ONLP_STATUS_E_PARAM;
    }
}

int
onlp_sfpi_init(void)
{
    /* Called at initialization time */
    return ONLP_STATUS_OK;
}

int onlp_sfpi_port_map(int port, int* rport)
{
    int p_type = onlp_sfpi_port_type(port);
    if(p_type < 0) {
        return ONLP_STATUS_E_INVALID;
    }
    *rport = port;
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    int p;
    AIM_BITMAP_CLR_ALL(bmap);

    for(p = 0; p < NUM_OF_ALL_PORT; p++) {
        AIM_BITMAP_SET(bmap, p);
    }

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
    int p_type = onlp_sfpi_port_type(port);
    if(p_type < 0) {
        return ONLP_STATUS_E_INVALID;
    }

    int present_data;
    int rv;
    if(onlp_file_read_int(&present_data, INV_SFP_PREFIX"port%d/present", PORT_TO_PATH_ID(port) )!= ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }
    switch(present_data) {
    case 1:
    case 0:
        return !present_data;
    default:
        AIM_LOG_ERROR("Unvalid present status %d from port(%d)\r\n",present_data,port);
        return ONLP_STATUS_E_INTERNAL;
    }
    return rv;
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    AIM_BITMAP_CLR_ALL(dst);
    int port;
    for(port = 0; port < NUM_OF_ALL_PORT; port++) {
        if(onlp_sfpi_is_present(port) == true) {
            AIM_BITMAP_MOD(dst, port, 1);
        } else if(onlp_sfpi_is_present(port) == false) {
            AIM_BITMAP_MOD(dst, port, 0);
        } else {
            return ONLP_STATUS_E_INTERNAL;
        }
    }
    return ONLP_STATUS_OK;
}

static int
onlp_sfpi_is_rx_los(int port)
{
    int rxlos;
    int rv;
    int p_type = onlp_sfpi_port_type(port);

    if(p_type == ONLP_PORT_TYPE_SFP) {   /* don't support to parse QSFP rxlos value */
        if(onlp_file_read_int(&rxlos, INV_SFP_PREFIX"port%d/rxlos", PORT_TO_PATH_ID(port) )!= ONLP_STATUS_OK) {
            return ONLP_STATUS_E_INTERNAL;
        }
        if(rxlos==0) {
            rv=false;
        } else if(rxlos==1) {
            rv=true;
        }
    } else {
        return ONLP_STATUS_E_INVALID;
    }

    return rv;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    AIM_BITMAP_CLR_ALL(dst);
    int port;
    int isrxlos;
    for(port = 0; port < NUM_OF_SFP_PORT; port++) {  /* don't support to parse QSFP rxlos value */
        if(onlp_sfpi_is_present(port) == true) {
            isrxlos = onlp_sfpi_is_rx_los(port);
            if(isrxlos == true) {
                AIM_BITMAP_MOD(dst, port, 1);
            } else if(isrxlos == false) {
                AIM_BITMAP_MOD(dst, port, 0);
            } else {
                return ONLP_STATUS_E_INTERNAL;
            }
        }
    }
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_port2chan(int port)  // port start from 0
{
    int port_type=onlp_sfpi_port_type(port);

    if(port_type<0) {
        return ONLP_STATUS_E_INVALID;
    } else {
        return sfp_mux_index[port];
    }
}

int
onlp_sfpi_dom_read(int port, uint8_t data[256])
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_sfpi_post_insert(int port, sff_info_t* info)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    /*
     * Read the SFP eeprom into data[]
     *
     * Return MISSING if SFP is missing.
     * Return OK if eeprom is read
     */
    memset(data, 0, 256);

    if(onlp_sfpi_port_type(port) < 0) {
        return ONLP_STATUS_E_INVALID;
    }
    int sts;
    int bus = onlp_sfpi_port2chan(port);

    sts = onlp_i2c_block_read(bus, QSFP_DEV_ADDR, 0, 256, data, ONLP_I2C_F_FORCE);
    if(sts < 0) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
        return ONLP_STATUS_E_MISSING;
    }
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_dev_readb(int port, uint8_t devaddr, uint8_t addr)
{
    if(onlp_sfpi_port_type(port) < 0) {
        return ONLP_STATUS_E_INVALID;
    }
    int bus = onlp_sfpi_port2chan(port);
    return onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    if(onlp_sfpi_port_type(port) < 0) {
        return ONLP_STATUS_E_INVALID;
    }
    int bus = onlp_sfpi_port2chan(port);
    return onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_dev_readw(int port, uint8_t devaddr, uint8_t addr)
{
    if(onlp_sfpi_port_type(port) < 0) {
        return ONLP_STATUS_E_INVALID;
    }
    int bus = onlp_sfpi_port2chan(port);
    return onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    if(onlp_sfpi_port_type(port) < 0) {
        return ONLP_STATUS_E_INVALID;
    }
    int bus = onlp_sfpi_port2chan(port);
    return onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_control_supported(int port, onlp_sfp_control_t control, int* rv)
{
    *rv = 0;
    int p_type = onlp_sfpi_port_type(port);
    if(p_type < 0) {
        return ONLP_STATUS_E_INVALID;
    }
    switch (control) {
    case ONLP_SFP_CONTROL_RX_LOS:
        if(p_type == ONLP_PORT_TYPE_SFP) {
            *rv = 1;
        }
        break;
    case ONLP_SFP_CONTROL_RESET_STATE:
        if(p_type == ONLP_PORT_TYPE_QSFP) {
            *rv = 1;
        }
        break;
    case ONLP_SFP_CONTROL_TX_DISABLE:
        if(p_type == ONLP_PORT_TYPE_SFP) {
            *rv = 1;
        }
        break;
    case ONLP_SFP_CONTROL_LP_MODE:
        if(p_type == ONLP_PORT_TYPE_QSFP) {
            *rv = 1;
        }
        break;
    default:
        break;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{
    int ret = ONLP_STATUS_E_UNSUPPORTED;
    int p_type = onlp_sfpi_port_type(port);
    if(p_type < 0) {
        return ONLP_STATUS_E_INVALID;
    }
    switch (control) {
    case ONLP_SFP_CONTROL_RESET_STATE:
        if(p_type == ONLP_PORT_TYPE_QSFP) {
            ret = onlp_file_write_int(value, INV_SFP_PREFIX"port%d/reset", PORT_TO_PATH_ID(port) );
        }
        break;
    case ONLP_SFP_CONTROL_TX_DISABLE:
        if(p_type == ONLP_PORT_TYPE_SFP) {
            ret = onlp_file_write_int(value, INV_SFP_PREFIX"port%d/tx_disable", PORT_TO_PATH_ID(port));
        }
        break;
    case ONLP_SFP_CONTROL_LP_MODE:
        if(p_type == ONLP_PORT_TYPE_QSFP) {
            ret = onlp_file_write_int(value, INV_SFP_PREFIX"port%d/lpmod", PORT_TO_PATH_ID(port));
        }
        break;
    default:
        break;
    }

    return ret;
}

int
onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
{
    int ret = ONLP_STATUS_E_UNSUPPORTED;
    int p_type = onlp_sfpi_port_type(port);
    if(p_type < 0) {
        return ONLP_STATUS_E_INVALID;
    }
    switch (control) {
    case ONLP_SFP_CONTROL_RESET_STATE:
        if(p_type == ONLP_PORT_TYPE_QSFP) {
            /*the value of /port(id)/reset
            0: in reset state; 1:not in reset state*/
            ret = onlp_file_read_int(value, INV_SFP_PREFIX"port%d/reset", PORT_TO_PATH_ID(port));
            if(ret==ONLP_STATUS_OK) {
                *value=(!(*value));
            }
        }
        break;
    case ONLP_SFP_CONTROL_RX_LOS:
        if(p_type == ONLP_PORT_TYPE_SFP) {
            ret = onlp_file_read_int(value, INV_SFP_PREFIX"port%d/rxlos", PORT_TO_PATH_ID(port));
        }
        break;
    case ONLP_SFP_CONTROL_TX_DISABLE:
        if(p_type == ONLP_PORT_TYPE_SFP) {
            ret = onlp_file_read_int(value, INV_SFP_PREFIX"port%d/tx_disable", PORT_TO_PATH_ID(port));
        }
        break;
    case ONLP_SFP_CONTROL_LP_MODE:
        if(p_type == ONLP_PORT_TYPE_QSFP) {
            ret = onlp_file_read_int(value, INV_SFP_PREFIX"port%d/lpmod", PORT_TO_PATH_ID(port));
        }
        break;
    case ONLP_SFP_CONTROL_TX_FAULT:
        if(p_type == ONLP_PORT_TYPE_SFP) {
            ret = onlp_file_read_int(value, INV_SFP_PREFIX"port%d/tx_fault", PORT_TO_PATH_ID(port));
        }
        break;
    default:
        break;
    }

    return ret;
}

int
onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}

void
onlp_sfpi_debug(int port, aim_pvs_t* pvs)
{
    aim_printf(pvs, "Debug data for port %d goes here.", port);
}

int
onlp_sfpi_ioctl(int port, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
