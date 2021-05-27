/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2018 Alpha Networks Incorporation.
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
#include <onlp/platformi/psui.h>
#include <onlplib/mmap.h>
#include <stdio.h>
#include <string.h>
#include "platform_lib.h"

#define PSUI_DEBUG_FLAG         0


#define PSU_STATUS_INTERRUPT    3
#define PSU_STATUS_PRESENT      2
#define PSU_STATUS_POWER_OK     1
#define PSU_STATUS_POWER_ON     0

#define PSU_NODE_MAX_INT_LEN    8
#define PSU_NODE_MAX_PATH_LEN   64
#define PSUI_CPLD_BUS_ID        0
#define PSUI_POWER_CPLD_ADDR    0x5F
#define RPS_PSU_EEPROM_ADDR     0x50
#define RPS_PSU_MICRO_P_ADDR    0x58

#define PSUI_MANUFACTURER_NAME_REG  0x0C
#define PSUI_PRODUCT_NAME_REG_PART1       0x20
#define PSUI_PRODUCT_NAME_REG_PART2       0x2A

#define PSUI_PRODUCT_SER_NO_REG         0x35
#define PSUI_RPS_STATUS_REG0        0x03 /* PSU Status Register for PSU#1 */ //from DCGS_TYPE1_ Power_CPLD_Spec_v02_20190611.docx
#define PSUI_RPS_STATUS_REG1        0x04 /* PSU Status Register for PSU#2 */

#define PSUI_PRODUCT_SER_NO_SIZE    19
#define PSUI_PRODUCT_SER_NO_LEN     (PSUI_PRODUCT_SER_NO_SIZE + 1)
#define PSUI_PRODUCT_NAME_SIZE_PART1     8
#define PSUI_PRODUCT_NAME_SIZE_PART2     10

struct psui_reg_data_word
{
    uint8_t reg;
    uint16_t value;
};

struct psui_reg_data_word regs_word[] = { { 0x88, 0 },  /* READ_Vin */
    { 0x8b, 0 },  /* READ_Vout */
    { 0x89, 0 },  /* READ_Iin */
    { 0x8c, 0 },  /* READ_Iout */
    { 0x96, 0 },  /* READ_Pout */
    { 0x97, 0 }   /* READ_Pin */
};


#define GET_RPS_STATUS_BIT(x,n) (((x) >> (n)) & 1)
#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_PSU(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)


int
psu_stx60d0_linear_format(int exponent, int mantissa)
{
    int multiplier = 1000; /* Units are "milli-" */

    return (exponent >= 0) ? (mantissa << exponent) * multiplier :
                             (mantissa * multiplier) / (1 << -exponent);
}

int
onlp_psui_init(void)
{
    DIAG_PRINT("%s", __FUNCTION__);
    return ONLP_STATUS_OK;
}


static int
psu_info_get_product_name(int id, UI8_T *data)
{
    DIAG_PRINT("%s, id:%d", __FUNCTION__, id);
    int ret = 0;

    UI8_T pn_part1[PSUI_PRODUCT_NAME_SIZE_PART1+ 1] = { 0 };
    UI8_T pn_part2[PSUI_PRODUCT_NAME_SIZE_PART2+ 1] = { 0 };

    ret = i2c_read_block(id, RPS_PSU_EEPROM_ADDR, PSUI_PRODUCT_NAME_REG_PART1, pn_part1, PSUI_PRODUCT_NAME_SIZE_PART1);
    if (ret < 0 && PSUI_DEBUG_FLAG)
        printf("I2C command 0x%X Read Fail, id=%d\n", PSUI_PRODUCT_NAME_REG_PART1, id);

    ret = i2c_read_block(id, RPS_PSU_EEPROM_ADDR, PSUI_PRODUCT_NAME_REG_PART2, pn_part2, PSUI_PRODUCT_NAME_SIZE_PART2);
    if (ret < 0 && PSUI_DEBUG_FLAG)
        printf("I2C command 0x%X Read Fail, id=%d\n", PSUI_PRODUCT_NAME_REG_PART2, id);

     strcat((char*)data, (char*)pn_part1); 
     strcat((char*)data, (char*)pn_part2); 

    //printf("[%s(%d)] data %s\n",  __func__, __LINE__, data);

    return ret;
}

static int
psu_info_get_product_ser(psu_type_t psu_type, int id, UI8_T *data)
{
    int ret = 0;
    int addr = 0;
    DIAG_PRINT("%s, id:%d", __FUNCTION__, id);

    if (psu_type == PSU_TYPE_AC_F2B)
    {
        addr = PSUI_PRODUCT_SER_NO_REG;
    }
    else
    {
        addr = PSUI_PRODUCT_SER_NO_REG;
    }

    ret = i2c_read_block(id, RPS_PSU_EEPROM_ADDR, addr, data, PSUI_PRODUCT_SER_NO_SIZE);
    if (ret < 0)
        printf("I2C command 0x%X Read Fail, id=%d\n", addr, id);

    //printf("[%s(%d)]RPS_PSU_EEPROM_ADDR, offset 0x%X, product_ser:%s\n", __func__, __LINE__, addr, data);
  
    return ret;
}

