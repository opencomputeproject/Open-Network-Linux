#ifndef __NET_DEF_H
#define __NET_DEF_H

/*customized definition*/

#define DEFAULT_CLTE (4)
/* <TBD> use enum to define reply code*/
/*use define temporary*/
#define REC_SFF_IO_UNSUPPORTED (0x06)

typedef enum {
    PLATFORM_NBA820,
    PLATFORM_END,
} platform_id_t;

#define PLATFORM_ID (PLATFORM_NBA820)
#define SWPS_VERSION ("SWPS_GA_v0.0.23")

#endif /*__NET_DEF_H*/
