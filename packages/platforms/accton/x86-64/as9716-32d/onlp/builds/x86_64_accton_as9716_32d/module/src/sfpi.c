/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2013 Accton Technology Corporation.
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
#include "x86_64_accton_as9716_32d_int.h"
#include "x86_64_accton_as9716_32d_log.h"

#define NUM_OF_PORT 34

#define PORT_EEPROM_FORMAT              "/sys/bus/i2c/devices/%d-0050/eeprom"
#define MODULE_PRESENT_FORMAT		    "/sys/bus/i2c/devices/%d-00%d/module_present_%d"
#define MODULE_RXLOS_FORMAT             "/sys/bus/i2c/devices/%d-00%d/module_rx_los_%d"
#define MODULE_TXFAULT_FORMAT           "/sys/bus/i2c/devices/%d-00%d/module_tx_fault_%d"
#define MODULE_TXDISABLE_FORMAT         "/sys/bus/i2c/devices/%d-00%d/module_tx_disable_%d"

int sfp_map_bus[] ={25, 26, 27, 28, 29, 30, 31, 32,
                    33, 34, 35, 36, 37, 38, 39, 40,
                    41, 42, 43, 44, 45, 46, 47, 48,
                    49, 50, 51, 52, 53, 54, 55, 56,
                    57, 58};

/************************************************************
 *
 * SFPI Entry Points
 *
 ***********************************************************/

int
onlp_sfpi_type_get(onlp_oid_id_t port, onlp_sfp_type_t* rtype)
{
    if (port < 32)
        *rtype = ONLP_SFP_TYPE_QSFP28;
    else
        *rtype = ONLP_SFP_TYPE_SFP;
    return 0;
}

