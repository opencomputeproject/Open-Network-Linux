/**************************************************************************//**
 *
 * @file
 * @brief x86_64_accton_as9716_32d Porting Macros.
 *
 * @addtogroup x86_64_accton_as9716_32d-porting
 * @{
 *
 *****************************************************************************/
#ifndef __x86_64_accton_as9716_32d_PORTING_H__
#define __x86_64_accton_as9716_32d_PORTING_H__


/* <auto.start.portingmacro(ALL).define> */
#if X86_64_ACCTON_AS9716_32D_CONFIG_PORTING_INCLUDE_STDLIB_HEADERS == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#endif

#ifndef X86_64_ACCTON_AS9716_32D_MALLOC
    #if defined(GLOBAL_MALLOC)
        #define X86_64_ACCTON_AS9716_32D_MALLOC GLOBAL_MALLOC
    #elif X86_64_ACCTON_AS9716_32D_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTON_AS9716_32D_MALLOC malloc
    #else
        #error The macro X86_64_ACCTON_AS9716_32D_MALLOC is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_ACCTON_AS9716_32D_FREE
    #if defined(GLOBAL_FREE)
        #define X86_64_ACCTON_AS9716_32D_FREE GLOBAL_FREE
    #elif X86_64_ACCTON_AS9716_32D_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTON_AS9716_32D_FREE free
    #else
        #error The macro X86_64_ACCTON_AS9716_32D_FREE is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_ACCTON_AS9716_32D_MEMSET
    #if defined(GLOBAL_MEMSET)
        #define X86_64_ACCTON_AS9716_32D_MEMSET GLOBAL_MEMSET
    #elif X86_64_ACCTON_AS9716_32D_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTON_AS9716_32D_MEMSET memset
    #else
        #error The macro X86_64_ACCTON_AS9716_32D_MEMSET is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_ACCTON_AS9716_32D_MEMCPY
    #if defined(GLOBAL_MEMCPY)
        #define X86_64_ACCTON_AS9716_32D_MEMCPY GLOBAL_MEMCPY
    #elif X86_64_ACCTON_AS9716_32D_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTON_AS9716_32D_MEMCPY memcpy
    #else
        #error The macro X86_64_ACCTON_AS9716_32D_MEMCPY is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_ACCTON_AS9716_32D_STRNCPY
    #if defined(GLOBAL_STRNCPY)
        #define X86_64_ACCTON_AS9716_32D_STRNCPY GLOBAL_STRNCPY
    #elif X86_64_ACCTON_AS9716_32D_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTON_AS9716_32D_STRNCPY strncpy
    #else
        #error The macro X86_64_ACCTON_AS9716_32D_STRNCPY is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_ACCTON_AS9716_32D_VSNPRINTF
    #if defined(GLOBAL_VSNPRINTF)
        #define X86_64_ACCTON_AS9716_32D_VSNPRINTF GLOBAL_VSNPRINTF
    #elif X86_64_ACCTON_AS9716_32D_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTON_AS9716_32D_VSNPRINTF vsnprintf
    #else
        #error The macro X86_64_ACCTON_AS9716_32D_VSNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_ACCTON_AS9716_32D_SNPRINTF
    #if defined(GLOBAL_SNPRINTF)
        #define X86_64_ACCTON_AS9716_32D_SNPRINTF GLOBAL_SNPRINTF
    #elif X86_64_ACCTON_AS9716_32D_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTON_AS9716_32D_SNPRINTF snprintf
    #else
        #error The macro X86_64_ACCTON_AS9716_32D_SNPRINTF is required but cannot be defined.
    #endif
#endif

#ifndef X86_64_ACCTON_AS9716_32D_STRLEN
    #if defined(GLOBAL_STRLEN)
        #define X86_64_ACCTON_AS9716_32D_STRLEN GLOBAL_STRLEN
    #elif X86_64_ACCTON_AS9716_32D_CONFIG_PORTING_STDLIB == 1
        #define X86_64_ACCTONAS9716_32D_STRLEN strlen
    #else
        #error The macro X86_64_ACCTON_AS7726_32X_STRLEN is required but cannot be defined.
    #endif
#endif

/* <auto.end.portingmacro(ALL).define> */


#endif /* __x86_64_accton_as9716_32d_PORTING_H__ */
/* @} */
