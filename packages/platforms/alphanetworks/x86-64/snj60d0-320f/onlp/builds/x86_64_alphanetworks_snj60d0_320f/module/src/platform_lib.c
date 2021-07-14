/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2018 Alpha Networks Incorporation
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
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <onlplib/file.h>
#include <AIM/aim.h>
#include <onlp/platformi/sfpi.h>
#include "platform_lib.h"

#include <onlplib/i2c.h>

#define DEBUG_FLAG 0
#define PSU_MODEL_NAME_LEN          10
#define PSU_SERIAL_NUMBER_LEN       19
#define PSU_SERIAL_NUMBER_OFFSET    0x35

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


/*
i2c APIs: access i2c device by ioctl
static int i2c_read(int i2cbus, int addr, int offset, int length, char* data)
static int i2c_read_byte(int i2cbus, int addr, int offset, char* data)
static int i2c_read_word(int i2cbus, int addr, int command)
static int i2c_write_byte(int i2cbus, int addr, int offset, char val)
static int i2c_write_bit(int i2cbus, int addr, int offset, int bit, char val)
*/
int i2c_read(int i2cbus, int addr, int offset, int length, char* data)
{
    int file;
    int i;
    char filename[20];

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

    #if 0/*check funcs*/
    unsigned long funcs;
    if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
        AIM_LOG_INFO("Error: Could not get the adapter\r\n");
        return -1;
    }
    //I2C_SMBUS_BLOCK_DATA
    #endif

    /*set slave address set_slave_addr(file, address, force))*/

    if (ioctl(file, I2C_SLAVE_FORCE, addr) < 0)
    //if (ioctl(file, I2C_SLAVE, addr) < 0)
    {
        AIM_LOG_INFO("Error: Could not set address to 0x%02x, %s \r\n", addr, strerror(errno));
        close(file);
        return -errno;
    }

    #if 1
    int res = 0;
    for (i = 0; i<length; i++)
    {
        res = i2c_smbus_read_byte_data(file, offset+i);
        if (res < 0 && DEBUG_FLAG)
        {
            AIM_LOG_INFO("Error: i2c_smbus_read_byte_data offset:%d\r\n", offset+i);
            close(file);
            return res;
        }
        data[i] = res;
    }

    #else

    /*i2c_smbus_read_block_data(file, bank, cblock);*/
    int res = 0;
    unsigned char cblock[288];
    res = i2c_smbus_read_block_data(file, 0, cblock);
    i2c_smbus_read_byte_data(file, i+j)
    if(res < 0)
    {
        AIM_LOG_INFO("Error: read failed res:%d\r\n", res);
        return -1;
    }

    if((offset + length) > res)
    {
        AIM_LOG_INFO("Error: Size out of range offset:%d l:%d res:%d\r\n", offset, length, res);
        return -1;
    }

    for (i = 0; i < length; i++)
    {
        data[i] = cblock[offset + i];
    }
    #endif

    close(file);
    return 0;
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

int i2c_read_rps_status(int i2cbus, int addr, int offset)
{
    int file;
    //int i;
    char filename[20];

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

    #if 0/*check funcs*/
    unsigned long funcs;
    if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
        AIM_LOG_INFO("Error: Could not get the adapter\r\n");
        return -1;
    }
    //I2C_SMBUS_BLOCK_DATA
    #endif

    /*set slave address set_slave_addr(file, address, force))*/

    //if (ioctl(file, I2C_SLAVE_FORCE, addr) < 0)
    if (ioctl(file, I2C_SLAVE, addr) < 0)
    {
        AIM_LOG_INFO("Error: Could not set address to 0x%02x, %s \r\n", addr, strerror(errno));
        close(file);
        return -errno;
    }

    int res = 0;
    res = i2c_smbus_read_byte_data(file, offset);
    if (res < 0 && DEBUG_FLAG)
    {
        AIM_LOG_INFO("Error: i2c_smbus_read_byte_data offset:%d\r\n", offset);
        close(file);
        return res;
    }

    close(file);
    return res;
}

