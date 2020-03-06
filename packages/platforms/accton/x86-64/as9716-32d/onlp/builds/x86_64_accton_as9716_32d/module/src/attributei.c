#include <onlp/platformi/attributei.h>
#include <onlp/stdattrs.h>
#include <onlplib/file.h>
#include "platform_lib.h"

int
onlp_attributei_onie_info_get(onlp_oid_t oid, onlp_onie_info_t* rp)
{
    int rv;
    
    if(oid != ONLP_OID_CHASSIS) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }

    if(rp == NULL) {
        return ONLP_STATUS_OK;
    }
    rv=onlp_onie_decode_file(rp, IDPROM_PATH_1);
    if (rv!=ONLP_STATUS_OK)
    {
        return onlp_onie_decode_file(rp, IDPROM_PATH_2);
        
    }
    else
        return rv;
    
}

int
onlp_attributei_asset_info_get(onlp_oid_t oid, onlp_asset_info_t* rp)
{
    int rv;
    int v[3] = {0};
    
    if(oid != ONLP_OID_CHASSIS) {
        return ONLP_STATUS_E_UNSUPPORTED;
    }
    if(rp == NULL) {
        return 1;
    }
      
    rp->oid = oid;
    rp->manufacturer = aim_strdup("Accton");
    if(ONLP_SUCCESS(rv = onlp_file_read_int(v, "/sys/bus/i2c/devices/19-0060/version")) &&
       ONLP_SUCCESS(rv = onlp_file_read_int(v+1, "/sys/bus/i2c/devices/20-0061/version")) &&
       ONLP_SUCCESS(rv = onlp_file_read_int(v+2, "/sys/bus/i2c/devices/21-0062/version"))) {
        rp->firmware_revision =
            aim_fstrdup("%d.%d.%d", v[0], v[1], v[2]);
    }

    return rv;
}
