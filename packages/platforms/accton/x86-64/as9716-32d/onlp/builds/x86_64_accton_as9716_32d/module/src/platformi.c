
#include <onlp/platformi/base.h>
#include "platform_lib.h"

#include "x86_64_accton_as9716_32d_int.h"
#include "x86_64_accton_as9716_32d_log.h"


#define NUM_OF_FAN_ON_MAIN_BROAD   6
#define PREFIX_PATH_ON_CPLD_DEV    "/sys/bus/i2c/devices/"
#define NUM_OF_CPLD                3
#define FAN_DUTY_CYCLE_MAX         (100)
#define FAN_DUTY_CYCLE_75          (75)
#define FAN_DUTY_CYCLE_50          (50)

const char*
onlp_platformi_get(void)
{
    return "x86-64-accton-as9716-32d-r0";
}

int
onlp_platformi_sw_init(void)
{
    return ONLP_STATUS_OK;
}


/*Read fanN_direction=1: The air flow of Fan6 is ¡§AFI-Back to Front¡¨
 *                    0: The air flow of Fan6 is ¡§AFO-Front to back¡¨
 */
/*
 Thermal policy:
 a.Defaut fan pwm=100%
 b.One fan fail, set to fan pwm=100%
 1. For AFI:
   Default fan pwm will be 100%(fan_state=LEVEL_FAN_MAX).
   If all below case meet with, set to 75%(LEVEL_FAN_MID).
 MB board
 LM75-1(0X48)<=57
 LM75-2(0X49)<=47.3
 LM75-3(0X4A)<=45
 LM75-4(0X4C)<=45.1
 LM75-5(0X4E)<=40.75
 LM75-6(0X4F)<=42.1
 CPU board
 Core<=44 
 LM75-1(0X4B)<=35
 
   When fan_state=LEVEL_FAN_MID, meet with below case,  Fan pwm will be 100%(LEVEL_FAN_DAX)
 (MB board)
   LM75-1(0X48)>=61.5
   LM75-2(0X49)>=51.5
   LM75-3(0X4A)>=49.4
   LM75-4(0X4C)>=49.4
   LM75-5(0X4E)>=45.1
   LM75-6(0X4F)>=46.75
 (CPU board)
   Core>=48 
   LM75-1(0X4B)>=38.5

 2. For AFO:
  At default, FAN pwm was 100%(LEVEL_FAN_MAX). If all below case meet with, set to 75%(LEVEL_FAN_MID).
 (MB board)
 LM75-1(0X48)<=59
 LM75-2(0X49)<=53.5
 LM75-3(0X4A)<=55.3
 LM75-4(0X4C)<=50.3
 LM75-5(0X4E)<=50
 LM75-6(0X4F)<=52.5
 (CPU board)
 Core<=59
 LM75-1(0X4B)<=41.1

 When FAN pwm was 75%(LEVEL_FAN_MID). If all below case meet with, set to 50%(LEVEL_FAN_DEF).
 (MB board)
 LM75-1(0X48)<=55.8
 LM75-2(0X49)<=50.5
 LM75-3(0X4A)<=51.1
 LM75-4(0X4C)<=47.6
 LM75-5(0X4E)<=45.75
 LM75-6(0X4F)<=50.1
 (CPU board)
 Core<=57
 LM75-1(0X4B)<=36.6

 When fan_speed 50%(LEVEL_FAN_DEF).
 Meet with below case, Fan pwm will be 75%(LEVEL_FAN_MID)
 (MB board)
 LM75-1(0X48)>=70
 LM75-2(0X49)>=66
 LM75-3(0X4A)>=68
 LM75-4(0X4C)>=62
 LM75-5(0X4E)>=62
 LM75-6(0X4F)>=67
 (CPU board)
 Core>=77
 LM75-1(0X4B)>=50

 
 When FAN pwm was 75%(LEVEL_FAN_MID). If all below case meet with, set to 100%(LEVEL_FAN_MAX).
 (MB board)
 LM75-1(0X48)>=67
 LM75-2(0X49)>=62.5
 LM75-3(0X4A)>=65
 LM75-4(0X4C)>=59
 LM75-5(0X4E)>=58.5
 LM75-6(0X4F)>=63
 (CPU board)
 Core >=69
 LM75-1(0X4B)>=49
 
 Yellow Alarm
 MB board
 LM75-1(0X48)>=68
 LM75-2(0X49)>=64
 LM75-3(0X4A)>=65
 LM75-4(0X4C)>=61
 LM75-5(0X4E)>=60
 LM75-6(0X4F)>=64
 CPU Board
 Core>=70
 LM75-1(0X4B)>=68

 Red Alarm
 MB board
 LM75-1(0X48)>=72
 LM75-2(0X49)>=68
 LM75-3(0X4A)>=69
 LM75-4(0X4C)>=65
 LM75-5(0X4E)>=64
 LM75-6(0X4F)>=68
 CPU Board
 Core>=74
 LM75-1(0X4B)>=72

 Shutdown
 MB board
 LM75-1(0X48)>=77
 LM75-2(0X49)>=73
 LM75-3(0X4A)>=74
 LM75-4(0X4C)>=70
 LM75-5(0X4E)>=69
 LM75-6(0X4F)>=73
 CPU Board
 Core>=79
 LM75-1(0X4B)>=77


 */

