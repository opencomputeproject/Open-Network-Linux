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

#define LOCAL_ID_TO_FAN_ID(id) (id-1)

typedef enum sys_led_mode_e {
    SYS_LED_MODE_OFF = 0,
    SYS_LED_MODE_0_5_HZ = 1,
    SYS_LED_MODE_1_HZ = 2,
    SYS_LED_MODE_2_HZ = 3,
    SYS_LED_MODE_ON = 7
} sys_led_mode_t;

/* function declarations*/
static int _sys_onlp_led_mode_convert(sys_led_mode_t grn_mode, sys_led_mode_t red_mode, onlp_led_mode_t* pmode);
static int _sys_onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info);
static int _fan_onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info);
static int _sys_onlp_ledi_mode_set(onlp_led_mode_t onlp_mode);

/*
 * Get the information for the given LED OID.
 */
#define MAKE_MGMT_LED_INFO_NODE			\
    {						\
        { ONLP_LED_ID_CREATE(ONLP_LED_MGMT), "MGMT LED" , 0 }, \
        ONLP_LED_STATUS_PRESENT,		\
        SYS_LED_CAPS,		\
    }

#define MAKE_LED_INFO_NODE_ON_FAN(fan_id)	\
    {						\
        { ONLP_LED_ID_CREATE(ONLP_LED_FAN##fan_id), \
          "FAN LED "#fan_id,			\
          ONLP_FAN_ID_CREATE(ONLP_FAN_##fan_id) \
        },					\
        0,	FAN_LED_CAPS,					\
    }

static onlp_led_info_t __onlp_led_info[] = {
    {},
    MAKE_MGMT_LED_INFO_NODE,
    MAKE_LED_INFO_NODE_ON_FAN(1),
    MAKE_LED_INFO_NODE_ON_FAN(2),
    MAKE_LED_INFO_NODE_ON_FAN(3),
    MAKE_LED_INFO_NODE_ON_FAN(4),
};



/* convert platform led type to onlp_led_mode type*/
static int _sys_onlp_led_mode_convert(sys_led_mode_t grn_mode, sys_led_mode_t red_mode, onlp_led_mode_t* pmode)
{
    int rv = ONLP_STATUS_OK;
    *pmode = ONLP_LED_MODE_OFF;

    switch(grn_mode) {
    case SYS_LED_MODE_0_5_HZ:
    case SYS_LED_MODE_1_HZ:
    case SYS_LED_MODE_2_HZ:
        if(red_mode==SYS_LED_MODE_OFF) {
            *pmode = ONLP_LED_MODE_GREEN_BLINKING;
        } else {
            return ONLP_STATUS_E_INVALID;
        }
        break;
    case SYS_LED_MODE_ON:
        if(red_mode==SYS_LED_MODE_ON) {
            *pmode = ONLP_LED_MODE_ORANGE;
        } else if(red_mode==SYS_LED_MODE_OFF) {
            *pmode = ONLP_LED_MODE_GREEN;
        } else {
            return ONLP_STATUS_E_INVALID;
        }
        break;
    case SYS_LED_MODE_OFF:
        if(red_mode==SYS_LED_MODE_OFF) {
            *pmode = ONLP_LED_MODE_OFF;
        } else if( (red_mode>=SYS_LED_MODE_0_5_HZ) && (red_mode<=SYS_LED_MODE_2_HZ) ) {
            *pmode =ONLP_LED_MODE_RED_BLINKING;
        } else if(red_mode==SYS_LED_MODE_ON) {
            *pmode=ONLP_LED_MODE_RED;
        }
        break;
    default:
        return ONLP_STATUS_E_INVALID;
        break;
    }
    return rv;
}


static int _sys_onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
    int rv = ONLP_STATUS_OK;
    int grn_mode, red_mode;
    int led_id= ONLP_OID_ID_GET(id);
    *info= __onlp_led_info[led_id];

    rv = onlp_file_read_int((int*)&grn_mode, NET_SYSLED_PREFIX"grn_led");
    if(rv != ONLP_STATUS_OK) {
        return rv;
    }
    rv = onlp_file_read_int((int*)&red_mode, NET_SYSLED_PREFIX"red_led");
    if(rv != ONLP_STATUS_OK) {
        return rv;
    }

    rv = _sys_onlp_led_mode_convert(grn_mode, red_mode, &info->mode);
    if(rv != ONLP_STATUS_OK) {
        return rv;
    }

    info->status = (info->mode==ONLP_LED_MODE_OFF)? \
                   REMOVE_STATE(info->status,ONLP_LED_STATUS_ON) : ADD_STATE(info->status,ONLP_LED_STATUS_ON);

    return rv;
}

static int _fan_onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
    int rv = ONLP_STATUS_OK;
    int grn_mode, red_mode;
    int led_id=ONLP_OID_ID_GET(id);
    int fan_id=LOCAL_ID_TO_FAN_ID(led_id);
    *info=__onlp_led_info[led_id];

    rv = onlp_ledi_status_get(id, &info->status);
    if( rv != ONLP_STATUS_OK ) {
        return rv;
    }

    if( info->status & ONLP_LED_STATUS_PRESENT) {
        info->caps = FAN_LED_CAPS;
        rv = onlp_file_read_int((int*)&grn_mode, NET_HWMON_PREFIX"fan_led_grn%d", fan_id);
        if(rv != ONLP_STATUS_OK ) {
            return rv;
        }
        rv = onlp_file_read_int((int*)&red_mode, NET_HWMON_PREFIX"fan_led_red%d", fan_id);
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
            rv = ONLP_STATUS_E_INVALID;
        }
    } else {
        info->mode = ONLP_LED_MODE_OFF;
    }
    return rv;
}

