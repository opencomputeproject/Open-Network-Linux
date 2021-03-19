#ifndef __LC_DEV_H
#define __LC_DEV_H

#define EXTERNAL_RST_DELAY (10) /*ms*/
typedef enum {
    LC_LED_CTRL_OFF = 0,
    LC_LED_CTRL_GREEN_ON,
    LC_LED_CTRL_RED_ON,
    LC_LED_CTRL_AMBER_ON,
    LC_LED_CTRL_GREEN_BLINK,
    LC_LED_CTRL_RED_BLINK,
    LC_LED_CTRL_AMBER_BLINK,
    LC_LED_CTRL_NUM,
} lc_led_ctrl_t;

typedef enum {
    LC_UNKNOWN_TYPE = 0,
    LC_100G_TYPE,
    LC_400G_TYPE,
    LC_TYPE_NUM,
} lc_type_t;

/*function declaration*/
int lc_dev_lc_cpld_init(int lc_id);
int lc_dev_power_set(int lc_id, bool on);
int lc_dev_power_ready(int lc_id, bool *ready);
void lc_dev_phy_ready(unsigned long bitmap, bool *ready);
int lc_dev_prs_get(unsigned long *bitmap);
int lc_dev_reset_set(int lc_id, u8 lv);
int lc_dev_reset_get(int lc_id, u8 *lv);
int lc_dev_type_get(int lc_id, lc_type_t *type);
int lc_dev_type_get_text(int lc_id, char *buf, int size);
int lc_dev_mux_l1_reset(int lc_id, u8 lv);
bool lc_dev_mux_l1_is_alive(int lc_id);
int lc_dev_mux_l2_reset(int lc_id, int mux_l2_id, u8 lv);
int lc_dev_mux_reset_set(int lc_id, int lv);
int lc_dev_mux_l1_i2c_ch_get(int lc_id);
int lc_dev_led_set(int lc_id, lc_led_ctrl_t ctrl);
int lc_dev_led_st_get(int lc_id, lc_led_ctrl_t *ctrl);
int lc_dev_init(int platform_id, int io_no_init);
void lc_dev_deinit(void);
int lc_dev_hdlr(void);
struct sff_io_driver_t *sff_io_drv_get_lcdev(void);
int lc_dev_over_temp_asserted(int lc_id, bool *asserted);
int lc_dev_over_temp_deasserted(int lc_id, bool *deasserted);
int lc_dev_temp_get_text(int lc_id, char *buf, int size);
int lc_dev_temp_th_get_text(int lc_id, char *buf, int size);
int lc_dev_phy_reset_set(int lc_id, u8 val);
int lc_sff_intr_hdlr_byCard(int lc_id);
int lc_dev_led_boot_amber_set(int lc_id, bool on);
int lc_dev_temp_th_set(int lc_id, int temp);
int lc_dev_ej_r_get(unsigned long *bitmap);
int lc_dev_ej_l_get(unsigned long *bitmap);
#endif /*__LC_DEV_H*/
