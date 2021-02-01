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
#include <onlplib/file.h>
#include <onlp/platformi/ledi.h>
#include "platform_lib.h"

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_LED(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

#define LED_FORMAT "/sys/class/leds/as5915_18x_led::%s/brightness"

/* LED related data
 */
enum led_light_mode { /*must be the same with the definition @ kernel driver */
    LED_MODE_OFF = 0,
    LED_MODE_GREEN,
    LED_MODE_GREEN_BLINK,
    LED_MODE_AMBER,
    LED_MODE_AMBER_BLINK,
    LED_MODE_RED,
    LED_MODE_RED_BLINK,
    LED_MODE_BLUE,
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
{LED_LOC,  LED_MODE_OFF,         ONLP_LED_MODE_OFF},
{LED_LOC,  LED_MODE_BLUE_BLINK,  ONLP_LED_MODE_BLUE_BLINKING},
{LED_DIAG, LED_MODE_GREEN_BLINK, ONLP_LED_MODE_GREEN_BLINKING},
{LED_DIAG, LED_MODE_GREEN,       ONLP_LED_MODE_GREEN},
{LED_DIAG, LED_MODE_AMBER,       ONLP_LED_MODE_ORANGE},
{LED_DIAG, LED_MODE_OFF,         ONLP_LED_MODE_OFF},
{LED_PSU1, LED_MODE_AUTO,        ONLP_LED_MODE_AUTO},
{LED_PSU2, LED_MODE_AUTO,        ONLP_LED_MODE_AUTO},
{LED_FAN,  LED_MODE_GREEN_BLINK, ONLP_LED_MODE_GREEN_BLINKING},
{LED_FAN,  LED_MODE_GREEN,       ONLP_LED_MODE_GREEN},
{LED_FAN,  LED_MODE_AMBER,       ONLP_LED_MODE_ORANGE},
{LED_FAN,  LED_MODE_OFF,         ONLP_LED_MODE_OFF},
{LED_ALARM, LED_MODE_GREEN_BLINK,ONLP_LED_MODE_GREEN_BLINKING},
{LED_ALARM, LED_MODE_RED,        ONLP_LED_MODE_RED},
{LED_ALARM, LED_MODE_OFF,        ONLP_LED_MODE_OFF},
};

static char *leds[] =  /* must map with onlp_led_id */
{
    NULL,
    "loc",
    "diag",
    "psu1",
    "psu2",
    "fan",
    "alarm",
    "7segment",
};

/*
 * Get the information for the given LED OID.
 */
static onlp_led_info_t linfo[] =
{
    { }, /* Not used */
    {
        { ONLP_LED_ID_CREATE(LED_LOC), "Chassis LED 1 (LOC LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_BLUE_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_DIAG), "Chassis LED 2 (DIAG LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING | ONLP_LED_CAPS_ORANGE,
    },
    {
        { ONLP_LED_ID_CREATE(LED_PSU1), "Chassis LED 3 (PSU1 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_AUTO,
    },
    {
        { ONLP_LED_ID_CREATE(LED_PSU2), "Chassis LED 4 (PSU2 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_AUTO,
    },
    {
        { ONLP_LED_ID_CREATE(LED_FAN), "Chassis LED 5 (FAN LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING | ONLP_LED_CAPS_ORANGE,
    },
    {
        { ONLP_LED_ID_CREATE(LED_ALARM), "Chassis LED 6 (ALARM LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_GREEN_BLINKING | ONLP_LED_CAPS_RED,
    },
    {
        { ONLP_LED_ID_CREATE(LED_7SEGMENT), "Chassis LED 7 (7-SEGMENT LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_CHAR,
    },
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

/*
 * This function will be called prior to any other onlp_ledi_* functions.
 */
int
onlp_ledi_init(void)
{
    return ONLP_STATUS_OK;
}

/*
 * Set the character
 * Set c as  0 to 9  will set the character accordingly.
 * Set c as 10 to 14 will turn off the 7-segment led
 * Set c as 15 will turn on the dot led
 * */
int onlp_ledi_char_set(onlp_oid_t id, char c)
{
    int  lid;
    VALIDATE(id);

    lid = ONLP_OID_ID_GET(id);

    if (!(linfo[lid].caps & ONLP_LED_CAPS_CHAR)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    switch (c)
    {
        case '0' ... '9': c -= 48; break; // convert character to number
        case '.': c = 15; break; // convert dot '.' to integer
        default: c = 10;  break; // turn off
    }

    if (onlp_file_write_int(c, LED_FORMAT, leds[lid]) != 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

/** Get the current character */
int onlp_ledi_char_get(onlp_oid_t id, char* c)
{
    int  lid, value;
    VALIDATE(id);

    lid = ONLP_OID_ID_GET(id);

    if (!(linfo[lid].caps & ONLP_LED_CAPS_CHAR)) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    /* Get LED character */
    if (onlp_file_read_int(&value, LED_FORMAT, leds[lid]) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    switch (value)
    {
        case 0 ... 9: *c = (value + 48); break; // convert to number character
        case 15: *c = 46; break; // convert to dot '.' character
        default: *c = 0;  break;
    }

    return ONLP_STATUS_OK;
}

int
onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
    int  lid, value;
    VALIDATE(id);

    lid = ONLP_OID_ID_GET(id);

	/* Set the onlp_oid_hdr_t and capabilities */
    *info = linfo[ONLP_OID_ID_GET(id)];

    if (linfo[lid].caps & ONLP_LED_CAPS_CHAR) {
        int ret = onlp_ledi_char_get(id, &info->character);

        if (ret != ONLP_STATUS_OK) {
            return ret;
        }

        if (info->character) {
            info->mode = ONLP_LED_MODE_ON;
        }

        return ret;
    }

    /* Get LED mode */
    if (onlp_file_read_int(&value, LED_FORMAT, leds[lid]) < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    info->mode = driver_to_onlp_led_mode(lid, value);

    /* Set the on/off status */
    if (info->mode != ONLP_LED_MODE_OFF) {
        info->status |= ONLP_LED_STATUS_ON;
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
    VALIDATE(id);

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
    int  lid;
    VALIDATE(id);

    lid = ONLP_OID_ID_GET(id);

    if (linfo[lid].caps & ONLP_LED_CAPS_CHAR) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if (onlp_file_write_int(onlp_to_driver_led_mode(lid , mode), LED_FORMAT, leds[lid]) != 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}
