/**************************************************************************//**
 *
 * @file
 * @brief mlnx_common Main Interface Header
 *
 * @addtogroup mlnx_common
 * @{
 *
 *****************************************************************************/
#ifndef __MLNX_COMMON_H__
#define __MLNX_COMMON_H__

#include <onlp/platformi/thermali.h>
#include <onlp/platformi/ledi.h>
#include <onlp/fan.h>
#include <onlp/psu.h>
#include <onlp/led.h>

#define PLATFORM_NAME_MAX_LEN		64

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif

#define LED_TYPE_1 1
#define LED_TYPE_2 2
#define LED_TYPE_3 3

/* led common id */
#define LED_RESERVED 0
#define LED_SYSTEM 1
/*led type 1 id */
#define LED_FAN1 2
#define LED_FAN2 3
#define LED_FAN3 4
#define LED_FAN4 5
#define LED_PSU 6
/*led type 2 id */
#define LED_FAN 2
#define LED_PSU1 3
#define LED_PSU2 4
#define LED_UID 5

/*led type 3 id */
#define LED_FAN5 6
#define LED_FAN6 7
#define LED_PSU_T3 8

#define PERCENTAGE_MIN 60.0
#define PERCENTAGE_MAX 100.0
#define RPM_MAGIC_MIN  153.0
#define RPM_MAGIC_MAX  255.0

#define PSU_FAN_RPM_MIN 11700.0
#define PSU_FAN_RPM_MAX 19500.0

#define PROJECT_NAME
#define LEN_FILE_NAME 80

/* 1 -without eeprom, 2 - with eeprom */
#define PSU_TYPE_1 1
#define PSU_TYPE_2 2
#define PSU_TYPE_3 3

#define FAN_MODEL	"MEC012579"

#define FAN_TYPE_NO_EEPROM 1
#define FAN_TYPE_EEPROM 2

#define _MAKE_FAN_PATH_ON_MAIN_BOARD(prj,id) \
    { #prj"fan"#id"_status", \
      #prj"fan"#id"_speed_get", \
      #prj"fan"#id"_speed_set", \
      #prj"fan"#id"_min", \
      #prj"fan"#id"_max" }

#define MAKE_FAN_PATH_ON_MAIN_BOARD(prj,id) _MAKE_FAN_PATH_ON_MAIN_BOARD(prj,id)

#define MAKE_FAN_PATH_ON_PSU(psu_id, fan_id) \
    {"psu"#psu_id"_status", \
     "psu"#psu_id"_fan"#fan_id"_speed_get", "", "", "",}

#define MAKE_FAN_INFO_NODE_ON_MAIN_BOARD(id) \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##id##_ON_MAIN_BOARD), "Chassis Fan "#id, 0 }, \
        0x0, \
        (ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_PERCENTAGE | \
         ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_SET_RPM), \
        0, \
        0, \
        ONLP_FAN_MODE_INVALID, \
    }

#define MAKE_FAN_INFO_NODE_ON_PSU(psu_id, fan_id) \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##fan_id##_ON_PSU##psu_id), "Chassis PSU-"#psu_id" Fan "#fan_id, 0 }, \
        0x0, \
        (ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE), \
        0, \
        0, \
        ONLP_FAN_MODE_INVALID, \
    }

typedef struct fan_path_S
{
    char status[LEN_FILE_NAME];
    char r_speed_get[LEN_FILE_NAME];
    char r_speed_set[LEN_FILE_NAME];
    char min[LEN_FILE_NAME];
    char max[LEN_FILE_NAME];
} fan_path_T;

/** Specific platform info structure. */
typedef struct mlnx_platform_info_s {
    char onl_platform_name[PLATFORM_NAME_MAX_LEN];
    int sfp_num;
    int led_num;
    int psu_num;
    int fan_num;
    int thermal_num;
    int cpld_num;
    bool psu_fixed;
    bool fan_fixed;
    int* min_fan_speed;
    int* max_fan_speed;
    onlp_thermal_info_t* tinfo;
    char** thermal_fnames;
    onlp_led_info_t* linfo;
    char** led_fnames;
    int psu_type;
    int led_type;
    onlp_fan_info_t* finfo;
    fan_path_T* fan_fnames;
    int fan_type;
    int fan_per_module;
    int first_psu_fan_id;
} mlnx_platform_info_t;

#define PSU1_ID 1
#define PSU2_ID 2
#define HW_MANAGEMENT_PATH "/var/run/hw-management"
#define SFP_PATH HW_MANAGEMENT_PATH"/sfp"
#define THERMAL_PATH "/var/run/hw-management/thermal"
#define PSU_MODULE_PREFIX THERMAL_PATH"/psu%d_%s"
#define PSU_POWER_PREFIX HW_MANAGEMENT_PATH"/power/psu%d_%s"
#define IDPROM_PATH HW_MANAGEMENT_PATH"/eeprom/%s%d_info"
#define PSU_VPD_PATH HW_MANAGEMENT_PATH"/eeprom/%s%d_vpd"

typedef enum psu_type {
    PSU_TYPE_UNKNOWN,
    PSU_TYPE_AC_F2B,
    PSU_TYPE_AC_B2F
} psu_type_t;

/* CPU thermal_threshold */
typedef enum cpu_thermal_threshold_e {
    CPU_THERMAL_THRESHOLD_WARNING_DEFAULT  = 87000,
    CPU_THERMAL_THRESHOLD_ERROR_DEFAULT    = 100000,
    CPU_THERMAL_THRESHOLD_SHUTDOWN_DEFAULT = 105000,
} cpu_thermal_threshold_t;

/* Shortcut for CPU thermal threshold value. */
#define CPU_THERMAL_THRESHOLD_INIT_DEFAULTS  \
    { CPU_THERMAL_THRESHOLD_WARNING_DEFAULT, \
      CPU_THERMAL_THRESHOLD_ERROR_DEFAULT,   \
      CPU_THERMAL_THRESHOLD_SHUTDOWN_DEFAULT }

/* Asic thermal_threshold */
typedef enum asic_thermal_threshold_e {
    ASIC_THERMAL_THRESHOLD_WARNING_DEFAULT  = 105000,
    ASIC_THERMAL_THRESHOLD_ERROR_DEFAULT    = 115000,
    ASIC_THERMAL_THRESHOLD_SHUTDOWN_DEFAULT = 120000,
} asic_thermal_threshold_t;

/* Shortcut for CPU thermal threshold value. */
#define ASIC_THERMAL_THRESHOLD_INIT_DEFAULTS  \
    { ASIC_THERMAL_THRESHOLD_WARNING_DEFAULT, \
      ASIC_THERMAL_THRESHOLD_ERROR_DEFAULT,   \
      ASIC_THERMAL_THRESHOLD_SHUTDOWN_DEFAULT }

int mc_get_kernel_ver(void);

int mc_get_platform_info(mlnx_platform_info_t* mlnx_platform);

int onlp_fani_get_min_rpm(int id);

int psu_fan_read_eeprom(int psu_index, onlp_fan_info_t* fan_info);

int psu_fan_read_eeprom_vpd(int psu_index, onlp_fan_info_t* fan_info);

int psu_read_eeprom(int psu_index, onlp_psu_info_t* psu_info);

mlnx_platform_info_t* get_platform_info();

psu_type_t get_psu_type(int id, char* modelname, int modelname_len);

#endif /* __MLNX_COMMON_H__ */
/* @} */
