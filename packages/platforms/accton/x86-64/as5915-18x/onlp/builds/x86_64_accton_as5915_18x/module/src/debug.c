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
#include "x86_64_accton_as5915_18x_int.h"

#if x86_64_accton_as5915_18x_CONFIG_INCLUDE_DEBUG == 1

#include <unistd.h>

static char help__[] =
    "Usage: debug [options]\n"
    "    -c CPLD Versions\n"
    "    -h Help\n"
    ;

int
x86_64_accton_as5915_18x_debug_main(int argc, char* argv[])
{
    int c = 0;
    int help = 0;
    int rv = 0;

    while( (c = getopt(argc, argv, "ch")) != -1) {
        switch(c)
            {
            case 'c': c = 1; break;
            case 'h': help = 1; rv = 0; break;
            default: help = 1; rv = 1; break;
            }

    }

    if(help || argc == 1) {
        printf("%s", help__);
        return rv;
    }

    if(c) {
        printf("Not implemented.\n");
    }


    return 0;
}

#endif
