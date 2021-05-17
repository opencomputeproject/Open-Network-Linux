/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2014 Accton Technology Corporation.
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

#define PSU_STATUS_PRESENT    1
#define PSU_STATUS_POWER_GOOD 1

#define PSU_NODE_MAX_INT_LEN  8
#define PSU_NODE_MAX_PATH_LEN 64

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_PSU(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

static int
psu_status_info_get(int id, char *node, int *value)
{
    int ret = 0;
    char buf[PSU_NODE_MAX_INT_LEN + 1] = {0};
    char node_path[PSU_NODE_MAX_PATH_LEN] = {0};

    *value = 0;

    if (PSU1_ID == id) {
        sprintf(node_path, "%s%s", PSU1_AC_HWMON_PREFIX, node);
    }
    else if (PSU2_ID == id) {
        sprintf(node_path, "%s%s", PSU2_AC_HWMON_PREFIX, node);
    }

    ret = deviceNodeReadString(node_path, buf, sizeof(buf), 0);

    if (ret == 0) {
        *value = atoi(buf);
    }

    return ret;
}

static int
psu_pmbus_info_get(int id, char *node, int *value)
{
    int  ret = 0;
    char buf[PSU_NODE_MAX_INT_LEN + 1]    = {0};
    char node_path[PSU_NODE_MAX_PATH_LEN] = {0};

    *value = 0;

    if (PSU1_ID == id) {
        sprintf(node_path, "%s%s", PSU1_AC_PMBUS_PREFIX, node);
    }
    else {
        sprintf(node_path, "%s%s", PSU2_AC_PMBUS_PREFIX, node);
    }

    ret = deviceNodeReadString(node_path, buf, sizeof(buf), 0);

    if (ret == 0) {
        *value = atoi(buf);
    }

    return ret;
}

int
onlp_psui_init(void)
{
    return ONLP_STATUS_OK;
}

static int
psu_ym2651_info_get(onlp_psu_info_t* info)
{
    int val   = 0;
    int index = ONLP_OID_ID_GET(info->hdr.id);

	if (info->status & ONLP_PSU_STATUS_FAILED) {
	    return ONLP_STATUS_OK;
	}

    /* Set the associated oid_table */
    info->hdr.coids[0] = ONLP_FAN_ID_CREATE(index + CHASSIS_FAN_COUNT);
    info->hdr.coids[1] = ONLP_THERMAL_ID_CREATE(index + CHASSIS_THERMAL_COUNT);

    /* Read voltage, current and power */
    if (psu_pmbus_info_get(index, "psu_v_out", &val) == 0) {
        info->mvout = val;
        info->caps |= ONLP_PSU_CAPS_VOUT;
    }

    if (psu_pmbus_info_get(index, "psu_i_out", &val) == 0) {
        info->miout = val;
        info->caps |= ONLP_PSU_CAPS_IOUT;
    }

    if (psu_pmbus_info_get(index, "psu_p_out", &val) == 0) {
        info->mpout = val;
        info->caps |= ONLP_PSU_CAPS_POUT;
    } 

    psu_pmbus_serial_number_get(index, info->serial, sizeof(info->serial));

    return ONLP_STATUS_OK;
}

static int
psu_acbel_info_get(onlp_psu_info_t* info)
{
    int val   = 0;
    int index = ONLP_OID_ID_GET(info->hdr.id);

    if (info->status & ONLP_PSU_STATUS_FAILED) {
        return ONLP_STATUS_OK;
    }

    /* Set the associated oid_table */
    info->hdr.coids[0] = ONLP_FAN_ID_CREATE(index + CHASSIS_FAN_COUNT);
    info->hdr.coids[1] = ONLP_THERMAL_ID_CREATE(index + CHASSIS_THERMAL_COUNT);

    /* Read voltage, current and power */
    if (psu_pmbus_info_get(index, "psu_v_out", &val) == 0) {
        info->mvout = val;
        info->caps |= ONLP_PSU_CAPS_VOUT;
    }

    if (psu_pmbus_info_get(index, "psu_i_out", &val) == 0) {
        info->miout = val;
        info->caps |= ONLP_PSU_CAPS_IOUT;
    }

    if (psu_pmbus_info_get(index, "psu_p_out", &val) == 0) {
        info->mpout = val;
        info->caps |= ONLP_PSU_CAPS_POUT;
    }

    psu_acbel_serial_number_get(index, info->serial, sizeof(info->serial));

    return ONLP_STATUS_OK;
}

#include <onlplib/i2c.h>
#define DC12V_750_REG_TO_CURRENT(low, high) (((low << 4 | high >> 4) * 20 * 1000) / 754)
#define DC12V_750_REG_TO_VOLTAGE(low, high) ((low << 4 | high >> 4) * 25)

static int
psu_dc12v_750_info_get(onlp_psu_info_t* info)
{
    int pid = ONLP_OID_ID_GET(info->hdr.id);
	int bus = (PSU1_ID == pid) ? 18 : 17;
	int iout_low, iout_high;
	int vout_low, vout_high;

    /* Set capability
     */
    info->caps = ONLP_PSU_CAPS_DC12;
    
	if (info->status & ONLP_PSU_STATUS_FAILED) {
	    return ONLP_STATUS_OK;
	}

	/* Get current
	 */
    iout_low  = onlp_i2c_readb(bus, 0x6f, 0x0, ONLP_I2C_F_FORCE);
	iout_high = onlp_i2c_readb(bus, 0x6f, 0x1, ONLP_I2C_F_FORCE);

	if ((iout_low >= 0) && (iout_high >= 0)) {
        info->miout = DC12V_750_REG_TO_CURRENT(iout_low, iout_high);
        info->caps |= ONLP_PSU_CAPS_IOUT;
	}

	/* Get voltage
	 */
	vout_low  = onlp_i2c_readb(bus, 0x6f, 0x2, ONLP_I2C_F_FORCE);
	vout_high = onlp_i2c_readb(bus, 0x6f, 0x3, ONLP_I2C_F_FORCE);

	if ((vout_low >= 0) && (vout_high >= 0)) {
        info->mvout = DC12V_750_REG_TO_VOLTAGE(vout_low, vout_high);
        info->caps |= ONLP_PSU_CAPS_VOUT;
	}

	/* Get power based on current and voltage
	 */
	if ((info->caps & ONLP_PSU_CAPS_IOUT) && (info->caps & ONLP_PSU_CAPS_VOUT)) {
		info->mpout = (info->miout * info->mvout) / 1000;
		info->caps |= ONLP_PSU_CAPS_POUT;
	}

    return ONLP_STATUS_OK;
}

/*
 * Get all information about the given PSU oid.
 */
static onlp_psu_info_t pinfo[] =
{
    { }, /* Not used */
    {
        { ONLP_PSU_ID_CREATE(PSU1_ID), "PSU-1", 0 },
    },
    {
        { ONLP_PSU_ID_CREATE(PSU2_ID), "PSU-2", 0 },
    }
};

int
onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* info)
{
    int val   = 0;
    int ret   = ONLP_STATUS_OK;
    int index = ONLP_OID_ID_GET(id);
    psu_type_t psu_type;

    VALIDATE(id);

    memset(info, 0, sizeof(onlp_psu_info_t));
    *info = pinfo[index]; /* Set the onlp_oid_hdr_t */

    /* Get the present state */
    if (psu_status_info_get(index, "psu_present", &val) != 0) {
        printf("Unable to read PSU(%d) node(psu_present)\r\n", index);
    }

    if (val != PSU_STATUS_PRESENT) {
        info->status &= ~ONLP_PSU_STATUS_PRESENT;
        return ONLP_STATUS_OK;
    }
    info->status |= ONLP_PSU_STATUS_PRESENT;


    /* Get power good status */
    if (psu_status_info_get(index, "psu_power_good", &val) != 0) {
        printf("Unable to read PSU(%d) node(psu_power_good)\r\n", index);
    }

    if (val != PSU_STATUS_POWER_GOOD) {
        info->status |=  ONLP_PSU_STATUS_FAILED;
    }


    /* Get PSU type
     */
    psu_type = get_psu_type(index, info->model, sizeof(info->model));

    switch (psu_type) {
        case PSU_TYPE_AC_F2B_3YPOWER:
        case PSU_TYPE_AC_B2F_3YPOWER:
            info->caps = ONLP_PSU_CAPS_AC;
            ret = psu_ym2651_info_get(info);
            break;
        case PSU_TYPE_AC_F2B_ACBEL:
        case PSU_TYPE_AC_B2F_ACBEL:
            info->caps = ONLP_PSU_CAPS_AC;
            ret = psu_acbel_info_get(info);
            break;
		case PSU_TYPE_DC_48V_F2B:
		case PSU_TYPE_DC_48V_B2F:
			info->caps = ONLP_PSU_CAPS_DC48;
			ret = psu_ym2651_info_get(info);
			break;
		case PSU_TYPE_DC_12V_F2B:
		case PSU_TYPE_DC_12V_B2F:
		case PSU_TYPE_DC_12V_FANLESS:
			ret = psu_dc12v_750_info_get(info);
			break;
        case PSU_TYPE_UNKNOWN:  /* User insert a unknown PSU or unplugged.*/
            info->status |= ONLP_PSU_STATUS_UNPLUGGED;
            info->status &= ~ONLP_PSU_STATUS_FAILED;
            ret = ONLP_STATUS_OK;
            break;
        default:
            ret = ONLP_STATUS_E_UNSUPPORTED;
            break;
    }

    return ret;
}

int
onlp_psui_ioctl(onlp_oid_t pid, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

