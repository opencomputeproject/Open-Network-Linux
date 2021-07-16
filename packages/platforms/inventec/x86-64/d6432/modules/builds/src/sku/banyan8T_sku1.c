/* handle sff io pin control , mux gpio .. cpld  etc*/
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
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/pci.h>
#include "inv_fpga.h"
#include "sku_common.h"

typedef enum {
    SKU_SFF_PRS_TYPE,
    SKU_SFF_INTR_TYPE,
    SKU_SFF_LPMODE_TYPE,
    SKU_SFF_RESET_TYPE,
    SKU_SFF_MODSEL_TYPE,
    SKU_SFF_IO_TYPE_NUM,
} sku_sff_io_type_t;

typedef enum {
    SKU_SFF_OC_TYPE,
    SKU_SFF_POWER_SET_TYPE,
    SKU_SFF_POWER_GET_TYPE,
    SKU_SFF_EXTRA_IO_TYPE_NUM,
} sku_sff_extra_io_type_t;

struct i2c_config_t {
    int i2c_ch_id;
    u8 addr;
};

#define PCIE_BUS_NAME ("0000:08:00.0")
#define CPLD_ID_NUM (2)
#define I2C_CH_ID_NUM (2)

struct cplddev_t {
    struct i2c_config_t i2c_config[CPLD_ID_NUM];
    struct inv_i2c_client_t inv_i2c_client[I2C_CH_ID_NUM];
    struct ldata_format_t sff_extra_io_st[SKU_SFF_EXTRA_IO_TYPE_NUM];
};
struct fpga_sff_io_func_t {
    int (*set)(struct fpga_dev *fpga, unsigned long bitmap);
    int (*get)(struct fpga_dev *fpga, unsigned long *bitmap);
};

struct sku_t {
    struct cplddev_t cpld;
    struct ldata_format_t sff_io_st[SKU_SFF_IO_TYPE_NUM];
    struct fpga_sff_io_func_t sff_io_func[SKU_SFF_IO_TYPE_NUM];
    struct fpga_dev *fpga;
    int  (*sff_polling_status_get)(struct fpga_dev *fpga, int port);
    int  (*sff_polling_set)(struct fpga_dev *fpga, int port, bool en);
    int  (*sff_polling_get)(struct fpga_dev *fpga, int port);
};

struct sff_reg_t {

    int cpld_id;
    u8 offset;
    int size; /*unit byte*/
    int port_1st; /*is multiply of 8*/
    int port_last;
    bool is_end;
};
/*<TBD>*/
const static struct sff_reg_t sff_oc_reg_tbl[] = {

    {.cpld_id = 0, .offset = 0, .size = 6, .port_1st = 0, .port_last = 31},
    {.is_end = true}
};

const static struct sff_reg_t sff_power_status_reg_tbl[] = {
    {.cpld_id = 0, .offset = 0x1A, .size = 1, .port_1st = 0, .port_last = 7},
    {.cpld_id = 0, .offset = 0x1B, .size = 1, .port_1st = 8, .port_last = 15},
    {.cpld_id = 0, .offset = 0x1C, .size = 1, .port_1st = 16, .port_last = 23},
    {.cpld_id = 0, .offset = 0x16, .size = 1, .port_1st = 24, .port_last = 31},
    {.is_end = true}
};
const static struct sff_reg_t sff_power_set_reg_tbl[] = {
    {.cpld_id = 0, .offset = 0x17, .size = 1, .port_1st = 0, .port_last = 7},
    {.cpld_id = 0, .offset = 0x18, .size = 1, .port_1st = 8, .port_last = 15},
    {.cpld_id = 0, .offset = 0x19, .size = 1, .port_1st = 16, .port_last = 23},
    {.cpld_id = 0, .offset = 0x15, .size = 1, .port_1st = 24, .port_last = 31},
    {.is_end = true}
};
const static struct sff_reg_t *sff_extra_io_reg[SKU_SFF_EXTRA_IO_TYPE_NUM] = {
    
    [SKU_SFF_OC_TYPE] = sff_oc_reg_tbl,
    [SKU_SFF_POWER_GET_TYPE] = sff_power_status_reg_tbl,
    [SKU_SFF_POWER_SET_TYPE] = sff_power_set_reg_tbl
};

