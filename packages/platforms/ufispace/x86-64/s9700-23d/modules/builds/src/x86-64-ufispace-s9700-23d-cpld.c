/*
 * A i2c cpld driver for the ufispace_apollo
 *
 * Copyright (C) 2017-2019 UfiSpace Technology Corporation.
 * Jason Tsai <jason.cy.tsai@ufispace.com>
 *
 * Based on ad7414.c
 * Copyright 2006 Stefan Roese <sr at denx.de>, DENX Software Engineering
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include "x86-64-ufispace-s9700-23d-cpld.h"

#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...) \
    printk(KERN_INFO "%s:%s[%d]: " fmt "\r\n", \
            __FILE__, __func__, __LINE__, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

#define I2C_READ_BYTE_DATA(ret, lock, i2c_client, reg) \
{ \
    mutex_lock(lock); \
    ret = i2c_smbus_read_byte_data(i2c_client, reg); \
    mutex_unlock(lock); \
}
#define I2C_WRITE_BYTE_DATA(ret, lock, i2c_client, reg, val) \
{ \
        mutex_lock(lock); \
        ret = i2c_smbus_write_byte_data(i2c_client, reg, val); \
        mutex_unlock(lock); \
}

/* CPLD sysfs attributes index  */
enum apollo_cpld_sysfs_attributes {
    CPLD_ACCESS_REG,
    CPLD_REGISTER_VAL,
    //CPLD_PORT_START,
    //CPLD_PORTS,    
    CPLD_VERSION,
    CPLD_ID,
    CPLD_BOARD_TYPE,
    CPLD_EXT_BOARD_TYPE,
    CPLD_INTERRUPT,
    CPLD_SFP_PORT_STATUS,
    CPLD_SFP_PORT_CONFIG,
    CPLD_QSFPDD_NIF_PORT_START,
    CPLD_QSFPDD_NIF_PORTS,
    CPLD_QSFPDD_NIF_PORT_STATUS_0,
    CPLD_QSFPDD_NIF_PORT_STATUS_1,
    CPLD_QSFPDD_NIF_PORT_STATUS_2,
    CPLD_QSFPDD_NIF_PORT_STATUS_3,
    CPLD_QSFPDD_NIF_PORT_STATUS_4,
    CPLD_QSFPDD_NIF_PORT_STATUS_5,
    CPLD_QSFPDD_NIF_PORT_STATUS_6,
    CPLD_QSFPDD_NIF_PORT_STATUS_7,
    CPLD_QSFPDD_NIF_PORT_STATUS_8,
    CPLD_QSFPDD_NIF_PORT_STATUS_9,
    CPLD_QSFPDD_NIF_PORT_CONFIG_0,
    CPLD_QSFPDD_NIF_PORT_CONFIG_1,
    CPLD_QSFPDD_NIF_PORT_CONFIG_2,
    CPLD_QSFPDD_NIF_PORT_CONFIG_3,
    CPLD_QSFPDD_NIF_PORT_CONFIG_4,
    CPLD_QSFPDD_NIF_PORT_CONFIG_5,
    CPLD_QSFPDD_NIF_PORT_CONFIG_6,
    CPLD_QSFPDD_NIF_PORT_CONFIG_7,
    CPLD_QSFPDD_NIF_PORT_CONFIG_8,
    CPLD_QSFPDD_NIF_PORT_CONFIG_9,
    CPLD_QSFPDD_FAB_PORT_START,
    CPLD_QSFPDD_FAB_PORTS,
    CPLD_QSFPDD_FAB_PORT_STATUS_0,
    CPLD_QSFPDD_FAB_PORT_STATUS_1,
    CPLD_QSFPDD_FAB_PORT_STATUS_2,
    CPLD_QSFPDD_FAB_PORT_STATUS_3,
    CPLD_QSFPDD_FAB_PORT_STATUS_4,
    CPLD_QSFPDD_FAB_PORT_STATUS_5,
    CPLD_QSFPDD_FAB_PORT_STATUS_6,
    CPLD_QSFPDD_FAB_PORT_STATUS_7,
    CPLD_QSFPDD_FAB_PORT_STATUS_8,
    CPLD_QSFPDD_FAB_PORT_STATUS_9,
    CPLD_QSFPDD_FAB_PORT_CONFIG_0,
    CPLD_QSFPDD_FAB_PORT_CONFIG_1,
    CPLD_QSFPDD_FAB_PORT_CONFIG_2,
    CPLD_QSFPDD_FAB_PORT_CONFIG_3,
    CPLD_QSFPDD_FAB_PORT_CONFIG_4,
    CPLD_QSFPDD_FAB_PORT_CONFIG_5,
    CPLD_QSFPDD_FAB_PORT_CONFIG_6,
    CPLD_QSFPDD_FAB_PORT_CONFIG_7,
    CPLD_QSFPDD_FAB_PORT_CONFIG_8,
    CPLD_QSFPDD_FAB_PORT_CONFIG_9,
    CPLD_10GMUX_CONFIG,
    CPLD_BMC_STATUS,
    CPLD_BMC_WATCHDOG,
    CPLD_USB_STATUS,
    CPLD_RESET_CONTROL,
    CPLD_RESET_MAC,
    CPLD_RESET_MAC_2,
    CPLD_SFP_LED,
    CPLD_SFP_LED_BLINK,
    CPLD_QSFPDD_FAB_LED_0,
    CPLD_QSFPDD_FAB_LED_1,
    CPLD_QSFPDD_FAB_LED_2,
    CPLD_QSFPDD_FAB_LED_3,
    CPLD_QSFPDD_FAB_LED_4,
    CPLD_QSFPDD_FAB_LED_5,
    CPLD_QSFPDD_FAB_LED_6,
    CPLD_QSFPDD_FAB_LED_7,
    CPLD_QSFPDD_FAB_LED_8,
    CPLD_QSFPDD_FAB_LED_9,
    CPLD_QSFPDD_FAB_LED_10,
    CPLD_QSFPDD_FAB_LED_11,
    CPLD_QSFPDD_FAB_LED_12,
    CPLD_SYSTEM_LED_0,
    CPLD_SYSTEM_LED_1,
    CPLD_PSU_STATUS_0,
    CPLD_PSU_STATUS_1,
};

