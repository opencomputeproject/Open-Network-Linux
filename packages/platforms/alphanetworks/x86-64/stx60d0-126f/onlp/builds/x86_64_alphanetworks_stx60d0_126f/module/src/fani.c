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
 * Fan Platform Implementation Defaults.
 *
 ***********************************************************/
#include <onlp/platformi/fani.h>
#include <onlplib/mmap.h>
#include <fcntl.h>
#include <math.h>
#include "platform_lib.h"

#define DEBUG                           0

#define CPLD_FAN1_STATUS_ADDR_OFFSET    0x11  //bit 0: Fan 0 Direction, bit 1: Fan 0 Present
#define CPLD_FAN2_STATUS_ADDR_OFFSET    0x12  //bit 0: Fan 1 Direction, bit 1: Fan 1 Present
#define CPLD_FAN3_STATUS_ADDR_OFFSET    0x13
#define CPLD_FAN4_STATUS_ADDR_OFFSET    0x14

#define BMC_FAN1_SENSOR_NUM                 0x31
#define BMC_FAN2_SENSOR_NUM                 0x32
#define BMC_FAN3_SENSOR_NUM                 0x33
#define BMC_FAN4_SENSOR_NUM                 0x34

#define BMC_FAN1_PWM_NUM                 0x01
#define BMC_FAN2_PWM_NUM                 0x02
#define BMC_FAN3_PWM_NUM                 0x03
#define BMC_FAN4_PWM_NUM                 0x04

#define CPLD_FAN1_SPEED_ADDR_OFFSET   0x1A
#define CPLD_FAN2_SPEED_ADDR_OFFSET   0x19
#define CPLD_FAN3_SPEED_ADDR_OFFSET   0x18
#define CPLD_FAN4_SPEED_ADDR_OFFSET   0x17

#define CPLD_FAN_MIN_RPS_ADDR_OFFSET        0x22
#define CPLD_FAN_PWM_ADDR_OFFSET            0x23
#define CPLD_FAN_SEL_CONTROL_ADDR_OFFSET    0x24  /* 0: CPLD control, 1: BMC control */

#define FAN1_EEPROM_BUS     21
#define FAN_EEPROM_ADDR     0x57
#define FAN_MODEL_OFFSET    0x67
#define FAN_MODEL_LENGTH    13
#define FAN_SERIAL_OFFSET   0x76
#define FAN_SERIAL_LENGTH   9

#define FAN_MAX_RPM  21000

#if 1 //PSU FAN
#define PSU_PMBus_ADDR          0x58
#define READ_FAN_SPEED_REG      0x90
#define READ_FAN_STATUS_1_2_REG  0x81
//#define PSU_ID_OFFSET           4
#define PSU_ID_OFFSET          (PSU1_BUS_ID - FAN_1_ON_PSU1)
#endif

#define FAN_DIR_FRONT_TO_BACK   0
#define FAN_DIR_BACK_TO_FRONT   1



/* Static fan information */
onlp_fan_info_t linfo[] = {
    { }, /* Not used */
    {
        { ONLP_FAN_ID_CREATE(FAN_1), "Chassis Fan 1", 0 },
        0x0,
        ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_SET_RPM,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
    },
    {
        { ONLP_FAN_ID_CREATE(FAN_2), "Chassis Fan 2", 0 },
        0x0,
        ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_SET_RPM,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
    },
    {
        { ONLP_FAN_ID_CREATE(FAN_3), "Chassis Fan 3", 0 },
        0x0,
        ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_SET_RPM,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
    },
    {
        { ONLP_FAN_ID_CREATE(FAN_4), "Chassis Fan 4", 0 },
        0x0,
        ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_SET_RPM,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
    },
    {
        { ONLP_FAN_ID_CREATE(FAN_1_ON_PSU1), "Chassis PSU-1 Fan 1", 0 },
        0x0,
        ONLP_FAN_CAPS_GET_RPM,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
    },
    {
        { ONLP_FAN_ID_CREATE(FAN_1_ON_PSU2), "Chassis PSU-2 Fan 1", 0 },
        0x0,
        ONLP_FAN_CAPS_GET_RPM,
        0,
        0,
        ONLP_FAN_MODE_INVALID,
    },
};

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_FAN(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)