static int
psu_info_get_status(int id, char *data)
{
    DIAG_PRINT("%s, id:%d", __FUNCTION__, id);
    int ret = 0;

    if (id == PSU1_ID)
    {
        ret = bmc_i2c_read_byte(BMC_CPLD_I2C_BUS_ID, BMC_CPLD_I2C_ADDR, PSUI_RPS_STATUS_REG0, data);
        if (ret < 0)
            printf("I2C command 0x%X Read Fail, BMC_CPLD_I2C_BUS_ID=%d\n", PSUI_RPS_STATUS_REG0, BMC_CPLD_I2C_BUS_ID);
    }
    else if (id == PSU2_ID)
    {
        ret = bmc_i2c_read_byte(BMC_CPLD_I2C_BUS_ID, BMC_CPLD_I2C_ADDR, PSUI_RPS_STATUS_REG1, data);
        if (ret < 0)
            printf("I2C command 0x%X Read Fail, BMC_CPLD_I2C_BUS_ID=%d\n", PSUI_RPS_STATUS_REG1, BMC_CPLD_I2C_BUS_ID);
    }

    return ret;

}

static int
psu_stx60d0_info_get(int id, onlp_psu_info_t *info)
{
    DIAG_PRINT("%s, id:%d", __FUNCTION__, id);
    //int val   = 0;
    //int index = ONLP_OID_ID_GET(info->hdr.id);

    /* Set capability
     */
    info->caps = ONLP_PSU_CAPS_AC;

    if (info->status & ONLP_PSU_STATUS_FAILED)
    {
        return ONLP_STATUS_OK;
    }

    /* Set the associated oid_table */
    //info->hdr.coids[0] = ONLP_FAN_ID_CREATE(index + CHASSIS_FAN_COUNT);
    //info->hdr.coids[1] = ONLP_THERMAL_ID_CREATE(index + CHASSIS_THERMAL_COUNT);

#if 1
    int i = 0;
    int status;
    int exponent, mantissa;
    for (i = 0; i < 6; i++)
    {
        //printf("I2C register 0x%X \n", regs_word[i].reg);
        status = i2c_read_word(id, RPS_PSU_MICRO_P_ADDR, regs_word[i].reg);
        if (status < 0)
            printf("I2C command 0x%X Read Fail\n", regs_word[i].reg);
        else
        {
            //printf("I2C command 0x%X Read Success: 0x%02x\n", regs_word[i].reg, status);
            regs_word[i].value = status;
        }
    }

    info->caps |= ONLP_PSU_CAPS_VOUT;
    info->caps |= ONLP_PSU_CAPS_VIN;
    info->caps |= ONLP_PSU_CAPS_IOUT;
    info->caps |= ONLP_PSU_CAPS_IIN;
    info->caps |= ONLP_PSU_CAPS_POUT;
    info->caps |= ONLP_PSU_CAPS_PIN;

    /* Linear_5s_11s */
    exponent = psu_two_complement_to_int(regs_word[0].value >> 11, 5, 0x1f);
    mantissa = psu_two_complement_to_int(regs_word[0].value & 0x7ff, 11, 0x7ff);
    info->mvin = psu_stx60d0_linear_format(exponent, mantissa);

    /* Linear_16u */
    exponent = -12;
    mantissa = regs_word[1].value;
    info->mvout = psu_stx60d0_linear_format(exponent, mantissa);

    exponent = psu_two_complement_to_int(regs_word[2].value >> 11, 5, 0x1f);
    mantissa = psu_two_complement_to_int(regs_word[2].value & 0x7ff, 11, 0x7ff);
    info->miin = psu_stx60d0_linear_format(exponent, mantissa);

    exponent = psu_two_complement_to_int(regs_word[3].value >> 11, 5, 0x1f);
    mantissa = psu_two_complement_to_int(regs_word[3].value & 0x7ff, 11, 0x7ff);
    info->miout = psu_stx60d0_linear_format(exponent, mantissa);

    exponent = psu_two_complement_to_int(regs_word[4].value >> 11, 5, 0x1f);
    mantissa = psu_two_complement_to_int(regs_word[4].value & 0x7ff, 11, 0x7ff);
    info->mpout = psu_stx60d0_linear_format(exponent, mantissa);

    exponent = psu_two_complement_to_int(regs_word[5].value >> 11, 5, 0x1f);
    mantissa = psu_two_complement_to_int(regs_word[5].value & 0x7ff, 11, 0x7ff);
    info->mpin = psu_stx60d0_linear_format(exponent, mantissa);

#endif
    return ONLP_STATUS_OK;
}

