#include <onlp/onlp.h>
#include <onlplib/file.h>
#include <unistd.h>
#include <fcntl.h>
#include "platform_lib.h"
#include <onlp/platformi/sfpi.h>
#include "x86_64_accton_as7726_32x_log.h"


static int _onlp_file_write(char *filename, char *buffer, int buf_size, int data_len)
{
    int fd;
    int len;

    if ((buffer == NULL) || (buf_size < 0)) {
        return -1;
    }

    if ((fd = open(filename, O_WRONLY, S_IWUSR)) == -1) {
        return -1;
    }

    if ((len = write(fd, buffer, buf_size)) < 0) {
        close(fd);
        return -1;
    }

    if ((close(fd) == -1)) {
        return -1;
    }

    if ((len > buf_size) || (data_len != 0 && len != data_len)) {
        return -1;
    }

    return 0;
}

int onlp_file_write_integer(char *filename, int value)
{
    char buf[8] = {0};
    sprintf(buf, "%d", value);

    return _onlp_file_write(filename, buf, (int)strlen(buf), 0);
}

int onlp_file_read_binary(char *filename, char *buffer, int buf_size, int data_len)
{
    int fd;
    int len;

    if ((buffer == NULL) || (buf_size < 0)) {
        return -1;
    }

    if ((fd = open(filename, O_RDONLY)) == -1) {
        return -1;
    }

    if ((len = read(fd, buffer, buf_size)) < 0) {
        close(fd);
        return -1;
    }

    if ((close(fd) == -1)) {
        return -1;
    }

    if ((len > buf_size) || (data_len != 0 && len != data_len)) {
        return -1;
    }

    return 0;
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

#define I2C_PSU_MODEL_NAME_LEN 11
#define I2C_PSU_FAN_DIR_LEN    3
#include <ctype.h>
psu_type_t get_psu_type(int id, char* modelname, int modelname_len)
{
    char *node = NULL;
    char  model_name[I2C_PSU_MODEL_NAME_LEN + 1] = {0};
    char  fan_dir[I2C_PSU_FAN_DIR_LEN + 1] = {0};


    /* Check AC model name */
    node = (id == PSU1_ID) ? PSU1_AC_HWMON_NODE(psu_model_name) : PSU2_AC_HWMON_NODE(psu_model_name);

    if (onlp_file_read_string(node, model_name, sizeof(model_name), 0) != 0) {
        return PSU_TYPE_UNKNOWN;
    }

    if(isspace(model_name[strlen(model_name)-1])) {
        model_name[strlen(model_name)-1] = 0;
    }

    if (strncmp(model_name, "YM-2651Y", 8) == 0) {
        if (modelname) {
            aim_strlcpy(modelname, model_name, 8+1);
        }

        node = (id == PSU1_ID) ? PSU1_AC_PMBUS_NODE(psu_fan_dir) : PSU2_AC_PMBUS_NODE(psu_fan_dir);
        if (onlp_file_read_string(node, fan_dir, sizeof(fan_dir), 0) != 0) {
            return PSU_TYPE_UNKNOWN;
        }

        if (strncmp(fan_dir, "F2B", strlen("F2B")) == 0) {
            return PSU_TYPE_AC_F2B_3YPOWER;
        }

        if (strncmp(fan_dir, "B2F", strlen("B2F")) == 0) {
            return PSU_TYPE_AC_B2F_3YPOWER;
        }
    }

    if (strncmp(model_name, "YM-2651V", 8) == 0) {
        if (modelname) {
            aim_strlcpy(modelname, model_name, 8+1);
        }

        node = (id == PSU1_ID) ? PSU1_AC_PMBUS_NODE(psu_fan_dir) : PSU2_AC_PMBUS_NODE(psu_fan_dir);
        if (onlp_file_read_string(node, fan_dir, sizeof(fan_dir), 0) != 0) {
            return PSU_TYPE_UNKNOWN;
        }

        if (strncmp(fan_dir, "F2B", strlen("F2B")) == 0) {
            return PSU_TYPE_DC_48V_F2B;
        }

        if (strncmp(fan_dir, "B2F", strlen("B2F")) == 0) {
            return PSU_TYPE_DC_48V_B2F;
        }
    }

    if (strncmp(model_name, "FSF019", 6) == 0) {
        if (modelname) {
            aim_strlcpy(modelname, model_name, 11+1); /* Copy full model name */
        }

        /* Read model */
        char *string = NULL;
        char *prefix = (id == PSU1_ID) ? PSU1_AC_PMBUS_PREFIX : PSU2_AC_PMBUS_PREFIX;
        int len = onlp_file_read_str(&string, "%s""psu_fan_dir", prefix);
        if (!string || len <= 0) {
            return PSU_TYPE_UNKNOWN;
        }

        aim_strlcpy(fan_dir, string, (len+1));
        aim_free(string);

        if (strncmp(fan_dir, "F2B", strlen("F2B")) == 0) {
            return PSU_TYPE_AC_F2B_ACBEL;
        }

        if (strncmp(fan_dir, "B2F", strlen("B2F")) == 0) {
            return PSU_TYPE_AC_B2F_ACBEL;
        }
    }

    return PSU_TYPE_UNKNOWN;
}

int psu_pmbus_info_get(int id, char *node, int *value)
{
    int  ret = 0;
    char path[PSU_NODE_MAX_PATH_LEN] = {0};

    *value = 0;

    if (PSU1_ID == id) {
        sprintf(path, "%s%s", PSU1_AC_PMBUS_PREFIX, node);
    }
    else {
        sprintf(path, "%s%s", PSU2_AC_PMBUS_PREFIX, node);
    }

    if (onlp_file_read_int(value, path) < 0) {
        AIM_LOG_ERROR("Unable to read status from file(%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ret;
}

int psu_pmbus_info_set(int id, char *node, int value)
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

#define PSU_SERIAL_NUMBER_LEN	18

int psu_pmbus_serial_number_get(int id, char *serial, int serial_len)
{
	int   size = 0;
	int   ret  = ONLP_STATUS_OK;
	char *prefix = NULL;

	if (serial == NULL || serial_len < PSU_SERIAL_NUMBER_LEN) {
		return ONLP_STATUS_E_PARAM;
	}

	prefix = (id == PSU1_ID) ? PSU1_AC_PMBUS_PREFIX : PSU2_AC_PMBUS_PREFIX;

	ret = onlp_file_read((uint8_t*)serial, PSU_SERIAL_NUMBER_LEN, &size, "%s%s", prefix, "psu_mfr_serial");
    if (ret != ONLP_STATUS_OK || size != PSU_SERIAL_NUMBER_LEN) {
		return ONLP_STATUS_E_INTERNAL;

    }

	serial[PSU_SERIAL_NUMBER_LEN] = '\0';
	return ONLP_STATUS_OK;
}

int psu_acbel_serial_number_get(int id, char *serial, int serial_len)
{
    char *serial_number = NULL;
    char *prefix = (id == PSU1_ID) ? PSU1_AC_HWMON_PREFIX : PSU2_AC_HWMON_PREFIX;

    int len = onlp_file_read_str(&serial_number, "%s""psu_serial_number", prefix);
    if (!serial_number || len <= 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    aim_strlcpy(serial, serial_number, (len+1));
    aim_free(serial_number);

    return ONLP_STATUS_OK;
}