/* CPLD sysfs attributes hook functions  */
static ssize_t read_access_register(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_access_register(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_register_value(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_register_value(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_cpld_version(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_cpld_id(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_board_type(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_ext_board_type(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_cpld_interrupt(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_sfp_port_status(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_sfp_port_config(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_sfp_port_config(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_10gmux_config(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_10gmux_config(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_bmc_status(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_bmc_watchdog(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_bmc_watchdog(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_usb_status(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_reset_control(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_reset_control(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_reset_mac(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_reset_mac(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_reset_mac_2(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_reset_mac_2(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
//NIF
static ssize_t get_qsfpdd_nif_port_start(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t get_qsfpdd_nif_ports(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_qsfpdd_nif_port_status(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_qsfpdd_nif_port_config(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_qsfpdd_nif_port_config(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
//FAB                
static ssize_t get_qsfpdd_fab_port_start(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t get_qsfpdd_fab_ports(struct device *dev,
                struct device_attribute *da, char *buf);                      
static ssize_t read_qsfpdd_fab_port_status(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t read_qsfpdd_fab_port_config(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_qsfpdd_fab_port_config(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
//LED                
static ssize_t read_qsfpdd_fab_led(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_qsfpdd_fab_led(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
static ssize_t read_system_led(struct device *dev,
                struct device_attribute *da, char *buf);
static ssize_t write_system_led(struct device *dev,
        struct device_attribute *da, const char *buf, size_t count);
//PSU
static ssize_t read_psu_status(struct device *dev,
                struct device_attribute *da, char *buf);
static LIST_HEAD(cpld_client_list);  /* client list for cpld */
static struct mutex list_lock;  /* mutex for client list */

struct cpld_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

struct cpld_data {
    int index;                  /* CPLD index */
    struct mutex access_lock;       /* mutex for cpld access */
    u8 access_reg;              /* register to access */
};

/* CPLD device id and data */
static const struct i2c_device_id apollo_cpld_id[] = {
    { "s9700_23d_cpld1",  cpld1 },
    { "s9700_23d_cpld2",  cpld2 },
    { "s9700_23d_cpld3",  cpld3 },
    {}
};

/* Addresses scanned for apollo_cpld */
static const unsigned short cpld_i2c_addr[] = { 0x30, 0x31, 0x32, I2C_CLIENT_END };

/* define all support register access of cpld in attribute */
static SENSOR_DEVICE_ATTR(cpld_access_register, S_IWUSR | S_IRUGO,
        read_access_register, write_access_register, CPLD_ACCESS_REG);
static SENSOR_DEVICE_ATTR(cpld_register_value, S_IWUSR | S_IRUGO,
        read_register_value, write_register_value, CPLD_REGISTER_VAL);
static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO,
                read_cpld_version, NULL, CPLD_VERSION);
static SENSOR_DEVICE_ATTR(cpld_id, S_IRUGO, read_cpld_id, NULL, CPLD_ID);
static SENSOR_DEVICE_ATTR(cpld_board_type, S_IRUGO,
                read_board_type, NULL, CPLD_BOARD_TYPE);
static SENSOR_DEVICE_ATTR(cpld_ext_board_type, S_IRUGO,
        read_ext_board_type, NULL, CPLD_EXT_BOARD_TYPE);
static SENSOR_DEVICE_ATTR(cpld_interrupt, S_IRUGO,
        read_cpld_interrupt, NULL, CPLD_INTERRUPT);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_status, S_IRUGO,
            read_sfp_port_status, NULL, CPLD_SFP_PORT_STATUS);
static SENSOR_DEVICE_ATTR(cpld_sfp_port_config, S_IWUSR | S_IRUGO,
    read_sfp_port_config, write_sfp_port_config, CPLD_SFP_PORT_CONFIG);
static SENSOR_DEVICE_ATTR(cpld_10gmux_config, S_IWUSR | S_IRUGO,
                read_10gmux_config, write_10gmux_config,
                CPLD_10GMUX_CONFIG);
static SENSOR_DEVICE_ATTR(cpld_bmc_status, S_IRUGO,
                read_bmc_status, NULL, CPLD_BMC_STATUS);
static SENSOR_DEVICE_ATTR(cpld_bmc_watchdog, S_IWUSR | S_IRUGO,
                read_bmc_watchdog, write_bmc_watchdog,
                CPLD_BMC_WATCHDOG);
static SENSOR_DEVICE_ATTR(cpld_usb_status, S_IRUGO,
                read_usb_status, NULL, CPLD_USB_STATUS);
static SENSOR_DEVICE_ATTR(cpld_reset_control, S_IWUSR | S_IRUGO,
                read_reset_control, write_reset_control,
                CPLD_RESET_CONTROL);
static SENSOR_DEVICE_ATTR(cpld_reset_mac, S_IWUSR | S_IRUGO,
                read_reset_mac, write_reset_mac,
                CPLD_RESET_MAC);
static SENSOR_DEVICE_ATTR(cpld_reset_mac_2, S_IWUSR | S_IRUGO,
                read_reset_mac_2, write_reset_mac_2,
                CPLD_RESET_MAC_2);
//NIF                
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_start, S_IRUGO,
            get_qsfpdd_nif_port_start, NULL, CPLD_QSFPDD_NIF_PORT_START);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_ports, S_IRUGO,
                get_qsfpdd_nif_ports, NULL, CPLD_QSFPDD_NIF_PORTS);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_status_0, S_IRUGO,
        read_qsfpdd_nif_port_status, NULL, CPLD_QSFPDD_NIF_PORT_STATUS_0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_status_1, S_IRUGO,
        read_qsfpdd_nif_port_status, NULL, CPLD_QSFPDD_NIF_PORT_STATUS_1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_status_2, S_IRUGO,
        read_qsfpdd_nif_port_status, NULL, CPLD_QSFPDD_NIF_PORT_STATUS_2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_status_3, S_IRUGO,
        read_qsfpdd_nif_port_status, NULL, CPLD_QSFPDD_NIF_PORT_STATUS_3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_status_4, S_IRUGO,
        read_qsfpdd_nif_port_status, NULL, CPLD_QSFPDD_NIF_PORT_STATUS_4);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_status_5, S_IRUGO,
        read_qsfpdd_nif_port_status, NULL, CPLD_QSFPDD_NIF_PORT_STATUS_5);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_status_6, S_IRUGO,
        read_qsfpdd_nif_port_status, NULL, CPLD_QSFPDD_NIF_PORT_STATUS_6);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_status_7, S_IRUGO,
        read_qsfpdd_nif_port_status, NULL, CPLD_QSFPDD_NIF_PORT_STATUS_7);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_status_8, S_IRUGO,
        read_qsfpdd_nif_port_status, NULL, CPLD_QSFPDD_NIF_PORT_STATUS_8);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_status_9, S_IRUGO,
        read_qsfpdd_nif_port_status, NULL, CPLD_QSFPDD_NIF_PORT_STATUS_9);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_config_0, S_IWUSR | S_IRUGO,
                read_qsfpdd_nif_port_config, write_qsfpdd_nif_port_config,
                CPLD_QSFPDD_NIF_PORT_CONFIG_0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_config_1, S_IWUSR | S_IRUGO,
                read_qsfpdd_nif_port_config, write_qsfpdd_nif_port_config, 
                CPLD_QSFPDD_NIF_PORT_CONFIG_1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_config_2, S_IWUSR | S_IRUGO,
                read_qsfpdd_nif_port_config, write_qsfpdd_nif_port_config,
                CPLD_QSFPDD_NIF_PORT_CONFIG_2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_config_3, S_IWUSR | S_IRUGO,
                read_qsfpdd_nif_port_config, write_qsfpdd_nif_port_config,
                CPLD_QSFPDD_NIF_PORT_CONFIG_3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_config_4, S_IWUSR | S_IRUGO,
                read_qsfpdd_nif_port_config, write_qsfpdd_nif_port_config,
                CPLD_QSFPDD_NIF_PORT_CONFIG_4);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_config_5, S_IWUSR | S_IRUGO,
                read_qsfpdd_nif_port_config, write_qsfpdd_nif_port_config,
                CPLD_QSFPDD_NIF_PORT_CONFIG_5);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_config_6, S_IWUSR | S_IRUGO,
                read_qsfpdd_nif_port_config, write_qsfpdd_nif_port_config,
                CPLD_QSFPDD_NIF_PORT_CONFIG_6);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_config_7, S_IWUSR | S_IRUGO,
                read_qsfpdd_nif_port_config, write_qsfpdd_nif_port_config,
                CPLD_QSFPDD_NIF_PORT_CONFIG_7);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_config_8, S_IWUSR | S_IRUGO,
                read_qsfpdd_nif_port_config, write_qsfpdd_nif_port_config,
                CPLD_QSFPDD_NIF_PORT_CONFIG_8);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_nif_port_config_9, S_IWUSR | S_IRUGO,
                read_qsfpdd_nif_port_config, write_qsfpdd_nif_port_config,
                CPLD_QSFPDD_NIF_PORT_CONFIG_9);
//FAB                       
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_start, S_IRUGO,
            get_qsfpdd_fab_port_start, NULL, CPLD_QSFPDD_FAB_PORT_START);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_ports, S_IRUGO,
                get_qsfpdd_fab_ports, NULL, CPLD_QSFPDD_FAB_PORTS);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_status_0, S_IRUGO,
        read_qsfpdd_fab_port_status, NULL, CPLD_QSFPDD_FAB_PORT_STATUS_0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_status_1, S_IRUGO,
        read_qsfpdd_fab_port_status, NULL, CPLD_QSFPDD_FAB_PORT_STATUS_1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_status_2, S_IRUGO,
        read_qsfpdd_fab_port_status, NULL, CPLD_QSFPDD_FAB_PORT_STATUS_2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_status_3, S_IRUGO,
        read_qsfpdd_fab_port_status, NULL, CPLD_QSFPDD_FAB_PORT_STATUS_3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_status_4, S_IRUGO,
        read_qsfpdd_fab_port_status, NULL, CPLD_QSFPDD_FAB_PORT_STATUS_4);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_status_5, S_IRUGO,
        read_qsfpdd_fab_port_status, NULL, CPLD_QSFPDD_FAB_PORT_STATUS_5);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_status_6, S_IRUGO,
        read_qsfpdd_fab_port_status, NULL, CPLD_QSFPDD_FAB_PORT_STATUS_6);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_status_7, S_IRUGO,
        read_qsfpdd_fab_port_status, NULL, CPLD_QSFPDD_FAB_PORT_STATUS_7);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_status_8, S_IRUGO,
        read_qsfpdd_fab_port_status, NULL, CPLD_QSFPDD_FAB_PORT_STATUS_8);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_status_9, S_IRUGO,
        read_qsfpdd_fab_port_status, NULL, CPLD_QSFPDD_FAB_PORT_STATUS_9);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_config_0, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_port_config, write_qsfpdd_fab_port_config,
                CPLD_QSFPDD_FAB_PORT_CONFIG_0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_config_1, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_port_config, write_qsfpdd_fab_port_config, 
                CPLD_QSFPDD_FAB_PORT_CONFIG_1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_config_2, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_port_config, write_qsfpdd_fab_port_config,
                CPLD_QSFPDD_FAB_PORT_CONFIG_2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_config_3, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_port_config, write_qsfpdd_fab_port_config,
                CPLD_QSFPDD_FAB_PORT_CONFIG_3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_config_4, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_port_config, write_qsfpdd_fab_port_config,
                CPLD_QSFPDD_FAB_PORT_CONFIG_4);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_config_5, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_port_config, write_qsfpdd_fab_port_config,
                CPLD_QSFPDD_FAB_PORT_CONFIG_5);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_config_6, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_port_config, write_qsfpdd_fab_port_config,
                CPLD_QSFPDD_FAB_PORT_CONFIG_6);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_config_7, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_port_config, write_qsfpdd_fab_port_config,
                CPLD_QSFPDD_FAB_PORT_CONFIG_7);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_config_8, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_port_config, write_qsfpdd_fab_port_config,
                CPLD_QSFPDD_FAB_PORT_CONFIG_8);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_port_config_9, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_port_config, write_qsfpdd_fab_port_config,
                CPLD_QSFPDD_FAB_PORT_CONFIG_9);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_led_0, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_led, write_qsfpdd_fab_led, CPLD_QSFPDD_FAB_LED_0);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_led_1, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_led, write_qsfpdd_fab_led, CPLD_QSFPDD_FAB_LED_1);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_led_2, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_led, write_qsfpdd_fab_led, CPLD_QSFPDD_FAB_LED_2);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_led_3, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_led, write_qsfpdd_fab_led, CPLD_QSFPDD_FAB_LED_3);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_led_4, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_led, write_qsfpdd_fab_led, CPLD_QSFPDD_FAB_LED_4);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_led_5, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_led, write_qsfpdd_fab_led, CPLD_QSFPDD_FAB_LED_5);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_led_6, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_led, write_qsfpdd_fab_led, CPLD_QSFPDD_FAB_LED_6);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_led_7, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_led, write_qsfpdd_fab_led, CPLD_QSFPDD_FAB_LED_7);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_led_8, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_led, write_qsfpdd_fab_led, CPLD_QSFPDD_FAB_LED_8);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_led_9, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_led, write_qsfpdd_fab_led, CPLD_QSFPDD_FAB_LED_9);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_led_10, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_led, write_qsfpdd_fab_led, CPLD_QSFPDD_FAB_LED_10);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_led_11, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_led, write_qsfpdd_fab_led, CPLD_QSFPDD_FAB_LED_11);
