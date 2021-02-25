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
#include <unistd.h>

#include <onlplib/i2c.h>
#include <onlplib/file.h>
#include <onlp/platformi/ledi.h>

#include "platform_lib.h"

#define VALIDATE(_id)                           \
    do                                          \
    {                                           \
        if(!ONLP_OID_IS_LED(_id))               \
        {                                       \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

/* LED related data */
enum onlp_led_id
{
    LED_RESERVED = 0,
    LED_SYS1,
    LED_SYS2
};

typedef struct led_address_s {
    enum onlp_led_id id;
    uint8_t bus;
    uint8_t devaddr;
    uint8_t offset;
} led_address_t;

typedef struct led_mode_info_s {
    onlp_led_mode_t  mode;
    uint8_t regval;
} led_mode_info_t;

static led_address_t led_addr[] =
{
    { }, /* Not used */
    {LED_SYS1, 33, 0x32, 0x3e},
    {LED_SYS2, 33, 0x32, 0x3f},
};

static led_mode_info_t led_mode_info[] =
{
    {ONLP_LED_MODE_OFF, 0x0},
    {ONLP_LED_MODE_OFF, 0x8},
    {ONLP_LED_MODE_BLUE, 0x1},
    {ONLP_LED_MODE_BLUE_BLINKING, 0x9},
    {ONLP_LED_MODE_GREEN, 0x2},
    {ONLP_LED_MODE_GREEN_BLINKING, 0xa},
    {ONLP_LED_MODE_RED, 0x4},
    {ONLP_LED_MODE_RED_BLINKING, 0xc},
};

/*
 * Get the information for the given LED OID.
 */
static onlp_led_info_t linfo[] =
{
    { }, /* Not used */
    {
        { ONLP_LED_ID_CREATE(LED_SYS1), "Chassis LED 1 (SYS LED 1)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF |
        ONLP_LED_CAPS_GREEN  | ONLP_LED_CAPS_GREEN_BLINKING |
        ONLP_LED_CAPS_RED    | ONLP_LED_CAPS_RED_BLINKING   |
        ONLP_LED_CAPS_BLUE   | ONLP_LED_CAPS_BLUE_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_SYS2), "Chassis LED 1 (SYS LED 2)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF |
        ONLP_LED_CAPS_GREEN  | ONLP_LED_CAPS_GREEN_BLINKING |
        ONLP_LED_CAPS_RED    | ONLP_LED_CAPS_RED_BLINKING   |
        ONLP_LED_CAPS_BLUE   | ONLP_LED_CAPS_BLUE_BLINKING,
    },
};

/*
 * This function will be called prior to any other onlp_ledi_* functions.
 */
int
onlp_ledi_init(void)
{
    return ONLP_STATUS_OK;
}

static int
reg_value_to_onlp_led_mode(enum onlp_led_id id, int value)
{
    int i;

    for (i = 0; i < AIM_ARRAYSIZE(led_mode_info); i++)
    {
        if (value != led_mode_info[i].regval)
        {
            continue;
        }

        return led_mode_info[i].mode;
    }

    return ONLP_LED_MODE_AUTO;
}

static int
onlp_led_mode_to_reg_value(enum onlp_led_id id, onlp_led_mode_t onlp_led_mode)
{
    int i;

    for (i = 0; i < AIM_ARRAYSIZE(led_mode_info); i++)
    {
        if (onlp_led_mode != led_mode_info[i].mode)
        {
            continue;
        }

        return led_mode_info[i].regval;
    }

    return 0;
}

int
onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
    int  lid, value;

    VALIDATE(id);
    lid = ONLP_OID_ID_GET(id);

    /* Set the onlp_oid_hdr_t and capabilities */
    *info = linfo[ONLP_OID_ID_GET(id)];
    uint8_t bus, i2c_addr, mux_i2c_addr, mux_chn;
    int fpga_id, rd_size, wr_size;
    uint8_t byte_buf[128];

    fpga_id=0;
    bus=led_addr[lid].bus;
    mux_i2c_addr=0x88;
    mux_chn=0;
    i2c_addr =led_addr[lid].devaddr;
    rd_size =1;
    wr_size=1;
    byte_buf[0]=led_addr[lid].offset;
    if((fpga_proc_i2c_addr_read(fpga_id, bus, mux_i2c_addr, mux_chn
                                    , i2c_addr, rd_size, wr_size, byte_buf)) != 0)
    {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    value=byte_buf[0];
    info->mode = reg_value_to_onlp_led_mode(lid, value);
    /* Set the on/off status */
    if (info->mode != ONLP_LED_MODE_OFF)
    {
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

    if (!on_or_off)
    {
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
    int  lid, value;
    uint8_t bus, i2c_addr, mux_i2c_addr, mux_chn;
    int fpga_id, wr_size;
    uint8_t byte_buf[128];

    VALIDATE(id);
    lid = ONLP_OID_ID_GET(id);

    value = onlp_led_mode_to_reg_value(lid, mode);
    fpga_id=0;
    bus=led_addr[lid].bus;
    mux_i2c_addr=0x88;
    mux_chn=0;
    i2c_addr =led_addr[lid].devaddr;
    wr_size=2;
    byte_buf[0]=led_addr[lid].offset;
    byte_buf[1]=value;
    if(fpga_proc_i2c_write(fpga_id, bus, mux_i2c_addr, mux_chn, i2c_addr, wr_size, byte_buf) != 0)
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