static int fpga_func_load(struct sku_t *sku);
static int inv_i2c_smbus_read_block_data(struct inv_i2c_client_t *inv_i2c_client, u8 offset, int len, u8 *buf);
static int inv_i2c_smbus_write_block_data(struct inv_i2c_client_t *inv_i2c_client, u8 offset, int len, const u8 *buf);
static int sku_init(int platform_id, int io_no_init);
static void sku_deinit(void);
static int sku_hdlr(void);
static int sku_sff_get_ready_action(int lc_id, int port);
static int sku_sff_detected_action(int lc_id, int port);
static int sku_sff_ioout_set(struct sku_t *sku, sku_sff_io_type_t type, unsigned long bitmap);
static int sku_sff_ioout_get(struct sku_t *sku, sku_sff_io_type_t type, unsigned long *bitmap);
static int cpld_ioout_set(struct cplddev_t *dev, sku_sff_extra_io_type_t type, unsigned long bitmap);
static int cpld_ioout_get(struct cplddev_t *dev, sku_sff_extra_io_type_t type, unsigned long *bitmap);
static int cpld_ioin_get(struct cplddev_t *dev, sku_sff_extra_io_type_t type, unsigned long *bitmap);
/*sff io*/
static int sku_sff_prs_get(int lc_id, unsigned long *bitmap);
static int sku_sff_intr_get(int lc_id, unsigned long *bitmap);
static int sku_sff_reset_set(int lc_id, unsigned long bitmap);
static int sku_sff_reset_get(int lc_id, unsigned long *bitmap);
static int sku_sff_lpmode_set(int lc_id, unsigned long bitmap);
static int sku_sff_lpmode_get(int lc_id, unsigned long *bitmap);
static int sku_sff_modsel_set(int lc_id, unsigned long bitmap);
static int sku_sff_modsel_get(int lc_id, unsigned long *bitmap);
/*sff extra io*/
static int sku_sff_power_set(int lc_id, unsigned long bitmap);
static int sku_sff_power_get(int lc_id, unsigned long *bitmap);
static int sku_sff_oc_get(int lc_id, unsigned long *bitmap);

static struct sku_t banyan8TSku1;
/*<TBD> need to fill right info after get cpld reg */
const static struct i2c_config_t i2c_config_tbl[CPLD_ID_NUM] = {
    [0] = { .i2c_ch_id = 0, .addr = 0x77},
    [1] = { .i2c_ch_id = 1, .addr = 0x33},
};
static const int i2cChTbl[I2C_CH_ID_NUM] = {
    [0] = 2,
    [1] = 2
};

static struct i2c_client *invI2cClient = NULL;

struct sff_io_driver_t banyan8T_sku1_sff_io_drv = {
    .prs = {
        .set = NULL,
        .get = sku_sff_prs_get
    },
    .intr = {
        .set = NULL,
        .get = sku_sff_intr_get
    },
    .reset = {
        .set = sku_sff_reset_set,
        .get = sku_sff_reset_get
    },
    .lpmode = {
        .set = sku_sff_lpmode_set,
        .get = sku_sff_lpmode_get
    },
    .modsel = {
        .set = sku_sff_modsel_set,
        .get = sku_sff_modsel_get
    },
};

struct pltfm_func_t banyan8T_sku1_pltfm_func = {
    .init = sku_init,
    .deinit = sku_deinit,
    .io_hdlr = sku_hdlr,
    .mux_reset_set = NULL,
    .mux_reset_get = NULL,
    .i2c_is_alive = NULL,
    .sff_get_ready_action = sku_sff_get_ready_action,
    .sff_detected_action = sku_sff_detected_action,
    .sff_oc = {
        .set = NULL,
        .get = sku_sff_oc_get
    },
    .sff_power = {
        .set = sku_sff_power_set,
        .get = sku_sff_power_get
    },
};

