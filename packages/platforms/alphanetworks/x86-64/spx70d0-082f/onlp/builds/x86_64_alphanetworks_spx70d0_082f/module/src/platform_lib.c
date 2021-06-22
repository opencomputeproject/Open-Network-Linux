/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2020 Alpha Networks Incorporation
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
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <execinfo.h>  /* for backtrace() */
#include <AIM/aim.h>
#include <onlp/platformi/sfpi.h>
#include "platform_lib.h"

#define DEBUG_FLAG 0

int deviceNodeWrite(char *filename, char *buffer, int buf_size, int data_len)
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

int deviceNodeWriteInt(char *filename, int value, int data_len)
{
    char buf[8] = {0};
    sprintf(buf, "%d", value);

    return deviceNodeWrite(filename, buf, sizeof(buf)-1, data_len);
}

int deviceNodeReadBinary(char *filename, char *buffer, int buf_size, int data_len)
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

int deviceNodeReadString(char *filename, char *buffer, int buf_size, int data_len)
{
    int ret;

    if (data_len >= buf_size || data_len < 0) {
        return -1;
    }
    
    ret = deviceNodeReadBinary(filename, buffer, buf_size-1, data_len);

    if (ret == 0) {
        if (data_len) {
            buffer[data_len] = '\0';
        }
        else {
            buffer[buf_size-1] = '\0';
        }
    }
    
    return ret;
}


