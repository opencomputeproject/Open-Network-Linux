/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *        Copyright 2014, 2015 Big Switch Networks, Inc.
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
 * SFP Platform Implementation Interface.
 *
 ***********************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <onlplib/sfp.h>
#include <onlplib/file.h>
#include <onlp/platformi/sfpi.h>
#include <x86_64_ufispace_s9700_53dx/x86_64_ufispace_s9700_53dx_config.h>
#include "x86_64_ufispace_s9700_53dx_log.h"
#include "platform_lib.h"

int
onlp_sfpi_init(void)
{  
    lock_init();
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    int p;
    for(p = 0; p < PORT_NUM; p++) {
        AIM_BITMAP_SET(bmap, p);
    }
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_is_present(int port)
{
    int status=1;
    
    //QSFP, QSFPDD, SFP Ports
    if (port < (QSFP_NUM + QSFPDD_NUM)) { //QSFP, QSFPDD
        if (qsfp_present_get(port, &status) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("qsfp_presnet_get() failed, port=%d\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else if (port >= (QSFP_NUM + QSFPDD_NUM) && port < PORT_NUM) { //SFP
        if (sfp_present_get(port, &status) != ONLP_STATUS_OK) {
            AIM_LOG_ERROR("sfp_presnet_get() failed, port=%d\n", port);
            return ONLP_STATUS_E_INTERNAL;
        }
    } else { //unkonwn ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    return status;
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int p = 0;
    int rc = 0;

    for (p = 0; p < PORT_NUM; p++) {
        rc = onlp_sfpi_is_present(p);
        AIM_BITMAP_MOD(dst, p, rc);
    }

    return ONLP_STATUS_OK;
}

/*
 * This function reads the SFPs idrom and returns in
 * in the data buffer provided.
 */
int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{   
    char eeprom_path[512];
    int size = 0;
    int real_port = 0;
    int ret;
    
    memset(eeprom_path, 0, sizeof(eeprom_path));    
    memset(data, 0, 256);

    //QSFP, QSFPDD, SFP real ports
    if (port >= 20 && port <=39) { //QSFP
        real_port = (port % 2) == 0 ? port + 1 : port - 1;
        snprintf(eeprom_path, sizeof(eeprom_path), "/sys/bus/i2c/devices/%d-0050/eeprom", real_port+25);
    } else if (port >= 0 && port < (QSFP_NUM + QSFPDD_NUM)) { //QSFPDD
        real_port = port;
        snprintf(eeprom_path, sizeof(eeprom_path), "/sys/bus/i2c/devices/%d-0050/eeprom", real_port+25);
    } else if (port >= (QSFP_NUM + QSFPDD_NUM) && port < PORT_NUM) { //SFP
        real_port = port - QSFP_NUM - QSFPDD_NUM;
        if (real_port == 0) {
            ret = system("ethtool -m eth1 raw on length 256 > /tmp/.sfp.eth1.eeprom");
            if (ret == 0) {
                snprintf(eeprom_path, sizeof(eeprom_path), "/tmp/.sfp.eth1.eeprom");
            } else {
                AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
                return ONLP_STATUS_E_INTERNAL;
            }
        } else {
            ret = system("ethtool -m eth2 raw on length 256 > /tmp/.sfp.eth2.eeprom");
            if (ret == 0) {
                snprintf(eeprom_path, sizeof(eeprom_path), "/tmp/.sfp.eth2.eeprom");
            } else {
                AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
                return ONLP_STATUS_E_INTERNAL;
            }
        }
    } else { //unknown ports
        AIM_LOG_ERROR("unknown ports, port=%d\n", port);
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(onlp_file_read(data, 256, &size, eeprom_path) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d)\r\n", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (size != 256) {
        AIM_LOG_ERROR("Unable to read eeprom from port(%d), size is different!\r\n", port);
        check_and_do_i2c_mux_reset(port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int i=0, value=0, rc=0;
    int qsfpx_num = (QSFP_NUM + QSFPDD_NUM);

    /* Populate bitmap - QSFP and QSFPDD ports */
    for(i = 0; i < qsfpx_num; i++) {
        AIM_BITMAP_MOD(dst, i, 0);
    }

    /* Populate bitmap - SFP+ ports */
    for(i = qsfpx_num; i < PORT_NUM; i++) {
        if ((rc=onlp_sfpi_control_get(i, ONLP_SFP_CONTROL_RX_LOS, &value)) != ONLP_STATUS_OK) {
            return ONLP_STATUS_E_INTERNAL;
        } else {
            AIM_BITMAP_MOD(dst, i, value);
        }
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_control_get(int port, onlp_sfp_control_t control, int* value)
{
    int rc;
    uint8_t data[8];
    int data_len = 0, data_value = 0, data_mask = 0;
    char sysfs_path[128];
    int cpld_addr = 0x30;
    int qsfpx_num = (QSFP_NUM + QSFPDD_NUM);
    
    memset(data, 0, sizeof(data));
    memset(sysfs_path, 0, sizeof(sysfs_path));

    //QSFPDD ports are not supported
    if (port < qsfpx_num || port >= PORT_NUM) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    //sysfs path
    if (control == ONLP_SFP_CONTROL_RX_LOS || control == ONLP_SFP_CONTROL_TX_FAULT) {
        snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_port_status", I2C_BUS_1, cpld_addr);
    } else if (control == ONLP_SFP_CONTROL_TX_DISABLE) {
        snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_port_config", I2C_BUS_1, cpld_addr);
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if ((rc = onlp_file_read(data, sizeof(data), &data_len, sysfs_path)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, %s", rc, sysfs_path);
        return ONLP_STATUS_E_INTERNAL;
    }

    //hex to int
    data_value = (int) strtol((char *)data, NULL, 0);

    switch(control)
        {
        case ONLP_SFP_CONTROL_RX_LOS:
            {
                //SFP+ Port 0
                if ( port == qsfpx_num ) {
                    data_mask = 0x04;
                } else { //SFP+ Port 1
                    data_mask = 0x40;
                }
                *value = data_value & data_mask;
                rc = ONLP_STATUS_OK;
                break;
            }

        case ONLP_SFP_CONTROL_TX_FAULT:
            {
                //SFP+ Port 0
                if ( port == qsfpx_num ) {
                    data_mask = 0x02;
                } else { //SFP+ Port 1
                    data_mask = 0x20;
                }
                *value = data_value & data_mask;
                rc = ONLP_STATUS_OK;
                break;
            }

        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                //SFP+ Port 0
                if ( port == qsfpx_num ) {
                    data_mask = 0x01;
                } else { //SFP+ Port 1
                    data_mask = 0x10;
                }
                *value = data_value & data_mask;
                rc = ONLP_STATUS_OK;
                break;
            }

        default:
            rc = ONLP_STATUS_E_UNSUPPORTED;
        }

    return rc;
}

int
onlp_sfpi_control_set(int port, onlp_sfp_control_t control, int value)
{
    int rc;
    uint8_t data[8];
    int data_len = 0, data_value = 0, data_mask = 0;
    char sysfs_path[128];
    int cpld_addr = 0x30;
    int qsfpx_num = (QSFP_NUM + QSFPDD_NUM);
    int reg_val = 0;
    
    memset(data, 0, sizeof(data));
    memset(sysfs_path, 0, sizeof(sysfs_path));

    //QSFPDD ports are not supported
    if (port < qsfpx_num || port >= PORT_NUM) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    //sysfs path
    if (control == ONLP_SFP_CONTROL_TX_DISABLE) {
        snprintf(sysfs_path, sizeof(sysfs_path), SYS_DEV "%d-%04x/cpld_sfp_port_config", I2C_BUS_1, cpld_addr);
    } else {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if ((rc = onlp_file_read(data, sizeof(data), &data_len, sysfs_path)) != ONLP_STATUS_OK) {
        AIM_LOG_ERROR("onlp_file_read failed, error=%d, %s", rc, sysfs_path);
        return ONLP_STATUS_E_INTERNAL;
    }

    //hex to int
    data_value = (int) strtol((char *)data, NULL, 0);

    switch(control)
        {

        case ONLP_SFP_CONTROL_TX_DISABLE:
            {
                //SFP+ Port 0
                if (port == qsfpx_num) {
                    data_mask = 0x01;
                } else { //SFP+ Port 1
                    data_mask = 0x10;
                }

                if (value == 1) {
                    reg_val = data_value | data_mask;
                } else {
                    reg_val = data_value & ~data_mask;
                }
                if ((rc = onlp_file_write_int(reg_val, sysfs_path)) != ONLP_STATUS_OK) {
                    AIM_LOG_ERROR("Unable to write cpld_sfp_port_config, error=%d, sysfs=%s, reg_val=%x", rc, sysfs_path, reg_val);
                    return ONLP_STATUS_E_INTERNAL;
                }
                rc = ONLP_STATUS_OK;
                break;
            }

        default:
            rc = ONLP_STATUS_E_UNSUPPORTED;
        }

    return rc;
}

/*
 * De-initialize the SFPI subsystem.
 */
int
onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}
