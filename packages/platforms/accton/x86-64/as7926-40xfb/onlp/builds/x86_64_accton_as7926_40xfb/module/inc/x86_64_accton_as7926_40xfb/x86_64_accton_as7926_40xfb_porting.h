/**************************************************************************//**
 *
 * @file
 * @brief x86_64_accton_as7926_40xfb Porting Macros.
 *
 * @addtogroup x86_64_accton_as7926_40xfb-porting
 * @{
 *
 *****************************************************************************/
#ifndef __x86_64_accton_as7926_40xfb_PORTING_H__
#define __x86_64_accton_as7926_40xfb_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if X86_64_ACCTON_AS7926_40XFB_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef X86_64_ACCTON_AS7926_40XFB_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define X86_64_ACCTON_AS7926_40XFB_MALLOC GLOBAL_MALLOC
    #elif X86_64_ACCTON_AS7926_40XFB_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTON_AS7926_40XFB_MALLOC malloc
    #else
        #error The macro X86_64_ACCTON_AS7926_40XFB_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_ACCTON_AS7926_40XFB_FREE
    #if defined(GLOBAL_FREE)
        #define X86_64_ACCTON_AS7926_40XFB_FREE GLOBAL_FREE
    #elif X86_64_ACCTON_AS7926_40XFB_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTON_AS7926_40XFB_FREE free
    #else
        #error The macro X86_64_ACCTON_AS7926_40XFB_FREE is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_ACCTON_AS7926_40XFB_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define X86_64_ACCTON_AS7926_40XFB_MEMSET GLOBAL_MEMSET
    #elif X86_64_ACCTON_AS7926_40XFB_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTON_AS7926_40XFB_MEMSET memset
    #else
        #error The macro X86_64_ACCTON_AS7926_40XFB_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_ACCTON_AS7926_40XFB_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define X86_64_ACCTON_AS7926_40XFB_MEMCPY GLOBAL_MEMCPY
    #elif X86_64_ACCTON_AS7926_40XFB_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTON_AS7926_40XFB_MEMCPY memcpy
    #else
        #error The macro X86_64_ACCTON_AS7926_40XFB_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_ACCTON_AS7926_40XFB_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define X86_64_ACCTON_AS7926_40XFB_STRNCPY GLOBAL_STRNCPY
    #elif X86_64_ACCTON_AS7926_40XFB_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTON_AS7926_40XFB_STRNCPY strncpy
    #else
        #error The macro X86_64_ACCTON_AS7926_40XFB_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_ACCTON_AS7926_40XFB_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define X86_64_ACCTON_AS7926_40XFB_VSNPRINTF GLOBAL_VSNPRINTF
    #elif X86_64_ACCTON_AS7926_40XFB_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTON_AS7926_40XFB_VSNPRINTF vsnprintf
    #else
        #error The macro X86_64_ACCTON_AS7926_40XFB_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_ACCTON_AS7926_40XFB_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define X86_64_ACCTON_AS7926_40XFB_SNPRINTF GLOBAL_SNPRINTF
    #elif X86_64_ACCTON_AS7926_40XFB_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTON_AS7926_40XFB_SNPRINTF snprintf
    #else
        #error The macro X86_64_ACCTON_AS7926_40XFB_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_ACCTON_AS7926_40XFB_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define X86_64_ACCTON_AS7926_40XFB_STRLEN GLOBAL_STRLEN
    #elif X86_64_ACCTON_AS7926_40XFB_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTON_AS7926_40XFB_STRLEN strlen
    #else
        #error The macro X86_64_ACCTON_AS7926_40XFB_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __x86_64_accton_as7926_40xfb_PORTING_H__ */
/* @} */
