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
#ifndef __VENDOR_DRIVER_POOL_H__
#define __VENDOR_DRIVER_POOL_H__

#include <onlp/onlp.h>
#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/fani.h>
#include <onlp/platformi/psui.h>
#include <onlp/platformi/thermali.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/i2c-dev.h>
#include <AIM/aim.h>
#include "x86_64_delta_agcv208s_log.h"

#define VENDOR_MAX_NAME_SIZE 30
#define CPLD_DEV 1
#define BMC_DEV 2

#define FAN_INFO_ENTRY_INIT(_id, _desc, _caps) \
    {                                          \
        {                                      \
            .id = ONLP_FAN_ID_CREATE(_id),     \
            .description = _desc,              \
            .poid = 0,                         \
        },                                     \
            .caps = _caps,                     \
    }

#define PSU_INFO_ENTRY_INIT(_id, _desc)                                                                  \
    {                                                                                                    \
        {                                                                                                \
            .id = ONLP_PSU_ID_CREATE(_id),                                                               \
            .description = _desc,                                                                        \
            .poid = 0,                                                                                   \
        },                                                                                               \
            .caps = (ONLP_PSU_CAPS_DC12 | ONLP_PSU_CAPS_VOUT | ONLP_PSU_CAPS_IOUT | ONLP_PSU_CAPS_POUT), \
    }

#define LED_INFO_ENTRY_INIT(_id, _desc, _caps, _defaultm) \
    {                                                     \
        {                                                 \
            .id = ONLP_LED_ID_CREATE(_id),                \
            .description = _desc,                         \
            .poid = 0,                                    \
        },                                                \
            .caps = _caps,                                \
            .mode = _defaultm,                            \
            .status = ONLP_LED_STATUS_PRESENT,            \
    }

#define THERMAL_INFO_ENTRY_INIT(_id, _desc)                                                                                                   \
    {                                                                                                                                         \
        {                                                                                                                                     \
            .id = ONLP_THERMAL_ID_CREATE(_id),                                                                                                \
            .description = _desc,                                                                                                             \
            .poid = 0,                                                                                                                        \
        },                                                                                                                                    \
            .status = ONLP_THERMAL_STATUS_PRESENT,                                                                                            \
            .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE ), \
            .mcelsius = 0,                                                                                                                    \
    }

typedef struct fan_data_s
{
    uint16_t fan_def_speed;
    uint16_t fan_max_speed;
    uint16_t fan_tolerance;
} fan_data_t;

typedef struct thrml_data_s
{
    uint16_t low_threshold;
    uint16_t high_threshold;
} thermal_data_t;

typedef struct eeprom_data_s
{
    uint8_t alen;
    uint16_t len;
    uint16_t offset;
} eeprom_data_t;

typedef struct vendor_dev_s
{
    char *dev_name;
    char *bus_drv_name;
    char *dev_drv_name;
    int bus;
    uint8_t dev;
    int id;
} vendor_dev_t;

typedef struct vendor_dev_oc_s
{
    int type; /* 2 -> CPLD; 1 -> MUX; 0 -> invalid */
    char *bus_drv_name;
    int bus;
    uint8_t dev;
    uint8_t addr;  /* FOR CPLD & MUX */
    uint8_t value; /* FOR CPLD & MUX */
    uint8_t mask;  /* FOR CPLD */
    uint8_t match; /* FOR CPLD */
} vendor_dev_oc_t;

typedef struct vendor_dev_io_pin_s
{
    int type; /* 2 -> BMC; 1 -> CPLD; 0 -> invalid */
    char name[VENDOR_MAX_NAME_SIZE];
    char *bus_drv_name;
    int bus;
    uint8_t dev; /* DEVICE_TYPE + UNIT */
    uint8_t addr;
    uint8_t mask;
    uint8_t match;
} vendor_dev_io_pin_t;

typedef struct vendor_dev_led_pin_s
{
    char name[VENDOR_MAX_NAME_SIZE];
    char *bus_drv_name;
    int bus;
    uint8_t dev;
    uint8_t addr;
    uint8_t mask;
    uint8_t match;
    onlp_led_mode_t mode;
} vendor_dev_led_pin_t;

