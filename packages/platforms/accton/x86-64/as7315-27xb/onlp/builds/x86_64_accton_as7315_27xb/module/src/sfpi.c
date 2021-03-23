/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2017 Accton Technology Corporation.
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
#include <onlplib/i2c.h>
#include <onlplib/file.h>
#include "x86_64_accton_as7315_27xb_int.h"
#include "x86_64_accton_as7315_27xb_log.h"


enum port_type {
    PORT_TYPE_SFP,
    PORT_TYPE_QSFP,
    PORT_TYPE_MAX
};

#define NUM_SFP     24
#define NUM_QSFP    3
#define NUM_PORT    (NUM_SFP+NUM_QSFP)
#define SIDE_BAND_PATH                  "/sys/bus/i2c/devices/%d-00%02x/"
#define PORT_EEPROM_FORMAT              "/sys/bus/i2c/devices/%d-0050/eeprom"

const int PORT_NUM[PORT_TYPE_MAX] = {NUM_SFP, NUM_QSFP};
const int SB_CFG[PORT_TYPE_MAX][2] = {{8, 0x63}, {7, 0x64}};

/************************************************************
 *
 * SFPI Entry Points
 *
 ***********************************************************/
static enum port_type get_port_type(int port)
{
    if (port < NUM_SFP)
        return PORT_TYPE_SFP;
    else if (port < NUM_PORT)
        return PORT_TYPE_QSFP;
    else
        return ONLP_STATUS_E_INVALID ;
}

static int port_to_index(int port)
{
    if ((port > (NUM_SFP-1)) && (port < NUM_PORT))
        return (port%NUM_SFP)+1;
    else
        return (port%NUM_SFP)+1;
}