typedef union
{
    unsigned char val;
    struct
    {
        unsigned char fan_direction :1; //bit 0
        unsigned char fan_present :1; //bit 1
        unsigned char fan0_rps_err :1;
        unsigned char fan1_rps_err :1;
        unsigned char :4;  /* reserved */
    }bit;

}_CPLD_FAN_STATUS_REG_T;


/* Get the CPLD address offset for fan status */
static char
_onlp_fani_get_fan_status_addr_offset(int local_id)
{
    DIAG_PRINT("%s, local_id=%d", __FUNCTION__, local_id);
    switch (local_id)
    {
        case FAN_1:
            return CPLD_FAN1_STATUS_ADDR_OFFSET;
        case FAN_2:
            return CPLD_FAN2_STATUS_ADDR_OFFSET;
        case FAN_3:
            return CPLD_FAN3_STATUS_ADDR_OFFSET;
        case FAN_4:
            return CPLD_FAN4_STATUS_ADDR_OFFSET;
    }
    return 0;
}

/* Get the BMC fan sensor number for fan speed */
static char
_onlp_fani_get_bmc_fan_num(int local_id)
{
    DIAG_PRINT("%s, local_id=%d", __FUNCTION__, local_id);
    switch (local_id)
    {
        case FAN_1:
            return BMC_FAN1_SENSOR_NUM;
        case FAN_2:
            return BMC_FAN2_SENSOR_NUM;
        case FAN_3:
            return BMC_FAN3_SENSOR_NUM;
        case FAN_4:
            return BMC_FAN4_SENSOR_NUM;
    }
    return 0;
}

/* Get the BMC fan PWM number */
static char
_onlp_fani_get_bmc_pwm_num(int local_id)
{
    DIAG_PRINT("%s, local_id=%d", __FUNCTION__, local_id);
    switch (local_id)
    {
        case FAN_1:
            return BMC_FAN1_PWM_NUM;
        case FAN_2:
            return BMC_FAN2_PWM_NUM;
        case FAN_3:
            return BMC_FAN3_PWM_NUM;
        case FAN_4:
            return BMC_FAN4_PWM_NUM;
    }
    return 0;
}

static int
_onlp_fani_info_get_fan(int local_id, onlp_fan_info_t *info)
{
    DIAG_PRINT("%s, local_id=%d", __FUNCTION__, local_id);
    _CPLD_FAN_STATUS_REG_T fan_status_reg;
    char status_addr_offset = 0;
    char bmc_fan_number = 0;
    char bmc_pwm_number = 0;
    char data = 0;
    unsigned char tach_speed0 = 0;
    int ret = 0;

    /* Get the CPLD address offset for fan status */
    status_addr_offset = _onlp_fani_get_fan_status_addr_offset(local_id);

    ret = bmc_i2c_read_byte(BMC_CPLD_I2C_BUS_ID, BMC_CPLD_I2C_ADDR, status_addr_offset, &data);
    if (ret < 0)
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    if (DEBUG)
        AIM_LOG_INFO("%s:%d id[%d],status_addr_offset[%02x],read_byte[%02x]\n",
                     __FUNCTION__, __LINE__, local_id, (unsigned char)status_addr_offset, (unsigned char)data);

    fan_status_reg.val = data;

    /* Get fan air flow direction */
    if (fan_status_reg.bit.fan_direction == FAN_DIR_FRONT_TO_BACK)
        info->status |= ONLP_FAN_STATUS_F2B;
    else
        info->status |= ONLP_FAN_STATUS_B2F;

    /* Get fan present */
    if (fan_status_reg.bit.fan_present == 1)
        info->status |= ONLP_FAN_STATUS_PRESENT;
    else
        info->status |= ONLP_FAN_STATUS_FAILED;

    /* Check Speed Error*/
    if (fan_status_reg.bit.fan0_rps_err == 1 || fan_status_reg.bit.fan1_rps_err == 1)
    {
        info->status |= ONLP_FAN_STATUS_FAILED;
    }

    /* Get the BMC fan sensor number for fan speed */
    bmc_fan_number = _onlp_fani_get_bmc_fan_num(local_id);

    ret = bmc_read_raw_fan_speed(bmc_fan_number, (char *)&tach_speed0);
    if (ret < 0)
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    if (DEBUG)
        AIM_LOG_INFO("%s:%d id[%d],bmc_fan_number[%02x],read_byte[%02x]\n",
                     __FUNCTION__, __LINE__, local_id, (unsigned char)bmc_fan_number, (unsigned char)tach_speed0);

    /* Get fan rpm */
    info->rpm = (int)tach_speed0 * 118;

    /* Get the BMC pwm number */
    bmc_pwm_number = _onlp_fani_get_bmc_pwm_num(local_id);

    ret = bmc_read_raw_fan_pwm(bmc_pwm_number, (char *)&data);
    if (ret < 0)
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    if (DEBUG)
        AIM_LOG_INFO("%s:%d id[%d],bmc_pwm_number[%02x],read_byte[%02x]\n",
                     __FUNCTION__, __LINE__, local_id, bmc_pwm_number, (unsigned char)data);

    /* Get fan duty cycle */
    info->percentage = data;

    return ONLP_STATUS_OK;
}

