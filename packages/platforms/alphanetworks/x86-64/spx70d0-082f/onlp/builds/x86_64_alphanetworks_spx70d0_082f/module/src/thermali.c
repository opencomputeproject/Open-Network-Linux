/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *        Copyright 2014, 2015 Big Switch Networks, Inc.
 *        Copyright 2020 Alpha Networks Incorporation
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
 * Thermal Sensor Platform Implementation.
 *
 ***********************************************************/
#include <unistd.h>
#include <onlplib/mmap.h>
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include <fcntl.h>
#include "platform_lib.h"

#define LOCAL_DEBUG 0

#define THERMALI_LM75_AMBIENT_ID 1
#define THERMALI_LM75_HOT_SPOT_ID 2
#define THERMALI_TMP435_LOCAL_ID 3
#define THERMALI_TMP435_REMOTE_ID 4

/* IPMI Table 22-14 define Request Data byte 2 as I2C address. 
  * [7:1] -Slave address and [0] - reserved.
  * So Thermal Physical Address in the I2C is 0x49( 0100 1001) should transfer to 0x92 (1001 0010).
  */
#define BMC_THERMALI_LM75_HOT_SPOT_ADDR 0x92    /* Thermal addr */
#define BMC_THERMALI_LM75_AMBIENT_ADDR  0x90    /* Thermal addr */
#define BMC_THERMALI_TMP435_ADDR  0x9A    /* Thermal addr */

#define THERMALI_LM75_HOT_SPOT_ADDR 0x49    /* Thermal addr */
#define THERMALI_LM75_AMBIENT_ADDR  0x48    /* Thermal addr */
#define THERMALI_TMP435_ADDR  0x4D    /* Thermal addr */

#define THERMALI_TMP435_LOCAL_HIGH_BYTE_OFFSET  0x00  
#define THERMALI_TMP435_LOCAL_LOW_BYTE_OFFSET  0x15  
#define THERMALI_TMP435_REMOTE_HIGH_BYTE_OFFSET  0x01  
#define THERMALI_TMP435_REMOTE_LOW_BYTE_OFFSET  0x10  

 #define BMC_THERMALI_LM75_BUS_ID   BMC_I2C6
 #define BMC_THERMALI_TMP435_BUS_ID   BMC_I2C6


#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_THERMAL(_id)) {         \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)




enum onlp_thermal_id
{
    THERMAL_RESERVED = 0,
    THERMAL_1_ON_MAIN_BROAD_HOT_SPOT,         /* The LM75 on Main Board for hot spot */
    THERMAL_2_ON_MAIN_BROAD_AMBIENT,          /* The LM75 on Main Board for ambient */
    THERMAL_3_ON_MAIN_BROAD_LOCAL_BMC,        /* The TMP435 on Main Board for local monitoring the board temperature near the BMC */
    THERMAL_3_ON_MAIN_BROAD_REMOTE_BCM88470   /* The TMP435 on Main Board for remote monitoring BCM88470 on-chip temperature */
};

static int thermali_tmp435_low_byte_to_temp(unsigned char lowByte_data)
{
    int temp=0;

    /* TMP435 Local and Remote Temperature Low Bytes.
      * Resolution is 0.0625 degress C/count. All possible values are shown. 
      */
    switch (lowByte_data)
    {
        case 0: /* 0x00 */
            temp =  0; //0
            break;

        case 16: /* 0x10 */
            temp =  62; //0.0625 * 1000
            break;

        case 32: /* 0x20 */
            temp =  125; //0.1250 * 1000
            break;

        case 48: /* 0x30 */
            temp =  187; //0.1870 * 1000
            break;

        case 64: /* 0x40 */
            temp =  250; //0.2500 * 1000
            break;

        case 80: /* 0x50 */
            temp =  312; //0.3125 * 1000
            break;

        case 96: /* 0x60 */
            temp =  375; //0.3750 * 1000
            break;

        case 112: /* 0x70 */
            temp =  435; //0.4375 * 1000
            break;

        case 128: /* 0x80 */
            temp =  500; //0.5000 * 1000
            break;

        case 144: /* 0x90 */
            temp =  562; //0.5625 * 1000
            break;

        case 160: /* 0xA0 */
            temp =  625; //0.6250 * 1000
            break;

        case 176: /* 0xB0 */
            temp =  687; //0.6875 * 1000
            break;

        case 192: /* 0xC0 */
            temp =  750; //0.7500 * 1000
            break;

        case 208: /* 0xD0 */
            temp =  812; //0.8125 * 1000
            break;

        case 224: /* 0xE0 */
            temp =  875; //0.8750 * 1000
            break;

        case 240: /* 0xF0 */
            temp =  937; //0.9375 * 1000
            break;

    }

    return temp;
}