static SENSOR_DEVICE_ATTR(cpld_qsfpdd_fab_led_12, S_IWUSR | S_IRUGO,
                read_qsfpdd_fab_led, write_qsfpdd_fab_led, CPLD_QSFPDD_FAB_LED_12);
static SENSOR_DEVICE_ATTR(cpld_system_led_0, S_IWUSR | S_IRUGO,
                read_system_led, write_system_led, CPLD_SYSTEM_LED_0);
static SENSOR_DEVICE_ATTR(cpld_system_led_1, S_IWUSR | S_IRUGO,
                read_system_led, write_system_led, CPLD_SYSTEM_LED_1);
static SENSOR_DEVICE_ATTR(cpld_psu_status_0, S_IRUGO,
        read_psu_status, NULL, CPLD_PSU_STATUS_0);
static SENSOR_DEVICE_ATTR(cpld_psu_status_1, S_IRUGO,
        read_psu_status, NULL, CPLD_PSU_STATUS_1);        
/* define support attributes of cpldx , total 3 */
/* cpld 1 */
static struct attribute *apollo_cpld1_attributes[] = {
    &sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_board_type.dev_attr.attr,
    &sensor_dev_attr_cpld_ext_board_type.dev_attr.attr,
    &sensor_dev_attr_cpld_interrupt.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_status.dev_attr.attr,
    &sensor_dev_attr_cpld_sfp_port_config.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_start.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_ports.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_status_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_status_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_status_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_status_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_status_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_status_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_status_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_status_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_status_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_status_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_config_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_config_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_config_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_config_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_config_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_config_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_config_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_config_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_config_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_nif_port_config_9.dev_attr.attr,
    &sensor_dev_attr_cpld_10gmux_config.dev_attr.attr,
    &sensor_dev_attr_cpld_bmc_status.dev_attr.attr,
    &sensor_dev_attr_cpld_bmc_watchdog.dev_attr.attr,
    &sensor_dev_attr_cpld_usb_status.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_control.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_mac.dev_attr.attr,
    &sensor_dev_attr_cpld_reset_mac_2.dev_attr.attr,
    &sensor_dev_attr_cpld_psu_status_0.dev_attr.attr,
    &sensor_dev_attr_cpld_psu_status_1.dev_attr.attr,
    &sensor_dev_attr_cpld_system_led_0.dev_attr.attr,
    &sensor_dev_attr_cpld_system_led_1.dev_attr.attr,
    NULL
};

