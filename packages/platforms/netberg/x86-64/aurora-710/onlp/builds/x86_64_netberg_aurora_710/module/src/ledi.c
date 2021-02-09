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
 *
 ***********************************************************/
#include <onlp/platformi/ledi.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "platform_lib.h"

/*
 * Get the information for the given LED OID.
 */
static onlp_led_info_t led_info[] =
{
    { }, /* Not used */
    {
        { LED_OID_SYSTEM, "Chassis LED 1 (SYS LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_GREEN,
    },
    {
        { LED_OID_FAN, "Chassis LED 2 (FAN LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_ORANGE | 
        ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
    },
    {
        { LED_OID_PSU1, "Chassis LED 3 (PSU1 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_ORANGE | 
        ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
    },
    {
        { LED_OID_PSU2, "Chassis LED 4 (PSU2 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_ORANGE | 
        ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
    },
    {
        { LED_OID_FAN_TRAY1, "Rear LED 1 (FAN TRAY1 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_ORANGE | 
        ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
    },
    {
        { LED_OID_FAN_TRAY2, "Rear LED 2 (FAN TRAY2 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_ORANGE | 
        ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
    },
    {
        { LED_OID_FAN_TRAY3, "Rear LED 3 (FAN TRAY3 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_ORANGE | 
        ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
    },
    {
        { LED_OID_FAN_TRAY4, "Rear LED 4 (FAN TRAY4 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_ORANGE | 
        ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO,
    }
};

extern int sys_fan_info_get(onlp_fan_info_t* info, int id);

/*
 * This function will be called prior to any other onlp_ledi_* functions.
 */
int
onlp_ledi_init(void)
{  
    return ONLP_STATUS_OK;
}

int
onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
    int led_id, pw_exist, pw_good, rc, psu_mask, fan_id;
    int exist_offset, good_offset, i2c_bus;
    onlp_fan_info_t fan_info;
    
    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    memset(&fan_info, 0, sizeof(onlp_fan_info_t));
    led_id = ONLP_OID_ID_GET(id);

    *info = led_info[led_id];

    if (id == LED_OID_PSU1 || id == LED_OID_PSU2) {

        psu_mask = PSU_MUX_MASK;

        if (id == LED_OID_PSU1) {
            i2c_bus = I2C_BUS_PSU1;
            exist_offset = PSU1_PRESENT_OFFSET;
            good_offset = PSU1_PWGOOD_OFFSET;
        } else {
            i2c_bus = I2C_BUS_PSU2;
            exist_offset = PSU2_PRESENT_OFFSET;
            good_offset = PSU2_PWGOOD_OFFSET;
        }
        /* check psu status */
        if ((rc = psu_present_get(&pw_exist, exist_offset, i2c_bus, psu_mask)) 
                != ONLP_STATUS_OK) {
            return ONLP_STATUS_E_INTERNAL;
        }
        if ((rc = psu_pwgood_get(&pw_good, good_offset, i2c_bus, psu_mask)) 
                != ONLP_STATUS_OK) {
            return ONLP_STATUS_E_INTERNAL;
        }
        /* psu not present */
        if (pw_exist != PSU_STATUS_PRESENT) {
            info->status &= ~ONLP_LED_STATUS_ON;
            info->mode = ONLP_LED_MODE_OFF;
        } else if (pw_good != PSU_STATUS_POWER_GOOD) {
            info->status |= ONLP_LED_STATUS_ON;
            info->mode |= ONLP_LED_MODE_ORANGE;
        } else {
            info->status |= ONLP_LED_STATUS_ON;
            info->mode |= ONLP_LED_MODE_GREEN;
        }
    } else if (id == LED_OID_FAN) {
        info->status |= ONLP_LED_STATUS_ON;
        info->mode |= ONLP_LED_MODE_GREEN;
        for (fan_id=FAN_ID_FAN1; fan_id<=FAN_ID_FAN8; ++fan_id) {
            rc = sys_fan_info_get(&fan_info, fan_id);
            if (rc != ONLP_STATUS_OK || fan_info.status & ONLP_FAN_STATUS_FAILED) {
                info->mode &= ~ONLP_LED_MODE_GREEN;
                info->mode |= ONLP_LED_MODE_ORANGE;
                break;
            }
        }
    } else if (id == LED_OID_SYSTEM) {
        info->status |= ONLP_LED_STATUS_ON;
        info->mode |= ONLP_LED_MODE_GREEN;
    } else {
        info->status |= ONLP_LED_STATUS_ON;
        info->mode |= ONLP_LED_MODE_ON;
    }

    return ONLP_STATUS_OK;
}

/*
 * Turn an LED on or off.
 *
 * This function will only be called if the LED OID supports the ONOFF
 * capability.
 *
 * What 'on' means in terms of colors or modes for multimode LEDs is
 * up to the platform to decide. This is intended as baseline toggle mechanism.
 */
int
onlp_ledi_set(onlp_oid_t id, int on_or_off)
{
    if (!on_or_off) {
        return onlp_ledi_mode_set(id, ONLP_LED_MODE_OFF);
    }

    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function puts the LED into the given mode. It is a more functional
 * interface for multimode LEDs.
 *
 * Only modes reported in the LED's capabilities will be attempted.
 */
int
onlp_ledi_mode_set(onlp_oid_t id, onlp_led_mode_t mode)
{
    int led_id, rc;

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    led_id = ONLP_OID_ID_GET(id);
    switch (led_id) {
        case LED_SYSTEM_LED:
            rc = system_led_set(mode);
            break;
        case LED_FAN_LED:
            rc = fan_led_set(mode);
            break;
        case LED_PSU1_LED:
            rc = psu1_led_set(mode);
            break;
        case LED_PSU2_LED:
            rc = psu2_led_set(mode);
            break;
        case LED_FAN_TRAY1:
        case LED_FAN_TRAY2:
        case LED_FAN_TRAY3:
        case LED_FAN_TRAY4:
            rc = fan_tray_led_set(id, mode);
            break;
        default:
            return ONLP_STATUS_E_INTERNAL;
            break;
    }

    return rc;
}

int
onlp_ledi_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_OK;
}
