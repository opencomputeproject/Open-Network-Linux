#include <onlp/platformi/chassisi.h>
#include "platform_lib.h"

int
onlp_chassisi_hdr_get(onlp_oid_id_t id, onlp_oid_hdr_t* hdr)
{
    int i;
    onlp_oid_t* e = hdr->coids;

    ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
    ONLP_OID_STATUS_FLAG_SET(hdr, OPERATIONAL);

    /* THERMALs Item */
    for (i=1; i<=CHASSIS_THERMAL_COUNT; i++)
    {
        *e++ = ONLP_THERMAL_ID_CREATE(i);
    }

    /* LEDs Item */
    for (i=1; i<=CHASSIS_LED_COUNT; i++)
    {
        *e++ = ONLP_LED_ID_CREATE(i);
    }
    
     /* 2 PSUs on the chassis */
    for (i = 1; i <= CHASSIS_PSU_COUNT; i++)
    {
        *e++ = ONLP_PSU_ID_CREATE(i);
    }

    /* Fans Item */
    for (i=1; i<=CHASSIS_FAN_COUNT; i++)
    {
        *e++ = ONLP_FAN_ID_CREATE(i);
    }

    /* 34 Ports */
    for(i = 1; i <= 34; i++) {
        *e++ = ONLP_SFP_ID_CREATE(i);
    }

    return ONLP_STATUS_OK;
}
int onlp_chassisi_info_get(onlp_oid_id_t id, onlp_chassis_info_t* info)
{
    onlp_chassisi_hdr_get(id, &info->hdr);
    return ONLP_STATUS_OK;
}
