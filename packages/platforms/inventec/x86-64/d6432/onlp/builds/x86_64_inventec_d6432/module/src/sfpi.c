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

#define SFP_PRESENT '0'
#define SFP_NOT_PRESENT '1'
#define SFP_ISOLATED '1'
#define SFP_NOT_ISOLATED '0'
#define SFP_PAGE_IDLE 0
#define SFP_PAGE_LOCKED 1
#define MUX_START_INDEX 14
#define QSFP_DEV_ADDR 0x50
#define NUM_OF_QSFP_DD_PORT 32
#define NUM_OF_ALL_PORT (NUM_OF_QSFP_DD_PORT)

#define FRONT_PORT_TO_MUX_INDEX(port) (port+MUX_START_INDEX)
#define PORT_TO_PATH_ID(port)         (port+1)
#define VALIDATE_PORT(p) { if ((p < 0) || (p >= NUM_OF_ALL_PORT)) return ONLP_STATUS_E_PARAM; }

static int _get_swps_page_select_lock(int port);
static int _free_swps_page_select_lock(int port);

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
     * Ports {0, 63}
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
     * Return 1 if present.
     * Return 0 if not present.
     * Return < 0 if error.
     */
    VALIDATE_PORT(port);
    int rv,len;
    char buf[ONLP_CONFIG_INFO_STR_MAX];
    if(onlp_file_read( (uint8_t*)buf, ONLP_CONFIG_INFO_STR_MAX, &len, INV_SFP_PREFIX"present") != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }
    if(buf[port] == SFP_NOT_PRESENT) {
        rv = 0;
    } else if (buf[port] == SFP_PRESENT) {
        rv = 1;
    } else {
        AIM_LOG_ERROR("Unvalid present status %d from port(%d)\r\n",buf[port],port);
        return ONLP_STATUS_E_INTERNAL;
    }
    return rv;
}

