/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2014 Accton Technology Corporation.
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
#include <onlp/platformi/ledi.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include "platform_lib.h"

#define prefix_path "/sys/class/leds/accton_as9716_32d_led::"
#define filename    "brightness"


/* LED related data
 */
enum onlp_led_id
{
    LED_RESERVED = 0,
    LED_DIAG,
    LED_LOC,
    LED_FAN,
    LED_PSU1,
    LED_PSU2,
};

enum led_light_mode {
    LED_MODE_OFF = 0,
    LED_MODE_GREEN,
    LED_MODE_AMBER,
    LED_MODE_RED,
    LED_MODE_BLUE,
    LED_MODE_GREEN_BLINK,
    LED_MODE_AMBER_BLINK,
    LED_MODE_RED_BLINK,
    LED_MODE_BLUE_BLINK,
    LED_MODE_AUTO,
    LED_MODE_UNKNOWN
};

typedef struct led_light_mode_map {
    enum onlp_led_id id;
    enum led_light_mode driver_led_mode;
    enum onlp_led_mode_e onlp_led_mode;
} led_light_mode_map_t;

led_light_mode_map_t led_map[] = {
{LED_DIAG, LED_MODE_OFF,         ONLP_LED_MODE_OFF},
{LED_DIAG, LED_MODE_GREEN,       ONLP_LED_MODE_GREEN},
{LED_DIAG, LED_MODE_AMBER,       ONLP_LED_MODE_ORANGE},
{LED_DIAG, LED_MODE_GREEN_BLINK, ONLP_LED_MODE_GREEN_BLINKING},

{LED_LOC, LED_MODE_OFF,          ONLP_LED_MODE_OFF},
{LED_LOC, LED_MODE_AMBER,        ONLP_LED_MODE_ORANGE},
{LED_LOC, LED_MODE_GREEN_BLINK,  ONLP_LED_MODE_GREEN_BLINKING},

{LED_FAN, LED_MODE_OFF,   ONLP_LED_MODE_OFF},
{LED_FAN, LED_MODE_GREEN, ONLP_LED_MODE_GREEN},
{LED_FAN, LED_MODE_AMBER, ONLP_LED_MODE_ORANGE},

{LED_PSU1, LED_MODE_OFF,   ONLP_LED_MODE_OFF},
{LED_PSU1, LED_MODE_GREEN, ONLP_LED_MODE_GREEN},
{LED_PSU1, LED_MODE_AMBER, ONLP_LED_MODE_ORANGE},

{LED_PSU2, LED_MODE_OFF,   ONLP_LED_MODE_OFF},
{LED_PSU2, LED_MODE_GREEN, ONLP_LED_MODE_GREEN},
{LED_PSU2, LED_MODE_AMBER, ONLP_LED_MODE_ORANGE},

};

static char last_path[][10] =  /* must map with onlp_led_id */
{
    "reserved",
    "diag",
    "loc",
    "fan",
    "psu1",
    "psu2",
};

static onlp_led_info_t linfo[] =
{
    { }, /* Not used */
    ONLP_CHASSIS_LED_INFO_ENTRY_INIT(LED_DIAG, "LED 1 (DIAG LED)",
                                     ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_ORANGE | ONLP_LED_CAPS_GREEN_BLINKING),
    ONLP_CHASSIS_LED_INFO_ENTRY_INIT(LED_LOC, "LED 2 (LOC LED)",
                                     ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_ORANGE | ONLP_LED_CAPS_GREEN_BLINKING),
    ONLP_CHASSIS_LED_INFO_ENTRY_INIT(LED_FAN, "LED 3 (FAN LED)",
                                     ONLP_LED_CAPS_OFF |ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_ORANGE),
    ONLP_CHASSIS_LED_INFO_ENTRY_INIT(LED_PSU1,"LED 4 (PSU1 LED)",
                                     ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_ORANGE | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO),
    ONLP_CHASSIS_LED_INFO_ENTRY_INIT(LED_PSU2,"LED 5 (PSU2 LED)",
                                     ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_ORANGE | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_AUTO),
};

static int driver_to_onlp_led_mode(enum onlp_led_id id, enum led_light_mode driver_led_mode)
{
    int i, nsize = sizeof(led_map)/sizeof(led_map[0]);

    for (i = 0; i < nsize; i++)
    {
        if (id == led_map[i].id && driver_led_mode == led_map[i].driver_led_mode)
        {
            return led_map[i].onlp_led_mode;
        }
    }

    return 0;
}

static int onlp_to_driver_led_mode(enum onlp_led_id id, onlp_led_mode_t onlp_led_mode)
{
    int i, nsize = sizeof(led_map)/sizeof(led_map[0]);

    for(i = 0; i < nsize; i++)
    {
        if (id == led_map[i].id && onlp_led_mode == led_map[i].onlp_led_mode)
        {
            return led_map[i].driver_led_mode;
        }
    }

    return 0;
}

int
onlp_ledi_hdr_get(onlp_oid_t oid, onlp_oid_hdr_t* hdr)
{
    *hdr = linfo[ONLP_OID_ID_GET(oid)].hdr;
    return 0;
}

int
onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
    int  local_id;
    char data[2] = {0};
    char fullpath[PATH_MAX] = {0};

    local_id = ONLP_OID_ID_GET(id);

    /* get fullpath */
    sprintf(fullpath, "%s%s/%s", prefix_path, last_path[local_id], filename);

    /* Set the onlp_oid_hdr_t and capabilities */
    *info = linfo[ONLP_OID_ID_GET(id)];

    /* Set LED mode */
    if (onlp_file_read_string(fullpath, data, sizeof(data), 0) != 0) {
        DEBUG_PRINT("%s(%d)\r\n", __FUNCTION__, __LINE__);
        return ONLP_STATUS_E_INTERNAL;
    }

    info->mode = driver_to_onlp_led_mode(local_id, atoi(data));

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
    int  local_id;
    char fullpath[PATH_MAX] = {0};

    local_id = ONLP_OID_ID_GET(id);
    sprintf(fullpath, "%s%s/%s", prefix_path, last_path[local_id], filename);

    if (onlp_file_write_integer(fullpath, onlp_to_driver_led_mode(local_id, mode)) != 0)
    {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/*
 * Generic LED ioctl interface.
 */
int
onlp_ledi_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
