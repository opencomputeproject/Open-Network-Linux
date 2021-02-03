/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2017 Accton Technology Corporation.
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
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <onlplib/file.h>
#include <onlp/onlp.h>
#include "platform_lib.h"

#define TTY_DEVICE                      "/dev/ttyACM0"
#define TTY_PROMPT                      "@bmc:"
#define TTY_I2C_TIMEOUT                 80000
#define TTY_BMC_LOGIN_TIMEOUT           1000000
#define TTY_RETRY                       20
#define MAXIMUM_TTY_BUFFER_LENGTH       1024
#define MAXIMUM_TTY_STRING_LENGTH       (MAXIMUM_TTY_BUFFER_LENGTH - 1)

static int  tty_fd = -1;
static char tty_buf[MAXIMUM_TTY_BUFFER_LENGTH] = {0};

static int tty_open(void)
{
    int i = 20;
    struct termios attr;

    if (tty_fd > -1) {
        return 0;
    }

    do {
        if ((tty_fd = open(TTY_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY)) > -1) {
            tcgetattr(tty_fd, &attr);
            attr.c_cflag = B57600 | CS8 | CLOCAL | CREAD;
            attr.c_iflag = IGNPAR;
            attr.c_oflag = 0;
            attr.c_lflag = 0;
            attr.c_cc[VMIN] = (unsigned char)
                ((MAXIMUM_TTY_STRING_LENGTH > 0xFF) ?  0xFF : MAXIMUM_TTY_STRING_LENGTH);
            attr.c_cc[VTIME] = 0;
            cfsetospeed(&attr, B57600);
            cfsetispeed(&attr, B57600);
            tcsetattr(tty_fd, TCSANOW, &attr);
            return 0;
        }

        i--;
        usleep(100000);
    } while (i > 0);

    return -1;
}

static int tty_close(void)
{
    close(tty_fd);
    tty_fd = -1;
    return 0;
}

static int tty_exec_buf(unsigned long udelay, const char *str)
{
    if (tty_fd < 0)
        return -1;

    write(tty_fd, tty_buf, strlen(tty_buf)+1);
    usleep(udelay);
    memset(tty_buf, 0, MAXIMUM_TTY_BUFFER_LENGTH);
    read(tty_fd, tty_buf, MAXIMUM_TTY_BUFFER_LENGTH);
    return (strstr(tty_buf, str) != NULL) ? 0 : -1;
}

/* Clear Rx buffer by reading it out */
static int tty_clear_rxbuf(void) {
    if (tty_fd < 0)
        return ONLP_STATUS_E_GENERIC;

    read(tty_fd, tty_buf, MAXIMUM_TTY_BUFFER_LENGTH);
    memset(tty_buf, 0, MAXIMUM_TTY_BUFFER_LENGTH);
    return ONLP_STATUS_OK;
}

static int tty_login(void)
{
    int i = 10;

    for (i = 1; i <= TTY_RETRY; i++) {
        snprintf(tty_buf, MAXIMUM_TTY_BUFFER_LENGTH, "\r\r");
        if (!tty_exec_buf(TTY_BMC_LOGIN_TIMEOUT, TTY_PROMPT)) {
            return 0;
        }

        if (strstr(tty_buf, "bmc login:") != NULL)
        {
            snprintf(tty_buf, MAXIMUM_TTY_BUFFER_LENGTH, "root\r");
            
            if (!tty_exec_buf(TTY_BMC_LOGIN_TIMEOUT, "Password:")) {
                snprintf(tty_buf, MAXIMUM_TTY_BUFFER_LENGTH, "0penBmc\r");
                if (!tty_exec_buf(TTY_BMC_LOGIN_TIMEOUT, TTY_PROMPT)) {
                    return 0;
                }

            }
        }
        usleep(50000);
    }

    return -1;
}

