/************************************************************
 * sysi.c
 *
 *           Copyright 2018 Inventec Technology Corporation.
 *
 ************************************************************
 *
 ***********************************************************/
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>

#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include <onlplib/onie.h>
#include <onlp/platformi/sysi.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/fani.h>
#include <onlp/platformi/psui.h>

#include "x86_64_inventec_d6356j_int.h"
#include "x86_64_inventec_d6356j_log.h"

#include "platform_lib.h"

static onlp_oid_t __oid_info[] = {
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_PHY),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_CORE0),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_CORE1),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_CORE2),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_CPU_CORE3),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_1_ON_MAIN_BROAD),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_2_ON_MAIN_BROAD),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_3_ON_MAIN_BROAD),
    ONLP_THERMAL_ID_CREATE(ONLP_THERMAL_4_ON_MAIN_BROAD),
    ONLP_FAN_ID_CREATE(ONLP_FAN_1),
    ONLP_FAN_ID_CREATE(ONLP_FAN_2),
    ONLP_FAN_ID_CREATE(ONLP_FAN_3),
    ONLP_FAN_ID_CREATE(ONLP_FAN_4),
    ONLP_FAN_ID_CREATE(ONLP_FAN_5),
    ONLP_FAN_ID_CREATE(ONLP_FAN_6),
    ONLP_FAN_ID_CREATE(ONLP_FAN_7),
    ONLP_FAN_ID_CREATE(ONLP_FAN_8),
    ONLP_PSU_ID_CREATE(ONLP_PSU_1),
    ONLP_PSU_ID_CREATE(ONLP_PSU_2),
    ONLP_LED_ID_CREATE(ONLP_LED_MGMT),
    ONLP_LED_ID_CREATE(ONLP_LED_FAN1),
    ONLP_LED_ID_CREATE(ONLP_LED_FAN2),
    ONLP_LED_ID_CREATE(ONLP_LED_FAN3),
    ONLP_LED_ID_CREATE(ONLP_LED_FAN4),
    0/*end*/
};

static int _sysi_version_parsing(char* file_str, char* str_buf, char* str_buf2, char* version);

static int _sysi_version_parsing(char* file_str, char* str_buf, char* str_buf2, char* version)
{
    int rv = ONLP_STATUS_OK;
    int len;
    char buf[ONLP_CONFIG_INFO_STR_MAX * 4];
    char *temp;
    char cpld_v1[ONLP_CONFIG_INFO_STR_MAX];
    char cpld_v2[ONLP_CONFIG_INFO_STR_MAX];

    rv = onlp_file_read((uint8_t*)buf, ONLP_CONFIG_INFO_STR_MAX * 4, &len, file_str);
    if ( rv != ONLP_STATUS_OK ) {
        return rv;
    }
    temp = strstr(buf, str_buf);
    if (temp) {
        temp += strlen(str_buf);
        sscanf(temp, "%s", cpld_v1);
    } else {
        return ONLP_STATUS_E_INVALID;
    }
    temp = strstr(temp, str_buf2);
    if (temp) {
        temp += strlen(str_buf2);
        sscanf(temp, "%s", cpld_v2);
    } else {
        return ONLP_STATUS_E_INVALID;
    }
    snprintf(version, ONLP_CONFIG_INFO_STR_MAX, "c1_%s c2_%s", cpld_v1, cpld_v2);

    return rv;
}

const char*
onlp_sysi_platform_get(void)
{
    return "x86-64-inventec-d6356j-r0";
}


/*
 * This function is called to return the physical base address
 * of the ONIE boot rom.
 *
 * The ONLP framework will mmap() and parse the ONIE TLV structure
 * from the given data.
 *
 * If you platform does not support a mappable address for the ONIE
 * eeprom then you should not provide this function at all.
 *
 * For the purposes of this example we will provide it but
 * return UNSUPPORTED (which is all the default implementation does).
 *
 */
int
onlp_sysi_onie_data_phys_addr_get(void** pa)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}


/*
 * If you cannot provide a base address you must provide the ONLP
 * framework the raw ONIE data through whatever means necessary.
 *
 * This function will be called as a backup in the event that
 * onlp_sysi_onie_data_phys_addr_get() fails.
 */
int
onlp_sysi_onie_data_get(uint8_t** data, int* size)
{
    int ret=ONLP_STATUS_E_INVALID;
    int eeprom_size;
    int rv;
    uint8_t* eeprom_data;

    FILE* fp  = fopen(INV_EEPROM_PATH, "rb");

    if(fp) {
        fseek(fp, 0L, SEEK_END);
        eeprom_size = ftell(fp);
        rewind(fp);
        eeprom_data = aim_malloc(eeprom_size);

        rv = fread(eeprom_data, 1, eeprom_size, fp);
        fclose(fp);

        if(rv == eeprom_size) {
            ret=ONLP_STATUS_OK;
            *data=eeprom_data;
            *size=eeprom_size;
        }

    }


    return ret;
}

/*
 * IF the ONLP frame calles onlp_sysi_onie_data_get(),
 * if will call this function to free the data when it
 * is finished with it.
 *
 * This function is optional, and depends on the data
 * you return in onlp_sysi_onie_data_get().
 */
void
onlp_sysi_onie_data_free(uint8_t* data)
{
    /*
     * We returned a static array in onlp_sysi_onie_data_get()
     * so no free operation is required.
     */
    if (data) {
        aim_free(data);
    }
}


int
onlp_sysi_onie_info_get (onlp_onie_info_t *onie)
{
    int rv = ONLP_STATUS_OK;

    rv = onlp_onie_decode_file(onie, INV_EEPROM_PATH);

    onie->_hdr_id_string = aim_fstrdup("TlvInfo");
    onie->_hdr_version = 0x1;
    return rv;
}


int
onlp_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int rv = ONLP_STATUS_OK;
    char cpld_str[ONLP_CONFIG_INFO_STR_MAX] = {0};
    char version[ONLP_CONFIG_INFO_STR_MAX];
    pi->cpld_versions = NULL;

    rv = _sysi_version_parsing( INV_INFO_PREFIX"info", "The CPLD version is ", "The CPLD2 version is ", version);
    if ( rv != ONLP_STATUS_OK ) {
        return rv;
    }
    snprintf(cpld_str, ONLP_CONFIG_INFO_STR_MAX, "%s%s ", cpld_str, version);
    /*cpld version*/
    if (strlen(cpld_str) > 0) {
        pi->cpld_versions = aim_fstrdup("%s", cpld_str);
    }
    return rv;
}

void
onlp_sysi_platform_info_free(onlp_platform_info_t* pi)
{
    if (pi->cpld_versions) {
        aim_free(pi->cpld_versions);
    }
    if (pi->other_versions) {
        aim_free(pi->other_versions);
    }
    return;
}


int
onlp_sysi_oids_get(onlp_oid_t* table, int max)
{
    int i;
    onlp_oid_t* e = table;
    memset(table, 0, max * sizeof(onlp_oid_t));

    for (i = 0; i < max; i++) {
        if (__oid_info[i] == 0) {
            break;
        }
        *e++ = __oid_info[i];
    }
    return ONLP_STATUS_OK;
}

int
onlp_sysi_platform_manage_fans(void)
{
    /*Ensure switch manager is working*/
    PLATFORM_HWMON_DIAG_LOCK;
    return ONLP_STATUS_OK;
}

int
onlp_sysi_platform_manage_leds(void)
{
    /*Ensure switch manager is working*/
    PLATFORM_HWMON_DIAG_LOCK;
    return ONLP_STATUS_OK;
}