static int _sys_onlp_ledi_mode_set(onlp_led_mode_t onlp_mode)
{
    int rv = ONLP_STATUS_OK;

    if( onlp_mode == ONLP_LED_MODE_OFF) {
        rv = onlp_file_write_int(SYS_LED_MODE_OFF, NET_SYSLED_PREFIX"grn_led");
        if(rv != ONLP_STATUS_OK ) {
            return rv;
        }
        rv = onlp_file_write_int(SYS_LED_MODE_OFF, NET_SYSLED_PREFIX"red_led");
    } else {
        rv = ONLP_STATUS_E_UNSUPPORTED;
    }
    return rv;
}



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
    int rv = ONLP_STATUS_OK;
    VALIDATE(id);
    int led_id= ONLP_OID_ID_GET(id);
    if( led_id >= ONLP_LED_MAX) {
        return ONLP_STATUS_E_INVALID;
    }

    switch(led_id) {
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

/**
 * @brief Get the LED operational status.
 * @param id The LED OID
 * @param rv [out] Receives the operational status.
 */
int onlp_ledi_status_get(onlp_oid_t id, uint32_t* rv)
{
    int result = ONLP_STATUS_OK;
    onlp_led_info_t* info;

    VALIDATE(id);
    int led_id = ONLP_OID_ID_GET(id);
    int fan_id = LOCAL_ID_TO_FAN_ID( led_id);
    int grn_mode, red_mode, mode;
    onlp_led_mode_t pmode;
    uint32_t fan_status;

    if( led_id >= ONLP_LED_MAX) {
        result = ONLP_STATUS_E_INVALID;
    }
    if(result == ONLP_STATUS_OK) {
        info = &__onlp_led_info[led_id];
        switch(led_id) {
        case ONLP_LED_MGMT:
            result = onlp_file_read_int((int*)&grn_mode, NET_SYSLED_PREFIX"grn_led");
            if(result != ONLP_STATUS_OK) {
                return result;
            }
            result = onlp_file_read_int((int*)&red_mode, NET_SYSLED_PREFIX"red_led");
            if(result != ONLP_STATUS_OK) {
                return result;
            }

            result = _sys_onlp_led_mode_convert(grn_mode, red_mode, &pmode);
            if(result != ONLP_STATUS_OK) {
                return result;
            }

            if( pmode != ONLP_LED_MODE_OFF) {
                info->status = ADD_STATE(info->status,ONLP_LED_STATUS_ON);
            } else {
                info->status = REMOVE_STATE(info->status, ONLP_LED_STATUS_ON);
            }

            *rv = info->status;
            break;
        case ONLP_LED_FAN1:
        case ONLP_LED_FAN2:
        case ONLP_LED_FAN3:
        case ONLP_LED_FAN4:
            result = onlp_fani_status_get((&info->hdr)->poid, &fan_status);
            if(result != ONLP_STATUS_OK) {
                return result;
            }

            if(fan_status & ONLP_FAN_STATUS_PRESENT) {
                info->status = ADD_STATE(info->status,ONLP_LED_STATUS_PRESENT);

                result = onlp_file_read_int((int*)&mode, NET_HWMON_PREFIX"fan_led_grn%d", fan_id);
                if(result != ONLP_STATUS_OK) {
                    return result;
                }
                result = onlp_file_read_int((int*)&mode, NET_HWMON_PREFIX"fan_led_red%d", fan_id);
                if(result != ONLP_STATUS_OK) {
                    return result;
                }
                info->status = (mode==1)? \
                               ADD_STATE(info->status,ONLP_LED_STATUS_ON):REMOVE_STATE(info->status,ONLP_LED_STATUS_ON);

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

/**
 * @brief Get the LED header.
 * @param id The LED OID
 * @param rv [out] Receives the header.
 */
int onlp_ledi_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* rv)
{
    int result = ONLP_STATUS_OK;
    onlp_led_info_t* info;

    VALIDATE(id);

    int led_id = ONLP_OID_ID_GET(id);
    if( led_id >= ONLP_LED_MAX) {
        result = ONLP_STATUS_E_INVALID;
    } else {

        info = &__onlp_led_info[led_id];
        *rv = info->hdr;
    }
    return result;
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
    onlp_led_mode_t mode;
    VALIDATE(id);
    int local_id;
    local_id = ONLP_OID_ID_GET(id);
    int idx = LOCAL_ID_TO_INFO_IDX(local_id);

    if(local_id >= ONLP_LED_MAX) {
        return ONLP_STATUS_E_INVALID;
    }

    if (__onlp_led_info[idx].caps & ONLP_LED_CAPS_ON_OFF) {
        mode = on_or_off?ONLP_LED_MODE_ON:ONLP_LED_MODE_OFF;
        return onlp_ledi_mode_set(id, mode);
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
    int rv = ONLP_STATUS_OK;
    VALIDATE(id);
    int local_id;
    local_id = ONLP_OID_ID_GET(id);

    if(local_id >= ONLP_LED_MAX) {
        return ONLP_STATUS_E_INVALID;
    }
    switch(local_id) {
    case ONLP_LED_MGMT:
        rv = _sys_onlp_ledi_mode_set(mode);
        break;
    case ONLP_LED_FAN1:
    case ONLP_LED_FAN2:
    case ONLP_LED_FAN3:
    case ONLP_LED_FAN4:
        rv = ONLP_STATUS_E_UNSUPPORTED;
        break;
    default:
        rv = ONLP_STATUS_E_INVALID;
        break;
    }

    return rv;
}

/*
 * Generic LED ioctl interface.
 */
int
onlp_ledi_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

