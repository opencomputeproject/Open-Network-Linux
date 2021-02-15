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
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <onlp/onlp.h>
#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include <AIM/aim.h>

#include "platform_lib.h"

int 
psu_thermal_get(onlp_thermal_info_t* info, int thermal_id)
{
    int pw_exist, pw_good, exist_offset, good_offset;
    int offset, i2c_bus, rc;
    int value, buf, psu_mask;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];    

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if (thermal_id == THERMAL_ID_PSU1_1) {
        i2c_bus = I2C_BUS_PSU1;
        offset = PSU_THERMAL1_OFFSET;
        exist_offset = PSU1_PRESENT_OFFSET;
        good_offset = PSU1_PWGOOD_OFFSET;
    } else if (thermal_id == THERMAL_ID_PSU1_2) {
        i2c_bus = I2C_BUS_PSU1;
        offset = PSU_THERMAL2_OFFSET;
        exist_offset = PSU1_PRESENT_OFFSET;
        good_offset = PSU1_PWGOOD_OFFSET;
    } else if (thermal_id == THERMAL_ID_PSU2_1) {
        i2c_bus = I2C_BUS_PSU2;
        offset = PSU_THERMAL1_OFFSET;
        exist_offset = PSU2_PRESENT_OFFSET;
        good_offset = PSU2_PWGOOD_OFFSET;
    } else if (thermal_id == THERMAL_ID_PSU2_2) {
        i2c_bus = I2C_BUS_PSU2;
        offset = PSU_THERMAL2_OFFSET;
        exist_offset = PSU2_PRESENT_OFFSET;
        good_offset = PSU2_PWGOOD_OFFSET;
    }
    
    psu_mask = PSU_MUX_MASK;

    /* check psu status */
    if ((rc = psu_present_get(&pw_exist, exist_offset, I2C_BUS_0, psu_mask)) 
            != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (pw_exist != PSU_STATUS_PRESENT) {
        info->mcelsius = 0;
        info->status &= ~ONLP_THERMAL_STATUS_PRESENT;
        return ONLP_STATUS_OK;
    } else {
        info->status |= ONLP_THERMAL_STATUS_PRESENT;
    } 
    
    if ((rc = psu_pwgood_get(&pw_good, good_offset, I2C_BUS_0, psu_mask)) 
            != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }   
    
    if (pw_good != PSU_STATUS_POWER_GOOD) {        
        info->mcelsius = 0;
        return ONLP_STATUS_OK;
    }

    value = onlp_i2c_readw(i2c_bus, PSU_REG, offset, ONLP_I2C_F_FORCE);

    y_value = (value & 0x07FF);
    if ((value & 0x8000)&&(y_value)) {
        n_value = 0xF0 + (((value) >> 11) & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if (temp)
            snprintf(result, sizeof(result), "%d.%04d", y_value/temp, ((y_value%temp)*10000)/temp);
    } else {
        n_value = (((value) >> 11) & 0x0F);
        snprintf(result, sizeof(result), "%d", (y_value*(1<<n_value)));
    }
    
    buf = atof((const char *)result);
    info->mcelsius = (int)(buf * 1000);
    
    return ONLP_STATUS_OK;
}

int 
psu_fan_info_get(onlp_fan_info_t* info, int id)
{
    int pw_exist, pw_good, exist_offset, good_offset;
    int i2c_bus, psu_mask, rc;
    unsigned int tmp_fan_rpm, fan_rpm;

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
 
    if (id == FAN_ID_PSU_FAN1) {
        i2c_bus = I2C_BUS_PSU1;
        exist_offset = PSU1_PRESENT_OFFSET;
        good_offset = PSU1_PWGOOD_OFFSET;
    } else if (id == FAN_ID_PSU_FAN2) {
        i2c_bus = I2C_BUS_PSU2;
        exist_offset = PSU2_PRESENT_OFFSET;
        good_offset = PSU2_PWGOOD_OFFSET;
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }

    psu_mask = PSU_MUX_MASK;
    
    /* check psu status */
    if ((rc = psu_present_get(&pw_exist, exist_offset, I2C_BUS_0, psu_mask)) 
            != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (pw_exist != PSU_STATUS_PRESENT) {
        info->rpm = 0;
        info->status &= ~ONLP_FAN_STATUS_PRESENT;
        return ONLP_STATUS_OK;
    } else {
        info->status |= ONLP_FAN_STATUS_PRESENT;
    } 
    
    if ((rc = psu_pwgood_get(&pw_good, good_offset, I2C_BUS_0, psu_mask))
            != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    if (pw_good != PSU_STATUS_POWER_GOOD) {        
        info->rpm = 0;
        return ONLP_STATUS_OK;
    }
        
    tmp_fan_rpm = onlp_i2c_readw(i2c_bus, PSU_REG, PSU_FAN_RPM_OFFSET, ONLP_I2C_F_FORCE);

    fan_rpm = (unsigned int)tmp_fan_rpm;
    fan_rpm = (fan_rpm & 0x07FF) * (1 << ((fan_rpm >> 11) & 0x1F));
    info->rpm = (int)fan_rpm;
    
    return ONLP_STATUS_OK;
}

int 
psu_vout_get(onlp_psu_info_t* info, int i2c_bus)
{
    int v_value = 0;
    int n_value = 0;
    unsigned int temp = 0;
    char result[32];
    double dvalue;
    memset(result, 0, sizeof(result));

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    n_value = onlp_i2c_readb(i2c_bus, PSU_REG, PSU_VOUT_OFFSET1, ONLP_I2C_F_FORCE);
    if (n_value < 0) {
        return ONLP_STATUS_E_INTERNAL; 
    }
    
    v_value = onlp_i2c_readw(i2c_bus, PSU_REG, PSU_VOUT_OFFSET2, ONLP_I2C_F_FORCE);
    if (v_value < 0) {
        return ONLP_STATUS_E_INTERNAL; 
    }
    
    if (n_value & 0x10) {
        n_value = 0xF0 + (n_value & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if (temp)
            snprintf(result, sizeof(result), "%d.%04d", v_value/temp, ((v_value%temp)*10000)/temp);
    } else {
        snprintf(result, sizeof(result), "%d", (v_value*(1<<n_value)));
    }
    
    dvalue = atof((const char *)result);
    if (dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_VOUT;
        info->mvout = (int)(dvalue * 1000);
    }
    
    return ONLP_STATUS_OK;
}

int 
psu_iout_get(onlp_psu_info_t* info, int i2c_bus)
{
    int value;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];
    memset(result, 0, sizeof(result));
    double dvalue;   

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    value = onlp_i2c_readw(i2c_bus, PSU_REG, PSU_IOUT_OFFSET, ONLP_I2C_F_FORCE);
    if (value < 0) {
        return ONLP_STATUS_E_INTERNAL; 
    }
    
    y_value = (value & 0x07FF);
    if ((value & 0x8000)&&(y_value))
    {
        n_value = 0xF0 + (((value) >> 11) & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if (temp) {           
            snprintf(result, sizeof(result), "%d.%04d", y_value/temp, ((y_value%temp)*10000)/temp);
        }
    } else {
        n_value = (((value) >> 11) & 0x0F);
        snprintf(result, sizeof(result), "%d", (y_value*(1<<n_value)));
    }
    
    dvalue = atof((const char *)result);
    if (dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_IOUT;
        info->miout = (int)(dvalue * 1000);
    }
    
    return ONLP_STATUS_OK;
}

int 
psu_pout_get(onlp_psu_info_t* info, int i2c_bus)
{
    int value;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];
    memset(result, 0, sizeof(result));
    double dvalue;

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    value = onlp_i2c_readw(i2c_bus, PSU_REG, PSU_POUT_OFFSET, ONLP_I2C_F_FORCE);
    if (value < 0) {
        return ONLP_STATUS_E_INTERNAL; 
    }
    
    y_value = (value & 0x07FF);
    if ((value & 0x8000)&&(y_value))
    {
        n_value = 0xF0 + (((value) >> 11) & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if (temp) {           
            snprintf(result, sizeof(result), "%d.%04d", y_value/temp, ((y_value%temp)*10000)/temp);
        }
    } else {
        n_value = (((value) >> 11) & 0x0F);
        snprintf(result, sizeof(result), "%d", (y_value*(1<<n_value)));
    }
    
    dvalue = atof((const char *)result);
    if (dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_POUT;
        info->mpout = (int)(dvalue * 1000);
    }
    
    return ONLP_STATUS_OK;
}

int 
psu_pin_get(onlp_psu_info_t* info, int i2c_bus)
{
    int value;
    unsigned int y_value = 0;
    unsigned char n_value = 0;
    unsigned int temp = 0;
    char result[32];
    memset(result, 0, sizeof(result));
    double dvalue;

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
        
    value = onlp_i2c_readw(i2c_bus, PSU_REG, PSU_PIN_OFFSET, ONLP_I2C_F_FORCE);
    if (value < 0) {
        return ONLP_STATUS_E_INTERNAL; 
    }

    y_value = (value & 0x07FF);
    if ((value & 0x8000)&&(y_value))
    {
        n_value = 0xF0 + (((value) >> 11) & 0x0F);
        n_value = (~n_value) +1;
        temp = (unsigned int)(1<<n_value);
        if (temp) {           
            snprintf(result, sizeof(result), "%d.%04d", y_value/temp, ((y_value%temp)*10000)/temp);
        }
    } else {
        n_value = (((value) >> 11) & 0x0F);
        snprintf(result, sizeof(result), "%d", (y_value*(1<<n_value)));
    }
    
    dvalue = atof((const char *)result);
    if (dvalue > 0.0) {
        info->caps |= ONLP_PSU_CAPS_PIN;
        info->mpin = (int)(dvalue * 1000);
    }
    
    return ONLP_STATUS_OK;
}

int 
psu_eeprom_get(onlp_psu_info_t* info, int id)
{
    uint8_t data[256];
    char eeprom_path[128];
    int data_len, i, rc;
    memset(data, 0, sizeof(data));
    memset(eeprom_path, 0, sizeof(eeprom_path));

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    if (id == PSU_ID_PSU1) {
        rc = onlp_file_read(data, sizeof(data), &data_len, PSU1_EEPROM_PATH);
    } else {
        rc = onlp_file_read(data, sizeof(data), &data_len, PSU2_EEPROM_PATH);
    }
    
    if (rc == ONLP_STATUS_OK)
    {
        i = 11;

        /* Manufacturer Name */
        data_len = (data[i]&0x0f);
        i++;
        i += data_len;

        /* Product Name */
        data_len = (data[i]&0x0f);
        i++;
        memcpy(info->model, (char *) &(data[i]), data_len);
        i += data_len;

        /* Product part,model number */
        data_len = (data[i]&0x0f);
        i++;
        i += data_len;

        /* Product Version */
        data_len = (data[i]&0x0f);
        i++;
        i += data_len;

        /* Product Serial Number */
        data_len = (data[i]&0x0f);
        i++;
        memcpy(info->serial, (char *) &(data[i]), data_len);
    } else {
        strcpy(info->model, "Missing");
        strcpy(info->serial, "Missing");
    }
    
    return ONLP_STATUS_OK;
}


int
psu_present_get(int *pw_exist, int exist_offset, int i2c_bus, int psu_mask)
{
    int psu_pres;     

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    psu_pres = onlp_i2c_readb(i2c_bus, PSU_STATE_REG, 0x0, 
                              ONLP_I2C_F_FORCE);
    if (psu_pres < 0) {
        return ONLP_STATUS_E_INTERNAL; 
    }

    *pw_exist = (((psu_pres >> exist_offset) & psu_mask) ? 0 : 1);
    return ONLP_STATUS_OK;
}

int
psu_pwgood_get(int *pw_good, int good_offset, int i2c_bus, int psu_mask)
{
    int psu_pwgood;      

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    
    psu_pwgood = onlp_i2c_readb(i2c_bus, PSU_STATE_REG, 0x0,
                                ONLP_I2C_F_FORCE);
    if (psu_pwgood < 0) {
        return ONLP_STATUS_E_INTERNAL; 
    }
    
    *pw_good = (((psu_pwgood >> good_offset) & psu_mask) ? 1 : 0);
    return ONLP_STATUS_OK;
}

int
qsfp_present_get(int port, int *pres_val)
{     
    int status, rc, gpio_num;

    if (port >= 1 && port <= 16) {
        gpio_num = 496 + ((port - 1) ^ 1);
    } else if (port >= 17 && port <= 32) {
        gpio_num = 464 + ((port - 1) ^ 1);
    } else if (port == 33) {
        gpio_num = 432;
    } else if (port == 34) {
        gpio_num = 433;
    }

    if ((rc = onlp_file_read_int(&status, "/sys/class/gpio/gpio%d/value",
                                 gpio_num)) != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }
   
    *pres_val = status;
    
    return ONLP_STATUS_OK;
}


int
system_led_set(onlp_led_mode_t mode)
{
    int rc;

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
	
    if(mode == ONLP_LED_MODE_GREEN) {        
        rc = onlp_i2c_modifyb(I2C_BUS_50, LED_REG, LED_OFFSET, LED_SYS_AND_MASK,
                              LED_SYS_GMASK, ONLP_I2C_F_FORCE);
    }
    else if(mode == ONLP_LED_MODE_ORANGE) {       
        rc = onlp_i2c_modifyb(I2C_BUS_50, LED_REG, LED_OFFSET, LED_SYS_AND_MASK,
                              LED_SYS_YMASK, ONLP_I2C_F_FORCE);
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    if (rc < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    return ONLP_STATUS_OK;
}

int
fan_led_set(onlp_led_mode_t mode)
{
    int rc;

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(mode == ONLP_LED_MODE_GREEN ) {
        rc = onlp_i2c_modifyb(I2C_BUS_50, LED_REG, LED_OFFSET, LED_FAN_AND_MASK,
                              LED_FAN_GMASK, ONLP_I2C_F_FORCE);
    }
    else if(mode == ONLP_LED_MODE_ORANGE) {
        rc = onlp_i2c_modifyb(I2C_BUS_50, LED_REG, LED_OFFSET, LED_FAN_AND_MASK,
                              LED_FAN_YMASK, ONLP_I2C_F_FORCE);
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    if (rc < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    return ONLP_STATUS_OK;
}

int
psu1_led_set(onlp_led_mode_t mode)
{
    int rc;

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
	
    if(mode == ONLP_LED_MODE_GREEN) {    
        rc = onlp_i2c_modifyb(I2C_BUS_50, LED_REG, LED_PWOK_OFFSET,
                              LED_PSU1_ON_AND_MASK, LED_PSU1_ON_OR_MASK, 
                              ONLP_I2C_F_FORCE);
        rc = onlp_i2c_modifyb(I2C_BUS_50, LED_REG, LED_OFFSET, 
                              LED_PSU1_AND_MASK, LED_PSU1_GMASK,
                              ONLP_I2C_F_FORCE);
    } else if(mode == ONLP_LED_MODE_ORANGE) {
        rc = onlp_i2c_modifyb(I2C_BUS_50, LED_REG, LED_PWOK_OFFSET,
                              LED_PSU1_ON_AND_MASK, LED_PSU1_ON_OR_MASK, 
                              ONLP_I2C_F_FORCE);
        rc = onlp_i2c_modifyb(I2C_BUS_50, LED_REG, LED_OFFSET,
                              LED_PSU1_AND_MASK, LED_PSU1_YMASK, 
                              ONLP_I2C_F_FORCE);      
    } else if(mode == ONLP_LED_MODE_OFF) {
        rc = onlp_i2c_modifyb(I2C_BUS_50, LED_REG, LED_PWOK_OFFSET,
                              LED_PSU1_OFF_AND_MASK, LED_PSU1_OFF_OR_MASK, 
                              ONLP_I2C_F_FORCE);
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    if (rc < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    return ONLP_STATUS_OK;
}

int
psu2_led_set(onlp_led_mode_t mode)
{
    int rc;

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(mode == ONLP_LED_MODE_GREEN) {
        rc = onlp_i2c_modifyb(I2C_BUS_50, LED_REG, LED_PWOK_OFFSET, 
                              LED_PSU2_ON_AND_MASK, LED_PSU2_ON_OR_MASK, 
                              ONLP_I2C_F_FORCE);
        rc = onlp_i2c_modifyb(I2C_BUS_50, LED_REG, LED_OFFSET, 
                              LED_PSU2_AND_MASK,  LED_PSU2_GMASK, 
                              ONLP_I2C_F_FORCE);
    } else if(mode == ONLP_LED_MODE_ORANGE) {        
        rc = onlp_i2c_modifyb(I2C_BUS_50, LED_REG, LED_PWOK_OFFSET, 
                              LED_PSU2_ON_AND_MASK, LED_PSU2_ON_OR_MASK, 
                              ONLP_I2C_F_FORCE);
        rc = onlp_i2c_modifyb(I2C_BUS_50, LED_REG, LED_OFFSET, 
                              LED_PSU2_AND_MASK, LED_PSU2_YMASK, 
                              ONLP_I2C_F_FORCE);
    } else if(mode == ONLP_LED_MODE_OFF) {        
        rc = onlp_i2c_modifyb(I2C_BUS_50, LED_REG, LED_PWOK_OFFSET, 
                              LED_PSU2_OFF_AND_MASK, LED_PSU2_OFF_OR_MASK, 
                              ONLP_I2C_F_FORCE);
    } else {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    
    if (rc < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    return ONLP_STATUS_OK;
}

int
fan_tray_led_set(onlp_oid_t id, onlp_led_mode_t mode)
{
    int rc, temp_id;
    int offset;
    int and_mask, or_mask;

    if ( bmc_enable ) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    temp_id = ONLP_OID_ID_GET(id);
    switch (temp_id) {
        case 5:
            // fan tray 1
            offset = 3;
            if (mode == ONLP_LED_MODE_GREEN) {
                and_mask = 0xCF;
                or_mask = 0x20;
            } else if (mode == ONLP_LED_MODE_ORANGE) {
                and_mask = 0xCF;
                or_mask = 0x10;
            }
            break;
        case 6:
            // fan tray 2
            offset = 3;
            if (mode == ONLP_LED_MODE_GREEN) {
                and_mask = 0xFC;
                or_mask = 0x02;
            } else if (mode == ONLP_LED_MODE_ORANGE) {
                and_mask = 0xFC;
                or_mask = 0x01;
            }
            break;
        case 7:
            // fan tray 3
            offset = 2;
            if (mode == ONLP_LED_MODE_GREEN) {
                and_mask = 0xCF;
                or_mask = 0x20;
            } else if (mode == ONLP_LED_MODE_ORANGE) {
                and_mask = 0xCF;
                or_mask = 0x10;
            }
            break;
        case 8:
            // fan tray 4
            offset = 2;
            if (mode == ONLP_LED_MODE_GREEN) {
                and_mask = 0xFC;
                or_mask = 0x02;
            } else if (mode == ONLP_LED_MODE_ORANGE) {
                and_mask = 0xFC;
                or_mask = 0x01;
            }
            break;
        default:
            return ONLP_STATUS_E_INTERNAL;
            break;
    }
    
    rc = onlp_i2c_modifyb(I2C_BUS_59, FAN_GPIO_ADDR, offset, and_mask,
                          or_mask, ONLP_I2C_F_FORCE);
    if (rc < 0) {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
sysi_platform_info_get(onlp_platform_info_t* pi)
{
    int cpld_release, cpld_version, cpld_rev;
             
    cpld_rev = onlp_i2c_readb(I2C_BUS_44, CPLD_REG, CPLD_VER_OFFSET, ONLP_I2C_F_FORCE);
    if (cpld_rev < 0) {
        return ONLP_STATUS_E_INTERNAL; 
    }
  
    cpld_release = (((cpld_rev) >> 6 & 0x01));
    cpld_version = (((cpld_rev) & 0x3F));
    
    pi->cpld_versions = aim_fstrdup(            
                "CPLD is %d version(0:RD 1:Release), Revision is 0x%02x\n", 
                cpld_release, cpld_version);
    
    return ONLP_STATUS_OK;
}

bool
onlp_sysi_bmc_en_get(void)
{
    int value;

    if (onlp_file_read_int(&value, BMC_EN_FILE_PATH) < 0) {
        // flag file not exist, default to not enable
        return false;
    }

    /* 1 - enable, 0 - no enable */
    if ( value ) 
        return true;

    return false;
}
