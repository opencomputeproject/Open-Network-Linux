/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
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
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <onlplib/file.h>
#include <onlp/platformi/fani.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/psui.h>
#include <onlp/platformi/sysi.h>
#include <onlp/platformi/thermali.h>
#include "platform_lib.h"
#include <mlnx_common/mlnx_common.h>
#include "x86_64_mlnx_msn4700_int.h"
#include "x86_64_mlnx_msn4700_log.h"

static const char* __ONL_PLATFORM_NAME = NULL;

int mc_get_platform_info(mlnx_platform_info_t* mlnx_platform)
{
	if (!__ONL_PLATFORM_NAME) {
		aim_strlcpy(mlnx_platform->onl_platform_name, "x86-64-mlnx-msn4700-all", PLATFORM_NAME_MAX_LEN);
	}
	else {
		aim_strlcpy(mlnx_platform->onl_platform_name, __ONL_PLATFORM_NAME, PLATFORM_NAME_MAX_LEN);
	}

	mlnx_platform->led_num = CHASSIS_LED_COUNT;
	mlnx_platform->fan_num = CHASSIS_FAN_COUNT;
	mlnx_platform->thermal_num = CHASSIS_THERMAL_COUNT;
	mlnx_platform->sfp_num = SFP_PORT_COUNT;
	mlnx_platform->psu_num = CHASSIS_PSU_COUNT;
	mlnx_platform->cpld_num = CPLD_COUNT;
	mlnx_platform->psu_fixed = false;
	mlnx_platform->fan_fixed = false;
	mlnx_platform->psu_type = PSU_TYPE_3;
	mlnx_platform->led_type = LED_TYPE_3;

	return ONLP_STATUS_OK;
}

int
onlp_sysi_platform_set(const char* platform)
{
    mlnx_platform_info_t* mlnx_platform;

    if(!strcmp(platform, "x86-64-mlnx-msn4700-r0")) {
        __ONL_PLATFORM_NAME = "x86-64-mlnx_msn4700-r0";
        mlnx_platform = get_platform_info();
        mc_get_platform_info(mlnx_platform);
        return ONLP_STATUS_OK;
    } else if(!strcmp(platform, "x86-64-mlnx-msn4410-r0")) {
        __ONL_PLATFORM_NAME = "x86-64-mlnx_msn4410-r0";
        mlnx_platform = get_platform_info();
        mc_get_platform_info(mlnx_platform);
        return ONLP_STATUS_OK;
    } else if(!strcmp(platform, "x86-64-mlnx-msn4700-all")) {
        __ONL_PLATFORM_NAME = "x86-64-mlnx-msn4700-all";
        return ONLP_STATUS_OK;
    }
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_sysi_init(void)
{
    return ONLP_STATUS_OK;
}