int bmc_tty_init(void)
{
    int i;
    if (tty_fd >= 0){
        return 0;
    }

    for (i = 1; i <= TTY_RETRY; i++) {
        if (tty_open() != 0) {
            AIM_LOG_ERROR("ERROR: Cannot open TTY device\n");
            continue;
        }
        if (tty_login() != 0) {
            AIM_LOG_ERROR("ERROR: Cannot login TTY device\n");
            tty_close();
            continue;
        }

        return 0;
    }

    AIM_LOG_ERROR("Unable to init bmc tty\r\n");
    return -1;
}

int bmc_tty_deinit(void)
{
        if( tty_fd != -1 ){
                tty_close();
        }
        else{
                AIM_LOG_ERROR("ERROR: TTY not open\n");
        }
        return 0;
}

int bmc_send_command(char *cmd)
{
    int i, ret = 0;
    
    for (i = 1; i <= TTY_RETRY; i++) {
        snprintf(tty_buf, MAXIMUM_TTY_BUFFER_LENGTH, "%s", cmd);
        ret = tty_exec_buf(TTY_I2C_TIMEOUT, TTY_PROMPT);
        if (ret != 0) {
//            AIM_LOG_ERROR("ERROR: bmc_send_command(%s) timed out\n", cmd);
            continue;
        }

        return 0;
    }

    AIM_LOG_ERROR("Unable to send command to bmc(%s)\r\n", cmd);
    return -1;
}

int bmc_file_read_str(char *file, char *result, int slen)
{
    char *curr = NULL;
    char cmd[88] = {0};
    char *delimit="^@\r\n";
    int flag = 0, ret = 0;

    ret = snprintf(cmd, sizeof(cmd), "cat %s\r\n", file);
    if( ret >= sizeof(cmd) ){
        AIM_LOG_ERROR("cmd size overwrite (%d,%d)\r\n", ret, sizeof(cmd));
        return ONLP_STATUS_E_INTERNAL;
    }
    bmc_send_command(cmd);

    strtok(tty_buf, delimit);
    flag = 0;
    while( (curr = strtok(NULL, delimit)) != NULL ){
        switch (*curr) {
        case '\n':
        case '\r':
        case '@':
        case '^':
            break;
        default:
            flag = 1;
            break;
        }
        if( flag ){
            break;
        }
    }

    ret = snprintf(result, slen-1, "%s", curr);
    if( ret >= (slen-1) ){
        AIM_LOG_ERROR("result size overwrite (%d,%d)\r\n", ret, slen-1);
        return ONLP_STATUS_E_INTERNAL;
    }
    return 0;
}

