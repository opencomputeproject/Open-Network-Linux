/************************************************************
 * ledi.c
 *
 *           Copyright 2018 Inventec Technology Corporation.
 *
 ************************************************************
 *
 ***********************************************************/
#include <onlp/platformi/ledi.h>
#include <onlplib/file.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <onlp/platformi/fani.h>

#include "platform_lib.h"

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_LED(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

/* LED related data
 */

/* CAPS*/
#define SYS_LED_CAPS ONLP_LED_CAPS_ON_OFF|ONLP_LED_CAPS_GREEN|ONLP_LED_CAPS_GREEN_BLINKING|  \
    ONLP_LED_CAPS_RED|ONLP_LED_CAPS_RED_BLINKING|ONLP_LED_CAPS_ORANGE
#define FAN_LED_CAPS ONLP_LED_CAPS_RED|ONLP_LED_CAPS_GREEN

#define LED_ID_TO_FAN_ID(id) (id-ONLP_LED_MGMT)
#define FAN_TO_BLADE_ID(fan_id) fan_id*2

typedef enum sys_led_mode_e {
    SYS_LED_MODE_OFF = 0,
    SYS_LED_MODE_0_5_HZ = 1,
    SYS_LED_MODE_1_HZ = 2,
    SYS_LED_MODE_2_HZ = 3,
    SYS_LED_MODE_ON = 7
} sys_led_mode_t;


/*
 * Get the information for the given LED OID.
 */
#define MAKE_MGMT_LED_INFO_NODE         \
    {                       \
        { ONLP_LED_ID_CREATE(ONLP_LED_MGMT), "MGMT LED" , 0 }, \
        ONLP_LED_STATUS_PRESENT,        \
        SYS_LED_CAPS,       \
    }

#define MAKE_LED_INFO_NODE_ON_FAN(fan_id)	\
    {						\
        { ONLP_LED_ID_CREATE(ONLP_LED_FAN##fan_id), \
          "FAN LED "#fan_id, 0,			\
        },					\
        0,	FAN_LED_CAPS,					\
    }

static onlp_led_info_t __onlp_led_info[] = {
    {},/* Not used */
    MAKE_MGMT_LED_INFO_NODE,
    MAKE_LED_INFO_NODE_ON_FAN(1),
    MAKE_LED_INFO_NODE_ON_FAN(2),
    MAKE_LED_INFO_NODE_ON_FAN(3),
    MAKE_LED_INFO_NODE_ON_FAN(4),
};

/*
 * This function will be called prior to any other onlp_ledi_* functions.
 */
int
onlp_ledi_init(void)
{
    return ONLP_STATUS_OK;
}

/* convert platform led type to onlp_led_mode type*/
static int _sys_onlp_led_mode_convert(sys_led_mode_t grn_mode, sys_led_mode_t red_mode, onlp_led_mode_t* pmode)
{
    int rv = ONLP_STATUS_OK;
    *pmode = ONLP_LED_MODE_OFF;

    switch(grn_mode) {
    case SYS_LED_MODE_0_5_HZ:
    case SYS_LED_MODE_1_HZ:
    case SYS_LED_MODE_2_HZ:
        *pmode = ONLP_LED_MODE_GREEN_BLINKING;
        break;
    case SYS_LED_MODE_ON:
        *pmode = ONLP_LED_MODE_GREEN;
        break;
    case SYS_LED_MODE_OFF:
        *pmode = ONLP_LED_MODE_OFF;
        break;
    default:
        return ONLP_STATUS_E_INVALID;
        break;
    }
    switch(red_mode) {
    case SYS_LED_MODE_0_5_HZ:
    case SYS_LED_MODE_1_HZ:
    case SYS_LED_MODE_2_HZ:
        if(grn_mode == SYS_LED_MODE_OFF) {
            *pmode = ONLP_LED_MODE_RED_BLINKING;
        } else {
            return ONLP_STATUS_E_INVALID;
        }
        break;
    case SYS_LED_MODE_ON:
        if(grn_mode == SYS_LED_MODE_OFF) {
            *pmode = ONLP_LED_MODE_RED;
        } else {
            return ONLP_STATUS_E_INVALID;
        }
        break;
    case SYS_LED_MODE_OFF:
        if(grn_mode == SYS_LED_MODE_OFF) {
            *pmode = ONLP_LED_MODE_OFF;
        }
        break;
    default:
        return ONLP_STATUS_E_INVALID;
        break;
    }
    return rv;
}

int onlp_ledi_status_get(onlp_oid_t id, uint32_t* rv)
{
    int result = ONLP_STATUS_OK;
    onlp_led_info_t* info;

    VALIDATE(id);
    int led_id = ONLP_OID_ID_GET(id);
    int fan_id = LED_ID_TO_FAN_ID(led_id);
    int mode;
    uint32_t fan_status;

    if(led_id >= ONLP_LED_MAX) {
        result = ONLP_STATUS_E_INVALID;
    }
    if(result == ONLP_STATUS_OK) {
        info = &__onlp_led_info[led_id];
        switch(led_id) {
        case ONLP_LED_FAN1:
        case ONLP_LED_FAN2:
        case ONLP_LED_FAN3:
        case ONLP_LED_FAN4:
            result = onlp_fani_status_get( ONLP_FAN_ID_CREATE(FAN_TO_BLADE_ID(fan_id)), &fan_status);
            if(result != ONLP_STATUS_OK) {
                return result;
            }

            if(fan_status & ONLP_FAN_STATUS_PRESENT) {
                info->status |= ONLP_LED_STATUS_PRESENT;
                info->status &= (~ONLP_LED_STATUS_ON);

                result = onlp_file_read_int((int*)&mode, INV_FAN_PREFIX"fan_led_grn%d", fan_id);
                if(result != ONLP_STATUS_OK) {
                    return result;
                }
                if(mode) {
                    info->status |= ONLP_LED_STATUS_ON;
                }

                result = onlp_file_read_int((int*)&mode, INV_FAN_PREFIX"fan_led_red%d", fan_id);
                if(result != ONLP_STATUS_OK) {
                    return result;
                }
                if(mode) {
                    info->status |= ONLP_LED_STATUS_ON;
                }
            } else {
                info->status = 0;
            }
            *rv = info->status;
            break;
        default:
            result = ONLP_STATUS_E_INVALID;
            break;
        }
    }

    return result;
}

static int _sys_onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
    int led_id;
    int rv = ONLP_STATUS_OK;
    sys_led_mode_t grn_mode, red_mode;
    led_id = ONLP_OID_ID_GET(id);
    /* Set the onlp_oid_hdr_t and capabilities */
    *info = __onlp_led_info[led_id];

    rv = onlp_file_read_int((int*)&grn_mode, INV_INFO_PREFIX"grn_led");
    if(rv != ONLP_STATUS_OK) {
        return rv;
    }
    rv = onlp_file_read_int((int*)&red_mode, INV_INFO_PREFIX"red_led");
    if(rv != ONLP_STATUS_OK) {
        return rv;
    }

    rv = _sys_onlp_led_mode_convert(grn_mode, red_mode, &info->mode);
    if(rv != ONLP_STATUS_OK) {
        return rv;
    }

    /* Set the on/off status */
    if (info->mode != ONLP_LED_MODE_OFF) {
        info->status |= ONLP_LED_STATUS_ON;
    } else {
        info->status &= (~ONLP_LED_STATUS_ON);
    }


    return rv;
}

static int _fan_onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
    int led_id;
    int rv = ONLP_STATUS_OK;
    int grn_mode, red_mode;

    led_id = ONLP_OID_ID_GET(id);
    int fan_id = LED_ID_TO_FAN_ID(led_id);
    /* Set the onlp_oid_hdr_t and capabilities */
    *info = __onlp_led_info[led_id];

    rv = onlp_ledi_status_get(id, &info->status);
    if( rv != ONLP_STATUS_OK ) {
        return rv;
    }

    if( info->status & ONLP_LED_STATUS_PRESENT) {
        info->caps = FAN_LED_CAPS;
        rv = onlp_file_read_int((int*)&grn_mode, INV_LED_PREFIX"fan_led_grn%d", fan_id);
        if(rv != ONLP_STATUS_OK ) {
            return rv;
        }
        rv = onlp_file_read_int((int*)&red_mode, INV_LED_PREFIX"fan_led_red%d", fan_id);
        if(rv != ONLP_STATUS_OK ) {
            return rv;
        }

        if(grn_mode == 1 && red_mode == 0) {
            info->mode = ONLP_LED_MODE_GREEN;
        } else if(grn_mode == 0 && red_mode == 1) {
            info->mode = ONLP_LED_MODE_RED;
        } else if(grn_mode == 0 && red_mode == 0) {
            info->mode = ONLP_LED_MODE_OFF;
        } else {
            AIM_LOG_ERROR("[ONLP][LED] Invalid fan led status in %s\n",__FUNCTION__);
            rv = ONLP_STATUS_E_INVALID;
        }
    } else {
        info->mode = ONLP_LED_MODE_OFF;
    }
    return rv;
}

int
onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
    int rv = ONLP_STATUS_OK;
    VALIDATE(id);
    int local_id;
    local_id = ONLP_OID_ID_GET(id);
    if(local_id >= ONLP_LED_MAX) {
        return ONLP_STATUS_E_INVALID;
    }

    switch(local_id) {
    case ONLP_LED_MGMT:
        rv = _sys_onlp_ledi_info_get(id, info);
        break;
    case ONLP_LED_FAN1:
    case ONLP_LED_FAN2:
    case ONLP_LED_FAN3:
    case ONLP_LED_FAN4:
        rv = _fan_onlp_ledi_info_get(id, info);
        break;
    default:
        rv = ONLP_STATUS_E_INVALID;
        break;
    }
    return rv;
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
    /*the led behavior is handled by another driver on D6332. Therefore, we're not supporting this attribute*/
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
    /*the led behavior is handled by another driver on D6332. Therefore, we're not supporting this attribute*/
    return ONLP_STATUS_E_UNSUPPORTED;
}