int
onlp_sfpi_is_isolated(int port)
{
    /*
     * Return 1 if isolated.
     * Return 0 if normal.
     * Return < 0 if error.
     */
    VALIDATE_PORT(port);
    int len;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    if(onlp_file_read( (uint8_t*)buf, ONLP_CONFIG_INFO_STR_MAX, &len, INV_SFP_PREFIX"isolated_port") != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }
    if(buf[port] == SFP_NOT_ISOLATED) {
        return 0;
    } else if (buf[port] == SFP_ISOLATED) {
        return 1;
    } else {
        AIM_LOG_ERROR("Port-%d: detected invalid isolated status:%d.\r\n",port, buf[port]);
    }
    return ONLP_STATUS_E_INTERNAL;
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
    /* rx los of QSFP-DD are not supported*/
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    /* rx los of QSFP-DD are not supported*/
    return ONLP_STATUS_E_UNSUPPORTED;
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

static int
_get_swps_page_select_lock(int port)
{
    int i   = 0;
    int err = -1;
    int rty_rd = 5;
    int rty_ms = 3;
    int retval = -1;
    int port_id = PORT_TO_PATH_ID(port);

    for (i=0; i<=rty_rd; i++) {
        err = onlp_file_read_int(&retval, INV_SFP_PREFIX"port%d/page_sel_lock", port_id);
        if ((err == ONLP_STATUS_OK) && (retval == SFP_PAGE_IDLE)){
            err = onlp_file_write_int(SFP_PAGE_LOCKED, INV_SFP_PREFIX"port%d/page_sel_lock", port_id);
            if (err == ONLP_STATUS_OK){
                return 1;
            }
        } else if (err == ONLP_STATUS_E_MISSING){
            // Some SWPS module doesn't support "page_sel_lock" and only has "page" attribute.
            // In this case, doesn't need to get page_sel_lock first.
            if (onlp_file_read_int(&retval, INV_SFP_PREFIX"port%d/page", port_id) == ONLP_STATUS_OK) {
                return 1;
            }
        } else {
            usleep(rty_ms * 1000);
            continue;
        }
    }
    return 0;
}

static int
_free_swps_page_select_lock(int port)
{
    int i   = 0;
    int err = -1;
    int rty_rd = 3;
    int rty_ms = 3;
    int retval = -1;
    int port_id = PORT_TO_PATH_ID(port);

    for (i=0; i<=rty_rd; i++) {
        err = onlp_file_write_int(SFP_PAGE_IDLE, INV_SFP_PREFIX"port%d/page_sel_lock", port_id);
        if (err == ONLP_STATUS_OK){
            return 1;
        } else if (err == ONLP_STATUS_E_MISSING){
            if (onlp_file_read_int(&retval, INV_SFP_PREFIX"port%d/page", port_id) == ONLP_STATUS_OK) {
                return 1;
            }
        } else {
            usleep(rty_ms * 1000);
            continue;
        }
    }
    // Free fail case: SWPS driver will implement auto free mechanism.
    return 0;
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
    VALIDATE_PORT(port);
    memset(data, 0, 256);

    int err = -1;
    int page_id = 0;
    int port_id = PORT_TO_PATH_ID(port);
    int bus_num = FRONT_PORT_TO_MUX_INDEX(port);

    if(onlp_sfpi_is_present(port) != 1) {
        AIM_LOG_ERROR("Port-%d can not read eeprom [err]:not present.\r\n", port);
        return ONLP_STATUS_E_MISSING;
    }
    if(onlp_sfpi_is_isolated(port) != 0) {
        AIM_LOG_ERROR("Port-%d can not read eeprom [err]:isolated.\r\n", port);
        return ONLP_STATUS_E_MISSING;
    }
    if (_get_swps_page_select_lock(port) != 1) {
        AIM_LOG_ERROR("Port-%d can not read eeprom [err]:lock fail.\r\n", port);
        return ONLP_STATUS_E_I2C;
    }
    err = onlp_file_write_int(page_id, INV_SFP_PREFIX"port%d/page", port_id);
    if(err != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Port-%d can not read eeprom [err]:select Page fail.\r\n", port);
        err = ONLP_STATUS_E_I2C;
        goto out_onlp_sfpi_eeprom_read;
    }
    err = onlp_i2c_block_read(bus_num, QSFP_DEV_ADDR, 0, 256, data, ONLP_I2C_F_FORCE);
    if(err != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Port-%d can not read eeprom [err]:I2C_Block_READ fail <err>:%d.\r\n", port, err);
    }

out_onlp_sfpi_eeprom_read:
    if (_free_swps_page_select_lock(port) != 1) {
        AIM_LOG_ERROR("Port-%d can not read eeprom [err]:free lock fail.\r\n", port);
    }
    return err;
}

int
onlp_sfpi_dev_readb(int port, uint8_t devaddr, uint8_t addr)
{
    VALIDATE_PORT(port);
    int bus = FRONT_PORT_TO_MUX_INDEX(port);
    return onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    VALIDATE_PORT(port);
    int bus = FRONT_PORT_TO_MUX_INDEX(port);
    return onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_dev_readw(int port, uint8_t devaddr, uint8_t addr)
{
    VALIDATE_PORT(port);
    int bus = FRONT_PORT_TO_MUX_INDEX(port);
    return onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr, uint16_t value)
{
    VALIDATE_PORT(port);
    int bus = FRONT_PORT_TO_MUX_INDEX(port);
    return onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_control_supported(int port, onlp_sfp_control_t control, int* rv)
{
    *rv = 0;
    if(port >= 0 && port < NUM_OF_QSFP_DD_PORT) {
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
    int port_id=PORT_TO_PATH_ID(port);

    if(port >= 0 && port < NUM_OF_QSFP_DD_PORT) {
        switch (control) {
        case ONLP_SFP_CONTROL_RESET_STATE:
            ret = onlp_file_write_int(value, INV_SFP_PREFIX"port%d/reset", port_id);
            break;
        case ONLP_SFP_CONTROL_LP_MODE:
            ret = onlp_file_write_int(value, INV_SFP_PREFIX"port%d/lpmod", port_id);
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
    int port_id = PORT_TO_PATH_ID(port);

    if(port >= 0 && port < NUM_OF_QSFP_DD_PORT) {
        switch (control) {
        /*the value of /port(id)/reset
        0: in reset state; 1:not in reset state*/
        case ONLP_SFP_CONTROL_RESET_STATE:
            ret = onlp_file_read_int(value, INV_SFP_PREFIX"port%d/reset", port_id);
            *value=(!(*value));
            break;
        case ONLP_SFP_CONTROL_LP_MODE:
            ret = onlp_file_read_int(value, INV_SFP_PREFIX"port%d/lpmode", port_id);
            break;
        default:
            break;
        }
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

