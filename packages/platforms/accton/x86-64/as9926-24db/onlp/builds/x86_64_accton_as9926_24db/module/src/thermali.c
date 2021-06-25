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
 * Thermal Sensor Platform Implementation.
 *
 ***********************************************************/
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include "platform_lib.h"

#define VALIDATE(_id)					\
	do {						\
		if(!ONLP_OID_IS_THERMAL(_id)) {         \
			return ONLP_STATUS_E_INVALID;	\
		}                                       \
	} while(0)

static char* ipmi_devfiles__[] =  /* must map with onlp_thermal_id */
{
	NULL,
	NULL,                  /* CPU_CORE files */
	"/sys/devices/platform/as9926_24db_thermal/temp1_input",
	"/sys/devices/platform/as9926_24db_thermal/temp2_input",
	"/sys/devices/platform/as9926_24db_thermal/temp3_input",
	"/sys/devices/platform/as9926_24db_thermal/temp4_input",
	"/sys/devices/platform/as9926_24db_thermal/temp5_input",
	"/sys/devices/platform/as9926_24db_thermal/temp6_input",
	"/sys/devices/platform/as9926_24db_thermal/temp7_input",
	"/sys/devices/platform/as9926_24db_thermal/temp8_input",
	"/sys/devices/platform/as9926_24db_thermal/temp9_input",
	"/sys/devices/platform/as9926_24db_psu/psu1_temp1_input",
	"/sys/devices/platform/as9926_24db_psu/psu2_temp1_input",
};

static char* cpu_coretemp_files[] =
{
	"/sys/devices/platform/coretemp.0*temp2_input",
	"/sys/devices/platform/coretemp.0*temp3_input",
	"/sys/devices/platform/coretemp.0*temp4_input",
	"/sys/devices/platform/coretemp.0*temp5_input",
	NULL,
};

#define AS9926_THERMAL_CAPS (ONLP_THERMAL_CAPS_GET_TEMPERATURE | \
			     ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD | \
			     ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD | \
			     ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)

/* Static values */
static onlp_thermal_info_t linfo[] = {
	{ }, /* Not used */
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_CPU_CORE), "CPU Core", 0}, 
	    ONLP_THERMAL_STATUS_PRESENT,
	    AS9926_THERMAL_CAPS, 0, {83000, 93000, 103000}
	},
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_MAIN_BROAD), "LM75_1 U61", 0},
	    ONLP_THERMAL_STATUS_PRESENT,
	    AS9926_THERMAL_CAPS, 0, {71000, 76000, 81000}
	},
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_MAIN_BROAD), "LM75_2 U83", 0},
	    ONLP_THERMAL_STATUS_PRESENT,
	    AS9926_THERMAL_CAPS, 0, {73000, 78000, 83000}
	},
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_MAIN_BROAD), "LM75_3 U3", 0},
	    ONLP_THERMAL_STATUS_PRESENT,
	    AS9926_THERMAL_CAPS, 0, {78000, 83000, 88000}
	},
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_4_ON_MAIN_BROAD), 
	    "LM75_Heater_CPU U20", 0},
	    ONLP_THERMAL_STATUS_PRESENT,
	    AS9926_THERMAL_CAPS, 0, {66000, 71000, 76000}
	},
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_5_ON_MAIN_BROAD), "LM75_5 U27", 0},
	    ONLP_THERMAL_STATUS_PRESENT,
	    AS9926_THERMAL_CAPS, 0, {66000, 71000, 76000}
	},
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_6_ON_MAIN_BROAD), "LM75_6 U80", 0},
	    ONLP_THERMAL_STATUS_PRESENT,
	    AS9926_THERMAL_CAPS, 0, {56000, 61000, 66000}
	},
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_7_ON_MAIN_BROAD), "LM75_7 U64", 0},
	    ONLP_THERMAL_STATUS_PRESENT,
	    AS9926_THERMAL_CAPS, 0, {59000, 64000, 69000}
	},
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_8_ON_MAIN_BROAD), "TMP432_1 U90", 0},
	    ONLP_THERMAL_STATUS_PRESENT,
	    AS9926_THERMAL_CAPS, 0, {90000, 100000, 110000}
	},
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_9_ON_MAIN_BROAD), "TMP432_2 U90", 0},
	    ONLP_THERMAL_STATUS_PRESENT,
	    AS9926_THERMAL_CAPS, 0, {90000, 100000, 110000}
	},
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_PSU1), 
	    "PSU-1 Thermal Sensor 1", ONLP_PSU_ID_CREATE(PSU1_ID)}, 
	    ONLP_THERMAL_STATUS_PRESENT,
	    ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
	},
	{ { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_PSU2), "PSU-2 Thermal Sensor 1", 
	    ONLP_PSU_ID_CREATE(PSU2_ID)}, 
	    ONLP_THERMAL_STATUS_PRESENT,
	    ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
	}
};

/*
 * This will be called to intiialize the thermali subsystem.
 */
int onlp_thermali_init(void)
{
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
int onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* info)
{
	int   tid;
	VALIDATE(id);
	
	tid = ONLP_OID_ID_GET(id);
	
	/* Set the onlp_oid_hdr_t and capabilities */		
	*info = linfo[tid];

	if(tid == THERMAL_CPU_CORE) {
		int rv = onlp_file_read_int_max(&info->mcelsius, 
						cpu_coretemp_files);
		return rv;
	}

	return onlp_file_read_int(&info->mcelsius, ipmi_devfiles__[tid]);		    
}
