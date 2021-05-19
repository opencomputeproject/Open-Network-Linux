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
 ************************************************************
 *
 *
 *
 ***********************************************************/
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <onlp/platformi/sysi.h>
#include <onlp/platformi/sfpi.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/fani.h>
#include "x86_64_delta_agcv208s_int.h"
#include "x86_64_delta_agcv208s_log.h"
#include "vendor_driver_pool.h"
#include "vendor_i2c_device_list.h"

/**
 * @brief Return the name of the the platform implementation.
 * @notes This will be called PRIOR to any other calls into the
 * platform driver, including the sysi_init() function below.
 *
 * The platform implementation name should match the current
 * ONLP platform name.
 *
 * IF the platform implementation name equals the current platform name,
 * initialization will continue.
 *
 * If the platform implementation name does not match, the following will be
 * attempted:
 *
 *    onlp_sysi_platform_set(current_platform_name);
 * If this call is successful, initialization will continue.
 * If this call fails, platform initialization will abort().
 *
 * The onlp_sysi_platform_set() function is optional.
 * The onlp_sysi_platform_get() is not optional.
 */
const char *onlp_sysi_platform_get(void)
{
    /*
        ONIE NAME USING "_" FOR MACHINE NAME, BUT ONL USING "-" FOR EACH FIELD
    */
    char *buffer = calloc(30, sizeof(char));
    int idx = 0;

    strncpy(buffer, platform_name, VENDOR_MAX_NAME_SIZE);

    while (buffer[idx] != '\0')
    {
        if (buffer[idx] == '_')
            buffer[idx] -= 50; // Change '_' to '-'
        idx++;
    }

    return buffer;
}

/**
 * @brief Attempt to set the platform personality
 * in the event that the current platform does not match the
 * reported platform.
 * @note Optional
 */
int onlp_sysi_platform_set(const char *platform)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Initialize the system platform subsystem.
 */
int onlp_sysi_init(void)
{
    vendor_driver_init();

    void *busDrv = (void *)vendor_find_driver_by_name("CYPRESSI2C");

    cpld_dev_driver_t *cpld =
        (cpld_dev_driver_t *)vendor_find_driver_by_name("CPLD");

    int cpld_idx = vendor_find_cpld_idx_by_name("port-cpld");

    if (cpld_idx < 0)
        return ONLP_STATUS_E_INTERNAL;

    vendor_dev_do_oc(cpld_o_list[cpld_idx]);

    cpld->writeb(
        busDrv,
        0x04,
        0x32,
        0x0a,
        0x0);
    cpld->writeb(
        busDrv,
        0x04,
        0x32,
        0x11,
        0x0);

    vendor_dev_do_oc(cpld_c_list[cpld_idx]);

    return ONLP_STATUS_OK;
}

/**
 * @brief Provide the physical base address for the ONIE eeprom.
 * @param param [out] physaddr Receives the physical address.
 * @notes If your platform provides a memory-mappable base
 * address for the ONIE eeprom data you can return it here.
 * The ONLP common code will then use this address and decode
 * the ONIE TLV specification data. If you cannot return a mappable
 * address due to the platform organization see onlp_sysi_onie_data_get()
 * instead.
 */
int onlp_sysi_onie_data_phys_addr_get(void **physaddr)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/**
 * @brief Return the raw contents of the ONIE system eeprom.
 * @param data [out] Receives the data pointer to the ONIE data.
 * @param size [out] Receives the size of the data (if available).
 * @notes This function is only necessary if you cannot provide
 * the physical base address as per onlp_sysi_onie_data_phys_addr_get().
 */