int psu_two_complement_to_int(uint16_t data, uint8_t valid_bit, int mask)
{
    uint16_t  valid_data  = data & mask;
    bool is_negative = valid_data >> (valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

int i2c_sequential_read(int i2cbus, int addr, int offset, int length, char* data)
{
    int file;
    int i;
    char filename[20];
    int res = 0;

    /*open i2c device*/
    snprintf(filename, sizeof(filename), "/dev/i2c/%d", i2cbus);
    filename[sizeof(filename) - 1] = '\0';
    file = open(filename, O_RDWR);

    if (file < 0) {
        sprintf(filename, "/dev/i2c-%d", i2cbus);
        file = open(filename, O_RDWR);
    }
    if (file < 0)
    {
        AIM_LOG_INFO("Unable to open id:%d %s\r\n", i2cbus, filename);
        return -1;
    }

    /*set slave address set_slave_addr(file, address, force))*/
    
    if (ioctl(file, I2C_SLAVE_FORCE, addr) < 0)
    {
        AIM_LOG_INFO("Error: Could not set address to 0x%02x, %s \r\n", addr, strerror(errno));
        close(file);
        return -errno;
    }
    /* 
      Sequential Read for at24c128
        use i2c_smbus_write_byte_data to write 24c128 address counter(two 8-bit data word addresses)
        24c128:
                    +----------------------------------------------------+
                    | S | Device  | Wr | A | 1st Word | A | 2nd Word | A |...
                    |   | Address |    |   | Address  |   | Address  |   |
                    +----------------------------------------------------+
        SMbus:
                    +----------------------------------------------------+
                    | S | Device  | Wr | A | Command | A | Data      | A |...
                    |   | Address |    |   |         |   |           |   |
                    +----------------------------------------------------+        
     
      */
    res = i2c_smbus_write_byte_data(file, (uint8_t)offset>>8,(uint8_t)offset);

    if(res != 0) {
        AIM_LOG_INFO("Error: Write start address failed, return code %d\n", res);
            return -1;
    }

    for (i = 0; i<length; i++)
    {
        res = i2c_smbus_read_byte(file);
  
        if (res < 0 && DEBUG_FLAG)
        {
            AIM_LOG_INFO("Error: i2c_smbus_read_byte_data offset:%d\r\n", offset+i);
            close(file);
            return res;
        }
        data[i] = res;
    }

    close(file);
    return 0;
}

int bmc_get_raw_bus_id(int bus)
{
    int ret = 0;
    int ipmi_bus_id;

    switch (bus)
    {
        case BMC_I2C1:
            ipmi_bus_id =  IPMI_I2C_BUS0_ID;
            break;

        case BMC_I2C2:
            ipmi_bus_id =  IPMI_I2C_BUS1_ID;
            break;

        case BMC_I2C3:
            ipmi_bus_id =  IPMI_I2C_BUS2_ID;
            break;

        case BMC_I2C4:
            ipmi_bus_id =  IPMI_I2C_BUS3_ID;
            break;

        case BMC_I2C5:
            ipmi_bus_id =  IPMI_I2C_BUS4_ID;
            break;

        case BMC_I2C6:
            ipmi_bus_id =  IPMI_I2C_BUS5_ID;
            break;

        default:
            return ret;
    }

    return ipmi_bus_id;
}

int bmc_command_read(char *cmd, char *data)
{
    int rc = 0;
    FILE * fp;
    char result_buf[80];
    int result_buf_size;

    memset(result_buf, 0, sizeof(result_buf));    
    result_buf_size = sizeof(result_buf);
    
    fp = popen(cmd, "r");
    if(NULL == fp)
    {
        printf("popen Fail! \n");
        return -1/*FALSE*/;
    }

    rc = fread(result_buf, 1, result_buf_size, fp);
    if(rc == result_buf_size)
    {
        printf("buffer too small\n");
        return -1/*FALSE*/;
    }    

   rc = pclose(fp);
   if(-1 == rc)
    {
        printf("pclose Fail! \n");
        return -1/*FALSE*/;
    }

//debug
//printf("[%s(%d)]result_buf:%s\n", __func__, __LINE__,result_buf);

    /* the return value of ipmitool is hex string, transfer to dec integer. ex:0x1f-> 31 */
    int result_dec = (int)strtol(result_buf,NULL,16);

//debug
//printf("[%s(%d)] result_dec:%d \n", __func__, __LINE__, result_dec);

    /* finally, return dec integer*/
    *data = result_dec;

    return 1/*TRUE*/;
     
}

/*
 * @brief
 *     This function is use IPMI tool to read register/asddress through BCM with PMBus command
 * 
 * @param cmd     IPMI command string
 * @param data    The value to get 
 * 
 * @return int
 * 
 */
int bmc_command_pmb_read(char *cmd, char *data)
{
    int rc = 0;
    FILE * fp;
    char result_buf[80];
    int result_buf_size;
    char ret_str[2], result_str[2];
    int result_dec;

    memset(result_buf, 0, sizeof(result_buf));    
    result_buf_size = sizeof(result_buf);
    
    fp = popen(cmd, "r");
    if(NULL == fp)
    {
        printf("popen Fail! \n");
        return -1/*FALSE*/;
    }

    rc = fread(result_buf, 1, result_buf_size, fp);
    if(rc == result_buf_size)
    {
        printf("buffer too small\n");
        return -1/*FALSE*/;
    }    

   rc = pclose(fp);
   if(-1 == rc)
    {
        printf("pclose Fail! \n");
        return -1/*FALSE*/;
    }

//debug
//printf("[%s(%d)]result_buf:%s\n", __func__, __LINE__,result_buf);

    /* the return value of ipmitool is hex string, transfer to dec integer. ex:0x1f-> 31 */
    /*
     *  Use IPMI tool with PMBus command the return value is like '10 39'
     *    10 is return code.
     *    39 is the value get from register/address.
     */
    sscanf(result_buf, "%s %s", ret_str, result_str);

    result_dec = (int)strtol(result_str, NULL, 16);
//debug
//printf("[%s(%d)] result_dec:%d \n", __func__, __LINE__, result_dec);

    /* finally, return dec integer*/
    *data = result_dec;

    return 1/*TRUE*/;
     
}

int bmc_i2c_read_byte(int bus, int devaddr, int offset, char* data)
{
    int ret = 0;
    char cmd[128] = {0};

    int raw_bus_id=0;

    raw_bus_id = bmc_get_raw_bus_id(bus);

    if (raw_bus_id)
        snprintf(cmd, sizeof(cmd), "ipmitool raw 0x06 0x52 0x%02x 0x%x 1 0x%02x", raw_bus_id, devaddr, offset);        
    else
        return ret;

//debug
//printf("[%s(%d)] cmd:%s \n", __func__, __LINE__, cmd);

    ret = bmc_command_read(cmd, data);

//debug
//printf("[%s(%d)] data:%d, ret %d \n", __func__, __LINE__, *data, ret);

    return ret;

}


/*
 * @brief
 *  This function use for read register through BMC with PMBus command.
 * 
 * @param bus      I2C bus ID
 * @parma devaddr  device address
 * @param pmb_cmd  PMBus command
 * @param offset   register offset
 * @param data     register data
 * 
 * @return int     True or False
 * 
 */
int bmc_i2c_pmb_cmd_read_byte(int bus, int devaddr, int pmb_cmd, int offset, char* data)
{
    int ret = 0;
    char cmd[128] = {0};

    int raw_bus_id=0;

    raw_bus_id = bmc_get_raw_bus_id(bus);

    if (raw_bus_id)
        snprintf(cmd, sizeof(cmd), "ipmitool raw 0x06 0x52 0x%02x 0x%x 2 %d 1 0x%02x", raw_bus_id, devaddr, pmb_cmd, offset);        
    else
        return ret;

//debug
//printf("[%s(%d)] cmd:%s \n", __func__, __LINE__, cmd);

    ret = bmc_command_pmb_read(cmd, data);

//debug
//printf("[%s(%d)] data:%d, ret %d \n", __func__, __LINE__, *data, ret);

    return ret;
}

int bmc_command_write(char *cmd)
{
    int rc = 0;
    FILE * fp;

    fp = popen(cmd, "w");
    if(NULL == fp)
    {
        printf("Error:popen Fail! \n");
        return -1/*FALSE*/;
    }

   rc = pclose(fp);
   if(-1 == rc)
    {
        printf("Error:pclose Fail! \n");
        return -1/*FALSE*/;
    }

    return 1/*TRUE*/;     
}

int bmc_i2c_write_byte(int bus, int devaddr, int offset, char value)    
{
    int ret = 0;
    char cmd[128] = {0};
    int raw_bus_id=0;

    raw_bus_id = bmc_get_raw_bus_id(bus);

    if (raw_bus_id)
        snprintf(cmd, sizeof(cmd), "ipmitool raw 0x06 0x52 0x%02x 0x%x 0x00 0x%02x 0x%02X", raw_bus_id, devaddr, offset, (unsigned char)value);
    else
        return ret;

    ret = bmc_command_write(cmd);
    
    return ret;
}

static char diag_flag=0;

char diag_flag_set(char d)
{
    diag_flag = d;
    return 0;
}

char diag_flag_get(void)
{
    return diag_flag;
}

char diag_debug_trace_on(void)
{
    system("echo 1 > /tmp/onlpi_dbg_trace");
    return 0;
}

char diag_debug_trace_off(void)
{
    system("echo 0 > /tmp/onlpi_dbg_trace");
    return 0;
}

char diag_debug_trace_check(void)
{
    char flag = 0;
    FILE* file = fopen ("/tmp/onlpi_dbg_trace", "r");
    if (file == NULL)
    {
        return 0;
    }
    flag = fgetc (file);
    fclose (file);

    return (flag == '1')?1:0;
}

char* sfp_control_to_str(int value)
{
    switch (value)
    {
        case ONLP_SFP_CONTROL_RESET:
            return "RESET";
        case ONLP_SFP_CONTROL_RESET_STATE:
            return "RESET_STATE";
        case ONLP_SFP_CONTROL_RX_LOS:
            return "RX_LOS";
        case ONLP_SFP_CONTROL_TX_FAULT:
            return "TX_FAULT";
        case ONLP_SFP_CONTROL_TX_DISABLE:
            return "TX_DISABLE";
        case ONLP_SFP_CONTROL_TX_DISABLE_CHANNEL:
            return "TX_DISABLE_CHANNEL";
        case ONLP_SFP_CONTROL_LP_MODE:
            return "LP_MODE";
        case ONLP_SFP_CONTROL_POWER_OVERRIDE:
            return "POWER_OVERRIDE";

        default:
            return "UNKNOW";
    }
    return "";
}

char diag_debug_pause_platform_manage_on(void)
{
    system("echo 1 > /tmp/onlpi_dbg_pause_pm");
    return 0;
}

char diag_debug_pause_platform_manage_off(void)
{
    system("echo 0 > /tmp/onlpi_dbg_pause_pm");
    return 0;
}

char diag_debug_pause_platform_manage_check(void)
{
    char flag = 0;
    FILE* file = fopen ("/tmp/onlpi_dbg_pause_pm", "r");
    if (file == NULL)
    {
        return 0;
    }
    flag = fgetc (file);
    fclose (file);

    return (flag == '1')?1:0;
}

#define ONIE_EEPROM_HEADER_LENGTH 11
int eeprom_tlv_read(uint8_t *rdata, char type, char *data)
{
    uint8_t i, j, TLV_length;
    int total_data_length = ((*(rdata + 9)) << 8) + (*(rdata + 10)) + ONIE_EEPROM_HEADER_LENGTH;
    char TLV_type;

    for (i = ONIE_EEPROM_HEADER_LENGTH; i < total_data_length;)
    {
        TLV_type = *(rdata + i);
        TLV_length = *(rdata + i + 1);
        //printf("Type:%d Len:%d\n", TLV_type,TLV_length);
        if (TLV_type == type)
        {
            for (j = 0; j < TLV_length; j++)
            {
                data[j] = *(rdata + i + j + 2);
                //printf("TLV match\n");
            }
            data[j] = '\0';
            return 0;
        }
        i += (TLV_length + 2);
        //printf("i:%d\n",i);
    }
    return 0;
}

#define BT_ARR_SZ           15
void dump_stack(void)
{
    void *array[BT_ARR_SZ];
    size_t size;
    char **strings;
    int i;
    int levels = BT_ARR_SZ;

    size = backtrace(array, BT_ARR_SZ);
    AIM_LOG_INFO("backtrace size[%d]\n", (int)size);

    strings = backtrace_symbols(array, size);
    if (strings == NULL)
    {
        AIM_LOG_INFO("(backtrace_symbols fail.)\n");
        for (i = 0; i < size; i++)
            AIM_LOG_INFO("  (%2d)%p\n", i, array[i]);
        return;
    }

    levels += 1;/* add 1 is because 1st calltrace will be skipped */
    if (levels > size)
        levels = size;

    /* skip 1st calltrace, because it would be this function! */
    for (i = 1; i < levels; i++)
        AIM_LOG_INFO("%s\n", strings[i]);
    
    free (strings);
}