/* cpld 2 */
static struct attribute *apollo_cpld2_attributes[] = {
    &sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_interrupt.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_start.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_ports.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_status_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_status_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_status_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_config_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_config_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_config_2.dev_attr.attr,
    NULL
};

/* cpld 3 */
static struct attribute *apollo_cpld3_attributes[] = {
    &sensor_dev_attr_cpld_access_register.dev_attr.attr,
    &sensor_dev_attr_cpld_register_value.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_id.dev_attr.attr,
    &sensor_dev_attr_cpld_interrupt.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_start.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_ports.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_status_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_status_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_status_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_status_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_status_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_status_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_status_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_status_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_status_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_status_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_config_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_config_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_config_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_config_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_config_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_config_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_config_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_config_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_config_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_port_config_9.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_led_0.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_led_1.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_led_2.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_led_3.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_led_4.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_led_5.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_led_6.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_led_7.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_led_8.dev_attr.attr,
    &sensor_dev_attr_cpld_qsfpdd_fab_led_9.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfpdd_fab_led_10.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfpdd_fab_led_11.dev_attr.attr,
	&sensor_dev_attr_cpld_qsfpdd_fab_led_12.dev_attr.attr,
    NULL
};

/* cpld 1 attributes group */
static const struct attribute_group apollo_cpld1_group = {
    .attrs = apollo_cpld1_attributes,
};
/* cpld 2 attributes group */
static const struct attribute_group apollo_cpld2_group = {
    .attrs = apollo_cpld2_attributes,
};
/* cpld 3 attributes group */
static const struct attribute_group apollo_cpld3_group = {
    .attrs = apollo_cpld3_attributes,
};