int _i2c_read_word_data(int i2cbus, int addr, int offset)
{
    int file;
    //int i;
    char filename[20];

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

    #if 0/*check funcs*/
    unsigned long funcs;
    if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
        AIM_LOG_INFO("Error: Could not get the adapter\r\n");
        return -1;
    }
    //I2C_SMBUS_BLOCK_DATA
    #endif

    /*set slave address set_slave_addr(file, address, force))*/

    //if (ioctl(file, I2C_SLAVE_FORCE, addr) < 0)
    if (ioctl(file, I2C_SLAVE, addr) < 0)
    {
        AIM_LOG_INFO("Error: Could not set address to 0x%02x, %s \r\n", offset, strerror(errno));
        close(file);
        return -errno;
    }

    #if 1
    int res = 0;

    res = i2c_smbus_read_word_data(file, offset);
    if (res < 0 && DEBUG_FLAG)
    {
        AIM_LOG_INFO("Error: i2c_smbus_read_word_data status:%d\r\n", res);
        //close(file);
        //return res;
    }

    #else

    /*i2c_smbus_read_block_data(file, bank, cblock);*/
    int res = 0;
    unsigned char cblock[288];
    res = i2c_smbus_read_block_data(file, 0, cblock);
    i2c_smbus_read_byte_data(file, i+j)
    if(res < 0)
    {
        AIM_LOG_INFO("Error: read failed res:%d\r\n", res);
        return -1;
    }

    if((offset + length) > res)
    {
        AIM_LOG_INFO("Error: Size out of range offset:%d l:%d res:%d\r\n", offset, length, res);
        return -1;
    }

    for (i = 0; i < length; i++)
    {
        data[i] = cblock[offset + i];
    }
    #endif

    close(file);
    return res;
}

int i2c_read_byte(int i2cbus, int addr, int offset, char* data)
{
    int ret;

    ret = i2c_read(i2cbus, addr, offset, 1, data);

    if (ret < 0 && DEBUG_FLAG)
    {
        AIM_LOG_INFO("Error: Read failed %d\r\n",ret);
    }

    //AIM_LOG_INFO("i2c_read_byte: bus:%d add:0x%x offset:%d ret:%d data:0x%x\r\n", i2cbus, addr, offset, ret, *data);

    return ret;
}


int i2c_read_word(int i2cbus, int addr, int offset)
{
    int ret;

    ret = _i2c_read_word_data(i2cbus, addr, offset);

    if (ret < 0 && DEBUG_FLAG)
    {
        AIM_LOG_INFO("Error: Read failed %d\r\n",ret);
    }

    //AIM_LOG_INFO("i2c_read_word: bus:%d add:0x%x offset:%d ret:%d\r\n", i2cbus, addr, offset, ret);

    return ret;
}
int _i2c_read_block_data(int i2cbus, int addr, uint8_t offset, uint8_t *data, int data_length)
{
    int file;
    char filename[20];

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

    #if 0/*check funcs*/
    unsigned long funcs;
    if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
        AIM_LOG_INFO("Error: Could not get the adapter\r\n");
        return -1;
    }
    //I2C_SMBUS_BLOCK_DATA
    #endif

    /*set slave address set_slave_addr(file, address, force))*/

    if (ioctl(file, I2C_SLAVE_FORCE, addr) < 0)
    //if (ioctl(file, I2C_SLAVE, addr) < 0)
    {
        AIM_LOG_INFO("Error: Could not set address to 0x%02x, %s \r\n", addr, strerror(errno));
        close(file);
        return -errno;
    }

    int res = 0;
    res = i2c_smbus_read_i2c_block_data(file, offset, data_length, data);
    if (res < 0 && DEBUG_FLAG)
    {
        AIM_LOG_INFO("Error: i2c_smbus_read_word_data status:%d\r\n", res);
        //close(file);
        //return res;
    }

    close(file);
    return res;
}

int i2c_read_block(int i2cbus, int addr, uint8_t offset, uint8_t *data, int length)
{
    int ret;

    ret = _i2c_read_block_data(i2cbus, addr, offset, data, length);

    if (ret < 0 && DEBUG_FLAG)
    {
        AIM_LOG_INFO("Error: Read failed %d\r\n", ret);
    }
    //AIM_LOG_INFO("i2c_read_block: bus:%d add:0x%x offset:%d ret:%d data:0x%x\r\n", i2cbus, addr, offset, ret, *data);

    return ret;
}