static int thermali_tmp435_remote_current_temp_get(int *remote_data)
{
    int ret = 0;
    unsigned char data_highB = 0;
    unsigned char data_lowB = 0;
    int milli_unit = 1000;  
    char high_byte_offset = THERMALI_TMP435_REMOTE_HIGH_BYTE_OFFSET;
    char low_byte_offset = THERMALI_TMP435_REMOTE_LOW_BYTE_OFFSET;
    int highB_value=0;
    int lowB_value=0;
    
    ret = bmc_i2c_read_byte(BMC_THERMALI_TMP435_BUS_ID, BMC_THERMALI_TMP435_ADDR, high_byte_offset, (char *)&data_highB);
    if (ret < 0)
        printf("I2C command 0x%X Read Fail, id=%d\n", BMC_THERMALI_TMP435_ADDR, BMC_THERMALI_TMP435_BUS_ID);

    ret = bmc_i2c_read_byte(BMC_THERMALI_TMP435_BUS_ID, BMC_THERMALI_TMP435_ADDR, low_byte_offset, (char *)&data_lowB);
    if (ret < 0)
        printf("I2C command 0x%X Read Fail, id=%d\n", BMC_THERMALI_TMP435_ADDR, BMC_THERMALI_TMP435_BUS_ID);

    highB_value= (int)((data_highB-64) * milli_unit);
    lowB_value= thermali_tmp435_low_byte_to_temp(data_lowB);    

   *remote_data = highB_value + lowB_value;

    return ret;
}


static int thermali_tmp435_local_current_temp_get(int *local_data)
{
    int ret = 0;
    unsigned char data_highB = 0;
    unsigned char data_lowB = 0;
    int milli_unit = 1000;  
    char high_byte_offset = THERMALI_TMP435_LOCAL_HIGH_BYTE_OFFSET;
    char low_byte_offset = THERMALI_TMP435_LOCAL_LOW_BYTE_OFFSET;
    int highB_value=0;
    int lowB_value=0;
    
    ret = bmc_i2c_read_byte(BMC_THERMALI_TMP435_BUS_ID, BMC_THERMALI_TMP435_ADDR, high_byte_offset, (char *)&data_highB);
    if (ret < 0)
        printf("I2C command 0x%X Read Fail, id=%d\n", BMC_THERMALI_TMP435_ADDR, BMC_THERMALI_TMP435_BUS_ID);

    ret = bmc_i2c_read_byte(BMC_THERMALI_TMP435_BUS_ID, BMC_THERMALI_TMP435_ADDR, low_byte_offset, (char *)&data_lowB);
    if (ret < 0)
        printf("I2C command 0x%X Read Fail, id=%d\n", BMC_THERMALI_TMP435_ADDR, BMC_THERMALI_TMP435_BUS_ID);

    highB_value= (int)((data_highB-64) * milli_unit);
    lowB_value= thermali_tmp435_low_byte_to_temp(data_lowB);    

    *local_data = highB_value + lowB_value;

    return ret;
}


static int thermali_tmp435_current_temp_get(int id, int *data)
{
    int ret = 0;

    if (id == THERMALI_TMP435_LOCAL_ID)
    {
        ret = thermali_tmp435_local_current_temp_get(data);
    }
    else if (id == THERMALI_TMP435_REMOTE_ID)
    {
        ret = thermali_tmp435_remote_current_temp_get(data);
    }

    return ret;
}


