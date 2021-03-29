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
#define AIM_FREE_IF_PTR(p) \
    do \
    { \
        if (p) { \
            aim_free(p); \
            p = NULL; \
        } \
    } while (0)

int onlp_file_write_integer(char *filename, int value)
{
    return onlp_file_write_int(value, filename, NULL);
}

int onlp_file_read_binary(char *filename, char *buffer, int buf_size, int data_len)
{
    if ((buffer == NULL) || (buf_size < 0)) {
        return -1;
    }

    return onlp_file_read((uint8_t*)buffer, buf_size, &data_len, "%s", filename);
}

int onlp_file_read_string(char *filename, char *buffer, int buf_size, int data_len)
{
    int ret;

    if (data_len >= buf_size) {
	    return -1;
	}

	ret = onlp_file_read_binary(filename, buffer, buf_size-1, data_len);

    if (ret == 0) {
        buffer[buf_size-1] = '\0';
    }

    return ret;
}

#define PSU_MODEL_NAME_LEN 9
#define PSU_FAN_DIR_LEN    3

psu_type_t get_psu_type(int id, char* modelname, int modelname_len)
{
    int   ret = 0;
    char *node = NULL;
    char  model_name[PSU_MODEL_NAME_LEN + 1] = {0};
    psu_type_t ptype = PSU_TYPE_UNKNOWN;
    
    /* Check AC model name */
    node = (id == PSU1_ID) ? PSU1_AC_HWMON_NODE(psu_model_name) : PSU2_AC_HWMON_NODE(psu_model_name);

    if (onlp_file_read_string(node, model_name, sizeof(model_name), 0) != 0) {
        return PSU_TYPE_UNKNOWN;
    }
    if (strncmp(model_name, "YM-2651Y", strlen("YM-2651Y")) == 0)
    {
        if (modelname)
        {
            aim_strlcpy(modelname, model_name, sizeof(model_name));
        }

        char *fan_str = NULL;

        node = (id == PSU1_ID) ? PSU1_AC_PMBUS_NODE(psu_fan_dir) : PSU2_AC_PMBUS_NODE(psu_fan_dir);
        ret = onlp_file_read_str(&fan_str, node);
        if (ret <= 0 || ret > PSU_FAN_DIR_LEN || fan_str == NULL)
        {
            ptype = PSU_TYPE_UNKNOWN;
        }
        else if (strncmp(fan_str, "F2B", PSU_FAN_DIR_LEN) == 0)
        {
            ptype = PSU_TYPE_AC_F2B;
        }
        else
        {
            ptype = PSU_TYPE_AC_B2F;
        }

        AIM_FREE_IF_PTR(fan_str);
    }
    if (strncmp(model_name, "YM-2651V", 8) == 0)
    {
        if (modelname)
        {
            aim_strlcpy(modelname, model_name, sizeof(model_name));
        }

        char *fan_str = NULL;

        node = (id == PSU1_ID) ? PSU1_AC_PMBUS_NODE(psu_fan_dir) : PSU2_AC_PMBUS_NODE(psu_fan_dir);
        ret = onlp_file_read_str(&fan_str, node);
        if (ret <= 0 || ret > PSU_FAN_DIR_LEN || fan_str == NULL)
        {
            ptype = PSU_TYPE_UNKNOWN;
        }
        else if (strncmp(fan_str, "F2B", PSU_FAN_DIR_LEN) == 0)
        {
            ptype = PSU_TYPE_DC_48V_F2B;
        }
        else
        {
            ptype = PSU_TYPE_DC_48V_B2F;
        }

        AIM_FREE_IF_PTR(fan_str);
    }

    return ptype;
}

int psu_ym2651y_pmbus_info_get(int id, char *node, int *value)
{
    int  ret = 0;
    char path[PSU_NODE_MAX_PATH_LEN] = {0};
    
    *value = 0;

    if (PSU1_ID == id) {
        ret = onlp_file_read_int(value, "%s%s", PSU1_AC_PMBUS_PREFIX, node);
    }
    else {
		ret = onlp_file_read_int(value, "%s%s", PSU2_AC_PMBUS_PREFIX, node);
    }

    if (ret < 0) {
        AIM_LOG_ERROR("Unable to read status from file(%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ret;
}

int psu_ym2651y_pmbus_info_set(int id, char *node, int value)
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

    if (onlp_file_write_integer(path, value) < 0) {
        AIM_LOG_ERROR("Unable to write data to file (%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

#define PSU_SERIAL_NUMBER_LEN	19

int psu_serial_number_get(int id, char *serial, int serial_len)
{
    int   size = 0;
    int   ret  = ONLP_STATUS_OK; 
    char *prefix = NULL;

    if (serial == NULL || serial_len < PSU_SERIAL_NUMBER_LEN)
    {
        return ONLP_STATUS_E_PARAM;
    }
    prefix = (id == PSU1_ID) ? PSU1_AC_PMBUS_PREFIX : PSU2_AC_PMBUS_PREFIX;
    ret = onlp_file_read((uint8_t*)serial, PSU_SERIAL_NUMBER_LEN, &size, "%s%s", prefix, "psu_mfr_serial");
    if (ret != ONLP_STATUS_OK || size != PSU_SERIAL_NUMBER_LEN) {
		return ONLP_STATUS_E_INTERNAL;

    }
    serial[PSU_SERIAL_NUMBER_LEN-1] = '\0';

    return ONLP_STATUS_OK;
}