static int sku_sff_ioin_update(struct sku_t *sku, sku_sff_io_type_t type)
{
    unsigned long bitmap = 0;
    int ret = 0;
    
    check_p(sku);
    if ((ret = sku->sff_io_func[type].get(sku->fpga, &bitmap)) < 0) {
        sku->sff_io_st[type].valid = false;
        return ret;
    }
    sku->sff_io_st[type].bitmap = bitmap;
    sku->sff_io_st[type].valid = true;
    return 0;
}

static int sku_sff_ioin_get(struct sku_t *sku, sku_sff_io_type_t type, unsigned long *bitmap)
{
    check_p(sku);
    check_p(bitmap);

    if (!(sku->sff_io_st[type].valid)) {
        return -EBADRQC;
    }
    *bitmap = sku->sff_io_st[type].bitmap;
    return 0;
}

static int sku_sff_ioout_set(struct sku_t *sku, sku_sff_io_type_t type, unsigned long bitmap)
{
    return sku->sff_io_func[type].set(sku->fpga, bitmap);
}

static int sku_sff_ioout_get(struct sku_t *sku, sku_sff_io_type_t type, unsigned long *bitmap)
{
    return sku->sff_io_func[type].get(sku->fpga, bitmap);
}

static int sku_sff_prs_get(int lc_id, unsigned long *bitmap)
{
    return sku_sff_ioin_get(&banyan8TSku1, SKU_SFF_PRS_TYPE, bitmap);
}

static int sku_sff_intr_get(int lc_id, unsigned long *bitmap)
{
    return sku_sff_ioin_get(&banyan8TSku1, SKU_SFF_INTR_TYPE, bitmap);
}

static int sku_sff_reset_set(int lc_id, unsigned long bitmap)
{
    return sku_sff_ioout_set(&banyan8TSku1, SKU_SFF_RESET_TYPE, bitmap);
}

static int sku_sff_lpmode_set(int lc_id, unsigned long bitmap)
{
    return sku_sff_ioout_set(&banyan8TSku1, SKU_SFF_LPMODE_TYPE, bitmap);
}

static int sku_sff_modsel_set(int lc_id, unsigned long bitmap)
{
    return sku_sff_ioout_set(&banyan8TSku1, SKU_SFF_MODSEL_TYPE, bitmap);
}

static int sku_sff_reset_get(int lc_id, unsigned long *bitmap)
{
    return sku_sff_ioout_get(&banyan8TSku1, SKU_SFF_RESET_TYPE, bitmap);
}

static int sku_sff_lpmode_get(int lc_id, unsigned long *bitmap)
{
    return sku_sff_ioout_get(&banyan8TSku1, SKU_SFF_LPMODE_TYPE, bitmap);
}

static int sku_sff_modsel_get(int lc_id, unsigned long *bitmap)
{
    return sku_sff_ioout_get(&banyan8TSku1, SKU_SFF_MODSEL_TYPE, bitmap);
}
/*sff extra io*/
static int sku_sff_power_set(int lc_id, unsigned long bitmap)
{
    return cpld_ioout_set(&(banyan8TSku1.cpld), SKU_SFF_POWER_SET_TYPE, bitmap);
}

static int sku_sff_power_get(int lc_id, unsigned long *bitmap)
{
    /*
      Based on the Allen's suggestion,
      It need to refer two bit information from EFUE-Power status and port present to get the correct power supply.
    */
    // unsigned long bitmap_from_power_status = 0;
    // unsigned long bitmap_from_present = 0;
    // cpld_ioout_get(&(banyan8TSku1.cpld), SKU_SFF_POWER_GET_TYPE, bitmap_from_power_status);
    // sku_sff_ioin_get(&banyan8TSku1, SKU_SFF_PRS_TYPE, bitmap_from_present);
    // return cpld_ioout_get(&(banyan8TSku1.cpld), SKU_SFF_POWER_GET_TYPE, (bitmap_from_power_status & bitmap_from_present));
    return cpld_ioout_get(&(banyan8TSku1.cpld), SKU_SFF_POWER_GET_TYPE, bitmap);
}