static int thermali_tmp75_current_temp_get(int id, char *data)
{
    int ret = 0;

    if (id == THERMALI_LM75_AMBIENT_ID)
    {
        ret = bmc_i2c_read_byte(BMC_THERMALI_LM75_BUS_ID, BMC_THERMALI_LM75_AMBIENT_ADDR, 0x0, data);
        if (ret < 0)
            printf("I2C command 0x%X Read Fail, id=%d\n", BMC_THERMALI_LM75_AMBIENT_ADDR, BMC_THERMALI_LM75_BUS_ID);
    }
    else if (id == THERMALI_LM75_HOT_SPOT_ID)
    {
        ret = bmc_i2c_read_byte(BMC_THERMALI_LM75_BUS_ID, BMC_THERMALI_LM75_HOT_SPOT_ADDR, 0x0, data);
        if (ret < 0)
            printf("I2C command 0x%X Read Fail, id=%d\n", BMC_THERMALI_LM75_HOT_SPOT_ADDR, BMC_THERMALI_LM75_BUS_ID);
    }
    return ret;
}

static int thermali_current_temp_get(int id, int *data)
{
    int ret = 0;
    int milli_unit = 1000;  
    char r_data[10] = { 0 };
    int tmp435_data;

    switch (id)
    {
        case THERMALI_LM75_AMBIENT_ID:
        case THERMALI_LM75_HOT_SPOT_ID:
            ret = thermali_tmp75_current_temp_get(id, r_data);
            if (ret < 0)
                printf("[thermali]get tmp75 current temperature fail!\n");
            else
                *data = (*r_data) * milli_unit;
            break;

        case THERMALI_TMP435_LOCAL_ID:
        case THERMALI_TMP435_REMOTE_ID:
            ret = thermali_tmp435_current_temp_get(id, &tmp435_data);
            if (ret < 0)
                printf("[thermali]get tmp435 current temperature fail!\n");
            else
                *data = tmp435_data;
            break;

        default:
            return ret;
    }

    return ret;
}


/* Static values */
static onlp_thermal_info_t linfo[] = {
    { }, /* Not used */
    { { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_MAIN_BROAD_HOT_SPOT), "Chassis Thermal Sensor 1 (HOT SPOT)", 0 },
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_MAIN_BROAD_AMBIENT), "Chassis Thermal Sensor 2 (AMBIENT)", 0 },
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_MAIN_BROAD_LOCAL_BMC), "Chassis Thermal Sensor 3 (TMP435 LOCAL)", 0 },
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_MAIN_BROAD_REMOTE_BCM88470), "Chassis Thermal Sensor 4 (TMP435 REMOTE)", 0 },
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    }
};


/*
 * This will be called to intiialize the thermali subsystem.
 */
int
onlp_thermali_init(void)
{
    DIAG_PRINT("%s", __FUNCTION__);
    return ONLP_STATUS_OK;
}


/*
 * Retrieve the information structure for the given thermal OID.
 *
 * If the OID is invalid, return ONLP_E_STATUS_INVALID.
 * If an unexpected error occurs, return ONLP_E_STATUS_INTERNAL.
 * Otherwise, return ONLP_STATUS_OK with the OID's information.
 *
 * Note -- it is expected that you fill out the information
 * structure even if the sensor described by the OID is not present.
 */
int
onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t *info)
{
    DIAG_PRINT("%s, id=%d", __FUNCTION__, id);
    int local_id;
    int temperature;
    
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);

    if (LOCAL_DEBUG)
        printf("\n[Debug][%s][%d][local_id: %d]", __FUNCTION__, __LINE__, local_id);

    /* Set the onlp_oid_hdr_t and capabilities */
    *info = linfo[local_id];
#if 0
    if(local_id == THERMAL_CPU_CORE)
    {
        int rv = onlp_file_read_int_max(&info->mcelsius, cpu_coretemp_files);
        return rv;
    }
#endif

    if (thermali_current_temp_get(local_id, &temperature) < 0)
        printf("[thermali]get current temperature fail!\n");

    info->mcelsius = temperature;

    if (LOCAL_DEBUG)
        printf("\n[Debug][%s][%d][save data: %d]\n", __FUNCTION__, __LINE__, info->mcelsius);

    return ONLP_STATUS_OK;
}


