/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *        Copyright 2014, 2015 Big Switch Networks, Inc.
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

/**************************************************************************//**
 *
 * @file
 * @brief x86_64_ufispace_s9700_23d Configuration Header
 *
 * @addtogroup x86_64_ufispace_s9700_23d-config
 * @{
 *
 *****************************************************************************/
#ifndef __X86_64_UFISPACE_S9700_23D_CONFIG_H__
#define __X86_64_UFISPACE_S9700_23D_CONFIG_H__

#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef X86_64_UFISPACE_S9700_23D_INCLUDE_CUSTOM_CONFIG
#include <x86_64_ufispace_s9700_23d_custom_config.h>
#endif

/* <auto.start.cdefs(X86_64_UFISPACE_S9700_23D_CONFIG_HEADER).header> */
#include <AIM/aim.h>
/**
 * X86_64_UFISPACE_S9700_23D_CONFIG_INCLUDE_LOGGING
 *
 * Include or exclude logging. */


#ifndef X86_64_UFISPACE_S9700_23D_CONFIG_INCLUDE_LOGGING
#define X86_64_UFISPACE_S9700_23D_CONFIG_INCLUDE_LOGGING 1
#endif

/**
 * X86_64_UFISPACE_S9700_23D_CONFIG_LOG_OPTIONS_DEFAULT
 *
 * Default enabled log options. */


#ifndef X86_64_UFISPACE_S9700_23D_CONFIG_LOG_OPTIONS_DEFAULT
#define X86_64_UFISPACE_S9700_23D_CONFIG_LOG_OPTIONS_DEFAULT AIM_LOG_OPTIONS_DEFAULT
#endif

/**
 * X86_64_UFISPACE_S9700_23D_CONFIG_LOG_BITS_DEFAULT
 *
 * Default enabled log bits. */


#ifndef X86_64_UFISPACE_S9700_23D_CONFIG_LOG_BITS_DEFAULT
#define X86_64_UFISPACE_S9700_23D_CONFIG_LOG_BITS_DEFAULT AIM_LOG_BITS_DEFAULT
#endif

/**
 * X86_64_UFISPACE_S9700_23D_CONFIG_LOG_CUSTOM_BITS_DEFAULT
 *
 * Default enabled custom log bits. */


#ifndef X86_64_UFISPACE_S9700_23D_CONFIG_LOG_CUSTOM_BITS_DEFAULT
#define X86_64_UFISPACE_S9700_23D_CONFIG_LOG_CUSTOM_BITS_DEFAULT 0
#endif

/**
 * X86_64_UFISPACE_S9700_23D_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */


#ifndef X86_64_UFISPACE_S9700_23D_CONFIG_PORTING_STDLIB
#define X86_64_UFISPACE_S9700_23D_CONFIG_PORTING_STDLIB 1
#endif

/**
 * X86_64_UFISPACE_S9700_23D_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */


#ifndef X86_64_UFISPACE_S9700_23D_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define X86_64_UFISPACE_S9700_23D_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS X86_64_UFISPACE_S9700_23D_CONFIG_PORTING_STDLIB
#endif

/**
 * X86_64_UFISPACE_S9700_23D_CONFIG_INCLUDE_UCLI
 *
 * Include generic uCli support. */


#ifndef X86_64_UFISPACE_S9700_23D_CONFIG_INCLUDE_UCLI
#define X86_64_UFISPACE_S9700_23D_CONFIG_INCLUDE_UCLI 0
#endif



/**
 * All compile time options can be queried or displayed
 */

/** Configuration settings structure. */
typedef struct x86_64_ufispace_s9700_23d_config_settings_s {
    /** name */
    const char* name;
    /** value */
    const char* value;
} x86_64_ufispace_s9700_23d_config_settings_t;

/** Configuration settings table. */
/** x86_64_ufispace_s9700_23d_config_settings table. */
extern x86_64_ufispace_s9700_23d_config_settings_t x86_64_ufispace_s9700_23d_config_settings[];

/**
 * @brief Lookup a configuration setting.
 * @param setting The name of the configuration option to lookup.
 */
const char* x86_64_ufispace_s9700_23d_config_lookup(const char* setting);

/**
 * @brief Show the compile-time configuration.
 * @param pvs The output stream.
 */
int x86_64_ufispace_s9700_23d_config_show(struct aim_pvs_s* pvs);

/* <auto.end.cdefs(X86_64_UFISPACE_S9700_23D_CONFIG_HEADER).header> */

#include "x86_64_ufispace_s9700_23d_porting.h"

#endif /* __X86_64_UFISPACE_S9700_23D_CONFIG_H__ */
/* @} */