static int sku_sff_oc_get(int lc_id, unsigned long *bitmap)
{
    return cpld_ioin_get(&(banyan8TSku1.cpld), SKU_SFF_OC_TYPE, bitmap);
}

static inline bool ch_id_valid(int id)
{
    return ((id >= 0 && id < I2C_CH_ID_NUM) ? true : false);
}

static struct inv_i2c_client_t *inv_i2c_client_find(struct cplddev_t *dev, struct i2c_config_t *config)
{
    struct inv_i2c_client_t *inv_i2c_client = NULL;
    int i2c_ch_id = 0;
    u8 addr = 0;

    if (!p_valid(dev) ||
        !p_valid(config)) {
        return NULL;
    }
    i2c_ch_id = config->i2c_ch_id;
    addr = config->addr;

    if (!ch_id_valid(i2c_ch_id)) {
        SKU_LOG_ERR("cant find i2c_ch_id:%d\n", i2c_ch_id);
        return NULL;
    }

    inv_i2c_client = &(dev->inv_i2c_client[i2c_ch_id]);
    if (!p_valid(inv_i2c_client)) {
        SKU_LOG_ERR("NULL inv_i2c_client i2c_ch_id:%d\n", i2c_ch_id);
        return NULL;
    }

    if (!p_valid(inv_i2c_client->client)) {
        SKU_LOG_ERR("NULL inv_i2c_client i2c_ch_id:%d\n", i2c_ch_id);
        return NULL;
    }
    inv_i2c_client->client->addr = addr;

    return inv_i2c_client;
}

static int inv_i2c_smbus_read_block_data(struct inv_i2c_client_t *inv_i2c_client, u8 offset, int len, u8 *buf)
{
    int ret = 0;

    check_p(inv_i2c_client);
    check_p(buf);

    mutex_lock(&inv_i2c_client->lock);
    ret = i2c_smbus_read_i2c_block_data_retry(inv_i2c_client->client, offset, len, buf);
    mutex_unlock(&inv_i2c_client->lock);

    return ret;
}

static int inv_i2c_smbus_write_block_data(struct inv_i2c_client_t *inv_i2c_client, u8 offset, int len, const u8 *buf)
{
    int ret = 0;

    check_p(inv_i2c_client);
    check_p(buf);

    mutex_lock(&inv_i2c_client->lock);
    ret = i2c_smbus_write_i2c_block_data_retry(inv_i2c_client->client, offset, len, buf);
    mutex_unlock(&inv_i2c_client->lock);

    return ret;
}
#if 0
static int cpld_ioin_update(struct cplddev_t *dev, sku_sff_extra_io_type_t type)
{
    int ret = 0;
    struct ldata_format_t *io_st = &(dev->sff_extra_io_st[type]);
    const struct sff_reg_t *sff_reg = sff_extra_io_reg[type];
    int i = 0;
    struct inv_i2c_client_t *inv_i2c_client = NULL;
    u8 *buf = NULL;
    int buf_offset = 0;
    unsigned long bitmap = 0;
    buf = (u8 *)(&bitmap);

    for (i = 0; true != sff_reg[i].is_end; i++) {

        if (!p_valid(inv_i2c_client = inv_i2c_client_find(dev, &(dev->i2c_config[sff_reg[i].cpld_id])))) {
            io_st->valid = false;
            return -EBADRQC;
        }
        buf_offset = sff_reg[i].port_1st / 8;
        if ((ret = inv_i2c_smbus_read_block_data(inv_i2c_client,
                   sff_reg[i].offset,
                   sff_reg[i].size,
                   buf + buf_offset)) < 0) {
            io_st->valid = false;
            return ret;
        }
    }

    io_st->valid = true;
    io_st->bitmap = bitmap;
    return 0;
}
#endif
static int cpld_ioin_get(struct cplddev_t *dev, sku_sff_extra_io_type_t type, unsigned long *bitmap)
{
    check_p(dev);
    check_p(bitmap);

    if (!dev->sff_extra_io_st[type].valid) {
        return -EBADRQC;
    }
    *bitmap = dev->sff_extra_io_st[type].bitmap;
    return 0;
}

