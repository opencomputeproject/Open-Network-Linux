/************************************************************
 * psui.c
 *
 *           Copyright 2018 Inventec Technology Corporation.
 *
 ************************************************************
 *
 ***********************************************************/
#include <stdio.h>
#include <string.h>
#include "platform_lib.h"
#include <onlp/platformi/psui.h>
#include <onlplib/file.h>

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_PSU(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

typedef enum hwmon_psu_state_e {
    HWMON_PSU_NORMAL = 0,
    HWMON_PSU_UNPOWERED = 2,
    HWMON_PSU_FAULT = 4,
    HWMON_PSU_NOT_INSTALLED = 7
} hwmon_psu_state_t;

#define PSU_CAPS ONLP_PSU_CAPS_VIN|ONLP_PSU_CAPS_VOUT|ONLP_PSU_CAPS_IIN|ONLP_PSU_CAPS_IOUT|ONLP_PSU_CAPS_PIN| ONLP_PSU_CAPS_POUT


/*
 * Get all information about the given PSU oid.
 */
#define MAKE_PSU_NODE_INFO(id)		\
    {					\
        {				\
            ONLP_PSU_ID_CREATE(ONLP_PSU_##id), "PSU-"#id, 0,		\
            {								\
                ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_1_ON_PSU##id),	\
                ONLP_FAN_ID_CREATE(ONLP_FAN_PSU_##id)			\
            }				\
        },				\
        "","", 0, PSU_CAPS,			\
    }


static onlp_psu_info_t __onlp_psu_info[ ] = {
    {},
    MAKE_PSU_NODE_INFO(1),
    MAKE_PSU_NODE_INFO(2)
};

int
onlp_psui_init(void)
{
    return ONLP_STATUS_OK;
}


int
onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* info)
{
    int ret   = ONLP_STATUS_OK;
    int psu_id = ONLP_OID_ID_GET(id);

    VALIDATE(id);

    if(psu_id >= ONLP_PSU_MAX) {
        AIM_LOG_ERROR("[ONLP][PSU] OID %d out of range in %s", psu_id, __FUNCTION__);
        return ONLP_STATUS_E_INVALID;
    }

    *info = __onlp_psu_info[psu_id];
    ret = onlp_psui_status_get(id, &info->status);
    if(ret != ONLP_STATUS_OK) {
        return ret;
    }

    if(info->status & ONLP_PSU_STATUS_PRESENT) {
        //snprintf(path,ONLP_CONFIG_INFO_STR_MAX, "/var/psu%d/",psu_id);

        snprintf(info->model, ONLP_CONFIG_INFO_STR_MAX, "N/A");
        snprintf(info->serial, ONLP_CONFIG_INFO_STR_MAX, "N/A");

        if(ret==ONLP_STATUS_OK) {
            ret=onlp_file_read_int( &info->mvin, INV_PSU_PREFIX"psoc_psu%d_vin", psu_id );
        }

        if(ret==ONLP_STATUS_OK) {
            ret=onlp_file_read_int( &info->mvout, INV_PSU_PREFIX"psoc_psu%d_vout", psu_id  );
        }

        if(ret==ONLP_STATUS_OK) {
            ret=onlp_file_read_int( &info->miin, INV_PSU_PREFIX"psoc_psu%d_iin", psu_id );
        }

        if(ret==ONLP_STATUS_OK) {
            ret=onlp_file_read_int( &info->miout, INV_PSU_PREFIX"psoc_psu%d_iout", psu_id );
        }

        if(ret==ONLP_STATUS_OK) {
            ret=onlp_file_read_int( &info->mpin, INV_PSU_PREFIX"psoc_psu%d_pin", psu_id );
        }

        if(ret==ONLP_STATUS_OK) {
            ret=onlp_file_read_int( &info->mpout, INV_PSU_PREFIX"psoc_psu%d_pout", psu_id );
        }
    }

    return ret;

}


/**
 * @brief Get the PSU's operational status.
 * @param id The PSU OID.
 * @param rv [out] Receives the operational status.
 */
int onlp_psui_status_get(onlp_oid_t id, uint32_t* rv)
{
    int result = ONLP_STATUS_OK;
    hwmon_psu_state_t psu_state;
    int psu_id;
    int len;
    char buf[ONLP_CONFIG_INFO_STR_MAX];

    VALIDATE(id);


    psu_id = ONLP_OID_ID_GET(id);

    if(psu_id >= ONLP_PSU_MAX) {
        AIM_LOG_ERROR("[ONLP][PSU] OID %d out of range in %s", psu_id, __FUNCTION__);
        result = ONLP_STATUS_E_INVALID;
    } else {
        result = onlp_file_read((uint8_t*)&buf, ONLP_CONFIG_INFO_STR_MAX, &len, INV_INFO_PREFIX"psu%d", LOCAL_ID_TO_INFO_IDX(psu_id) );
        if( result != ONLP_STATUS_OK ) {
            return result;
        }
        psu_state = (uint8_t)strtoul(buf, NULL, 0);
        if( psu_state == HWMON_PSU_UNPOWERED) {
            *rv = ONLP_PSU_STATUS_PRESENT|ONLP_PSU_STATUS_UNPLUGGED;
        } else if ( psu_state == HWMON_PSU_NORMAL) {
            *rv = ONLP_PSU_STATUS_PRESENT;
        } else if( psu_state == HWMON_PSU_NOT_INSTALLED ) {
            *rv = 0;
        } else {
            AIM_LOG_ERROR("[ONLP][PSU] Not defined PSU behavior detected on psu@%d\n", psu_id);
            result = ONLP_STATUS_E_INTERNAL;
        }
    }
    return result;
}

/**
 * @brief Get the PSU's oid header.
 * @param id The PSU OID.
 * @param rv [out] Receives the header.
 */
int onlp_psui_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* rv)
{
    int result = ONLP_STATUS_OK;
    onlp_psu_info_t* info;
    int psu_id=ONLP_OID_ID_GET(id);

    VALIDATE(id);

    if(psu_id >= ONLP_PSU_MAX) {
        result = ONLP_STATUS_E_INVALID;
    } else {
        info = &__onlp_psu_info[psu_id];
        *rv = info->hdr;
    }
    return result;
}


int
onlp_psui_ioctl(onlp_oid_t pid, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
