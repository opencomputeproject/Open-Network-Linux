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
#ifndef __VENDOR_DEVICE_LIST_H__
#define __VENDOR_DEVICE_LIST_H__

#include <onlp/onlp.h>
#include <onlplib/file.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <AIM/aim.h>
#include "vendor_driver_pool.h"

extern char *platform_name;

extern onlp_thermal_info_t onlp_thermal_info[];
extern onlp_fan_info_t onlp_fan_info[];
extern onlp_led_info_t onlp_led_info[];
extern onlp_psu_info_t onlp_psu_info[];

extern int led_list_size;
extern vendor_dev_t led_dev_list[];
extern vendor_dev_led_pin_t *led_color_list[];

extern int cpld_list_size;
extern vendor_dev_t cpld_dev_list[];
extern vendor_dev_oc_t *cpld_o_list[];
extern vendor_dev_oc_t *cpld_c_list[];
extern vendor_dev_io_pin_t cpld_version_list[];

extern int eeprom_list_size;
extern vendor_dev_t eeprom_dev_list[];
extern vendor_dev_oc_t *eeprom_o_list[];
extern vendor_dev_oc_t *eeprom_c_list[];
extern eeprom_data_t eeprom_dev_data_list[];

extern int thermal_list_size;
extern vendor_dev_t thermal_dev_list[];
extern vendor_dev_oc_t *thermal_o_list[];
extern vendor_dev_oc_t *thermal_c_list[];
extern thermal_data_t thermal_dev_data_list[];

extern int fan_list_size;
extern vendor_dev_t fan_dev_list[];
extern vendor_dev_io_pin_t fan_present_list[];
extern vendor_dev_oc_t *fan_o_list[];
extern vendor_dev_oc_t *fan_c_list[];
extern fan_data_t fan_dev_data_list[];

extern int psu_list_size;
extern vendor_dev_t psu_dev_list[];
extern vendor_dev_io_pin_t psu_present_list[];
extern vendor_dev_io_pin_t psu_power_good_list[];
extern vendor_dev_oc_t *psu_o_list[];
extern vendor_dev_oc_t *psu_c_list[];

extern int sfp_list_size;
extern vendor_dev_t sfp_dev_list[];
extern vendor_dev_io_pin_t sfp_present_list[];
extern vendor_dev_io_pin_t sfp_lpmode_list[];
extern vendor_dev_io_pin_t sfp_reset_list[];
extern vendor_dev_oc_t *sfp_o_list[];
extern vendor_dev_oc_t *sfp_c_list[];

#endif /* __VENDOR_DEVICE_LIST_H__ */