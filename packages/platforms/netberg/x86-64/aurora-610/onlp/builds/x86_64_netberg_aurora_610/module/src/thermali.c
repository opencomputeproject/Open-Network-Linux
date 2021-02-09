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
 * Thermal Sensor Platform Implementation.
 *
 ***********************************************************/
#include <unistd.h>
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include <fcntl.h>
#include <onlp/platformi/psui.h>
#include "platform_lib.h"

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_THERMAL(_id)) {         \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)


typedef struct thermali_path_s {
    char file[ONLP_CONFIG_INFO_STR_MAX];
} thermali_path_t;

#define MAKE_THERMAL_PATH_ON_CPU(id)            { NET_CTMP_PREFIX"temp"#id"_input"}
#define MAKE_THERMAL_PATH_ON_MAIN_BROAD(id)     { NET_HWMON_PREFIX"temp"#id"_input"}
#define MAKE_THERMAL1_PATH_ON_PSU(psu_id)   { NET_HWMON_PREFIX"thermal_psu"#psu_id}
#define MAKE_THERMAL2_PATH_ON_PSU(psu_id)       { NET_HWMON_PREFIX"thermal2_psu"#psu_id}

static thermali_path_t __path_list[ ] = {
    {},
    MAKE_THERMAL_PATH_ON_CPU(1),
    MAKE_THERMAL_PATH_ON_CPU(2),
    MAKE_THERMAL_PATH_ON_CPU(3),
    MAKE_THERMAL_PATH_ON_CPU(4),
    MAKE_THERMAL_PATH_ON_CPU(5),
    MAKE_THERMAL_PATH_ON_MAIN_BROAD(1),
    MAKE_THERMAL_PATH_ON_MAIN_BROAD(2),
    MAKE_THERMAL_PATH_ON_MAIN_BROAD(3),
    MAKE_THERMAL_PATH_ON_MAIN_BROAD(4),
    MAKE_THERMAL_PATH_ON_MAIN_BROAD(5),
    MAKE_THERMAL1_PATH_ON_PSU(1),
    MAKE_THERMAL2_PATH_ON_PSU(1),
    MAKE_THERMAL1_PATH_ON_PSU(2),
    MAKE_THERMAL2_PATH_ON_PSU(2)
};

#define MAKE_THERMAL_INFO_NODE_ON_CPU_PHY \
    {   { ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PHY), "CPU Physical", 0}, \
        ONLP_THERMAL_STATUS_PRESENT, \
        ONLP_THERMAL_CAPS_GET_TEMPERATURE, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS \
    }
