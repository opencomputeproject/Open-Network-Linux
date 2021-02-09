/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
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
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/utsname.h>
#include <linux/version.h>
#include <AIM/aim.h>
#include <onlplib/file.h>
#include <onlp/onlp.h>
#include <sys/mman.h>
#include "mlnx_common_log.h"
#include "mlnx_common_int.h"

#define MAX_PSU_VPD_LENGTH 256
#define MAX_VPD_FIELD_MAX_LENGTH 64

int
psu_fan_read_eeprom(int psu_index, onlp_fan_info_t* fan_info)
{
    const char sanity_check[]   = "MLNX";
    const uint8_t serial_len    = 24;
    char data[256] = {0};
    bool sanity_found = false;
    int index = 0, rv = 0, len = 0;

    rv = onlp_file_read((uint8_t* )data, sizeof(data)-1, &len,
                IDPROM_PATH, "psu", psu_index);
    if (rv < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Looking for sanity checker */
    while (index < sizeof(data) - sizeof(sanity_check) - 1) {
        if (!strncmp(&data[index], sanity_check, sizeof(sanity_check) - 1)) {
            sanity_found = true;
            break;
        }
        index++;
    }
    if (false == sanity_found) {
        return ONLP_STATUS_E_INVALID;
    }

    /* Serial number */
    index += strlen(sanity_check);
    if (fan_info) {
        aim_strlcpy(fan_info->serial, &data[index], sizeof(fan_info->serial));
    }

    /* Part number */
    index += serial_len;
    if (fan_info) {
        aim_strlcpy(fan_info->model, &data[index], sizeof(fan_info->model));
    }

    return ONLP_STATUS_OK;
}

int
psu_fan_read_eeprom_vpd(int psu_index, onlp_fan_info_t* fan_info)
{
	char file_data[MAX_PSU_VPD_LENGTH] = {0};
	char data[MAX_VPD_FIELD_MAX_LENGTH] = {0};
	char* vpd_field_str = NULL;
	int rv = 0, len=0;

	rv = onlp_file_read((uint8_t* )file_data, sizeof(file_data)-1, &len,
				PSU_VPD_PATH, "psu", psu_index);
	if (rv < 0) {
		return ONLP_STATUS_E_INTERNAL;
	}

	/* Serial number */
	vpd_field_str = strstr(file_data,"SN_VPD_FIELD");
	if(!vpd_field_str) {
		return ONLP_STATUS_E_INTERNAL;
	}
	rv = sscanf(vpd_field_str,"SN_VPD_FIELD: %s", data);
	if (rv != 1) {
		return ONLP_STATUS_E_INTERNAL;
	}
	if (fan_info) {
		aim_strlcpy(fan_info->serial, data, sizeof(fan_info->serial));
	}
	memset(data, 0, MAX_VPD_FIELD_MAX_LENGTH);

	/* Part number */
	vpd_field_str = strstr(file_data,"PN_VPD_FIELD");
	if(!vpd_field_str) {
		return ONLP_STATUS_E_INTERNAL;
	}
	rv = sscanf(vpd_field_str,"PN_VPD_FIELD: %s",  data);
	if (rv != 1) {
		return ONLP_STATUS_E_INTERNAL;
	}
	if (fan_info) {
		aim_strlcpy(fan_info->model, data, sizeof(fan_info->model));
	}
	return ONLP_STATUS_OK;
}

int
psu_read_eeprom(int psu_index, onlp_psu_info_t* psu_info)
{
    char file_data[MAX_PSU_VPD_LENGTH] = {0};
    char data[MAX_VPD_FIELD_MAX_LENGTH] = {0};
    char* vpd_field_str = NULL;
    int rv = 0, len=0;

    rv = onlp_file_read((uint8_t* )file_data, sizeof(file_data)-1, &len,
                PSU_VPD_PATH, "psu", psu_index);
    if (rv < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    /* Serial number */
    vpd_field_str = strstr(file_data,"SN_VPD_FIELD");
    if(!vpd_field_str) {
        return ONLP_STATUS_E_INTERNAL;
    }
    rv = sscanf(vpd_field_str,"SN_VPD_FIELD: %s", data);
    if (rv != 1) {
        return ONLP_STATUS_E_INTERNAL;
    }
    if (psu_info) {
        aim_strlcpy(psu_info->serial, data, sizeof(psu_info->serial));
    }
    memset(data, 0, MAX_VPD_FIELD_MAX_LENGTH);
    vpd_field_str = NULL;
    /* Part number */
    vpd_field_str = strstr(file_data,"PN_VPD_FIELD");
    if(!vpd_field_str) {
        return ONLP_STATUS_E_INTERNAL;
    }
    rv = sscanf(vpd_field_str,"PN_VPD_FIELD: %s",  data);
    if (rv != 1) {
        return ONLP_STATUS_E_INTERNAL;
    }
    if (psu_info) {
        aim_strlcpy(psu_info->model, data, sizeof(psu_info->model));
    }
    return ONLP_STATUS_OK;
}

int
mc_get_kernel_ver()
{
    struct utsname buff;
    char ver[4];
    char *p;
    int i = 0;

    if (uname(&buff) != 0)
        return ONLP_STATUS_E_INTERNAL;

    p = buff.release;

    while (*p) {
        if (isdigit(*p)) {
            ver[i] = strtol(p, &p, 10);
            i++;
            if (i >= 3)
                break;
        } else {
            p++;
        }
    }

    return KERNEL_VERSION(ver[0], ver[1], ver[2]);
}
