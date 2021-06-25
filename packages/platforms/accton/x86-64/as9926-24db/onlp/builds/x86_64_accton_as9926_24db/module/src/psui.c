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
#include <onlplib/file.h>
#include "platform_lib.h"

#define PSU_STATUS_PRESENT     1
#define PSU_STATUS_POWER_GOOD  1

#define VALIDATE(_id)					\
	do {						\
		if(!ONLP_OID_IS_PSU(_id)) {		\
			return ONLP_STATUS_E_INVALID;   \
		}					\
	} while(0)

#define AIM_FREE_IF_PTR(p)				\
	do {						\
		if (p) {				\
			aim_free(p); 			\
			p = NULL;			\
		} 					\
	} while (0)

int onlp_psui_init(void)
{
	return ONLP_STATUS_OK;
}

/*
 * Get all information about the given PSU oid.
 */
static onlp_psu_info_t pinfo[] =
{
	{ }, /* Not used */
	{
		{ ONLP_PSU_ID_CREATE(PSU1_ID), "PSU-1", 0, {0} },
		{ 0 }, { 0 }, 0, 0, 0, 0, 0, 0, 0, 0,
	},
	{
		{ ONLP_PSU_ID_CREATE(PSU2_ID), "PSU-2", 0, {0} },
		{ 0 }, { 0 }, 0, 0, 0, 0, 0, 0, 0, 0,
	}
};

int onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* info)
{
	int val   = 0;
	int ret   = ONLP_STATUS_OK;
	int pid = ONLP_OID_ID_GET(id);

	VALIDATE(id);

	memset(info, 0, sizeof(onlp_psu_info_t));
	*info = pinfo[pid]; /* Set the onlp_oid_hdr_t */

	/* Get the present state */
	ret = onlp_file_read_int(&val, "%s""psu%d_present", 
				 PSU_SYSFS_PATH, pid);
	if (ret < 0) {
		AIM_LOG_ERROR("Unable to read status \
			      from (%s""psu%d_present)\r\n", 
			      PSU_SYSFS_PATH, pid);
		return ONLP_STATUS_E_INTERNAL;
	}

	if (val != PSU_STATUS_PRESENT) {
		info->status &= ~ONLP_PSU_STATUS_PRESENT;
		return ONLP_STATUS_OK;
	}

	info->status |= ONLP_PSU_STATUS_PRESENT;
	info->caps = ONLP_PSU_CAPS_AC;

	/* Get power good status */
	ret = onlp_file_read_int(&val, "%s""psu%d_power_good", PSU_SYSFS_PATH, 
				 pid);
    
	if (ret < 0) {
		AIM_LOG_ERROR("Unable to read status \
			       from (%s""psu%d_power_good)\r\n", 
			       PSU_SYSFS_PATH, pid);
		return ONLP_STATUS_E_INTERNAL;
	}

	if (val != PSU_STATUS_POWER_GOOD) {
		info->status |=  ONLP_PSU_STATUS_FAILED;
	}

	if (info->status & ONLP_PSU_STATUS_FAILED) {
		return ONLP_STATUS_OK;
	}

	/* Read voltage, current and power */
	val = 0;
	if (onlp_file_read_int(&val, "%s""psu%d_vin", PSU_SYSFS_PATH, pid) == 0 
			       && val) {
		info->mvin  = val;
		info->caps |= ONLP_PSU_CAPS_VIN;
	}

	val = 0;
	if (onlp_file_read_int(&val, "%s""psu%d_vout", PSU_SYSFS_PATH, pid) == 0 
			       && val) {
		info->mvout = val;
		info->caps |= ONLP_PSU_CAPS_VOUT;
	}

	val = 0;
	if (onlp_file_read_int(&val, "%s""psu%d_iin", PSU_SYSFS_PATH, pid) == 0 
			       && val) {
		info->miin = val;
		info->caps |= ONLP_PSU_CAPS_IIN;
	}

	val = 0;
	if (onlp_file_read_int(&val, "%s""psu%d_iout", PSU_SYSFS_PATH, pid) == 0 
			       && val) {
		info->miout = val;
		info->caps |= ONLP_PSU_CAPS_IOUT;
	}

	val = 0;
	if (onlp_file_read_int(&val, "%s""psu%d_pin", PSU_SYSFS_PATH, pid) == 0 
			       && val) {
		info->mpin = val;
		info->caps |= ONLP_PSU_CAPS_PIN;
	}

	val = 0;
	if (onlp_file_read_int(&val, "%s""psu%d_pout", PSU_SYSFS_PATH, pid) == 0 
			       && val) {
		info->mpout = val;
		info->caps |= ONLP_PSU_CAPS_POUT;
	}

	/* Set the associated oid_table */
	val = 0;
	if (onlp_file_read_int(&val, "%s""psu%d_fan1_input", 
			       PSU_SYSFS_PATH, pid) == 0 && val) {
		info->hdr.coids[0] = ONLP_FAN_ID_CREATE(pid + 
							CHASSIS_FAN_COUNT);
	}

	val = 0;
	if (onlp_file_read_int(&val, "%s""psu%d_temp1_input", PSU_SYSFS_PATH,
			       pid) == 0 && val) {
		info->hdr.coids[1] = ONLP_THERMAL_ID_CREATE(
					pid + CHASSIS_THERMAL_COUNT);
	}

	/* Read model */
	char *string = NULL;
	int len = onlp_file_read_str(&string, "%s""psu%d_model", PSU_SYSFS_PATH, 
				     pid);
	if (string && len) {
		memcpy(info->model, string, len);
		info->model[len] = '\0';
	}
	AIM_FREE_IF_PTR(string);

	/* Read serial */
	len = onlp_file_read_str(&string, "%s""psu%d_serial", PSU_SYSFS_PATH, 
				 pid);
	if (string && len) {
		memcpy(info->serial, string, len);
		info->serial[len] = '\0';
	}
	AIM_FREE_IF_PTR(string);

	return ret;
}