int chk_numeric_char(char *data, int base)
{
    int len, i, orig = 0;
    if( *data == '\0' ){
        return 0;
    }
    len = (int) strlen(data);
    if( base == 10 ){
        for( i=0; i<len; i++ ){
            if( i && ( *(data+i) == 0xd ) ){
                break;
            }
            if( i && ( *(data+i) == 0xa ) ){
                break;
            }
            if( *(data+i) < '0' ){
                return 0;
            }
            if( *(data+i) > '9' ){
                return 0;
            }
        }
        return 1;
    }
    else if( base == 16 ){
        if( !memcmp(data, "0x", 2) ){
            if( len <= 2 ){
                return 0;
            }
            orig = 2;
        }
        else if( !memcmp(data, "0X", 2) ){
            if( len <= 2 ){
                return 0;
            }
            orig = 2;
        }
        for( i=orig; i<len; i++ ){
            if( (i > orig) && ( *(data+i) == 0xd ) ){
                break;
            }
            if( (i > orig) && ( *(data+i) == 0xa ) ){
                break;
            }
            if( !( ( (*(data+i) >= '0') && (*(data+i) <= '9') ) ||
                   ( (*(data+i) >= 'A') && (*(data+i) <= 'F') ) ||
                   ( (*(data+i) >= 'a') && (*(data+i) <= 'f') )
                 )
              ){
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

int
bmc_command_read_int(int* value, char *cmd, int base)
{
    int len;
    int i, j, ret;
    char *prev_str = NULL;
    char *current_str= NULL;

    ret = -1;
    for (j = 1; j <= TTY_RETRY; j++) {
        tty_clear_rxbuf();
        if (bmc_send_command(cmd) < 0) {
            return ONLP_STATUS_E_INTERNAL;
        }

        len = (int)strlen(cmd);
        prev_str = strstr(tty_buf, cmd);
        if (prev_str == NULL) {
            ret = -1;
            continue;
        }

        for (i = 1; i <= TTY_RETRY; i++) {
            current_str = strstr(prev_str + len, cmd);
            if(current_str == NULL) {
                if( !chk_numeric_char(prev_str + len, base) ) {
                    ret = -1;
                    continue;
                }
                *value = strtoul(prev_str + len, NULL, base);
                ret = 0;
                goto exit;
            } else {
                prev_str = current_str;
                ret = -1;
                continue;
            }
        }
    }

exit:
    return ret;
}


int
bmc_file_read_int(int* value, char *file, int base)
{
    char cmd[64] = {0};
    snprintf(cmd, sizeof(cmd), "cat %s\r\n", file);
    return bmc_command_read_int(value, cmd, base);
}

int
bmc_i2c_readb(uint8_t bus, uint8_t devaddr, int16_t addr)
{
    int ret = 0, value;
    char cmd[64] = {0};
    if (addr < 0) {
        snprintf(cmd, sizeof(cmd), "i2cget -f -y %d 0x%x\r\n", bus, devaddr);
    } else {
        snprintf(cmd, sizeof(cmd), "i2cget -f -y %d 0x%x 0x%02x\r\n", bus,
                 devaddr, (uint8_t)addr);
    }
    ret = bmc_command_read_int(&value, cmd, 16);
    return (ret < 0) ? ret : value;
}

int
bmc_i2c_writeb(uint8_t bus, uint8_t devaddr, uint8_t addr, uint8_t value)
{
    char cmd[64] = {0};
    snprintf(cmd, sizeof(cmd), "i2cset -f -y %d 0x%x 0x%02x 0x%x\r\n", bus, devaddr, addr, value);
    return bmc_send_command(cmd);
}

int
bmc_i2c_write_quick_mode(uint8_t bus, uint8_t devaddr, uint8_t value)
{
    char cmd[64] = {0};
    snprintf(cmd, sizeof(cmd), "i2cset -f -y %d 0x%x 0x%x\r\n", bus, devaddr, value);
    return bmc_send_command(cmd);
}

int
bmc_i2c_readw(uint8_t bus, uint8_t devaddr, uint8_t addr)
{
    int ret = 0, value;
    char cmd[64] = {0};

    snprintf(cmd, sizeof(cmd), "i2cget -f -y %d 0x%x 0x%02x w\r\n", bus, devaddr, addr);
    ret = bmc_command_read_int(&value, cmd, 16);
    return (ret < 0) ? ret : value;
}

int
bmc_i2c_readraw(uint8_t bus, uint8_t devaddr, uint8_t addr, char* data, int data_size)
{
    int data_len, i = 0;
    char cmd[64] = {0};
    char *str = NULL;
    snprintf(cmd, sizeof(cmd), "i2craw -w 0x%x -r 0 %d 0x%02x\r\n", addr, bus, devaddr);

    if (bmc_send_command(cmd) < 0) {
        AIM_LOG_ERROR("Unable to send command to bmc(%s)\r\n", cmd);
        return ONLP_STATUS_E_INTERNAL;
    }

    str = strstr(tty_buf, "Received:\r\n  ");
    if (str == NULL) {
        return -1;
    }

    /* first byte is data length */
    str += strlen("Received:\r\n  ");;
    data_len = strtoul(str, NULL, 16);
    if (data_size < data_len) {
        data_len = data_size;
    }

    for (i = 0; (i < data_len) && (str != NULL); i++) {
        str = strstr(str, " ") + 1; /* Jump to next token */
        data[i] = strtoul(str, NULL, 16);
    }

    data[i] = 0;
    return 0;    
}

