#ifndef __INV_DEF_H
#define __INV_DEF_H

/*customized definition*/

#define DEFAULT_CLTE (4)

#define RD_RELEASE (0)
#define OFFICIAL_RELEASE (1)
#define RELEASE_TYPE (OFFICIAL_RELEASE)

typedef enum {

    PLATFORM_MAPLE,
    PLATFORM_BANYAN,
    PLATFORM_4U,
    PLATFORM_BANYAN_8T_SKU1,
    PLATFORM_BANYAN_8T_SKU2,
    PLATFORM_BOCELLI,
    PLATFORM_END,

} platform_id_t;

#define PLATFORM_ID (PLATFORM_BANYAN)
#define SWPS_VERSION ("SWPS_GA_v0.1.0")

#endif /*__INV_DEF_H*/
