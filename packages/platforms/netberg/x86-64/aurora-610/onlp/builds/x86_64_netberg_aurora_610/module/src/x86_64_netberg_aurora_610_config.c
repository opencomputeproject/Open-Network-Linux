/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <x86_64_netberg_aurora_610/x86_64_netberg_aurora_610_config.h>

/* <auto.start.cdefs(x86_64_netberg_aurora_610_CONFIG_HEADER).source> */
#define __x86_64_netberg_aurora_610_config_STRINGIFY_NAME(_x) #_x
#define __x86_64_netberg_aurora_610_config_STRINGIFY_VALUE(_x) __x86_64_netberg_aurora_610_config_STRINGIFY_NAME(_x)
x86_64_netberg_aurora_610_config_settings_t x86_64_netberg_aurora_610_config_settings[] =
{
#ifdef x86_64_netberg_aurora_610_CONFIG_INCLUDE_LOGGING
    { __x86_64_netberg_aurora_610_config_STRINGIFY_NAME(x86_64_netberg_aurora_610_CONFIG_INCLUDE_LOGGING), __x86_64_netberg_aurora_610_config_STRINGIFY_VALUE(x86_64_netberg_aurora_610_CONFIG_INCLUDE_LOGGING) },
#else
{ x86_64_netberg_aurora_610_CONFIG_INCLUDE_LOGGING(__x86_64_netberg_aurora_610_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef x86_64_netberg_aurora_610_CONFIG_LOG_OPTIONS_DEFAULT
    { __x86_64_netberg_aurora_610_config_STRINGIFY_NAME(x86_64_netberg_aurora_610_CONFIG_LOG_OPTIONS_DEFAULT), __x86_64_netberg_aurora_610_config_STRINGIFY_VALUE(x86_64_netberg_aurora_610_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ x86_64_netberg_aurora_610_CONFIG_LOG_OPTIONS_DEFAULT(__x86_64_netberg_aurora_610_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef x86_64_netberg_aurora_610_CONFIG_LOG_BITS_DEFAULT
    { __x86_64_netberg_aurora_610_config_STRINGIFY_NAME(x86_64_netberg_aurora_610_CONFIG_LOG_BITS_DEFAULT), __x86_64_netberg_aurora_610_config_STRINGIFY_VALUE(x86_64_netberg_aurora_610_CONFIG_LOG_BITS_DEFAULT) },
#else
{ x86_64_netberg_aurora_610_CONFIG_LOG_BITS_DEFAULT(__x86_64_netberg_aurora_610_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef x86_64_netberg_aurora_610_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __x86_64_netberg_aurora_610_config_STRINGIFY_NAME(x86_64_netberg_aurora_610_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __x86_64_netberg_aurora_610_config_STRINGIFY_VALUE(x86_64_netberg_aurora_610_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ x86_64_netberg_aurora_610_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__x86_64_netberg_aurora_610_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef x86_64_netberg_aurora_610_CONFIG_PORTING_STDLIB
    { __x86_64_netberg_aurora_610_config_STRINGIFY_NAME(x86_64_netberg_aurora_610_CONFIG_PORTING_STDLIB), __x86_64_netberg_aurora_610_config_STRINGIFY_VALUE(x86_64_netberg_aurora_610_CONFIG_PORTING_STDLIB) },
#else
{ x86_64_netberg_aurora_610_CONFIG_PORTING_STDLIB(__x86_64_netberg_aurora_610_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef x86_64_netberg_aurora_610_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __x86_64_netberg_aurora_610_config_STRINGIFY_NAME(x86_64_netberg_aurora_610_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __x86_64_netberg_aurora_610_config_STRINGIFY_VALUE(x86_64_netberg_aurora_610_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ x86_64_netberg_aurora_610_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__x86_64_netberg_aurora_610_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef x86_64_netberg_aurora_610_CONFIG_INCLUDE_UCLI
    { __x86_64_netberg_aurora_610_config_STRINGIFY_NAME(x86_64_netberg_aurora_610_CONFIG_INCLUDE_UCLI), __x86_64_netberg_aurora_610_config_STRINGIFY_VALUE(x86_64_netberg_aurora_610_CONFIG_INCLUDE_UCLI) },
#else
{ x86_64_netberg_aurora_610_CONFIG_INCLUDE_UCLI(__x86_64_netberg_aurora_610_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef x86_64_netberg_aurora_610_CONFIG_INCLUDE_DEFAULT_FAN_DIRECTION
    { __x86_64_netberg_aurora_610_config_STRINGIFY_NAME(x86_64_netberg_aurora_610_CONFIG_INCLUDE_DEFAULT_FAN_DIRECTION), __x86_64_netberg_aurora_610_config_STRINGIFY_VALUE(x86_64_netberg_aurora_610_CONFIG_INCLUDE_DEFAULT_FAN_DIRECTION) },
#else
{ x86_64_netberg_aurora_610_CONFIG_INCLUDE_DEFAULT_FAN_DIRECTION(__x86_64_netberg_aurora_610_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __x86_64_netberg_aurora_610_config_STRINGIFY_VALUE
#undef __x86_64_netberg_aurora_610_config_STRINGIFY_NAME

const char*
x86_64_netberg_aurora_610_config_lookup(const char* setting)
{
    int i;
    for(i = 0; x86_64_netberg_aurora_610_config_settings[i].name; i++) {
        if(strcmp(x86_64_netberg_aurora_610_config_settings[i].name, setting)) {
            return x86_64_netberg_aurora_610_config_settings[i].value;
        }
    }
    return NULL;
}

int
x86_64_netberg_aurora_610_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; x86_64_netberg_aurora_610_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", x86_64_netberg_aurora_610_config_settings[i].name, x86_64_netberg_aurora_610_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(x86_64_netberg_aurora_610_CONFIG_HEADER).source> */