/* read access register from cpld data */
static ssize_t read_access_register(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg = data->access_reg;

    return sprintf(buf, "0x%x\n", reg);
}

/* write access register to cpld data */
static ssize_t write_access_register(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;

    if (kstrtou8(buf, 0, &reg) < 0)
        return -EINVAL;

    data->access_reg = reg;
    return count;
}

/* read the value of access register in cpld data */
static ssize_t read_register_value(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg = data->access_reg;
    int reg_val;

    I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);

    if (reg_val < 0)
        return -1;

    return sprintf(buf, "0x%x\n", reg_val);
}

/* wrtie the value to access register in cpld data */
static ssize_t write_register_value(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    int ret = -EIO;
    u8 reg = data->access_reg;
    u8 reg_val;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    I2C_WRITE_BYTE_DATA(ret, &data->access_lock, client, reg, reg_val);

    return count;
}

/* get qsfpdd port start number of the cpld device */
/* the start number use to tranlate qsfpdd port to cpld port */
/* the cpld port use to access the qsfpdd port register in cpld */
static ssize_t get_qsfpdd_nif_port_start(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    int port_base_num;

    if (attr->index == CPLD_QSFPDD_NIF_PORT_START) {
        port_base_num = data->index * 10;
        return sprintf(buf, "%d\n", port_base_num);
    }
    return -1;
}

/* get total qsfp port which contain register in the cpld device */
static ssize_t get_qsfpdd_nif_ports(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    //struct i2c_client *client = to_i2c_client(dev);
    //struct cpld_data *data = i2c_get_clientdata(client);
    int ports;

    if (attr->index == CPLD_QSFPDD_NIF_PORTS) {
        ports = 10;
        return sprintf(buf, "%d\n", ports);
    }
    return -1;
}

/* get qsfpdd port start number of the cpld device */
/* the start number use to tranlate qsfpdd port to cpld port */
/* the cpld port use to access the qsfpdd port register in cpld */
static ssize_t get_qsfpdd_fab_port_start(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    int port_base_num;

    if (attr->index == CPLD_QSFPDD_FAB_PORT_START) {
        port_base_num = data->index * 10;
        return sprintf(buf, "%d\n", port_base_num);
    }
    return -1;
}

/* get total qsfp port which contain register in the cpld device */
static ssize_t get_qsfpdd_fab_ports(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    //struct i2c_client *client = to_i2c_client(dev);
    //struct cpld_data *data = i2c_get_clientdata(client);
    int ports;

    if (attr->index == CPLD_QSFPDD_FAB_PORTS) {
        ports = 10;
        return sprintf(buf, "%d\n", ports);
    }
    return -1;
}