typedef struct afi_temp_range{
    int mid_to_max_temp[8];
    int max_to_mid_temp[8];
}afi_temp_range_t;
 
afi_temp_range_t afi_thermal_spec={
 {61500, 51500, 49400, 49400, 45100, 46750, 48000, 38500},
 {57000, 47300, 45000, 45100, 40750, 42100, 44000, 35000}
};

typedef struct afo_temp_range{
    int min_to_mid_temp[8];
    int mid_to_max_temp[8];
    int max_to_mid_temp[8];
    int mid_to_min_temp[8];
    int max_to_yellow_alarm_temp[8];
    int yellow_to_red_alarm_temp[8];
    int red_alarm_to_shutdown_temp[8];
} afo_temp_range_t; 
  
afo_temp_range_t afo_thermal_spec={
 {70000, 66000, 68000, 62000, 62000, 67000, 77000, 50000},
 {67000, 62000, 65000, 59000, 58500, 63000, 69000, 49000}, 
 {59000, 53500, 55300, 50300, 50000, 52500, 59000, 41100},
 {55800, 50500, 51100, 47600, 45750, 50100, 57000, 36600},
 {68000, 64000, 65000, 61000, 60000, 64000, 70000, 68000},
 {72000, 68000, 69000, 65000, 64000, 68000, 74000, 72000}, 
 {77000, 73000, 74000, 70000, 69000, 73000, 79000, 77000}
};

typedef struct fan_ctrl_policy {
   int duty_cycle; 
   int pwm;
   int state;
} fan_ctrl_policy_t;
   
/*For AFI. 2 state. LEVEL_FAN_MID(75%), LEVEL_FAN_MAX(100%)
  For AFO. 3 state. LEVEL_FAN_MIN(50%), LEVEL_FAN_MID(75%), LEVEL_FAN_MAX(100%)
 */
enum
{
   LEVEL_FAN_INIT=0,
   LEVEL_FAN_MIN=1,
   LEVEL_FAN_MID=2,
   LEVEL_FAN_MAX=3,
   LEVEL_FAN_DEF=LEVEL_FAN_MAX,
   LEVEL_FAN_YELLOW_ALARM=4,
   LEVEL_FAN_RED_ALARM=5,
   LEVEL_FAN_SHUTDOWN=6,
};

fan_ctrl_policy_t  fan_thermal_policy_f2b[] = { /*AFO*/
{50,  0x7, LEVEL_FAN_MIN},
{75,  0xb, LEVEL_FAN_MID},
{100, 0xf, LEVEL_FAN_MAX},
    
};

fan_ctrl_policy_t  fan_thermal_policy_b2f[] = { /*AFI*/
{75,  0xb, LEVEL_FAN_MID},
{100, 0xf, LEVEL_FAN_MAX}
};
    