int _i2c_read_i2c_block_data_dump(int i2cbus, int addr, uint8_t *data)
{
    int file;
    char filename[20];
    int res = 0;
    int i = 0;
    unsigned char cblock[288];

    /*open i2c device*/
    snprintf(filename, sizeof(filename), "/dev/i2c/%d", i2cbus);
    filename[sizeof(filename) - 1] = '\0';
    file = open(filename, O_RDWR);

    if (file < 0)
    {
        sprintf(filename, "/dev/i2c-%d", i2cbus);
        file = open(filename, O_RDWR);
    }
    if (file < 0)
    {
        AIM_LOG_INFO("Unable to open id:%d %s\r\n", i2cbus, filename);
        return -1;
    }

#if 0/*check funcs*/
    unsigned long funcs;
    if (ioctl(file, I2C_FUNCS, &funcs) < 0)
    {
        AIM_LOG_INFO("Error: Could not get the adapter\r\n");
        return -1;
    }
    //I2C_SMBUS_BLOCK_DATA
#endif

    /*set slave address set_slave_addr(file, address, force))*/

    if (ioctl(file, I2C_SLAVE_FORCE, addr) < 0)
    //if (ioctl(file, I2C_SLAVE, addr) < 0)
    {
        AIM_LOG_INFO("Error: Could not set address to 0x%02x, %s \r\n", addr, strerror(errno));
        close(file);
        return -errno;
    }

    for (res = 0; res < 256; res += i)
    {
        i = i2c_smbus_read_i2c_block_data(file, res, 32, cblock + res);
        if (i <= 0)
        {
            res = i;
            break;
        }
    }

    if (res < 0 && DEBUG_FLAG)
    {
        AIM_LOG_INFO("Error: i2c_smbus_read_word_data status:%d\r\n", res);
        close(file);
        return res;
    }

    if (res >= 256)
        res = 256;
    for (i = 0; i < res; i++) data[i] = cblock[i];

    close(file);
    return res;
}

int i2c_read_i2c_block_dump(int i2cbus, int addr, uint8_t *data)
{
    int ret;

    ret = _i2c_read_i2c_block_data_dump(i2cbus, addr, data);

    if (ret < 0 && DEBUG_FLAG)
    {
        AIM_LOG_INFO("Error: Read failed %d\r\n",ret);
    }
    //AIM_LOG_INFO("i2c_read_block: bus:%d add:0x%x offset:%d ret:%d data:0x%x\r\n", i2cbus, addr, offset, ret, *data);

    return ret;
}

int i2c_write_byte(int i2cbus, int addr, int offset, char val)
{
    int file;
    char filename[20];
    int res = 0;

    #if 0/*get current value*/
    char cur_val=0;
    res = i2c_read_byte(i2cbus, addr, offset, &cur_val);
    if (ret <0)
    {
        return res;
    }
    #endif

    /*open i2c device*/
    snprintf(filename, sizeof(filename), "/dev/i2c/%d", i2cbus);
    filename[sizeof(filename) - 1] = '\0';
    file = open(filename, O_RDWR);

    if (file < 0)
    {
        sprintf(filename, "/dev/i2c-%d", i2cbus);
        file = open(filename, O_RDWR);
    }
    if (file < 0)
    {
        AIM_LOG_INFO("Unable to open id:%d %s\r\n", i2cbus, filename);
        return -1;
    }

    /*set slave address set_slave_addr(file, address, force))*/

    //if (ioctl(file, I2C_SLAVE_FORCE, addr) < 0)
    if (ioctl(file, I2C_SLAVE, addr) < 0)
    {
        AIM_LOG_INFO("Error: Could not set address to 0x%02x, %s \r\n", addr, strerror(errno));
        close(file);
        return -errno;
    }

    res = i2c_smbus_write_byte_data(file, offset, val);
    if (res < 0)
    {
        AIM_LOG_INFO("Error: i2c_smbus_write_byte_data offset:%d val:0x%x r:%d\r\n", offset, val, res);
        close(file);
        return res;
    }
    close(file);
    return 0;
}