/* get cpdl version register value */
static ssize_t read_cpld_version(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_VERSION) {
        reg = CPLD_VERSION_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get cpdl id register value */
static ssize_t read_cpld_id(struct device *dev,
                struct device_attribute *da,
                char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_ID) {
        reg = CPLD_ID_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get board type register value */
static ssize_t read_board_type(struct device *dev,
                struct device_attribute *da,
                char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_BOARD_TYPE) {
        reg = CPLD_BOARD_TYPE_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get extend board type register value */
static ssize_t read_ext_board_type(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_EXT_BOARD_TYPE) {
        reg = CPLD_EXT_BOARD_TYPE_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get cpld interrupt register value */
static ssize_t read_cpld_interrupt(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_INTERRUPT) {
        reg = CPLD_INTERRUPT_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get qsfpdd port status register value */
static ssize_t read_qsfpdd_nif_port_status(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    u8 reg_base;
    int reg_val;

    if (attr->index >= CPLD_QSFPDD_NIF_PORT_STATUS_0 &&
        attr->index <= CPLD_QSFPDD_NIF_PORT_STATUS_9) {
        reg_base = CPLD_QSFPDD_NIF_PORT_STATUS_BASE_REG;
        reg = reg_base + (attr->index - CPLD_QSFPDD_NIF_PORT_STATUS_0);
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get qsfpdd port config register value */
static ssize_t read_qsfpdd_nif_port_config(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    u8 reg_base;
    int reg_val;

    if (attr->index >= CPLD_QSFPDD_NIF_PORT_CONFIG_0 &&
        attr->index <= CPLD_QSFPDD_NIF_PORT_CONFIG_9) {
        reg_base = CPLD_QSFPDD_NIF_PORT_CONFIG_BASE_REG;
        reg = reg_base + (attr->index - CPLD_QSFPDD_NIF_PORT_CONFIG_0);
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to qsfpdd port config register */
static ssize_t write_qsfpdd_nif_port_config(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    u8 reg_base;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index >= CPLD_QSFPDD_NIF_PORT_CONFIG_0 &&
        attr->index <= CPLD_QSFPDD_NIF_PORT_CONFIG_9) {
	reg_base = CPLD_QSFPDD_NIF_PORT_CONFIG_BASE_REG;
        reg = reg_base + (attr->index - CPLD_QSFPDD_NIF_PORT_CONFIG_0);
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get qsfpdd port status register value */
static ssize_t read_qsfpdd_fab_port_status(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    u8 reg_base;
    int reg_val;

    if (attr->index >= CPLD_QSFPDD_FAB_PORT_STATUS_0 &&
        attr->index <= CPLD_QSFPDD_FAB_PORT_STATUS_9) {
    	if (data->index == cpld2) {
            reg_base = CPLD2_QSFPDD_FAB_PORT_STATUS_BASE_REG;
        } else {
            reg_base = CPLD3_QSFPDD_FAB_PORT_STATUS_BASE_REG;
        }
        reg = reg_base + (attr->index - CPLD_QSFPDD_FAB_PORT_STATUS_0);
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get qsfpdd port config register value */
static ssize_t read_qsfpdd_fab_port_config(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    u8 reg_base;
    int reg_val;

    if (attr->index >= CPLD_QSFPDD_FAB_PORT_CONFIG_0 &&
        attr->index <= CPLD_QSFPDD_FAB_PORT_CONFIG_9) {
    	if (data->index == cpld2) {
    	    reg_base = CPLD2_QSFPDD_FAB_PORT_CONFIG_BASE_REG;
    	} else {
    	    reg_base = CPLD3_QSFPDD_FAB_PORT_CONFIG_BASE_REG;
    	}
        reg = reg_base + (attr->index - CPLD_QSFPDD_FAB_PORT_CONFIG_0);
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to qsfpdd port config register */
static ssize_t write_qsfpdd_fab_port_config(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    u8 reg_base;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index >= CPLD_QSFPDD_FAB_PORT_CONFIG_0 &&
        attr->index <= CPLD_QSFPDD_FAB_PORT_CONFIG_9) {
    	if (data->index == cpld2) {
    	    reg_base = CPLD2_QSFPDD_FAB_PORT_CONFIG_BASE_REG;
    	} else {
    	    reg_base = CPLD3_QSFPDD_FAB_PORT_CONFIG_BASE_REG;
    	}
        reg = reg_base + (attr->index - CPLD_QSFPDD_FAB_PORT_CONFIG_0);
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get sfp port status register value */
static ssize_t read_sfp_port_status(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_SFP_PORT_STATUS) {
        reg = CPLD_SFP_PORT_STATUS_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get sfp port config register value */
static ssize_t read_sfp_port_config(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_SFP_PORT_CONFIG) {
        reg = CPLD_SFP_PORT_CONFIG_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to sfp port config register */
static ssize_t write_sfp_port_config(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index == CPLD_SFP_PORT_CONFIG) {
        reg = CPLD_SFP_PORT_CONFIG_REG;
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get 10g mux config register value */
static ssize_t read_10gmux_config(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_10GMUX_CONFIG) {
        reg = CPLD_10GMUX_CONFIG_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to 10g mux config register */
static ssize_t write_10gmux_config(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index == CPLD_10GMUX_CONFIG) {
        reg = CPLD_10GMUX_CONFIG_REG;
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get bmc status register value */
static ssize_t read_bmc_status(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_BMC_STATUS) {
        reg = CPLD_BMC_STATUS_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get bmc watchdog register value */
static ssize_t read_bmc_watchdog(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_BMC_WATCHDOG) {
        reg = CPLD_BMC_WATCHDOG_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to bmc watchdog register */
static ssize_t write_bmc_watchdog(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index == CPLD_BMC_WATCHDOG) {
        reg = CPLD_BMC_WATCHDOG_REG;
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get usb status register value */
static ssize_t read_usb_status(struct device *dev,
                struct device_attribute *da,
                char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_USB_STATUS) {
        reg = CPLD_USB_STATUS_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* get reset control register value */
static ssize_t read_reset_control(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_RESET_CONTROL) {
        reg = CPLD_RESET_CONTROL_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to reset control register */
static ssize_t write_reset_control(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index == CPLD_RESET_CONTROL) {
        reg = CPLD_RESET_CONTROL_REG;
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get reset mac register value */
static ssize_t read_reset_mac(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_RESET_MAC) {
        reg = CPLD_RESET_MAC_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to reset mac register */
static ssize_t write_reset_mac(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index == CPLD_RESET_MAC) {
        reg = CPLD_RESET_MAC_REG;
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get reset mac 2 register value */
static ssize_t read_reset_mac_2(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index == CPLD_RESET_MAC_2) {
        reg = CPLD_RESET_MAC_2_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to reset mac 2 register */
static ssize_t write_reset_mac_2(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index == CPLD_RESET_MAC_2) {
        reg = CPLD_RESET_MAC_2_REG;
        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get qsfpdd port led register */
static ssize_t read_qsfpdd_fab_led(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    u8 reg_base=CPLD_QSFPDD_FAB_LED_BASE_REG;
    int reg_val;
    int port_index=0;

    if (attr->index >= CPLD_QSFPDD_FAB_LED_0 &&
        attr->index <= CPLD_QSFPDD_FAB_LED_12) {
        port_index = attr->index - CPLD_QSFPDD_FAB_LED_0;
        reg = reg_base + port_index/2;

        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set value to qsfpdd port led register */
static ssize_t write_qsfpdd_fab_led(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;
    int port_index=0;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index >= CPLD_QSFPDD_FAB_LED_0 &&
        attr->index <= CPLD_QSFPDD_FAB_LED_12) {
        port_index = attr->index - CPLD_QSFPDD_FAB_LED_0;
        reg = CPLD_QSFPDD_FAB_LED_BASE_REG + port_index/2;

        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get system led register */
static ssize_t read_system_led(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    u8 reg_base=CPLD_SYSTEM_LED_BASE_REG;
    int reg_val;
    int port_index=0;

    if (attr->index >= CPLD_SYSTEM_LED_0 &&
        attr->index <= CPLD_SYSTEM_LED_1) {
        port_index = attr->index - CPLD_SYSTEM_LED_0;
        reg = reg_base + port_index;

        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* set system led register */
static ssize_t write_system_led(struct device *dev,
                    struct device_attribute *da,
                    const char *buf,
                    size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg, reg_val;
    int ret;
    int port_index=0;

    if (kstrtou8(buf, 0, &reg_val) < 0)
        return -EINVAL;

    if (attr->index >= CPLD_SYSTEM_LED_0 &&
        attr->index <= CPLD_SYSTEM_LED_1) {
        port_index = attr->index - CPLD_SYSTEM_LED_0;
        reg = CPLD_SYSTEM_LED_BASE_REG + port_index;

        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                    client, reg, reg_val);
    }
    return count;
}

/* get psu status register */
static ssize_t read_psu_status(struct device *dev,
                    struct device_attribute *da,
                    char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct cpld_data *data = i2c_get_clientdata(client);
    u8 reg;
    int reg_val;

    if (attr->index >= CPLD_PSU_STATUS_0 &&
        attr->index <= CPLD_PSU_STATUS_1) {
        reg = CPLD_PSU_STATUS_BASE_REG;
        I2C_READ_BYTE_DATA(reg_val, &data->access_lock, client, reg);
        if (reg_val < 0)
            return -1;
        return sprintf(buf, "0x%02x\n", reg_val);
    }
    return -1;
}

/* add valid cpld client to list */
static void apollo_cpld_add_client(struct i2c_client *client)
{
    struct cpld_client_node *node = NULL;

    node = kzalloc(sizeof(struct cpld_client_node), GFP_KERNEL);
    if (!node) {
        dev_info(&client->dev,
            "Can't allocate cpld_client_node for index %d\n",
            client->addr);
        return;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &cpld_client_list);
    mutex_unlock(&list_lock);
}

/* remove exist cpld client in list */
static void apollo_cpld_remove_client(struct i2c_client *client)
{
    struct list_head    *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int found = 0;

    mutex_lock(&list_lock);
    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node,
                struct cpld_client_node, list);

        if (cpld_node->client == client) {
            found = 1;
            break;
        }
    }

    if (found) {
        list_del(list_node);
        kfree(cpld_node);
    }
    mutex_unlock(&list_lock);
}

/* cpld drvier probe */
static int apollo_cpld_probe(struct i2c_client *client,
                    const struct i2c_device_id *dev_id)
{
    int status;
    struct cpld_data *data = NULL;
    int ret = -EPERM;
    int idx;

    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    /* init cpld data for client */
    i2c_set_clientdata(client, data);
    mutex_init(&data->access_lock);

    if (!i2c_check_functionality(client->adapter,
                I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_info(&client->dev,
            "i2c_check_functionality failed (0x%x)\n",
            client->addr);
        status = -EIO;
        goto exit;
    }

    /* get cpld id from device */
    ret = i2c_smbus_read_byte_data(client, CPLD_ID_REG);

    if (ret < 0) {
        dev_info(&client->dev,
            "fail to get cpld id (0x%x) at addr (0x%x)\n",
            CPLD_ID_REG, client->addr);
        status = -EIO;
        goto exit;
    }

    CPLD_ID_ID_GET(ret,  idx);

    if (INVALID(idx, cpld1, cpld5)) {
        dev_info(&client->dev,
            "cpld id %d(device) not valid\n", idx);
        //status = -EPERM;
        //goto exit;
    }

#if 0
    /* change client name for each cpld with index */
    snprintf(client->name, sizeof(client->name), "%s_%d", client->name,
            data->index);
#endif

              data->index = dev_id->driver_data;

    /* register sysfs hooks for different cpld group */
    dev_info(&client->dev, "probe cpld with index %d\n", data->index);
    switch (data->index) {
    case cpld1:
        status = sysfs_create_group(&client->dev.kobj,
                    &apollo_cpld1_group);
        break;
    case cpld2:    	  
        status = sysfs_create_group(&client->dev.kobj,
                    &apollo_cpld2_group);
        break;
    case cpld3:
        status = sysfs_create_group(&client->dev.kobj,
                    &apollo_cpld3_group);
        break;
    default:
        status = -EINVAL;
    }

    if (status)
        goto exit;

    dev_info(&client->dev, "chip found\n");

    /* add probe chip to client list */
    apollo_cpld_add_client(client);

    return 0;
exit:
    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &apollo_cpld1_group);
        break;
    case cpld2:    	  
    	  sysfs_remove_group(&client->dev.kobj, &apollo_cpld2_group);
        break;
    case cpld3:
    	  sysfs_remove_group(&client->dev.kobj, &apollo_cpld3_group);
        break;
    default:
        break;
    }
    return status;
}

/* cpld drvier remove */
static int apollo_cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);

    switch (data->index) {
    case cpld1:
        sysfs_remove_group(&client->dev.kobj, &apollo_cpld1_group);
        break;
    case cpld2:
    	  sysfs_remove_group(&client->dev.kobj, &apollo_cpld2_group);
        break;
    case cpld3:
    	  sysfs_remove_group(&client->dev.kobj, &apollo_cpld3_group);
        break;
    }

    apollo_cpld_remove_client(client);
    return 0;
}

MODULE_DEVICE_TABLE(i2c, apollo_cpld_id);

static struct i2c_driver apollo_cpld_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name = "x86_64_ufispace_s9700_23d_cpld",
    },
    .probe = apollo_cpld_probe,
    .remove = apollo_cpld_remove,
    .id_table = apollo_cpld_id,
    .address_list = cpld_i2c_addr,
};

/* provide cpld register read */
/* cpld_idx indicate the index of cpld device */
int apollo_cpld_read(u8 cpld_idx,
                u8 reg)
{
    struct list_head *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EPERM;
    struct cpld_data *data;

    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node,
                    struct cpld_client_node, list);
        data = i2c_get_clientdata(cpld_node->client);
        if (data->index == cpld_idx) {
            DEBUG_PRINT("cpld_idx=%d, read reg 0x%02x",
                    cpld_idx, reg);
            I2C_READ_BYTE_DATA(ret, &data->access_lock,
                    cpld_node->client, reg);
            DEBUG_PRINT("cpld_idx=%d, read reg 0x%02x = 0x%02x",
                    cpld_idx, reg, ret);
        break;
        }
    }

    return ret;
}
//EXPORT_SYMBOL(apollo_cpld_read);

/* provide cpld register write */
/* cpld_idx indicate the index of cpld device */
int apollo_cpld_write(u8 cpld_idx,
                u8 reg,
                u8 value)
{
    struct list_head *list_node = NULL;
    struct cpld_client_node *cpld_node = NULL;
    int ret = -EIO;
    struct cpld_data *data;

    list_for_each(list_node, &cpld_client_list) {
        cpld_node = list_entry(list_node,
                    struct cpld_client_node, list);
        data = i2c_get_clientdata(cpld_node->client);

        if (data->index == cpld_idx) {
                        I2C_WRITE_BYTE_DATA(ret, &data->access_lock,
                        cpld_node->client,
                        reg, value);
            DEBUG_PRINT("cpld_idx=%d, write reg 0x%02x val 0x%02x, ret=%d",
                            cpld_idx, reg, value, ret);
            break;
        }
    }

    return ret;
}
//EXPORT_SYMBOL(apollo_cpld_write);

/* provide qsfp port status register read */
/* port_num indicate the front panel qsfp port number */
int apollo_cpld_get_qsfp_port_status_val(u8 port_num)
{
    u8 cpld_idx, cpld_port, reg;
    int reg_val;

    if (INVALID(port_num, QSFP_MIN_PORT_NUM, QSFP_MAX_PORT_NUM)) {
        DEBUG_PRINT("invalid input value %d", port_num);
        return -1;
    }
    QSFP_TO_CPLD_IDX(port_num, cpld_idx, cpld_port);
    reg = QSFP_PORT_STATUS_REG(cpld_port);
    DEBUG_PRINT("port_num=%d, cpld_idx=%d, cpld_port=%d, reg=0x%x",
                    port_num, cpld_idx, cpld_port, reg);
    reg_val = apollo_cpld_read(cpld_idx, reg);
    return reg_val;
}
//EXPORT_SYMBOL(apollo_cpld_get_qsfp_port_status_val);

/* provide qsfp port config register read */
/* port_num indicate the front panel qsfp port number */
int apollo_cpld_get_qsfp_port_config_val(u8 port_num)
{
    u8 cpld_idx, cpld_port, reg;
    int reg_val;

    if (INVALID(port_num, QSFP_MIN_PORT_NUM, QSFP_MAX_PORT_NUM)) {
        DEBUG_PRINT("invalid input value %d", port_num);
        return -1;
    }
    QSFP_TO_CPLD_IDX(port_num, cpld_idx, cpld_port);
    reg = QSFP_PORT_CONFIG_REG(cpld_port);
    DEBUG_PRINT("port_num=%d, cpld_idx=%d, cpld_port=%d, reg=0x%x",
                    port_num, cpld_idx, cpld_port, reg);
    reg_val = apollo_cpld_read(cpld_idx, reg);
    return reg_val;
}
//EXPORT_SYMBOL(apollo_cpld_get_qsfp_port_config_val);

/* provide qsfp port config register write */
/* port_num indicate the front panel qsfp port number */
int apollo_cpld_set_qsfp_port_config_val(u8 port_num,
                            u8 reg_val)
{
    u8 cpld_idx, cpld_port, reg, ret;

    if (INVALID(port_num, QSFP_MIN_PORT_NUM, QSFP_MAX_PORT_NUM)) {
        DEBUG_PRINT("invalid input value %d", port_num);
        return -1;
    }
    QSFP_TO_CPLD_IDX(port_num, cpld_idx, cpld_port);
    reg = QSFP_PORT_CONFIG_REG(cpld_port);
    DEBUG_PRINT("port_num=%d, cpld_idx=%d, cpld_port=%d, reg=0x%x",
                    port_num, cpld_idx, cpld_port, reg);
    ret = apollo_cpld_write(cpld_idx, reg, reg_val);
    return ret;
}
//EXPORT_SYMBOL(apollo_cpld_set_qsfp_port_config_val);

/* provide sfp port 0/1 status register read */
int apollo_cpld_get_sfp_port_status_val(void)
{
    u8 cpld_idx, reg;
    int reg_val;

    cpld_idx = cpld1;
    reg = CPLD_SFP_PORT_STATUS_REG;
    DEBUG_PRINT("cpld_idx=%d, reg=0x%x",
                    cpld_idx, reg);
    reg_val = apollo_cpld_read(cpld_idx, reg);
    return reg_val;
}
//EXPORT_SYMBOL(apollo_cpld_get_sfp_port_status_val);

/* provide qsfp port config register read */
/* port_num indicate the front panel qsfp port number */
int apollo_cpld_get_sfp_port_config_val(void)
{
    u8 cpld_idx, reg;
    int reg_val;

    cpld_idx = cpld1;
    reg = CPLD_SFP_PORT_CONFIG_REG;
    DEBUG_PRINT("cpld_idx=%d, reg=0x%x",
                cpld_idx, reg);
    reg_val = apollo_cpld_read(cpld_idx, reg);
    return reg_val;
}
//EXPORT_SYMBOL(apollo_cpld_get_sfp_port_config_val);

/* provide qsfp port config register write */
/* port_num indicate the front panel qsfp port number */
int apollo_cpld_set_sfp_port_config_val(u8 reg_val)
{
    u8 cpld_idx, reg, ret;

    cpld_idx = cpld1;
    reg = CPLD_SFP_PORT_CONFIG_REG;
    DEBUG_PRINT("cpld_idx=%d, reg=0x%x",
                cpld_idx, reg);
    ret = apollo_cpld_write(cpld_idx, reg, reg_val);
    return ret;
}
//EXPORT_SYMBOL(apollo_cpld_set_sfp_port_config_val);

static int __init apollo_cpld_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&apollo_cpld_driver);
}

static void __exit apollo_cpld_exit(void)
{
    i2c_del_driver(&apollo_cpld_driver);
}

MODULE_AUTHOR("Jason Tsai <jason.cy.tsai@ufispace.com>");
MODULE_DESCRIPTION("apollo_cpld driver");
MODULE_LICENSE("GPL");

module_init(apollo_cpld_init);
module_exit(apollo_cpld_exit);