static int cpld_ioout_set(struct cplddev_t *dev, sku_sff_extra_io_type_t type, unsigned long bitmap)
{
    int ret = 0;
    const struct sff_reg_t *sff_reg = sff_extra_io_reg[type];
    int i = 0;
    struct inv_i2c_client_t *inv_i2c_client = NULL;
    u8 *buf = NULL;
    int buf_offset = 0;
    check_p(dev);
    
    buf = (u8 *)(&bitmap);

    for (i = 0; true != sff_reg[i].is_end; i++) {

        if (!p_valid(inv_i2c_client = inv_i2c_client_find(dev, &(dev->i2c_config[sff_reg[i].cpld_id])))) {
            return -EBADRQC;
        }
        buf_offset = sff_reg[i].port_1st / 8;
        if ((ret = inv_i2c_smbus_write_block_data(inv_i2c_client,
                   sff_reg[i].offset,
                   sff_reg[i].size,
                   buf + buf_offset)) < 0) {
            return ret;
        }
    }

    return 0;
}
static int cpld_ioout_get(struct cplddev_t *dev, sku_sff_extra_io_type_t type, unsigned long *bitmap)
{
    int ret = 0;
    const struct sff_reg_t *sff_reg = sff_extra_io_reg[type];
    int i = 0;
    struct inv_i2c_client_t *inv_i2c_client = NULL;
    u8 *buf = NULL;
    int buf_offset = 0;
    unsigned long tmp = 0;
    
    check_p(dev);
    check_p(bitmap);
    
    buf = (u8 *)(&tmp);

    for (i = 0; true != sff_reg[i].is_end; i++) {

        if (!p_valid(inv_i2c_client = inv_i2c_client_find(dev, &(dev->i2c_config[sff_reg[i].cpld_id])))) {
            return -EBADRQC;
        }
        buf_offset = sff_reg[i].port_1st / 8;
        if ((ret = inv_i2c_smbus_read_block_data(inv_i2c_client,
                   sff_reg[i].offset,
                   sff_reg[i].size,
                   buf + buf_offset)) < 0) {
            return ret;
        }
    }

    *bitmap = tmp;
    return 0;
}

static int inv_i2c_client_init(int i2c_ch,  struct i2c_client **client)
{
    struct i2c_adapter *adap = NULL;

    check_p(client);
    
    adap = i2c_get_adapter(i2c_ch);
    if (!p_valid(adap)) {
        SKU_LOG_ERR("get adapter fail ch:%d\n", i2c_ch);
        return -EBADRQC;
    }

    SKU_LOG_DBG("get adapter ok ch:%d\n", i2c_ch);
    (*client)->adapter = adap;

    return 0;
}

static void inv_i2c_client_deinit_all(struct cplddev_t *self)
{
    struct i2c_client *client = NULL;
    int id = 0;
    if (!self) {
        return;
    }

    for (id = 0; id < I2C_CH_ID_NUM; id++) {
        client = self->inv_i2c_client[id].client;
        if (p_valid(client)) {
            if (p_valid(client->adapter)) {
                i2c_put_adapter(client->adapter);
                SKU_LOG_DBG("put_adapter:%d\n", id);
            }
        }
    }
}
static void inv_i2c_clients_destroy(struct cplddev_t *self)
{
    int i2c_ch_id = 0;

    if (p_valid(invI2cClient)) {
        kfree(invI2cClient);
    }
    for (i2c_ch_id = 0; i2c_ch_id < I2C_CH_ID_NUM; i2c_ch_id++) {
        self->inv_i2c_client[i2c_ch_id].client = NULL;
    }
}