int
onlp_sfpi_map_bus_index(int port)
{
    if(port < 0 || port >=NUM_OF_PORT)
        return ONLP_STATUS_E_INTERNAL;
    return sfp_map_bus[port];
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    /*
     * Ports {0, 34}
     */
    int p;
    
    for(p = 0; p < NUM_OF_PORT; p++) {
        AIM_BITMAP_SET(bmap, p);
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_is_present(onlp_oid_id_t port)
{
    /*
     * Return 1 if present.
     * Return 0 if not present.
     * Return < 0 if error.
     */
    int present;
    int bus, addr;
   
    if(port <0 || port > NUM_OF_PORT)
        return ONLP_STATUS_E_INTERNAL;
        
    if(port >=0 && port < 16)
    {
       addr = 61;
       bus  = 20;
    }
    else
    {
       addr = 62;
       bus  = 21;
    }

    if (onlp_file_read_int(&present, MODULE_PRESENT_FORMAT, bus, addr, (port+1)) < 0) {
        AIM_LOG_ERROR("Unable to read present status from port(%d)\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return present;
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    uint32_t bytes[5];
    int i = 0;
    uint64_t rx_los_all = 0;
    
    bytes[0]=bytes[1]=bytes[2]=bytes[3]=0x0;
    bytes[4]=0x03;
    for(i = AIM_ARRAYSIZE(bytes)-1; i >= 0; i--) {
        rx_los_all <<= 8;
        rx_los_all |= bytes[i];
    }

    /* Populate bitmap */
    for(i = 0; rx_los_all; i++) {
        AIM_BITMAP_MOD(dst, i, (rx_los_all & 1));
        rx_los_all >>= 1;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_dev_read(onlp_oid_id_t port, int devaddr, int addr,
                   uint8_t* dst, int size)
{
    int bus = onlp_sfpi_map_bus_index(port);
    
    return onlp_i2c_block_read(bus, devaddr, addr, size, dst, ONLP_I2C_F_FORCE);
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
    int size = 0;
    
    if(port <0 || port > NUM_OF_PORT)
        return ONLP_STATUS_E_INTERNAL;
    memset(data, 0, 256);

    if(onlp_file_read(data, 256, &size, PORT_EEPROM_FORMAT, onlp_sfpi_map_bus_index(port)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (size != 256) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), size is different!\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_dom_read(int port, uint8_t data[256])
{
    FILE* fp;
    char file[64] = {0};

    sprintf(file, PORT_EEPROM_FORMAT, onlp_sfpi_map_bus_index(port));
    fp = fopen(file, "r");
    if(fp == NULL) {
        AIM_LOG_ERROR("Unable to open the eeprom device file of port(%d)", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (fseek(fp, 256, SEEK_CUR) != 0) {
        fclose(fp);
        AIM_LOG_ERROR("Unable to set the file position indicator of port(%d)", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    int ret = fread(data, 1, 256, fp);
    fclose(fp);
    if (ret != 256) {
        AIM_LOG_ERROR("Unable to read the module_eeprom device file of port(%d)", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_dev_readb(onlp_oid_id_t port, int devaddr, int addr)
{
    int bus = onlp_sfpi_map_bus_index(port);
    return onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_dev_writeb(onlp_oid_id_t port, int devaddr, int addr, uint8_t value)
{
    int bus = onlp_sfpi_map_bus_index(port);
    return onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_dev_readw(onlp_oid_id_t port, int devaddr, int addr)
{
    int bus = onlp_sfpi_map_bus_index(port);
    return onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_dev_writew(onlp_oid_id_t port, int devaddr, int addr, uint16_t value)
{
    int bus = onlp_sfpi_map_bus_index(port);
    return onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
}

int
onlp_sfpi_control_set(onlp_oid_id_t port, onlp_sfp_control_t control, int value)
{
    int rv;
    int addr = 62;
    int bus  = 21;

    switch(control)
    {
        case ONLP_SFP_CONTROL_TX_DISABLE:
        {
                if(port==32 || port==33) {
                    if (onlp_file_write_int(0, MODULE_TXDISABLE_FORMAT, bus, addr, (port+1)) < 0) {
                        AIM_LOG_ERROR("Unable to set tx_disable status to port(%d)\r\n", port);
                        rv = ONLP_STATUS_E_INTERNAL;
                    }
                    else {
                        rv = ONLP_STATUS_OK;
                    }
                }
                else {
                    rv = ONLP_STATUS_E_UNSUPPORTED;
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
onlp_sfpi_control_get(onlp_oid_id_t port, onlp_sfp_control_t control, int* value)
{
    int rv;
    int addr = 62;
    int bus  = 21;
    
    switch(control)
    {
        case ONLP_SFP_CONTROL_RX_LOS:
        {
            if(port==32 || port==33) {
                if (onlp_file_read_int(value, MODULE_RXLOS_FORMAT, bus, addr, (port+1)) < 0) {
                    AIM_LOG_ERROR("Unable to read rx_loss status from port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                }
                else {
                   rv = ONLP_STATUS_OK;
                }
            }
            else {
                rv = ONLP_STATUS_E_UNSUPPORTED;
            }
            break;
        }

        case ONLP_SFP_CONTROL_TX_FAULT:
        {
            if(port==32 || port==33) {
                if (onlp_file_read_int(value, MODULE_TXFAULT_FORMAT, bus, addr, (port+1)) < 0) {
                    AIM_LOG_ERROR("Unable to read tx_fault status from port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                }
                else {
                    rv = ONLP_STATUS_OK;
                }
            }
            else {
                rv = ONLP_STATUS_E_UNSUPPORTED;
            }
            break;
        }

        case ONLP_SFP_CONTROL_TX_DISABLE:
        {
            if(port==32 || port==33) {
                if (onlp_file_read_int(value, MODULE_TXDISABLE_FORMAT, bus, addr, (port+1)) < 0) {
                    AIM_LOG_ERROR("Unable to read tx_disabled status from port(%d)\r\n", port);
                    rv = ONLP_STATUS_E_INTERNAL;
                }
                else {
                    rv = ONLP_STATUS_OK;
                }
            }
            else {
                rv = ONLP_STATUS_E_UNSUPPORTED;
            }
            break;
        }

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

