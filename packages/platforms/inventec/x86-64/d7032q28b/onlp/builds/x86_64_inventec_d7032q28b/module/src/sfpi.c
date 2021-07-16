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

#define SFP_PRESENT 0
#define SFP_NOT_PRESENT 1
#define MUX_START_INDEX 12
#define QSFP_DEV_ADDR 0x50
#define NUM_OF_QSFP_PORT 32
#define NUM_OF_ALL_PORT (NUM_OF_QSFP_PORT)

static const int sfp_mux_index[] = {
    22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37,
    6,  7,  8,  9, 10, 11, 12, 13,
    14, 15, 16, 17, 18, 19, 20, 21
};

//#define FRONT_PORT_TO_MUX_INDEX(port) (port+MUX_START_INDEX)
#define VALIDATE_PORT(p) { if ((p < 0) || (p >= NUM_OF_ALL_PORT)) return ONLP_STATUS_E_PARAM; }
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

int onlp_sfpi_port_map(int port, int* rport)
{
    VALIDATE_PORT(port);
    *rport = port;
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    /*
     * Ports {0, 32}
     */
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
     * Return 0 if present.
     * Return 1 if not present.
     * Return < 0 if error.
     */
    VALIDATE_PORT(port);
    int rv,present_data;
    rv= onlp_file_read_int(&present_data, INV_SFP_PREFIX"port%d/present", port);
    if(rv==ONLP_STATUS_OK) {
        if( present_data== SFP_NOT_PRESENT) {
            rv = 0;
        } else if (present_data == SFP_PRESENT ) {
            rv = 1;
        } else {
            AIM_LOG_ERROR("[ONLP][SFP] Unvalid present status %d from port(%d)\r\n",present_data,port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else {
        AIM_LOG_ERROR("[ONLP][SFP] Unable to read present status of port(%d)\n",port);
    }
    return rv;
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    AIM_BITMAP_CLR_ALL(dst);
    int port;
    for(port = 0; port < NUM_OF_ALL_PORT; port++) {
        if(onlp_sfpi_is_present(port) == 1) {
            AIM_BITMAP_MOD(dst, port, 1);
        } else if(onlp_sfpi_is_present(port) == 0) {
            AIM_BITMAP_MOD(dst, port, 0);
        } else {
            return ONLP_STATUS_E_INTERNAL;
        }
    }
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_is_rx_los(int port)
{
    /* rx los of QSFP are not supported*/
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    /* rx los of QSFP are not supported*/
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_sfpi_port2chan(int port)  // port start from 0
{
    if( port>=0 && port<NUM_OF_ALL_PORT) {
        return sfp_mux_index[port];
    }
    return ONLP_STATUS_E_INVALID;
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

    VALIDATE_PORT(port);
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
    VALIDATE_PORT(port);
    int bus = onlp_sfpi_port2chan(port);
    return onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    VALIDATE_PORT(port);
    int bus = onlp_sfpi_port2chan(port);
    return onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_dev_readw(int port, uint8_t devaddr, uint8_t addr)
{
    VALIDATE_PORT(port);
    int bus = onlp_sfpi_port2chan(port);
    return onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    VALIDATE_PORT(port);
    int bus = onlp_sfpi_port2chan(port);
    return onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_control_supported(int port, onlp_sfp_control_t control, int* rv)
{
    *rv = 0;
    if(port >= 0 && port < NUM_OF_QSFP_PORT) {
        switch (control) {
        case ONLP_SFP_CONTROL_RESET_STATE:
        case ONLP_SFP_CONTROL_LP_MODE:
            *rv = 1;
            break;
        default:
            break;
        }
    }
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{
    int ret = ONLP_STATUS_E_UNSUPPORTED;

    if(port >= 0 && port < NUM_OF_QSFP_PORT) {
        switch (control) {
        case ONLP_SFP_CONTROL_RESET_STATE:
            ret = onlp_file_write_int(value, INV_SFP_PREFIX"port%d/reset", port);
            break;
        case ONLP_SFP_CONTROL_LP_MODE:
            ret = onlp_file_write_int(value, INV_SFP_PREFIX"port%d/lpmod", port);
            break;
        default:
            break;
        }
    }
    return ret;
}

int
onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
{
    int ret = ONLP_STATUS_E_UNSUPPORTED;

    if(port >= 0 && port < NUM_OF_QSFP_PORT) {
        switch (control) {
        /*the value of /port(id)/reset
        0: in reset state; 1:not in reset state*/
        case ONLP_SFP_CONTROL_RESET_STATE:
            ret = onlp_file_read_int(value, INV_SFP_PREFIX"port%d/reset", port);
            *value=(!(*value));
            break;
        case ONLP_SFP_CONTROL_LP_MODE:
            ret = onlp_file_read_int(value, INV_SFP_PREFIX"port%d/lpmod", port);
            break;
        default:
            break;
        }
    } else {
        ret=ONLP_STATUS_E_INVALID;
        AIM_LOG_ERROR("[ONLP][SFP] Invalid port id in %s\n",__FUNCTION__);
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
