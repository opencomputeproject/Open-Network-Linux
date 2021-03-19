/*
 sff_eeprom.c
 provide api for accesing sff eeprom

 */
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include "sff_eeprom.h"
#include "net_def.h"
#include "net_swps.h"
#include "eeprom_config/eeprom_config.h"

/*
 * This module implement sff platform driver and provide sysfs attribute for access io control
 * and eeprom
 *
 */

#define I2C_BLOCK_READ_SUPPORT

#define EEPROM_LOG_ERR(fmt, args...) \
    do { \
        if (logLevel & EEPROM_ERR_LEV) \
        { \
            printk (KERN_ERR "[EEPROM]%s:"fmt,__FUNCTION__,  ##args); \
        } \
    } while (0)

#define EEPROM_LOG_INFO(fmt, args...) \
    do { \
        if (logLevel & EEPROM_INFO_LEV) \
        { \
            printk (KERN_INFO "[EEPROM]%s:"fmt,__FUNCTION__,  ##args); \
        } \
    } while (0)

#define EEPROM_LOG_DBG(fmt, args...) \
    do { \
        if (logLevel & EEPROM_DBG_LEV) \
        { \
            printk (KERN_INFO "[EEPROM]%s:"fmt,__FUNCTION__,  ##args); \
        } \
    } while (0)

#define QSFP_ID_OFFSET (0)
extern u32 logLevel;

struct sff_eeprom_t {
    int port;
    struct i2c_client *i2c_client;
    struct mutex lock;
};
struct eeprom_i2c_tbl_t *eepromI2cTbl = NULL;
struct i2c_client *sffEepromI2cClient = NULL;
struct sff_eeprom_t *sffEEprom = NULL;
int sff_eeprom_read_lc( int lc_id,
                        int port,
                        u8 slave_addr,
                        u8 offset,
                        u8 *buf,
                        size_t len);

int sff_eeprom_write_lc( int lc_id,
                        int port,
                        u8 slave_addr,
                        u8 offset,
                        const u8 *buf,
                        size_t len);

static int lcMaxPortNum = 0;
static struct sff_eeprom_driver_t sffEepromDrvFunc = {
    .eeprom_read = sff_eeprom_read_lc,
    .eeprom_write = sff_eeprom_write_lc,
};
struct sff_eeprom_driver_t *sff_eeprom_drv_get(void)
{
    return &sffEepromDrvFunc;
}    

static int _sff_eeprom_block_read(struct sff_eeprom_t *eeprom,
                                  u8 slave_addr,
                                  u8 offset,
                                  u8 *buf,
                                  size_t len)
{
    struct i2c_client *client = NULL;
    struct mutex *lock = NULL;
    int ret = 0;

    if(!eeprom ||
       !buf) {
        return -EINVAL;
    }
    lock = &(eeprom->lock);
    client = eeprom->i2c_client;

    if(!client) {
        return -EBADRQC;
    }

    mutex_lock(lock);
    client->addr = slave_addr;
    ret = i2c_smbus_read_i2c_block_data_retry(client, offset, len, buf);
    mutex_unlock(lock);
    return 0;

}

static int _sff_eeprom_read(struct sff_eeprom_t *eeprom,
                            u8 slave_addr,
                            u8 offset,
                            u8 *buf,
                            size_t len)
{
    struct i2c_client *client = NULL;
    struct mutex *lock = NULL;
    int ret = 0;
    int count = 0;
    u8 *ptr = NULL;

    if(!eeprom ||
       !buf) {
        return -EINVAL;
    }
    ptr = buf;
    lock = &(eeprom->lock);
    client = eeprom->i2c_client;

    if(!client) {
        return -EBADRQC;
    }
    
    mutex_lock(lock);
    client->addr = slave_addr;
    for(count = 0; count < len; count++) {
        ret = i2c_smbus_read_byte_data_retry(client, offset+count);
        if (ret < 0) {
            EEPROM_LOG_INFO("i2c_read fail ret:%d\n", ret);
            mutex_unlock(lock);

            return ret;
        }
        ptr[count] = (u8)ret;
    }

    mutex_unlock(lock);
#ifdef READBACK_CHECK
    for(count = 0; count < len; count++) {
        EEPROM_LOG_DBG("i2c_read ok buf[%d]:0x%x\n", count, buf[count]);
    }
#endif
    return 0;

}

static int _sff_eeprom_write(struct sff_eeprom_t *eeprom,
                             u8 slave_addr,
                             u8 offset,
                             const u8 *buf,
                             size_t len)
{
    struct i2c_client *client = NULL;
    struct mutex *lock = NULL;
    int ret = 0;
    int count = 0;
    const u8 *ptr = buf;

    if(!eeprom ||
       !buf) {
        return -EINVAL;
    }

    lock = &(eeprom->lock);
    client = eeprom->i2c_client;

    if(!client) {
        return -EBADRQC;
    }

    mutex_lock(lock);
    client->addr = slave_addr;
    for(count = 0; count < len; count++) {
        ret = i2c_smbus_write_byte_data_retry(client, offset+count, ptr[count]);
        if (ret < 0) {
            EEPROM_LOG_ERR("i2c_write fail ret:%d\n", ret);
            mutex_unlock(lock);
            return ret;
        }
    }

    mutex_unlock(lock);
    return 0;

}
void sff_eeprom_port_num_set(int port_num)
{
    lcMaxPortNum = port_num;
}    
static int port_to_new_port(int lc_id, int port)
{
    int new_port = 0;
#if 1 /*enable it while bring up*/
    new_port = (lcMaxPortNum * lc_id) + port;
#else 
    new_port = port;
#endif    
    return new_port;
}   
static bool port_range_valid(int new_port)
{
    int port_num = eepromI2cTbl->size;
    return ((new_port >= 0 && new_port < port_num) ? true : false);
}    
void sff_eeprom_read_no_retry(int lc_id, int port)
{
    struct sff_eeprom_t *eeprom = NULL;
    int new_port = 0;
    /*remapping*/
    new_port = port_to_new_port(lc_id, port); 
    /*check port range here*/
    if (!port_range_valid(new_port)) {
        EEPROM_LOG_ERR("port out of range: new port:%d \n", new_port);
        return;
    }

    eeprom = &sffEEprom[new_port];
    mutex_lock(&eeprom->lock);
    i2c_smbus_read_byte_data(eeprom->i2c_client, QSFP_ID_OFFSET);
    mutex_unlock(&eeprom->lock);
}   

int sff_eeprom_read_lc( int lc_id,
                        int port,
                        u8 slave_addr,
                        u8 offset,
                        u8 *buf,
                        size_t len)
{
    int new_port = 0;
    /*remapping*/
    new_port = port_to_new_port(lc_id, port); 
    /*check port range here*/
    if (!port_range_valid(new_port)) {
        EEPROM_LOG_ERR("port out of range: new port:%d \n", new_port);
        return -EBADRQC;
    }
#if defined (I2C_BLOCK_READ_SUPPORT)

    return _sff_eeprom_block_read(&sffEEprom[new_port], slave_addr, offset, buf, len);
#else
    return _sff_eeprom_read(&sffEEprom[new_port], slave_addr, offset, buf, len);

#endif
}    

int sff_eeprom_write_lc( int lc_id,
                        int port,
                        u8 slave_addr,
                        u8 offset,
                        const u8 *buf,
                        size_t len)
{
    int new_port = 0;
    /*remapping*/
    new_port = port_to_new_port(lc_id, port); 
    /*check port range here*/
    if (!port_range_valid(new_port)) {
        EEPROM_LOG_ERR("port out of range: new port:%d \n", new_port);
        return -EBADRQC;
    }

    return _sff_eeprom_write(&sffEEprom[new_port], slave_addr, offset, buf, len);
}    

static void sff_eeprom_clients_destroy(int size)
{
    int port = 0;
    
    if (p_valid(sffEepromI2cClient)) {
        kfree(sffEepromI2cClient);
    }
    for (port = 0; port < size; port++) {
        sffEEprom[port].i2c_client = NULL;
    }
}
static int sff_eeprom_clients_create(int size)
{
    struct i2c_client *client = NULL;
    int port = 0;
    
    client = kzalloc(sizeof(struct i2c_client) * size, GFP_KERNEL);

    if (!p_valid(client)) {
        return -ENOMEM;
    }
    sffEepromI2cClient = client;
    
    for (port = 0; port < size; port++) {
        sffEEprom[port].i2c_client = &sffEepromI2cClient[port];
    }
    
    return 0;
}
/*init i2c adapter*/
static int _sff_eeprom_client_init(struct sff_eeprom_t *eeprom,  struct eeprom_config_t *config)
{
    struct i2c_adapter *adap;
    struct i2c_client *client = NULL;

    if (!eeprom ||
        !config) {
        return -EBADRQC;
    }
    client = eeprom->i2c_client;
    adap = i2c_get_adapter(config->i2c_ch);
    if (!adap) {
        EEPROM_LOG_ERR("fail to get adapter ch:%d\n", config->i2c_ch);
        return -EBADRQC;
    }
    client->adapter = adap;
    client->addr = SFF_EEPROM_I2C_ADDR;

    eeprom->i2c_client = client;
    EEPROM_LOG_DBG("ok i2c_ch:%d\n", config->i2c_ch);
    mutex_init(&(eeprom->lock));
    return 0;

}
static void sff_eeprom_clients_deinit(int size)
{
    int port = 0;
    int port_num = size;
    struct i2c_client *client = NULL;

    if (!sffEEprom) {

        EEPROM_LOG_ERR("NULL pointer\n");
        return;
    }
    for (port = 0; port < port_num; port++) {
        client = sffEEprom[port].i2c_client;
        if (p_valid(client)) {
            if (p_valid(client->adapter)) {
                i2c_put_adapter(client->adapter);
            }
        }
    }
}


static int sff_eeprom_clients_init(struct eeprom_i2c_tbl_t *tbl)
{
    int port = 0;
    int port_num = 0;
    struct eeprom_config_t *map = NULL;
    int ret = 0;
    if(!tbl) {
        return -EBADRQC;
    }
    port_num = tbl->size;
    map = tbl->map;
    for (port = 0; port < port_num; port++) {

        if (_sff_eeprom_client_init(&sffEEprom[port], &map[port]) < 0) {
            ret =  -EBADRQC;
            break;
        }
    }
    if (ret < 0) {
        sff_eeprom_clients_deinit(eepromI2cTbl->size);
    }

    return 0;

}
static int sff_eeprom_objs_create(int size)
{

    sffEEprom = kzalloc(sizeof(struct sff_eeprom_t) * size, GFP_KERNEL);

    if(!p_valid(sffEEprom)) {
        return -ENOMEM;
    }

    return 0;
}

static struct eeprom_i2c_tbl_t *_platform_eeprom_info_load(int platform_id)
{
    int i = 0;
    struct eeprom_i2c_tbl_t *tbl = NULL;
    for (i = 0; platform_eeprom_info_tbl[i].platform_id != PLATFORM_END; i++) {
        if (platform_eeprom_info_tbl[i].platform_id == platform_id) {

            tbl = platform_eeprom_info_tbl[i].tbl;
            return tbl;
        }
    }

    return NULL;
}

static int eeprom_i2c_table_load(int platform_id)
{
    struct eeprom_i2c_tbl_t *tbl = NULL;
    
    tbl = _platform_eeprom_info_load(platform_id);
    if (!tbl) {
        return -EBADRQC;
    }
    eepromI2cTbl = tbl;
    return 0;
}
static void sff_eeprom_objs_destroy(void)
{
    if(p_valid(sffEEprom)) {
        kfree(sffEEprom);
    }
}
int sff_eeprom_init(int platform_id)
{
    if (eeprom_i2c_table_load(platform_id) < 0) {
        goto exit_err;
    }
    if (sff_eeprom_objs_create(eepromI2cTbl->size) < 0) {
        goto exit_err;
    }

    if (sff_eeprom_clients_create(eepromI2cTbl->size) < 0) {
        goto exit_kfree_obj;
    }
#if 1
    if (sff_eeprom_clients_init(eepromI2cTbl) < 0) {
        goto exit_kfree_client;
    }
#endif
    EEPROM_LOG_INFO("ok\n");
    return 0;

exit_kfree_client:
    sff_eeprom_clients_destroy(eepromI2cTbl->size);
exit_kfree_obj:
    sff_eeprom_objs_destroy();
exit_err:
    return -EBADRQC;
}

void sff_eeprom_deinit(void)
{
    sff_eeprom_clients_deinit(eepromI2cTbl->size);
    sff_eeprom_clients_destroy(eepromI2cTbl->size);
    sff_eeprom_objs_destroy();

    EEPROM_LOG_INFO("ok\n");
}