#define MAKE_THERMAL_INFO_NODE_ON_CPU_CORE(id) \
    {   { ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_CORE##id), "CPU Core "#id, 0},\
        ONLP_THERMAL_STATUS_PRESENT, \
        ONLP_THERMAL_CAPS_GET_TEMPERATURE, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS \
    }
#define MAKE_THERMAL_INFO_NODE_ON_MAIN_BROAD(id) \
    {   { ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_##id##_ON_MAIN_BROAD), "Thermal Sensor "#id, 0}, \
        ONLP_THERMAL_STATUS_PRESENT, \
        ONLP_THERMAL_CAPS_GET_TEMPERATURE, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS \
    }
#define MAKE_THERMAL_INFO_NODE_ON_PSU(thermal_id, psu_id) \
    {   { \
            ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_##thermal_id##_ON_PSU##psu_id), \
           "PSU-"#psu_id" Thermal Sensor "#thermal_id, \
            ONLP_PSU_ID_CREATE(ONLP_PSU_##psu_id) \
        }, \
        ONLP_THERMAL_CAPS_GET_TEMPERATURE, 0, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS \
    }

/* Static values */
static onlp_thermal_info_t __onlp_thermal_info[ ] = {
    {},
    MAKE_THERMAL_INFO_NODE_ON_CPU_PHY,
    MAKE_THERMAL_INFO_NODE_ON_CPU_CORE(0),
    MAKE_THERMAL_INFO_NODE_ON_CPU_CORE(1),
    MAKE_THERMAL_INFO_NODE_ON_CPU_CORE(2),
    MAKE_THERMAL_INFO_NODE_ON_CPU_CORE(3),
    MAKE_THERMAL_INFO_NODE_ON_MAIN_BROAD(1),
    MAKE_THERMAL_INFO_NODE_ON_MAIN_BROAD(2),
    MAKE_THERMAL_INFO_NODE_ON_MAIN_BROAD(3),
    MAKE_THERMAL_INFO_NODE_ON_MAIN_BROAD(4),
    MAKE_THERMAL_INFO_NODE_ON_MAIN_BROAD(5),
    MAKE_THERMAL_INFO_NODE_ON_PSU(1,1),
    MAKE_THERMAL_INFO_NODE_ON_PSU(2,1),
    MAKE_THERMAL_INFO_NODE_ON_PSU(1,2),
    MAKE_THERMAL_INFO_NODE_ON_PSU(2,2)
};

/*
 * This will be called to intiialize the thermali subsystem.
 */
int
onlp_thermali_init(void)
{
    return ONLP_STATUS_OK;
}

/*
 * Retrieve the information structure for the given thermal OID.
 *
 * If the OID is invalid, return ONLP_E_STATUS_INVALID.
 * If an unexpected error occurs, return ONLP_E_STATUS_INTERNAL.
 * Otherwise, return ONLP_STATUS_OK with the OID's information.
 *
 * Note -- it is expected that you fill out the information
 * structure even if the sensor described by the OID is not present.
 */
int
onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* info)
{
    VALIDATE(id);
    int ret;
    int thermal_id = ONLP_OID_ID_GET(id);
    if(thermal_id >= ONLP_THERMAL_MAX) {
        return ONLP_STATUS_E_INVALID;
    }

    /* Set the onlp_oid_hdr_t and capabilities */
    *info = __onlp_thermal_info[ thermal_id];
    ret = onlp_thermali_status_get(id, &info->status);
    if( ret != ONLP_STATUS_OK ) {
        return ret;
    }

    if(info->status & ONLP_THERMAL_STATUS_PRESENT) {
        ret = onlp_file_read_int(&info->mcelsius, __path_list[ thermal_id].file);
    }

    return ret;
}


/**
 * @brief Retrieve the thermal's operational status.
 * @param id The thermal oid.
 * @param rv [out] Receives the operational status.
 */
int onlp_thermali_status_get(onlp_oid_t id, uint32_t* rv)
{
    int ret = ONLP_STATUS_OK;
    onlp_thermal_info_t* info;
    VALIDATE(id);
    uint32_t psu_status;

    int thermal_id = ONLP_OID_ID_GET(id);
    if(thermal_id >= ONLP_THERMAL_MAX) {
        return ONLP_STATUS_E_INVALID;
    }
    info = &__onlp_thermal_info[ thermal_id];

    switch(thermal_id) {
    case ONLP_THERMAL_1_ON_PSU1:
    case ONLP_THERMAL_2_ON_PSU1:
    case ONLP_THERMAL_1_ON_PSU2:
    case ONLP_THERMAL_2_ON_PSU2:
        ret = onlp_psui_status_get((&info->hdr)->poid, &psu_status);
        if(ret != ONLP_STATUS_OK) {
            return ret;
        }
        info->status = (psu_status & ONLP_PSU_STATUS_PRESENT)?  \
                       ADD_STATE(info->status,ONLP_PSU_STATUS_PRESENT):0;
        break;
    default:
        break;
    }

    *rv = info->status;

    return ret;
}

/**
 * @brief Retrieve the thermal's oid header.
 * @param id The thermal oid.
 * @param rv [out] Receives the header.
 */
int onlp_thermali_hdr_get(onlp_oid_t id, onlp_oid_hdr_t* rv)
{
    onlp_thermal_info_t* info;
    VALIDATE(id);

    int thermal_id = ONLP_OID_ID_GET(id);
    if(thermal_id >= ONLP_THERMAL_MAX) {
        return ONLP_STATUS_E_INVALID;
    }
    info = &__onlp_thermal_info[ thermal_id];

    *rv = info->hdr;

    return ONLP_STATUS_OK;
}