void onlp_sysi_shutdown(void)
{
    char cmd_str[64];
    memset(cmd_str, 0x0, strlen(cmd_str));    
    snprintf(cmd_str, 63, "i2cset -y -f 19 0x60 0x60 0x10");
    system(cmd_str); 
}
    
int onlp_sysi_get_duty_cycle_by_fan_state(int state, int direction)
{
    int i;
    if(direction)
    {
        for(i=0; i< 2; i++)
        {
            if(state==fan_thermal_policy_b2f[i].state)
            {
                return fan_thermal_policy_b2f[i].duty_cycle;
            }
        }
    }
    else
    {
        for(i=0; i< 3; i++)
        {
            if(state==fan_thermal_policy_f2b[i].state)
            {
                return fan_thermal_policy_f2b[i].duty_cycle;
            }
        }
    }
    return 0;     
   
}    
/*
 * If only one PSU insert , and watt >800w. Must let DUT fan pwm >= 75% in AFO.
 *  Because the psu temp is high.
 */
 
/* Return 1: full load
 * Return 0: Not full load
 */ 
int onlp_sysi_check_psu_loading(void)
{
    int psu_power_good[2]={1, 1};
    int psu_p_in[2]={0, 0};
    int psu_p_out[2]={0, 0};
    int id=1;
    int check_psu_watt=0;
    
    for (id=1; id<=2; id++)
    {
        if (psu_status_info_get(id, "psu_power_good", &psu_power_good[id-1]) != 0) {
            AIM_LOG_ERROR("Unable to read PSU(%d) node(psu_power_good)\r\n", id);
        }
        if (psu_power_good[id-1] != PSU_STATUS_POWER_GOOD)
        {
            check_psu_watt=1;
        }
    }
    if (check_psu_watt)
    {
        for (id=1; id<=2; id++)
        {
            if(psu_power_good[id-1]== PSU_STATUS_POWER_GOOD)
            {  /*check watt*/
                 if (psu_pmbus_info_get(id, "psu_p_in", &psu_p_in[id-1]) == 0)
                 {
                    if (psu_p_in[id-1]/1000 > 800)
                    {
                        return 1;
                    }
                 }
                 if (psu_pmbus_info_get(id, "psu_p_out", &psu_p_out[id-1]) == 0)
                 {
                    if (psu_p_out[id-1]/1000 > 800)
                    {
                        return 1;
                    }
                 }
            }
        }
        return 0;
    }
    else
        return 0;
    
    
    return 0;
    
}

#define FAN_SPEED_CTRL_PATH "/sys/bus/i2c/devices/17-0066/fan_duty_cycle_percentage"
#define FAN_DIRECTION_PATH "/sys/bus/i2c/devices/17-0066/fan1_direction"
#define CHECK_TIMES 3
    
static int fan_state=LEVEL_FAN_INIT;
static int fan_fail = 0;

static int fan_alarm_state=LEVEL_FAN_INIT;
static int send_yellow_alarm=0;
static int send_red_alarm=0;
static int count_check=0;

