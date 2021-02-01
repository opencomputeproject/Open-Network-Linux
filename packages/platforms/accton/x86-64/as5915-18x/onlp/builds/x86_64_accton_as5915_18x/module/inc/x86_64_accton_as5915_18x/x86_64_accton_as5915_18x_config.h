/**************************************************************************//**
 *
 * @file
 * @brief x86_64_accton_as5915_18x Configuration Header
 *
 * @addtogroup x86_64_accton_as5915_18x-config
 * @{
 *
 *****************************************************************************/
#ifndef __x86_64_accton_as5915_18x_CONFIG_H__
#define __x86_64_accton_as5915_18x_CONFIG_H__

#ifdef GLOBAL_INCLUDE_CUSTOM_CONFIG
#include <global_custom_config.h>
#endif
#ifdef x86_64_accton_as5915_18x_INCLUDE_CUSTOM_CONFIG
#include <x86_64_accton_as5915_18x_custom_config.h>
#endif

/* <auto.start.cdefs(x86_64_accton_as5915_18x_CONFIG_HEADER).header> */
#include <AIM/aim.h>
/**
 * X86_64_ACCTON_AS5915_18X_CONFIG_INCLUDE_LOGGING
 *
 * Include or exclude logging. */


#ifndef X86_64_ACCTON_AS5915_18X_CONFIG_INCLUDE_LOGGING
#define X86_64_ACCTON_AS5915_18X_CONFIG_INCLUDE_LOGGING 1
#endif

/**
 * X86_64_ACCTON_AS5915_18X_CONFIG_LOG_OPTIONS_DEFAULT
 *
 * Default enabled log options. */


#ifndef X86_64_ACCTON_AS5915_18X_CONFIG_LOG_OPTIONS_DEFAULT
#define X86_64_ACCTON_AS5915_18X_CONFIG_LOG_OPTIONS_DEFAULT AIM_LOG_OPTIONS_DEFAULT
#endif

/**
 * X86_64_ACCTON_AS5915_18X_CONFIG_LOG_BITS_DEFAULT
 *
 * Default enabled log bits. */


#ifndef X86_64_ACCTON_AS5915_18X_CONFIG_LOG_BITS_DEFAULT
#define X86_64_ACCTON_AS5915_18X_CONFIG_LOG_BITS_DEFAULT AIM_LOG_BITS_DEFAULT
#endif

/**
 * X86_64_ACCTON_AS5915_18X_CONFIG_LOG_CUSTOM_BITS_DEFAULT
 *
 * Default enabled custom log bits. */


#ifndef X86_64_ACCTON_AS5915_18X_CONFIG_LOG_CUSTOM_BITS_DEFAULT
#define X86_64_ACCTON_AS5915_18X_CONFIG_LOG_CUSTOM_BITS_DEFAULT 0
#endif

/**
 * X86_64_ACCTON_AS5915_18X_CONFIG_PORTING_STDLIB
 *
 * Default all porting macros to use the C standard libraries. */


#ifndef X86_64_ACCTON_AS5915_18X_CONFIG_PORTING_STDLIB
#define X86_64_ACCTON_AS5915_18X_CONFIG_PORTING_STDLIB 1
#endif

/**
 * X86_64_ACCTON_AS5915_18X_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
 *
 * Include standard library headers for stdlib porting macros. */


#ifndef X86_64_ACCTON_AS5915_18X_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
#define X86_64_ACCTON_AS5915_18X_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS x86_64_accton_as5915_18x_CONFIG_PORTING_STDLIB
#endif

/**
 * X86_64_ACCTON_AS5915_18X_CONFIG_INCLUDE_UCLI
 *
 * Include generic uCli support. */


#ifndef X86_64_ACCTON_AS5915_18X_CONFIG_INCLUDE_UCLI
#define X86_64_ACCTON_AS5915_18X_CONFIG_INCLUDE_UCLI 0
#endif

/**
 * X86_64_ACCTON_AS5915_18X_CONFIG_INCLUDE_DEFAULT_FAN_DIRECTION
 *
 * Assume chassis fan direction is the same as the PSU fan direction. */


#ifndef X86_64_ACCTON_AS5915_18X_CONFIG_INCLUDE_DEFAULT_FAN_DIRECTION
#define X86_64_ACCTON_AS5915_18X_CONFIG_INCLUDE_DEFAULT_FAN_DIRECTION 0
#endif



/**
 * All compile time options can be queried or displayed
 */

/** Configuration settings structure. */
typedef struct x86_64_accton_as5915_18x_config_settings_s {
    /** name */
    const char* name;
    /** value */
    const char* value;
} x86_64_accton_as5915_18x_config_settings_t;

/** Configuration settings table. */
/** x86_64_accton_as5915_18x_config_settings table. */
extern x86_64_accton_as5915_18x_config_settings_t x86_64_accton_as5915_18x_config_settings[];

/**
 * @brief Lookup a configuration setting.
 * @param setting The name of the configuration option to lookup.
 */
const char* x86_64_accton_as5915_18x_config_lookup(const char* setting);

/**
 * @brief Show the compile-time configuration.
 * @param pvs The output stream.
 */
int x86_64_accton_as5915_18x_config_show(struct aim_pvs_s* pvs);

/* <auto.end.cdefs(x86_64_accton_as5915_18x_CONFIG_HEADER).header> */

#include "x86_64_accton_as5915_18x_porting.h"

#endif /* __x86_64_accton_as5915_18x_CONFIG_H__ */
/* @} */