int onlp_sysi_onie_data_get(uint8_t **data, int *size)
{
    int id = 0, rv = 0;
    uint8_t *rdata = aim_zmalloc(eeprom_dev_data_list[id].len);
    *size = eeprom_dev_data_list[id].len;

    void *busDrv = (void *)vendor_find_driver_by_name(eeprom_dev_list[id].bus_drv_name);
    eeprom_dev_driver_t *eeprom =
        (eeprom_dev_driver_t *)vendor_find_driver_by_name(eeprom_dev_list[id].dev_drv_name);

    vendor_dev_do_oc(eeprom_o_list[id]);
    rv = eeprom->load(
        busDrv,
        eeprom_dev_list[id].bus,
        eeprom_dev_list[id].dev,
        eeprom_dev_data_list[id].offset,
        eeprom_dev_data_list[id].alen,
        rdata);
    vendor_dev_do_oc(eeprom_c_list[id]);

    if (rv < 0)
    {
        return ONLP_STATUS_E_INVALID;
    }

    *data = rdata;

    return ONLP_STATUS_OK;
}

/**
 * @brief Free the data returned by onlp_sys_onie_data_get()
 * @param data The data pointer.
 * @notes If onlp_sysi_onie_data_get() is called to retreive the
 * contents of the ONIE system eeprom then this function
 * will be called to perform any cleanup that may be necessary
 * after the data has been used.
 */
void onlp_sysi_onie_data_free(uint8_t *data)
{
    aim_free(data);
}

