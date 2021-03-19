//port info
#ifndef __SFF_EEPROM_H
#define __SFF_EEPROM_H

struct eeprom_config_t {
    int port;
    int i2c_ch;
};
struct eeprom_i2c_tbl_t {

    struct eeprom_config_t *map;
    int size;
};

int sff_eeprom_init(int platform_id);
void sff_eeprom_deinit(void);
void sff_eeprom_port_num_set(int port_num);
void sff_eeprom_read_no_retry(int lc_id, int port);
struct sff_eeprom_driver_t *sff_eeprom_drv_get(void);
#endif /*__SFF_EEPROM_H*/