int current_duty_cycle, new_duty_cycle;
int
onlp_platformi_manage_fans(void)
{
    int i=0,k=0, ori_state=LEVEL_FAN_DEF, current_state=LEVEL_FAN_DEF;
    int fd, len, direction_val=1;
    int max_to_mid=0, mid_to_min=0; /* Only this flag equal to 8, otherwise state can not to be down.*/
    int psu_full_load=0;
    onlp_thermal_info_t thermal[8];
    char  buf[10] = {0};
    
    /* Get fan direction
     */
    if (onlp_file_read_int(&direction_val, FAN_DIRECTION_PATH) < 0) {
        AIM_LOG_ERROR("Unable to read status from file (%s)\r\n", FAN_DIRECTION_PATH); 
    }
    
    if(fan_state==LEVEL_FAN_INIT)
    {
        fan_state=LEVEL_FAN_MAX; /*This is default state*/
        onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(i), 100);
        return ONLP_STATUS_OK;
    }
    count_check++;
    if(count_check < CHECK_TIMES)    
        return ONLP_STATUS_OK;    
    else
        count_check=0;
        
    /* Get current temperature
     */
    for (i=2; i <5; i++)
    {
        if (onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(i), &thermal[k]) != ONLP_STATUS_OK  )
        {
            AIM_LOG_ERROR("Unable to read thermal status, set fans to 100% speed");
            onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(i), 100);
            return ONLP_STATUS_E_INTERNAL;
       }
        k++; 
    }    
    for (i=6; i <=8; i++)
    {
        if (onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(i), &thermal[k]) != ONLP_STATUS_OK  )
        {
            AIM_LOG_ERROR("Unable to read thermal status, set fans to 100% speed");
            onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(i), 100);
            return ONLP_STATUS_E_INTERNAL;
        }
        k++; 
    }
    if (onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(1), &thermal[6]) != ONLP_STATUS_OK  )
    {
        AIM_LOG_ERROR("Unable to read thermal status, set fans to 100% speed");
        onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(1), 100);
        return ONLP_STATUS_E_INTERNAL;
    }
    if (onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(5), &thermal[7]) != ONLP_STATUS_OK  )
    {
        AIM_LOG_ERROR("Unable to read thermal status, set fans to 100% speed");
        onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(5), 100);
        return ONLP_STATUS_E_INTERNAL;
    }
    /* Get current fan pwm percent
     */
    fd = open(FAN_SPEED_CTRL_PATH, O_RDONLY);
    if (fd == -1){
        AIM_LOG_ERROR("Unable to open fan speed control node (%s)", FAN_SPEED_CTRL_PATH);
        return ONLP_STATUS_E_INTERNAL;
    }
    len = read(fd, buf, sizeof(buf));
    close(fd);    
    if (len <= 0) {
        AIM_LOG_ERROR("Unable to read fan speed from (%s)", FAN_SPEED_CTRL_PATH);
        return ONLP_STATUS_E_INTERNAL;
    }
    current_duty_cycle = atoi(buf);
    ori_state=fan_state;
    current_state=fan_state;

    /* Check thermal is in which range. */
    max_to_mid=0;
    if(direction_val==1) /* AFI */
    {
        for (i=0; i <CHASSIS_THERMAL_COUNT; i++)
        {
            if(ori_state==LEVEL_FAN_MID)
            {
                if (thermal[i].mcelsius >= afi_thermal_spec.mid_to_max_temp[i])
                {
                   current_state=LEVEL_FAN_MAX;
                   break;  
                }
            }
            else
            {
                if (thermal[i].mcelsius <= afi_thermal_spec.max_to_mid_temp[i])
                {
                    max_to_mid++;
                }            
            }
        }
        if(max_to_mid==CHASSIS_THERMAL_COUNT && fan_state==LEVEL_FAN_MAX)
        {
            current_state=LEVEL_FAN_MID;
        }
    }
    else  /* AFO */
    {
        psu_full_load=onlp_sysi_check_psu_loading();
        for (i=0; i <CHASSIS_THERMAL_COUNT; i++)
        {
            if (ori_state==LEVEL_FAN_MID)
            {
                if (thermal[i].mcelsius >= afo_thermal_spec.mid_to_max_temp[i])
                {
                    current_state=LEVEL_FAN_MAX;
                    break;
                }
                else
                {
                    if (thermal[i].mcelsius <= afo_thermal_spec.mid_to_min_temp[i])
                    {
                        mid_to_min++;
                    }
                }
            }
            else if (ori_state==LEVEL_FAN_MIN)
            {
                if(psu_full_load)
                     current_state=LEVEL_FAN_MID;
                else if (thermal[i].mcelsius >= afo_thermal_spec.min_to_mid_temp[i])
                {
                    current_state=LEVEL_FAN_MID;               
                }
            }
            else
            {                
                if (thermal[i].mcelsius <= afo_thermal_spec.max_to_mid_temp[i])
                {
                    max_to_mid++;
                }    
                if (!fan_alarm_state)
                {
                    if (thermal[i].mcelsius >= afo_thermal_spec.max_to_yellow_alarm_temp[i])
                    {
                        fan_alarm_state=LEVEL_FAN_YELLOW_ALARM;
                        if(send_yellow_alarm==0)
                        {
                            send_yellow_alarm=1;
                            AIM_SYSLOG_WARN("Temperature high", "Temperature high","Alarm-Yellow for temperature high is detected");
                        }
                    }
                }
                else if (fan_alarm_state==LEVEL_FAN_YELLOW_ALARM)
                {
                    if (thermal[i].mcelsius >= afo_thermal_spec.yellow_to_red_alarm_temp[i])
                    {
                        fan_alarm_state=LEVEL_FAN_RED_ALARM;
                        if(send_red_alarm==0)
                        {
                            send_red_alarm=1;
                            AIM_SYSLOG_WARN("Temperature high", "Temperature high","Alarm-Red for temperature high is detected");
                        }
                    }
                }
                else if (fan_alarm_state==LEVEL_FAN_RED_ALARM)
                {
                    if (thermal[i].mcelsius >= afo_thermal_spec.red_alarm_to_shutdown_temp[i])
                    {
                        fan_alarm_state=LEVEL_FAN_SHUTDOWN;
                        sleep(1);
                        AIM_SYSLOG_CRIT("Temperature critical", "Temperature critical", "Alarm for temperature critical is detected, Shutdown DUT");
                        system("sync");
                        onlp_sysi_shutdown();                        
                    }
                }
                
            }
        }
        if(max_to_mid==CHASSIS_THERMAL_COUNT && ori_state==LEVEL_FAN_MAX)
        {
            current_state=LEVEL_FAN_MID;
            if (fan_alarm_state)
            {
                fan_alarm_state=0;
                send_yellow_alarm=0;
                send_red_alarm=0;
                AIM_SYSLOG_INFO("Temperature high is clean", "Temperature high is clear", "Alarm for temperature high is cleared");
            }
        }
        if(mid_to_min==CHASSIS_THERMAL_COUNT && ori_state==LEVEL_FAN_MID)
        {
            if (!psu_full_load)
            {
                current_state=LEVEL_FAN_MIN;
            }
        }
    }

    /* Get each fan status
     */
    for (i = 1; i <= NUM_OF_FAN_ON_MAIN_BROAD; i++)
    {
        onlp_fan_info_t fan_info;

        if (onlp_fani_info_get(ONLP_FAN_ID_CREATE(i), &fan_info) != ONLP_STATUS_OK)
        {
            AIM_LOG_ERROR("Unable to get fan(%d) status, try to set the other fans as full speed\r\n", i);
            if(current_duty_cycle != FAN_DUTY_CYCLE_MAX)
            {
                onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(1), FAN_DUTY_CYCLE_MAX);
            }
            fan_fail=1;
            break;
        }
       // if (fan_info.status & ONLP_FAN_STATUS_FAILED || !(fan_info.status & ONLP_FAN_STATUS_PRESENT))
        {
            AIM_LOG_ERROR("Fan(%d) is not working, set the other fans as full speed\r\n", i);
            if(current_duty_cycle != FAN_DUTY_CYCLE_MAX)
            {
                onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(1), FAN_DUTY_CYCLE_MAX);
            }
            fan_fail=1;
            break;
        }            	    
    }
    if(current_state!=ori_state)
    {
        fan_state=current_state;
        new_duty_cycle=onlp_sysi_get_duty_cycle_by_fan_state(current_state, direction_val);
        if(new_duty_cycle!=current_duty_cycle && !fan_fail)
        {
            onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(1), new_duty_cycle);
            return 0;
        }
        if(!new_duty_cycle && !fan_fail)
        {
            onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(1), FAN_DUTY_CYCLE_MAX);
        }
    }
  
    return 0;
}

int
onlp_platformi_manage_leds(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
