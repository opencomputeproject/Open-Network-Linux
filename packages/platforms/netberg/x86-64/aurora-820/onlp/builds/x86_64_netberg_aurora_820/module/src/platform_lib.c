/************************************************************
 * platform_lib.c
 ************************************************************
 *
 ***********************************************************/
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <AIM/aim.h>
#include <onlp/onlp.h>
#include <onlplib/file.h>
#include "platform_lib.h"



int platform_hwmon_diag_enable_read(int *enable)
{
    int rv = ONLP_STATUS_OK;
    rv = onlp_file_read_int((int*)enable, NET_INFO_PREFIX"diag");
    return rv;
}


int platform_hwmon_diag_enable_write(int enable)
{
    int rv = ONLP_STATUS_OK;
    rv = onlp_file_write_int(enable, NET_INFO_PREFIX"diag");
    return rv;
}
