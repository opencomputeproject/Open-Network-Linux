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
#include <fcntl.h>
#include <onlplib/file.h>
#include <onlp/onlp.h>
#include "platform_lib.h"

#define PSU_NODE_MAX_PATH_LEN 64
#define PSU_FAN_DIR_LEN         3
#define PSU_MODEL_NAME_LEN 		11
#define PSU_SERIAL_NUMBER_LEN	18

int get_psu_serial_number(int id, char *serial, int serial_len)
{
    int   ret  = 0;
	char *node = NULL;
    char *sn = aim_zmalloc(PSU_SERIAL_NUMBER_LEN + 1);

    if (!sn) {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    /* Read AC serial number */
    node = (id == PSU1_ID) ? PSU1_AC_HWMON_NODE(psu_serial_numer) : PSU2_AC_HWMON_NODE(psu_serial_numer);
    ret = onlp_file_read_str(&sn, node);
    if (ret <= 0 || ret > PSU_SERIAL_NUMBER_LEN) {
        aim_free(sn);
        return ONLP_STATUS_E_INVALID;
    }

    if (serial) {
        strncpy(serial, sn, PSU_SERIAL_NUMBER_LEN+1);
    }

    aim_free(sn);
	return ONLP_STATUS_OK;
}

psu_type_t get_psu_type(int id, char* modelname, int modelname_len)
{
    int   ret = 0;
    char *node = NULL;
    char *mn = aim_zmalloc(PSU_MODEL_NAME_LEN + 1);
    psu_type_t ptype = PSU_TYPE_UNKNOWN;

    /* Check AC model name */
    node = (id == PSU1_ID) ? PSU1_AC_HWMON_NODE(psu_model_name) : PSU2_AC_HWMON_NODE(psu_model_name);
    ret = onlp_file_read_str(&mn, node);
    if (ret <= 0 || ret > PSU_MODEL_NAME_LEN) {
        aim_free(mn);
        return PSU_TYPE_UNKNOWN;
    }

    if (modelname) {
        strncpy(modelname, mn, PSU_MODEL_NAME_LEN + 1);
    }
    
    if (strncmp(mn, "YM-1401A", 8) == 0) {
        char *fd = aim_zmalloc(PSU_FAN_DIR_LEN + 1 + 1);
        
        node = (id == PSU1_ID) ? PSU1_AC_PMBUS_NODE(psu_fan_dir) : PSU2_AC_PMBUS_NODE(psu_fan_dir);
        ret = onlp_file_read_str(&fd, node);

        if (ret <= 0 || ret > PSU_FAN_DIR_LEN) {
            ptype = PSU_TYPE_UNKNOWN;
        }
        else if (strncmp(fd, "B2F", PSU_FAN_DIR_LEN) == 0) {
            ptype = PSU_TYPE_AC_B2F;
        }
        else {
            ptype = PSU_TYPE_AC_F2B;
        }

        aim_free(fd);
    }
    else if (strncmp(mn, "YM-2401HCR", 10) == 0) {
        ptype = PSU_TYPE_DC_F2B;
    }
    else if (strncmp(mn, "YM-2401HDR", 10) == 0) {
        ptype = PSU_TYPE_DC_B2F;
    }
    else if (strncmp(mn, "DPS400AB33A", PSU_MODEL_NAME_LEN) == 0) {
        ptype = PSU_TYPE_AC_F2B;
    }
    else if (strncmp(mn, "DPS400AB34A", PSU_MODEL_NAME_LEN) == 0) {
        ptype = PSU_TYPE_AC_B2F;
    }

    aim_free(mn);
    return ptype;
}

int psu_ym1401_pmbus_info_get(int id, char *node, int *value)
{
    int  ret = 0;
    *value = 0;

    if (PSU1_ID == id) {
        ret = onlp_file_read_int(value, "%s%s", PSU1_AC_PMBUS_PREFIX, node);
    }
    else {
        ret = onlp_file_read_int(value, "%s%s", PSU2_AC_PMBUS_PREFIX, node);
    }

    if (ret < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ret;
}

int psu_ym1401_pmbus_info_set(int id, char *node, int value)
{
    char path[PSU_NODE_MAX_PATH_LEN] = {0};

	switch (id) {
	case PSU1_ID:
		sprintf(path, "%s%s", PSU1_AC_PMBUS_PREFIX, node);
		break;
	case PSU2_ID:
		sprintf(path, "%s%s", PSU2_AC_PMBUS_PREFIX, node);
		break;
	default:
		return ONLP_STATUS_E_UNSUPPORTED;
	};

    if (onlp_file_write_int(value, path) != 0) {
        AIM_LOG_ERROR("Unable to write data to file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}