typedef enum vendor_bmc_device_type_e
{
    VENDOR_TEMPERATURE = 0,
    VENDOR_FAN = 1,
    VENDOR_PSU = 3,
    VENDOR_FAN_PRESENT = VENDOR_FAN,
    VENDOR_PSU_PRESENT = VENDOR_PSU,
} vendor_bmc_device_type_t;

typedef struct vendor_driver_s
{
    char name[VENDOR_MAX_NAME_SIZE];
    void *dev_driver;
} vendor_driver_t;

typedef struct vendor_driver_node_s
{
    void *node;
    vendor_driver_t *driver_hdl;
} vendor_driver_node_t;

typedef struct i2c_bus_driver_s
{
    int (*get)(int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint16_t *data, uint8_t dlen);
    int (*set)(int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint16_t data, uint8_t dlen);
    int (*get_byte)(int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint16_t *data, uint8_t dlen);
    int (*set_byte)(int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint16_t data, uint8_t dlen);
    int (*block_read)(int bus, uint8_t dev, uint16_t daddr, uint8_t *ReplyBuf, uint16_t BufSize);
    int (*block_write)(int bus, uint8_t dev, uint16_t daddr, uint8_t *value, uint16_t BufSize);
    int (*i2c_block_read)(int bus, uint8_t dev, uint16_t daddr, uint8_t *ReplyBuf, uint16_t BufSize);
    int (*i2c_block_write)(int bus, uint8_t dev, uint16_t daddr, uint8_t *value, uint16_t BufSize);
    int (*probe)(uint8_t bus, uint8_t dev);
} i2c_bus_driver_t;

typedef struct ipmi_bus_driver_s
{
    int (*get)(int bus, char *cmd, char *filter, char *data, uint32_t dlen);
    int (*set)(int bus, char *cmd, char *filter);
} ipmi_bus_driver_t;

typedef struct status_get_driver_s
{
    int (*status_get)(void *busDrvPtr, int bus, uint8_t dev, uint8_t addr, uint8_t *active);
} status_get_driver_t;

/* ONLY I2C */
typedef struct eeprom_dev_driver_s
{
    int (*readb)(void *busDrvPtr, int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint8_t *buf, uint16_t len);
    int (*writeb)(void *busDrvPtr, int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint8_t *buf, uint16_t len);
    int (*readw)(void *busDrvPtr, int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint16_t *buf, uint16_t len);
    int (*writew)(void *busDrvPtr, int bus, uint8_t dev, uint16_t addr, uint8_t alen, uint16_t *buf, uint16_t len);
    int (*load)(void *busDrvPtr, int bus, uint8_t dev, uint16_t start_addr, uint8_t alen, uint8_t *buf);
} eeprom_dev_driver_t;

/* ONLY I2C */
typedef struct cpld_dev_driver_s
{
    int (*readb)(void *busDrvPtr, int bus, uint8_t dev, uint8_t addr, uint8_t *value);
    int (*writeb)(void *busDrvPtr, int bus, uint8_t dev, uint8_t addr, uint8_t value);
} cpld_dev_driver_t;

typedef struct fan_dev_driver_s
{
    int (*rpm_get)(void *busDrvPtr, int bus, uint8_t dev, int id, int *rpm);
    int (*rpm_set)(void *busDrvPtr, int bus, uint8_t dev, int id, int rpm);
} fan_dev_driver_t;

typedef enum vendor_thermal_threshold_e
{
    VENDOR_THERMAL_LOW_THRESHOLD,
    VENDOR_THERMAL_HIGH_THRESHOLD,
} vendor_thermal_threshold_t;

typedef struct thermal_dev_driver_s
{
    int (*temp_get)(void *busDrvPtr, int bus, uint8_t dev, int id, int *temperature);
    int (*limit_get)(void *busDrvPtr, int bus, uint8_t dev, int id, int type, int *temperature);
    int (*limit_set)(void *busDrvPtr, int bus, uint8_t dev, int id, int type, int temperature);
} thermal_dev_driver_t;