int i2c_write_bit(int i2cbus, int addr, int offset, int bit, char val)
{

    int res = 0;
    char cur_val=0;
    char new_val=0;
    if (val != 0 && val != 1)
    {
        AIM_LOG_INFO("Error: i2c_write_bit error val:%d\r\n", val);
        return -1;
    }

    if (bit < 0 && bit > 7)
    {
        AIM_LOG_INFO("Error: i2c_write_bit error bit:%d\r\n", bit);
        return -1;
    }

    res = i2c_read_byte(i2cbus, addr, offset, &cur_val);
    if (res <0)
    {
        return res;
    }

    if (val == 1)
    {
        new_val = cur_val | (1<<bit);
    }
    else
    {
        new_val = cur_val & ~(1<<bit);
    }

    //AIM_LOG_INFO("i2c_write_bit %d-0x%x-%d cur:0x%x new:0x%x\r\n", i2cbus, addr, offset, cur_val, new_val);

    if (new_val == cur_val)
    {
        return 0;
    }

    res = i2c_write_byte(i2cbus, addr, offset, new_val);
    if (res <0)
    {
        return res;
    }

    return 0;
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

int psu_serial_number_get(int id, char *serial, int serial_len)
{
    int   len = 0;
    int   ret  = ONLP_STATUS_OK;
    char *prefix = NULL;
    char data[256] = {0};

    if (serial == NULL || serial_len < PSU_SERIAL_NUMBER_LEN) 
    {
        return ONLP_STATUS_E_PARAM;
    }

    switch(id)
    {
        case PSU1_ID:
            prefix = PSU1_AC_EEPROM_PREFIX;
            break;
        case PSU2_ID:
            prefix = PSU2_AC_EEPROM_PREFIX;
            break;
        default:
            break;
    }

    /* Copy serial number from PSU EEPROM */
    ret = onlp_file_read((uint8_t* )data, sizeof(data)-1, &len,
                "%s%s", prefix, "eeprom");
    if (ret < 0) 
    {
        return ONLP_STATUS_E_INTERNAL;
    }

    aim_strlcpy(serial, &data[PSU_SERIAL_NUMBER_OFFSET], PSU_SERIAL_NUMBER_LEN + 1);

    return ONLP_STATUS_OK;
}

psu_type_t psu_type_get(int id, char* modelname, int modelname_len)
{
    char *node = NULL;
    char  model_name[PSU_MODEL_NAME_LEN + 1] = {0};

    /* Check AC model name , AC can't be read from eeprom or model name */
    switch(id)
    {
        case PSU1_ID:
            node = PSU1_AC_PMBUS_NODE(psu_mfr_model);
            break;
        case PSU2_ID:
            node = PSU2_AC_PMBUS_NODE(psu_mfr_model);
            break;
        default:
            break;
    }

    if (deviceNodeReadString(node, model_name, sizeof(model_name), 0) != 0) 
    {
        return PSU_TYPE_UNKNOWN;
    }

    if (modelname) 
    {
        aim_strlcpy(modelname, model_name, PSU_MODEL_NAME_LEN + 1);
    }

    /* Current PSU, can't read fan_dir*/
    
    return PSU_TYPE_AC_F2B;
}

int psu_yesm1300am_pmbus_info_get(int id, char *node, int *value)
{
    char *prefix = NULL;
    *value = 0;

    switch(id)
    {
        case PSU1_ID:
            prefix = PSU1_AC_PMBUS_PREFIX;
            break;
        case PSU2_ID:
            prefix = PSU2_AC_PMBUS_PREFIX;
            break;
        default:
            break;
    }
    if (onlp_file_read_int(value, "%s%s", prefix, node) < 0) 
    {
        AIM_LOG_ERROR("Unable to read status from file(%s%s)\r\n", prefix, node);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int psu_yesm1300am_pmbus_info_set(int id, char *node, int value)
{
    char *prefix = NULL;

    switch(id)
    {
        case PSU1_ID:
            prefix = PSU1_AC_PMBUS_PREFIX;
            break;
        case PSU2_ID:
            prefix = PSU2_AC_PMBUS_PREFIX;
            break;
        default:
            break;
    }
    if (onlp_file_write_int(value, "%s%s", prefix, node) < 0) 
    {
        AIM_LOG_ERROR("Unable to write data to file (%s%s)\r\n", prefix, node);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}
