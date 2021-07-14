/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <x86_64_alphanetworks_snj60d0_320f/x86_64_alphanetworks_snj60d0_320f_config.h>

/* <auto.start.cdefs(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_HEADER).source> */
#define __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME(_x) #_x
#define __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_VALUE(_x) __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME(_x)
x86_64_alphanetworks_snj60d0_320f_config_settings_t x86_64_alphanetworks_snj60d0_320f_config_settings[] =
{
#ifdef X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_INCLUDE_LOGGING
    { __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_INCLUDE_LOGGING), __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_VALUE(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_INCLUDE_LOGGING) },
#else
{ X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_INCLUDE_LOGGING(__x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_LOG_OPTIONS_DEFAULT
    { __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_LOG_OPTIONS_DEFAULT), __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_VALUE(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_LOG_OPTIONS_DEFAULT) },
#else
{ X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_LOG_OPTIONS_DEFAULT(__x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_LOG_BITS_DEFAULT
    { __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_LOG_BITS_DEFAULT), __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_VALUE(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_LOG_BITS_DEFAULT) },
#else
{ X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_LOG_BITS_DEFAULT(__x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_LOG_CUSTOM_BITS_DEFAULT
    { __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_LOG_CUSTOM_BITS_DEFAULT), __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_VALUE(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_LOG_CUSTOM_BITS_DEFAULT) },
#else
{ X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_LOG_CUSTOM_BITS_DEFAULT(__x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_PORTING_STDLIB
    { __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_PORTING_STDLIB), __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_VALUE(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_PORTING_STDLIB) },
#else
{ X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_PORTING_STDLIB(__x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS
    { __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS), __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_VALUE(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS) },
#else
{ X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS(__x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME), "__undefined__" },
#endif
#ifdef X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_INCLUDE_UCLI
    { __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_INCLUDE_UCLI), __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_VALUE(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_INCLUDE_UCLI) },
#else
{ X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_INCLUDE_UCLI(__x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME), "__undefined__" },
#endif
    { NULL, NULL }
};
#undef __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_VALUE
#undef __x86_64_alphanetworks_snj60d0_320f_config_STRINGIFY_NAME

const char*
x86_64_alphanetworks_snj60d0_320f_config_lookup(const char* setting)
{
    int i;
    for(i = 0; x86_64_alphanetworks_snj60d0_320f_config_settings[i].name; i++) {
        if(strcmp(x86_64_alphanetworks_snj60d0_320f_config_settings[i].name, setting)) {
            return x86_64_alphanetworks_snj60d0_320f_config_settings[i].value;
        }
    }
    return NULL;
}

int
x86_64_alphanetworks_snj60d0_320f_config_show(struct aim_pvs_s* pvs)
{
    int i;
    for(i = 0; x86_64_alphanetworks_snj60d0_320f_config_settings[i].name; i++) {
        aim_printf(pvs, "%s = %s\n", x86_64_alphanetworks_snj60d0_320f_config_settings[i].name, x86_64_alphanetworks_snj60d0_320f_config_settings[i].value);
    }
    return i;
}

/* <auto.end.cdefs(X86_64_ALPHANETWORKS_SNJ60D0_320F_CONFIG_HEADER).source> */