static int
_onlp_fani_info_get_fan_on_psu(int local_id, onlp_fan_info_t *info)
{
    int ret = ONLP_STATUS_OK;
    int exponent = 0, mantissa = 0;
    uint16_t value = 0;


    DIAG_PRINT("%s, local_id=%d", __FUNCTION__, local_id);

    /* Get PSU fan speed */
    int psu_busid=0; 
    psu_busid = index_to_psu_busid(local_id - FAN_4);

    ret = i2c_read_word(psu_busid, PSU_PMBus_ADDR, READ_FAN_SPEED_REG);
    if (ret < 0 && DEBUG)
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ret);


    //printf("[%s]local_id:%d i2c read ret:%d\n",__FUNCTION__, local_id, ret);

    if (ret <= 0)
    {
        info->status |= ONLP_FAN_STATUS_FAILED;
    }
    else
    {
        info->status |= ONLP_FAN_STATUS_PRESENT;

        value = ret;
        //Linear Data Format
        exponent = psu_two_complement_to_int(value >> 11, 5, 0x1f);
        mantissa = psu_two_complement_to_int(value & 0x7ff, 11, 0x7ff);

        info->rpm = (exponent >= 0) ? (mantissa << exponent) : (mantissa) / (1 << -exponent);
    }

    //check Fan Fault, bit 7
    ret = i2c_read_word(PSU2_BUS_ID, PSU_PMBus_ADDR, READ_FAN_STATUS_1_2_REG);
    if (ret < 0 && DEBUG)
    {
        AIM_LOG_INFO("%s:%d fail[%d]\n", __FUNCTION__, __LINE__, ret);
    }
    else if ((ret & 0x80) == 0x80)
    {
        info->status |= ONLP_FAN_STATUS_FAILED;
    }



    return ONLP_STATUS_OK;
}

/*
 * This function will be called prior to all of onlp_fani_* functions.
 */
#define DEFAULT_FAN_SPEED 50

int
onlp_fani_init(void)
{
#if 0 //onlpdump call this and reset fan pwm.
    DIAG_PRINT("%s", __FUNCTION__);

    int i = 0;

    for (i = FAN_1;
         i <= FAN_4;
         i++)
    {
        onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(i), DEFAULT_FAN_SPEED);
    }
#endif
    return ONLP_STATUS_OK;
}

int
onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t *info)
{
    DIAG_PRINT("%s, id=%d", __FUNCTION__, id);

    int ret = ONLP_STATUS_OK;
    int local_id;

    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);

    *info = linfo[local_id];

    switch (local_id)
    {
        case FAN_1_ON_PSU1:
        case FAN_1_ON_PSU2:
            ret = _onlp_fani_info_get_fan_on_psu(local_id, info);
            break;

        default:
            ret = _onlp_fani_info_get_fan(local_id, info);
            break;
    }

    return ret;
}