int
onlp_sfpi_init(void)
{
    /* Called at initialization time */
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    /*
     * Ports {0, 54}
     */
    int p;

    for(p = 0; p < NUM_PORT; p++) {
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
    int present, type;
    int bus, offset;

    type = get_port_type(port);
    if (type < 0)
    {
        return type;
    }

    bus = SB_CFG[type][0];
    offset = SB_CFG[type][1];
    if (onlp_file_read_int(&present, SIDE_BAND_PATH"present_%d",
                           bus, offset, port_to_index(port)) < 0) {
        AIM_LOG_ERROR("Unable to read present status from port(%d)\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return present;
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i, j, start;
    int bus, offset;

    uint64_t    presence_all;
    uint64_t bmp[PORT_TYPE_MAX] = {0};


    for (i = 0; i < PORT_TYPE_MAX; i++) {
        char *string = NULL;
        bus = SB_CFG[i][0];
        offset = SB_CFG[i][1];
        if (onlp_file_read_str(&string, SIDE_BAND_PATH"module_present_all", bus, offset) < 0) {
            if ( string ) {
                aim_free( string );
            }
            AIM_LOG_ERROR("Unable to read presence_bitmap\r\n");
            return ONLP_STATUS_E_INTERNAL;
        }
        if (string) {
            bmp[i] = strtoul(string, NULL, 16);
            aim_free(string);
        }
    }

    presence_all = start = 0;
    for (i = 0; i < PORT_TYPE_MAX; i++) {
        j = PORT_NUM[i];
        presence_all |= (bmp[i] & ((1<<j) -1)) << start;
        start = j;
    }

    AIM_BITMAP_CLR_ALL(dst);
    /* Populate bitmap */
    for (i = 0; presence_all; i++) {
        if (i >= NUM_SFP) {
            i = (i - NUM_SFP) + NUM_SFP;
        }
        AIM_BITMAP_MOD(dst, i, (presence_all & 1));
        presence_all >>= 1;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i, j, start;
    int bus, offset;

    uint64_t    all_bmp;
    uint64_t bmp[PORT_TYPE_MAX] = {0};

    for (i = 0; i < PORT_TYPE_MAX; i++) {
        char *string = NULL;
        bus = SB_CFG[i][0];
        offset = SB_CFG[i][1];
        if (onlp_file_read_str(&string, SIDE_BAND_PATH"rx_los_all", bus, offset) < 0) {
            if (string) {
                aim_free(string);
            }
            AIM_LOG_ERROR("Unable to read presence_bitmap\r\n");
            return ONLP_STATUS_E_INTERNAL;
        }
        if (string) {
            bmp[i] = strtoul(string, NULL, 16);
            aim_free(string);
        }
    }

    all_bmp = start = 0;
    for (i = 0; i < PORT_TYPE_MAX; i++) {
        j = PORT_NUM[i];
        all_bmp |= (bmp[i] & ((1<<j) -1)) << start;
        start = j;
    }

    AIM_BITMAP_CLR_ALL(dst);
    /* Populate bitmap, QSFP has no rx_los signals here.*/
    for (i = 0; all_bmp; i++) {
        AIM_BITMAP_MOD(dst, i, (all_bmp & 1));
        all_bmp >>= 1;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
#if 1
    /*
     * Read the SFP eeprom into data[]
     *
     * Return MISSING if SFP is missing.
     * Return OK if eeprom is read
     */
    int type, bus;
    int size = 0;
    int bus_start[]= {26, 21}; /*26~49. QSFP:21,22,23 for 100G port, from left to right. */

    type = get_port_type(port);
    if (type < 0)
    {
        return ONLP_STATUS_E_INTERNAL;
    }


    if (type == PORT_TYPE_QSFP) {
        bus = bus_start[type];
        bus += port - NUM_SFP;
    }
    else {
        bus = bus_start[type] + (port%NUM_SFP);
    }

    memset(data, 0, 256);
    if(onlp_file_read(data, 256, &size, PORT_EEPROM_FORMAT, bus) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (size != 256) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), size is different!\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }
#endif
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{
    int type, bus, offset;
    int rv = ONLP_STATUS_OK;

    type = get_port_type(port);
    if (type < 0)
    {
        return type;
    }

    bus = SB_CFG[type][0];
    offset = SB_CFG[type][1];
    switch(control) {
    case ONLP_SFP_CONTROL_TX_DISABLE:
    {
        if (type == PORT_TYPE_QSFP) {
            rv = ONLP_STATUS_E_UNSUPPORTED;
            break;
        }
        if (onlp_file_write_int(value, SIDE_BAND_PATH"tx_disable_%d", bus, offset, port+1) < 0) {
            AIM_LOG_ERROR("Unable to set tx_disable status to port(%d)\r\n", port);
            rv = ONLP_STATUS_E_INTERNAL;
        }
        else {
            rv = ONLP_STATUS_OK;
        }
        break;
    }
    case ONLP_SFP_CONTROL_LP_MODE:
    {
        if (type == PORT_TYPE_SFP) {
            rv = ONLP_STATUS_E_UNSUPPORTED;
            break;
        }

        if (onlp_file_write_int(value, SIDE_BAND_PATH"low_power_mode_%d",
                                bus, offset, port_to_index(port)) < 0) {
            AIM_LOG_ERROR("Unable to set tx_disable status to port(%d)\r\n", port);
            rv = ONLP_STATUS_E_INTERNAL;
        }
        else {
            rv = ONLP_STATUS_OK;
        }
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
    char *attr;
    int type, bus, offset;
    int rv = ONLP_STATUS_OK;

    type = get_port_type(port);
    if (type < 0)
    {
        return type;
    }

    if (port < 0) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    /* Filter out unsupported control upon type */
    switch(control) {
    case ONLP_SFP_CONTROL_RX_LOS:
    case ONLP_SFP_CONTROL_TX_FAULT:
    case ONLP_SFP_CONTROL_TX_DISABLE:
        if (type == PORT_TYPE_QSFP) {
            return ONLP_STATUS_E_UNSUPPORTED;
        }
        break;
    case ONLP_SFP_CONTROL_LP_MODE:
        if (type == PORT_TYPE_SFP) {
            return ONLP_STATUS_E_UNSUPPORTED;
        }
        break;
    default:
        break;
    }

    /* Filter out unsupported control upon type */
    switch(control) {
    case ONLP_SFP_CONTROL_RX_LOS:
        attr = "rx_los";
        break;
    case ONLP_SFP_CONTROL_TX_FAULT:
        attr = "tx_fault";
        break;
    case ONLP_SFP_CONTROL_TX_DISABLE:
        attr = "tx_disable";
        break;
    case ONLP_SFP_CONTROL_LP_MODE:
        attr = "low_power_mode";
        break;
    default:
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    bus = SB_CFG[type][0];
    offset = SB_CFG[type][1];
    switch(control)
    {
    case ONLP_SFP_CONTROL_RX_LOS:
    case ONLP_SFP_CONTROL_TX_FAULT:
    case ONLP_SFP_CONTROL_TX_DISABLE:
    case ONLP_SFP_CONTROL_LP_MODE:
        rv = onlp_file_read_int(value, SIDE_BAND_PATH"%s_%d",
                                bus, offset, attr, port_to_index(port));

        if (rv < 0) {
            AIM_LOG_ERROR("Unable to read %s status from port(%d)\r\n", attr, port);
            rv = ONLP_STATUS_E_INTERNAL;
        }
        else {
            rv = ONLP_STATUS_OK;
        }
        break;

    default:
        rv = ONLP_STATUS_E_UNSUPPORTED;
    }

    return rv;
}

int
onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}