static int inv_i2c_clients_create(struct cplddev_t *self)
{
    int i2c_ch_id = 0;
    struct i2c_client *client = NULL;


    client = kzalloc(sizeof(struct i2c_client)*I2C_CH_ID_NUM, GFP_KERNEL);
    if (!p_valid(client)) {
        return -EBADRQC;
    }
    invI2cClient = client;
    /*build a link*/
    for (i2c_ch_id = 0; i2c_ch_id < I2C_CH_ID_NUM; i2c_ch_id++) {

        self->inv_i2c_client[i2c_ch_id].client = invI2cClient + i2c_ch_id;
    }
    return 0;
}
static int inv_i2c_client_init_all(struct cplddev_t *self)
{
    int i2c_ch_id = 0;
    struct inv_i2c_client_t *inv_i2c_client = NULL;
    int ret = 0;

    check_p(self);
    for (i2c_ch_id = 0; i2c_ch_id < I2C_CH_ID_NUM; i2c_ch_id++) {

        inv_i2c_client = &(self->inv_i2c_client[i2c_ch_id]);
        if ((ret = inv_i2c_client_init(i2cChTbl[i2c_ch_id], &(inv_i2c_client->client))) < 0) {
            break;
        }
        mutex_init(&(inv_i2c_client->lock));

    }
    if (ret < 0) {
        inv_i2c_client_deinit_all(self);
        return ret;
    }

    return 0;
}
static void config_load(void)
{
    int cpld_id = 0;

    for (cpld_id = 0; cpld_id < CPLD_ID_NUM; cpld_id++) {
        memcpy((banyan8TSku1.cpld.i2c_config) + cpld_id, i2c_config_tbl+cpld_id, sizeof(struct i2c_config_t));
    }
}

static int sku_init(int platform_id, int io_no_init)
{
    int ret = 0;
    (void)platform_id;

    config_load();
    if (fpga_func_load(&banyan8TSku1) < 0) {
        goto err_exit;
    }

    if (inv_i2c_clients_create(&banyan8TSku1.cpld) < 0) {
        goto err_exit;
    }

    if (inv_i2c_client_init_all(&banyan8TSku1.cpld) < 0) {
        goto kfree_inv_i2c_client;
    }

    SKU_LOG_INFO("ok\n");
    return 0;
kfree_inv_i2c_client:
    inv_i2c_clients_destroy(&banyan8TSku1.cpld);
err_exit:
    ret = -EBADRQC;
    return ret;

}

static void sku_deinit(void)
{
    inv_i2c_client_deinit_all(&banyan8TSku1.cpld);
    inv_i2c_clients_destroy(&banyan8TSku1.cpld);
    SKU_LOG_INFO("ok\n");
}

static int sku_hdlr(void)
{
    int ret = 0;
    struct sku_t *sku = &banyan8TSku1;

    if ((ret = sku_sff_ioin_update(sku, SKU_SFF_PRS_TYPE)) < 0) {
        return ret;
    }
    if ((ret = sku_sff_ioin_update(sku, SKU_SFF_INTR_TYPE)) < 0) {
        return ret;
    }
/*<TBD> how to verify the function? comment it out currently*/    
#if 0 
    if ((ret = cpld_ioin_update(&(sku->cpld), SKU_SFF_OC_TYPE)) < 0) {
        return ret;
    }
#endif    
    return 0;
}

static int sku_sff_get_ready_action(int lc_id, int port)
{
    int ret = 0;
    struct sku_t *sku = &banyan8TSku1;
    (void)lc_id;
    
    /*configur polling pages*/
    if ((ret = sku->sff_polling_set(sku->fpga, port, true)) < 0) {
        return ret;
    }
    return 0;
}

static int sku_sff_detected_action(int lc_id, int port)
{
    int ret = 0;
    struct sku_t *sku = &banyan8TSku1;
    (void)lc_id;
    
    if ((ret = sku->sff_polling_set(sku->fpga, port, false)) < 0) {
        return ret;
    }
    return 0;
}