typedef enum vendor_psu_parm_type_e
{
    PSU_PARM_MIN,
    PSU_PARM_VIN = PSU_PARM_MIN,
    PSU_PARM_IIN,
    PSU_PARM_VOUT,
    PSU_PARM_IOUT,
    PSU_PARM_POUT,
    PSU_PARM_PIN,
    PSU_PARM_MAX = PSU_PARM_PIN,
    PSU_PARM_UNKNOW
} vendor_psu_parm_type_t;

typedef struct vendor_psu_runtime_info_s
{
    int vin;
    int iin;
    int vout;
    int iout;
    int pout;
    int pin;
} vendor_psu_runtime_info_t;

typedef struct psu_dev_driver_s
{
    int (*model_get)(void *busDrvPtr, int bus, uint8_t dev, char *model);
    int (*serial_get)(void *busDrvPtr, int bus, uint8_t dev, char *serial);
    int (*volt_get)(void *busDrvPtr, int bus, uint8_t dev, int *volt);
    int (*amp_get)(void *busDrvPtr, int bus, uint8_t dev, int *amp);
    int (*watt_get)(void *busDrvPtr, int bus, uint8_t dev, int *watt);
    int (*runtime_info_get)(void *busDrvPtr, int bus, uint8_t dev, vendor_psu_runtime_info_t *runtimeInfo);
} psu_dev_driver_t;

typedef enum vendor_sfp_control_s
{
    SFP_CONTROL_RESET,
    SFP_CONTROL_RESET_STATE,
    SFP_CONTROL_RX_LOS,
    SFP_CONTROL_TX_FAULT,
    SFP_CONTROL_TX_DISABLE,
    SFP_CONTROL_TX_DISABLE_CHANNEL,
    SFP_CONTROL_LP_MODE,
    SFP_CONTROL_POWER_OVERRIDE,
    SFP_CONTROL_TEMPERATURE,
    SFP_CONTROL_VOLTAGE,
    SFP_CONTROL_TX_BIAS,
    SFP_CONTROL_RX_POWER,
    SFP_CONTROL_TX_POWER,
    SFP_CONTROL_LAST = SFP_CONTROL_TX_POWER,
    SFP_CONTROL_COUNT,
    SFP_CONTROL_INVALID = -1,
} vendor_sfp_control_t;

/* ONLY I2C */
typedef struct sfp_dev_driver_s
{
    int (*eeprom_load)(void *busDrvPtr, int bus, uint8_t dev, uint8_t *data);
    int (*eeprom_dom_load)(void *busDrvPtr, int bus, uint8_t dev, uint8_t *data);
    int (*eeprom_read)(void *busDrvPtr, int bus, uint8_t dev, uint8_t offset, uint16_t len, uint8_t *data);
    int (*eeprom_write)(void *busDrvPtr, int bus, uint8_t dev, uint8_t offset, uint16_t len, uint8_t *data);
    int (*eeprom_readb)(void *busDrvPtr, int bus, uint8_t dev, uint16_t offset, uint8_t *data);
    int (*eeprom_writeb)(void *busDrvPtr, int bus, uint8_t dev, uint16_t offset, uint8_t data);
    int (*eeprom_readw)(void *busDrvPtr, int bus, uint8_t dev, uint16_t offset, uint16_t *data);
    int (*eeprom_writew)(void *busDrvPtr, int bus, uint8_t dev, uint16_t offset, uint16_t data);
    int (*control_is_support)(int control, uint8_t *is_support);
    int (*control_get)(void *busDrvPtr, int bus, uint8_t dev, int control, int *status);
    int (*control_set)(void *busDrvPtr, int bus, uint8_t dev, int control, int status);
} sfp_dev_driver_t;

int vendor_find_cpld_idx_by_name(char *name);
int vendor_system_call_get(char *cmd, char *data);
int vendor_system_call_set(char *cmd);
int vendor_driver_init();
void *vendor_find_driver_by_name(const char *driver_name);
int vendor_dev_do_oc(vendor_dev_oc_t *dev_oc);
int vendor_get_status(vendor_dev_io_pin_t *present_info, int *present);

#endif /* __VENDOR_DRIVER_POOL_H__ */