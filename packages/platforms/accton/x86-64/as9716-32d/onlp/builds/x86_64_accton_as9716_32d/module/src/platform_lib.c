#include <onlp/onlp.h>
#include <onlplib/file.h>
#include <unistd.h>
#include <fcntl.h>
#include "platform_lib.h"
#include <onlp/platformi/sfpi.h>
#include "x86_64_accton_as9716_32d_log.h"


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

#define I2C_PSU_MODEL_NAME_LEN 10
#define I2C_PSU_FAN_DIR_LEN    3

psu_type_t get_psu_type(int id, char* modelname, int modelname_len)
{    
    char *node = NULL;
    char  model_name[I2C_PSU_MODEL_NAME_LEN + 1] = {0};   

    /* Check model name */
    node = (id == PSU1_ID) ? PSU1_AC_PMBUS_NODE(psu_mfr_model) : PSU2_AC_PMBUS_NODE(psu_mfr_model);
    memset(model_name, 0x0, I2C_PSU_MODEL_NAME_LEN + 1);
    memset(modelname, 0x0, modelname_len); 
    if (onlp_file_read_string(node, model_name, sizeof(model_name), 0) != 0) {
        
        return PSU_TYPE_UNKNOWN;
    }
	
    if (!strncmp(model_name, "FSH082", strlen("FSH082")))
    {
        if (modelname)
            aim_strlcpy(modelname, model_name, strlen("FSH082")<(modelname_len-1)?strlen("FSH082"):(modelname_len-1));
        
        return PSU_TYPE_ACBEL;
    }
    
    if (!strncmp(model_name, "YESM1300AM", strlen("YESM1300AM")))
    {
        if (modelname)
            aim_strlcpy(modelname, model_name, strlen("YESM1300AM")<(modelname_len-1)?strlen("YESM1300AM"):(modelname_len-1));

        return PSU_TYPE_YESM1300;
    }
    if (!strncmp(model_name, "YM-2651Y", strlen("YM-2651Y")))
    {
        if (modelname)
            aim_strlcpy(modelname, model_name, modelname_len-1);   
        return PSU_TYPE_YM2651Y;
    }

    return PSU_TYPE_UNKNOWN;
}
int 
psu_pmbus_info_get(int id, char *node, int *value)
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



int psu_ym2651y_pmbus_info_get(int id, char *node, int *value)
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

    if (serial == NULL || serial_len < PSU_SERIAL_NUMBER_LEN) {
        return ONLP_STATUS_E_PARAM;
    }

    prefix = (id == PSU1_ID) ? PSU1_AC_HWMON_PREFIX : PSU2_AC_HWMON_PREFIX;

    ret = onlp_file_read((uint8_t*)serial, PSU_SERIAL_NUMBER_LEN, &size, "%s%s", prefix, "psu_serial_number");
    if (ret != ONLP_STATUS_OK || size != PSU_SERIAL_NUMBER_LEN) {
        return ONLP_STATUS_E_INTERNAL;

    }
    serial[PSU_SERIAL_NUMBER_LEN-1] = '\0';

    return ONLP_STATUS_OK;
}

int psu_status_info_get(int id, char *node, int *value)
{
    int ret = 0;
    char path[PSU_NODE_MAX_PATH_LEN] = {0};
    
    *value = 0;
    if (PSU1_ID == id) {
        sprintf(path, "%s%s", PSU1_AC_HWMON_PREFIX, node);
    }
    else if (PSU2_ID == id) {
        sprintf(path, "%s%s", PSU2_AC_HWMON_PREFIX, node);
    }
    if (onlp_file_read_int(value, path) < 0) {
        AIM_LOG_ERROR("Unable to read status from file(%s)\r\n", path);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ret;
}