static int fpga_func_load(struct sku_t *sku)
{
    struct device *dev = NULL;
    struct fpga_dev *fpga = NULL;
    
    if (!p_valid(dev = find_dev(PCIE_BUS_NAME, &pci_bus_type))) {
        return -EBADRQC;
    }
    
    if (!p_valid(fpga = dev_get_drvdata(dev))) {
        return -EBADRQC;
    }
    if ( fpga == NULL ) {
        SKU_LOG_ERR("fpga is the NULL pointer \n");
        return -EBADRQC;
    }
    else {

        sku->fpga = fpga;

        if ( fpga->fpga_sff_prs_get == NULL )
        {
            SKU_LOG_ERR("fpga->fpga_sff_prs_get is the NULL pointer \n");
            return -EBADRQC;
        }
        if ( fpga->fpga_sff_intr_get == NULL )
        {
            SKU_LOG_ERR("fpga->fpga_sff_intr_get is the NULL pointer \n");
            return -EBADRQC;
        }
        if ( fpga->fpga_sff_lpmode_get == NULL )
        {
            SKU_LOG_ERR("fpga->fpga_sff_lpmode_get is the NULL pointer \n");
            return -EBADRQC;
        }
        if ( fpga->fpga_sff_lpmode_set == NULL )
        {
            SKU_LOG_ERR("fpga->fpga_sff_lpmode_set is the NULL pointer \n");
            return -EBADRQC;
        }
        if ( fpga->fpga_sff_reset_get == NULL )
        {
            SKU_LOG_ERR("fpga->fpga_sff_reset_get is the NULL pointer \n");
            return -EBADRQC;
        }
        if ( fpga->fpga_sff_reset_set == NULL )
        {
            SKU_LOG_ERR("fpga->fpga_sff_reset_set is the NULL pointer \n");
            return -EBADRQC;
        }
        if ( fpga->fpga_sff_modsel_get == NULL )
        {
            SKU_LOG_ERR("fpga->fpga_sff_modsel_get is the NULL pointer \n");
            return -EBADRQC;
        }
        if ( fpga->fpga_sff_modsel_set == NULL )
        {
            SKU_LOG_ERR("fpga->fpga_sff_modsel_set is the NULL pointer \n");
            return -EBADRQC;
        }
        if ( fpga->fpga_sff_polling_status_get == NULL )
        {
            SKU_LOG_ERR("fpga->fpga_sff_polling_status_get is the NULL pointer \n");
            return -EBADRQC;
        }
        if ( fpga->fpga_sff_polling_set == NULL )
        {
            SKU_LOG_ERR("fpga->fpga_sff_polling_set is the NULL pointer \n");
            return -EBADRQC;
        }
        if ( fpga->fpga_sff_polling_get == NULL )
        {
            SKU_LOG_ERR("fpga->fpga_sff_polling_get is the NULL pointer \n");
            return -EBADRQC;
        }
        /*sff io*/
        sku->sff_io_func[SKU_SFF_PRS_TYPE].get = fpga->fpga_sff_prs_get;
        sku->sff_io_func[SKU_SFF_INTR_TYPE].get = fpga->fpga_sff_intr_get;
        sku->sff_io_func[SKU_SFF_LPMODE_TYPE].get = fpga->fpga_sff_lpmode_get;
        sku->sff_io_func[SKU_SFF_LPMODE_TYPE].set = fpga->fpga_sff_lpmode_set;
        sku->sff_io_func[SKU_SFF_RESET_TYPE].get = fpga->fpga_sff_reset_get;
        sku->sff_io_func[SKU_SFF_RESET_TYPE].set = fpga->fpga_sff_reset_set;
        sku->sff_io_func[SKU_SFF_MODSEL_TYPE].get = fpga->fpga_sff_modsel_get;
        sku->sff_io_func[SKU_SFF_MODSEL_TYPE].set = fpga->fpga_sff_modsel_set;
        sku->sff_polling_status_get = fpga->fpga_sff_polling_status_get;
        sku->sff_polling_set = fpga->fpga_sff_polling_set;
        sku->sff_polling_get = fpga->fpga_sff_polling_get;
    }
    return 0;
}    