int onlp_sysi_onie_info_get(onlp_onie_info_t *onie)
{
    int rv = 0, id = 0;
    uint8_t data[eeprom_dev_data_list[id].len];
    data[0] = 0;

    void *busDrv = (void *)vendor_find_driver_by_name(eeprom_dev_list[id].bus_drv_name);
    eeprom_dev_driver_t *eeprom =
        (eeprom_dev_driver_t *)vendor_find_driver_by_name(eeprom_dev_list[id].dev_drv_name);

    if (onie == NULL)
        return 0;

    vendor_dev_do_oc(eeprom_o_list[id]);
    rv = eeprom->load(
        busDrv,
        eeprom_dev_list[id].bus,
        eeprom_dev_list[id].dev,
        eeprom_dev_data_list[id].offset,
        eeprom_dev_data_list[id].alen,
        data);
    vendor_dev_do_oc(eeprom_c_list[id]);

    rv = onlp_onie_decode(onie, data, eeprom_dev_data_list[id].len);

    if (rv < 0)
    {
        return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief This function returns the root oid list for the platform.
 * @param table [out] Receives the table.
 * @param max The maximum number of entries you can fill.
 */
int onlp_sysi_oids_get(onlp_oid_t *table, int max)
{
    int i;
    onlp_oid_t *e = table;
    memset(table, 0, max * sizeof(onlp_oid_t));

    for (i = 1; i <= psu_list_size; i++)
    {
        *e++ = ONLP_PSU_ID_CREATE(i);
    }

    for (i = 1; i <= thermal_list_size; i++)
    {
        *e++ = ONLP_THERMAL_ID_CREATE(i);
    }

    for (i = 1; i <= led_list_size; i++)
    {
        *e++ = ONLP_LED_ID_CREATE(i);
    }

    for (i = 1; i <= fan_list_size; i++)
    {
        *e++ = ONLP_FAN_ID_CREATE(i);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief This function provides a generic ioctl interface.
 * @param code context dependent.
 * @param vargs The variable argument list for the ioctl call.
 * @notes This is provided as a generic expansion and
 * and custom programming mechanism for future and non-standard
 * functionality.
 * @notes Optional
 */
int onlp_sysi_ioctl(int code, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

enum thermal_policy_level_e
{
    THERMAL_LEVEL_0,  /* 0%*/
    THERMAL_LEVEL_1,  /* 10%*/
    THERMAL_LEVEL_2,  /* 20%*/
    THERMAL_LEVEL_3,  /* 30%*/
    THERMAL_LEVEL_4,  /* 40%*/
    THERMAL_LEVEL_5,  /* 50%*/
    THERMAL_LEVEL_6,  /* 60%*/
    THERMAL_LEVEL_7,  /* 70%*/
    THERMAL_LEVEL_8,  /* 80%*/
    THERMAL_LEVEL_9,  /* 90%*/
    THERMAL_LEVEL_10, /* 100%*/
    THERMAL_LEVEL_SHUTDOWN
};

typedef struct policy_data_s
{
    int level;
    int instance;
    int highThreshold;
    int lowThreshold;
} policy_group_t;

typedef struct thermal_policy_s
{
    int fanPercentage;
    int nOfThermal;
    policy_group_t *policyGroup;
} thermal_policy_t;

static thermal_policy_t thermalPolicy[11] = {
    {0, 0, NULL},
    {10, 0, NULL},
    {20, 0, NULL},
    {30, 0, NULL},
    {40, 0, NULL},
    {50, 0, NULL},
    {60, 0, NULL},
    {70, 0, NULL},
    {80, 0, NULL},
    {90, 0, NULL},
    {100, 0, NULL},
};

/* Modify this array for your platform */
static policy_group_t policyGroup[] = {
    {4, 0, 40000, 0},
    {4, 1, 40000, 0},
    {4, 2, 40000, 0},
    {4, 3, 40000, 0},
    {6, 0, 52000, 37000},
    {6, 1, 52000, 37000},
    {6, 2, 52000, 37000},
    {6, 3, 52000, 37000},
    {8, 0, 62000, 49000},
    {8, 1, 62000, 49000},
    {8, 2, 62000, 49000},
    {8, 3, 62000, 49000},
    {10, 0, 77000, 59000},
    {10, 1, 76000, 59000},
    {10, 2, 77000, 59000},
    {10, 3, 80000, 59000},
    {11, 0, 0, 0},
};

/*
//FOR TEST
static policy_group_t policyGroup[] = {
    {4, 0, 31000, 0},
    {4, 1, 31000, 0},
    {4, 2, 31000, 0},
    {4, 3, 31000, 0},
    {6, 0, 33000, 30500},
    {6, 1, 33000, 30500},
    {6, 2, 33000, 30500},
    {6, 3, 33000, 30500},
    {8, 0, 35000, 32000},
    {8, 1, 35000, 32000},
    {8, 2, 35000, 32000},
    {8, 3, 35000, 32000},
    {10, 0, 77000, 34000},
    {10, 1, 76000, 34000},
    {10, 2, 77000, 34000},
    {10, 3, 80000, 34000},
    {11, 0, 0, 0},
};*/

/**
 * @brief Platform management initialization.
 */
int onlp_sysi_platform_manage_init(void)
{
    int index = 0;

    while (policyGroup[index].level != THERMAL_LEVEL_SHUTDOWN)
    {
        thermalPolicy[policyGroup[index].level].nOfThermal++;

        if (thermalPolicy[policyGroup[index].level].nOfThermal == 1)
        {
            thermalPolicy[policyGroup[index].level].policyGroup = &policyGroup[index];
        }

        index++;
    }

    return ONLP_STATUS_OK;
}

#define UP_OPERATION 2
#define KEEP_OPERATION 1
#define DOWN_OPERATION 0

static int currLevel = THERMAL_LEVEL_0;
static int currOperation = UP_OPERATION; /* Default to UP_OPERATION */

/**
 * @brief Perform necessary platform fan management.
 * @note This function should automatically adjust the FAN speeds
 * according to the platform conditions.
 */
int onlp_sysi_platform_manage_fans(void)
{
    return 0;
    int index = 0, nOfDown = 0;
    onlp_thermal_info_t info;
    policy_group_t *currPolicy = NULL;

    currPolicy = thermalPolicy[currLevel].policyGroup;
    AIM_LOG_MSG("[Thermal Policy] Now at level %d", currLevel);

    /**
     * Step 1. 
     * Check each thermal status on current level,
     * Change the "currOperation" and "currLevel" when the temperature
     * is higher or lower then threshold.
    */
    for (index = 0; index < thermalPolicy[currLevel].nOfThermal; index++)
    {
        onlp_thermal_info_get(ONLP_THERMAL_ID_CREATE(currPolicy->instance + 1), &info);

        if (info.mcelsius >= currPolicy->highThreshold)
        {
            AIM_LOG_MSG("[Thermal Policy] Thermal[%d] overhigh! UP_OPERATION!",
                        currPolicy->instance + 1);
            currOperation = UP_OPERATION;
            currLevel++;
            break;
        }
        else if (info.mcelsius < currPolicy->lowThreshold)
        {
            nOfDown++;

            if (nOfDown == thermalPolicy[currLevel].nOfThermal && nOfDown != 0)
            {
                AIM_LOG_MSG("[Thermal Policy] DOWN_OPERATION!");
                currOperation = DOWN_OPERATION;
                currLevel--;
                break;
            }
        }
        else
        {
            currOperation = KEEP_OPERATION;
        }

        currPolicy++;
    }

    /**
     * Step 2. 
     * Check the currOperation, if "KEEP_OPERATION" we do nothing to avoid
     * the unnecessary i2c read/write.
     * 
     * if not "KEEP_OPERATION", offset to the next available level, 
     * because not all of level has the policies.
    */
    if (currOperation == KEEP_OPERATION)
    {
        return ONLP_STATUS_OK;
    }

    while (!thermalPolicy[currLevel].nOfThermal)
    {
        currLevel =
            currOperation == UP_OPERATION ? currLevel + 1 : currLevel - 1;

        if (currLevel == THERMAL_LEVEL_SHUTDOWN)
        {
            AIM_LOG_ERROR("[Thermal Policy] THERMAL_LEVEL_SHUTDOWN!");
        }

        if (currLevel > THERMAL_LEVEL_SHUTDOWN || currLevel < THERMAL_LEVEL_0)
        {
            AIM_LOG_ERROR("[Thermal Policy] Unexpected Level: %d", currLevel);

            /*TODO*/
            currLevel = THERMAL_LEVEL_SHUTDOWN;
            return ONLP_STATUS_E_INVALID;
        }
    }

    AIM_LOG_MSG("[Thermal Policy] Change to level %d", currLevel);
    /**
     * Step 3. 
     * Do the thermal policy on specific level.
    */
    for (index = 0; index < fan_list_size; index++)
    {
        onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(index + 1),
                                 thermalPolicy[currLevel].fanPercentage);
    }

    return ONLP_STATUS_OK;
}

/**
 * @brief Perform necessary platform LED management.
 * @note This function should automatically adjust the LED indicators
 * according to the platform conditions.
 */
int onlp_sysi_platform_manage_leds(void)
{

    return ONLP_STATUS_OK;
}

/**
 * @brief Return custom platform information.
 */
int onlp_sysi_platform_info_get(onlp_platform_info_t *pi)
{
    int rv = 0, cpld_idx = 0;
    uint8_t cpld_version[cpld_list_size];
    char buffer[256] = "";

    void *busDrv = NULL;
    cpld_dev_driver_t *cpld = (cpld_dev_driver_t *)vendor_find_driver_by_name("CPLD");

    for (cpld_idx = 0; cpld_idx < cpld_list_size; cpld_idx++)
    {
        if (cpld_version_list[cpld_idx].type == 0)
            continue;

        busDrv = (void *)vendor_find_driver_by_name(cpld_version_list[cpld_idx].bus_drv_name);
        vendor_dev_do_oc(cpld_o_list[cpld_idx]);
        rv = cpld->readb(
            busDrv,
            cpld_version_list[cpld_idx].bus,
            cpld_version_list[cpld_idx].dev,
            cpld_version_list[cpld_idx].addr,
            &cpld_version[cpld_idx]);
        vendor_dev_do_oc(cpld_c_list[cpld_idx]);

        if (rv < 0)
        {
            return ONLP_STATUS_E_INTERNAL;
        }
        sprintf(buffer, "%s \n\t\t%s: 0x%02x ", buffer, cpld_version_list[cpld_idx].name, cpld_version[cpld_idx]);
    }

    pi->cpld_versions = aim_fstrdup(buffer);

    return ONLP_STATUS_OK;
}

/**
 * @brief Friee a custom platform information structure.
 */
void onlp_sysi_platform_info_free(onlp_platform_info_t *pi)
{
    aim_free(pi->cpld_versions);
}
/**
 * @brief Builtin platform debug tool.
 */
int onlp_sysi_debug(aim_pvs_t *pvs, int argc, char **argv)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