int onlp_fani_status_get(onlp_oid_t id, uint32_t *rv)
{

    DIAG_PRINT("%s, id=%d", __FUNCTION__, id);
    onlp_fan_info_t info;
    onlp_fan_status_t ret = 0;
    onlp_fani_info_get(id, &info);


    ret = info.status & (ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_FAILED);
    *rv = (uint32_t)ret;
    return 0;
}

int _onlp_fani_rpm_to_percentage(int rpm)
{
    int percentage = 0;
    percentage = round(rpm * 100 / FAN_MAX_RPM);

    if (percentage == 0 && rpm != 0)
    {
        percentage = 1;
    }
    DIAG_PRINT("%s, rpm=%d to percentage=%d", __FUNCTION__, rpm, percentage);

    return percentage;
}

/*
 * This function sets the speed of the given fan in RPM.
 *
 * This function will only be called if the fan supprots the RPM_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
/* 
  [Note]: By H/W design, fan speed is controlled using percentage.
          RPM value will be translated to percentage and it may produce some deviation.
          (the register size is 8-bit, so there is only 255 units to present the value.)
*/
int
onlp_fani_rpm_set(onlp_oid_t id, int rpm)
{
    DIAG_PRINT("%s, id=%d, rpm=%d", __FUNCTION__, id, rpm);

    if (rpm > FAN_MAX_RPM || rpm <= 0)
    {
        AIM_LOG_INFO("%s:%d rpm:%d is out of range. (1~%d)\n", __FUNCTION__, __LINE__, rpm, FAN_MAX_RPM);
        return ONLP_STATUS_E_PARAM;
    }
    return onlp_fani_percentage_set(id, _onlp_fani_rpm_to_percentage(rpm));
}


/*
 * This function sets the fan speed of the given OID as a percentage.
 *
 * This will only be called if the OID has the PERCENTAGE_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int
onlp_fani_percentage_set(onlp_oid_t id, int p)
{
    int ret = 0, local_id = 0;
    char data = (char)p;
    char bmc_pwm_number = 0;

    DIAG_PRINT("%s, id=%d, p=%d (0x%2X)", __FUNCTION__, id, p, (unsigned char)data);

    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);

    /* Not support to stop fan */
    if (p == 0)
        return ONLP_STATUS_E_INVALID;

    bmc_pwm_number = _onlp_fani_get_bmc_pwm_num(local_id);
    
    ret = bmc_write_raw_fan_pwm(bmc_pwm_number, data);
    if (ret < 0)
        AIM_LOG_INFO("%s:%d data[%02x] fail[%d]\n", __FUNCTION__, __LINE__, data, ret);

    if (DEBUG)
        AIM_LOG_INFO("%s:%d id[%d],bmc_pwm_number[%02x],write_byte[%02x]\n",
                     __FUNCTION__, __LINE__, local_id, bmc_pwm_number, (unsigned char)data);

    return ONLP_STATUS_OK;

}


/*
 * This function sets the fan speed of the given OID as per
 * the predefined ONLP fan speed modes: off, slow, normal, fast, max.
 *
 * Interpretation of these modes is up to the platform.
 *
 */
int
onlp_fani_mode_set(onlp_oid_t id, onlp_fan_mode_t mode)
{
    DIAG_PRINT("%s, ONLP_STATUS_E_UNSUPPORTED", __FUNCTION__);
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function sets the fan direction of the given OID.
 *
 * This function is only relevant if the fan OID supports both direction
 * capabilities.
 *
 * This function is optional unless the functionality is available.
 */
int
onlp_fani_dir_set(onlp_oid_t id, onlp_fan_dir_t dir)
{
    DIAG_PRINT("%s, ONLP_STATUS_E_UNSUPPORTED", __FUNCTION__);
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * Generic fan ioctl. Optional.
 */
int
onlp_fani_ioctl(onlp_oid_t id, va_list vargs)
{
    DIAG_PRINT("%s, ONLP_STATUS_E_UNSUPPORTED", __FUNCTION__);
    return ONLP_STATUS_E_UNSUPPORTED;
}