/*
 * Get all information about the given PSU oid.
 */
static onlp_psu_info_t pinfo[] =
{
    { }, /* Not used */
    {    /* PSU-1 is on i2c channel 5 for stx60d0*/
        { ONLP_PSU_ID_CREATE(PSU1_ID), "PSU-1", 0 },
    },
    {    /* PSU-2 is on i2c channel 6 for stx60d0*/
        { ONLP_PSU_ID_CREATE(PSU2_ID), "PSU-2", 0 },
    }
};

psu_type_t get_psu_type(int id, char *product_name, int productname_len)
{
    DIAG_PRINT("%s, id:%d", __FUNCTION__, id);
    UI8_T p_name[PSUI_PRODUCT_NAME_SIZE_PART1+PSUI_PRODUCT_NAME_SIZE_PART2+ 1] = { 0 };

    /* Check AC model name */
    if (psu_info_get_product_name(id, p_name) >= 0)
    {
        //printf("[psu_info_get_product_name] %s\n", p_name);
        if (product_name)
            memcpy(product_name, p_name, sizeof(p_name));

        return PSU_TYPE_AC_B2F;
    }
    return PSU_TYPE_UNKNOWN;
}


int
onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t *info)
{
    char name[ONLP_CONFIG_INFO_STR_MAX];
    char status;
    int ret = ONLP_STATUS_OK;
    int index = ONLP_OID_ID_GET(id);
    psu_type_t psu_type;

    UI8_T product_ser[PSUI_PRODUCT_SER_NO_LEN];

    UI8_T rps_status = 0, power_ok = 0, power_on = 0, power_present = 0;
    int psu_busid=0; 

    VALIDATE(id);

    memset(info, 0, sizeof(onlp_psu_info_t));
    memset(name, 0, sizeof(name));
    *info = pinfo[index]; /* Set the onlp_oid_hdr_t */

    /* Should Get psu status First, 
      * if power no good, do not get psu info,
      * if power not present, do not get any information.      
      */
      
    /* Get psu status from CPLD */
    if (psu_info_get_status(index, &status) < 0)
    {
        printf("Unable to read PSU(%d) item(psu status)\r\n", index);
    }
    else
    {
        rps_status = (UI8_T)status;

        power_present = GET_RPS_STATUS_BIT(rps_status, PSU_STATUS_PRESENT);
        power_ok = GET_RPS_STATUS_BIT(rps_status, PSU_STATUS_POWER_OK);
        power_on = GET_RPS_STATUS_BIT(rps_status, PSU_STATUS_POWER_ON);

        /* Empty */
        if (power_present)
        {
            info->status |= ONLP_PSU_STATUS_PRESENT;
        }

        if (!power_ok)
        {
            info->status |= ONLP_PSU_STATUS_FAILED;
        }

        if(!power_on)
        {
            info->status |= ONLP_PSU_STATUS_UNPLUGGED;
        }

        DIAG_PRINT("rps_status:0x%x ,info->status:0x%x\n", rps_status, info->status);
        DIAG_PRINT("present:%d, ok:%d, on:%d\n", power_present, power_ok, power_on);

    }

    if (!(info->status & ONLP_PSU_STATUS_PRESENT))
    {
        /* Just dispaly "Not present." when empty by Psu.c (packages\base\any\onlp\src\onlp\module\src) onlp_psu_dump()*/
        return ONLP_STATUS_OK;
    }

    psu_busid = index_to_psu_busid(index);

    /* Get PSU type and product name, bus ID=index+10*/
    psu_type = get_psu_type(psu_busid, info->model, sizeof(info->model));
   
    //debug
    DIAG_PRINT("%s, id:%d, index:%d, psu_type:%d\n", __FUNCTION__, id, index, psu_type);

    ret = psu_stx60d0_info_get(psu_busid, info); /* Get PSU electric info from PMBus */

    /* Get the product serial number, bus ID=index+10 */
    if (psu_info_get_product_ser(psu_type, psu_busid, product_ser) < 0)
    {
        printf("Unable to read PSU(%d) item(serial number)\r\n", index);
    }
    else
    {
        memcpy(info->serial, product_ser, sizeof(product_ser));
    }

    return ret;
}

int
onlp_psui_ioctl(onlp_oid_t pid, va_list vargs)
{
    DIAG_PRINT("%s, pid=%d", __FUNCTION__, pid);
    return ONLP_STATUS_E_UNSUPPORTED;
}

