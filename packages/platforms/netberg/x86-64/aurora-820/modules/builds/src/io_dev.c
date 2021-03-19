/*io_dev.c
 * handle sff io pin control , mux gpio .. cpld  etc*/
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
#include "net_swps.h"
#include "io_dev.h"
#include "net_def.h"
#include "io_config/io_config.h"


#define SFF_IO_LOG_ERR(fmt, args...) \
    do { \
        if (logLevel & DEV_ERR_LEV) \
        { \
            printk (KERN_ERR "[SFF_IO]%s:"fmt,__FUNCTION__,  ##args); \
        } \
    } while (0)

#define SFF_IO_LOG_INFO(fmt, args...) \
    do { \
        if (logLevel & DEV_INFO_LEV) \
        { \
            printk (KERN_INFO "[SFF_IO]%s:"fmt,__FUNCTION__,  ##args); \
        } \
    } while (0)

#define SFF_IO_LOG_DBG(fmt, args...) \
    do { \
        if (logLevel & DEV_DBG_LEV) \
        { \
            printk (KERN_INFO "[SFF_IO]%s:"fmt,__FUNCTION__,  ##args); \
        } \
    } while (0)

#define MUX_GPIO_LABEL "MUX_RST"
#define CPLD_INT_GPIO_LABEL "CPLD_IO_INT"

extern u32 logLevel;
struct io_dev_t ioDev;

#define ioexp_to_io(x) container_of(x, struct io_dev_t, ioexp_dev)
#define cpld_to_io(x) container_of(x, struct io_dev_t, cpld_io)

struct ioexp_config_t *ioexp_head = NULL;
struct ioexp_config_t *ioexp_tail = NULL;
struct i2c_client *ioexpI2cClient = NULL;
static int gpio_base = 0;
static int pca9555_word_read(struct io_dev_client_t *ioexp_client, u8 offset);
static int pca9555_word_write(struct io_dev_client_t *ioexp_client, u8 offset, u16 val);
static int pca9555_byte_read(struct io_dev_client_t *ioexp_client, u8 offset);
static int pca9555_byte_write(struct io_dev_client_t *ioexp_client, u8 offset, u16 val);
struct io_dev_client_t *ioexp_client_find(struct ioexp_dev_t *self, int ch_id, u8 addr);
static struct pca95xx_func_t *_pca95xx_func_find(struct ioexp_config_t *ioexp_config);
static struct pca95xx_func_t *pca95xx_func_find(struct ioexp_config_map_t *map, int ioexp_id);
static bool cpld_io_intr_is_asserted(struct cpld_io_t *cpld_io);
static void cpld_io_i2c_clients_destroy(struct cpld_io_t *cpld_io);
static void cpld_io_clients_destroy(struct cpld_io_t *cpld_io);
static int cpld_io_clients_create(struct cpld_io_t *cpld_io);
static struct i2c_client *cpld_io_i2c_client_create_init(int ch);
static int cpld_io_i2c_clients_create_init(struct cpld_io_t *cpld_io);
/*
typedef enum {
    IOEXP_LPMODE_TYPE,
    IOEXP_RESET_TYPE,
    IOEXP_TXDIABLE_TYPE,
    IOEXP_MODESEL_TYPE,
    IOEXP_TXDIABLE2_TYPE,
    IOEXP_OUTPUT_TYPE_NUM,
} ioexp_output_type_t;
*/
const int ioexp_out_def_val[IOEXP_OUTPUT_TYPE_NUM] = {
    [IOEXP_LPMODE_TYPE] = IO_LPMODE_DEFAULT,
    [IOEXP_RESET_TYPE] = IO_RESET_DEFAULT,
    [IOEXP_TXDISABLE_TYPE] = IO_TXDISABLE_DEFAULT,
    [IOEXP_MODESEL_TYPE] = IO_MODESEL_DEFAULT,
    [IOEXP_TXDISABLE2_TYPE] = IO_TXDISABLE_DEFAULT,
};

const char *ioexp_output_name[IOEXP_OUTPUT_TYPE_NUM] = {
    [IOEXP_LPMODE_TYPE] = "IOEXP_LPMODE",
    [IOEXP_RESET_TYPE] = "IOEXP_RESET",
    [IOEXP_TXDISABLE_TYPE] = "IOEXP_TXDISABLE",
    [IOEXP_MODESEL_TYPE] = "IOEXP_MODESEL_TYPE",
    [IOEXP_TXDISABLE2_TYPE] = "IOEXP_TXDISABLE2",
};
/*
typedef enum {
    IOEXP_PRS_TYPE,
    IOEXP_TXFAULT_TYPE,
    IOEXP_RXLOS_TYPE,
    IOEXP_INT_TYPE,
    IOEXP_TXFAULT2_TYPE,
    IOEXP_RXLOS2_TYPE,
    IOEXP_INPUT_TYPE_NUM,
} ioexp_input_type_t;
*/
const char *ioexp_input_st_name[IOEXP_INPUT_TYPE_NUM] = {
    [IOEXP_PRS_TYPE] = "IOEXP_PRS",
    [IOEXP_TXFAULT_TYPE] = "IOEXP_TXFAULT",
    [IOEXP_RXLOS_TYPE] = "IOEXP_RXLOS",
    [IOEXP_INT_TYPE] = "IOEXP_INT",
    [IOEXP_TXFAULT2_TYPE] = "IOEXP_TXFAULT2",
    [IOEXP_RXLOS2_TYPE] = "IOEXP_RXLOS2",
};

/*exported functions*/
/*sff io*/
int ioexp_prsL_all_get(int lc_id, unsigned long *bitmap);
int ioexp_intr_all_get(int lc_id, unsigned long *bitmap);
int ioexp_rx_los_all_get(int lc_id, unsigned long *bitmap) ;
int ioexp_tx_fault_all_get(int lc_id, unsigned long *bitmap);
int ioexp_oc_all_get(int lc_id, unsigned long *bitmap);

int ioexp_reset_set(int lc_id, int port, u8 reset);
int ioexp_reset_get(int lc_id, int port, u8 *reset);
int ioexp_lpmode_set(int lc_id, int port, u8 value);
int ioexp_lpmode_get(int lc_id, int port, u8 *value);
int ioexp_tx_disable_set(int lc_id, int port, u8 value);
int ioexp_tx_disable_get(int lc_id, int port, u8 *value);
int ioexp_modesel_set(int lc_id, int port, u8 value);
int ioexp_modesel_get(int lc_id, int port, u8 *value);
static int ioexp_power_set(int lc_id, int port, u8 val);
static int ioexp_power_get(int lc_id, int port, u8 *val);
static int ioexp_reset_all_set(int lc_id, unsigned long bitmap);
static int ioexp_reset_all_get(int lc_id, unsigned long *bitmap);
static int ioexp_lpmode_all_set(int lc_id, unsigned long bitmap);
static int ioexp_lpmode_all_get(int lc_id, unsigned long *bitmap);

struct sff_io_driver_t ioDevSffIoDrv = {
    .prs_all_get = ioexp_prsL_all_get,
    .intr_all_get = ioexp_intr_all_get,
    .oc_all_get = ioexp_oc_all_get,
    .rx_los_all_get = ioexp_rx_los_all_get,
    .tx_fault_all_get = ioexp_tx_fault_all_get,
    .reset_set = ioexp_reset_set,
    .reset_get = ioexp_reset_get,
    .reset_all_set = ioexp_reset_all_set,
    .reset_all_get = ioexp_reset_all_get,
    .power_set = ioexp_power_set,
    .power_get = ioexp_power_get,
    .lpmode_set = ioexp_lpmode_set,
    .lpmode_get = ioexp_lpmode_get,
    .lpmode_all_set = ioexp_lpmode_all_set,
    .lpmode_all_get = ioexp_lpmode_all_get,
    .tx_disable_set = ioexp_tx_disable_set,
    .tx_disable_get = ioexp_tx_disable_get,
    .mode_sel_set = ioexp_modesel_set,
    .mode_sel_get = ioexp_modesel_get,
};

struct sff_io_driver_t *sff_io_drv_get_iodev(void)
{
    return &ioDevSffIoDrv;
}

typedef enum {
    PCA95XX_INPUT,
    PCA95XX_OUTPUT,
    PCA95XX_CONFIG,
    PCA95XX_OFFSET_TYPE_NUM,

} pca95xx_offset_type_t;

struct pca95xx_func_t {
    u8 offset[PCA95XX_OFFSET_TYPE_NUM];
    int (*read)(struct io_dev_client_t *ioexp_client , u8 offset);
    int (*write)(struct io_dev_client_t *ioexp_client, u8 offset, u16 val);
};
struct pca95xx_func_t pca95xx_func[PCA95XX_TYPE_NUM] = {
    [PCA_UNKOWN_TYPE] =
    {   {0x00,0x02,0x06}, /*pca9555*/
        .read = pca9555_word_read,
        .write =  pca9555_word_write
    },
    [PCA9555_TYPE] =
    {   {0x00,0x02,0x06}, /*pca9555*/
        .read = pca9555_word_read,
        .write =  pca9555_word_write
    },

    [PCA9554_TYPE] =
    {   {0x00,0x01,0x03},/*pca9554*/
        .read = pca9555_byte_read,
        .write =  pca9555_byte_write
    },
};
static int valid_num_get(unsigned long bitmap)
{
    int num = 0;
    int i = 0;
    int size = sizeof(bitmap) * 8;

    for (i = 0; i < size; i++) {
        if (test_bit(i, &bitmap)) {
            num++;
        }
    }
    return num;
}
/*[note] when sff module is prsL , corrensponding bit be set*/
int ioexp_prsL_all_get(int lc_id, unsigned long *bitmap)
{
    int prs_num = ioDev.ioexp_dev.input_port_num[IOEXP_PRS_TYPE];
    struct ldata_format_t *ldata = &(ioDev.ioexp_dev.input_st[IOEXP_PRS_TYPE]);
    unsigned valid_num = valid_num_get(ldata->valid);
    if (prs_num != valid_num) {
        return -EBADRQC;
    }
    *bitmap = ldata->bitmap;
    return 0;
}
EXPORT_SYMBOL(ioexp_prsL_all_get);

int ioexp_oc_all_get(int lc_id, unsigned long *bitmap)
{
    return -1;
}
int ioexp_intr_all_get(int lc_id, unsigned long *bitmap)
{
    struct ldata_format_t *ldata = &(ioDev.ioexp_dev.input_st[IOEXP_INT_TYPE]);
    int intr_num = ioDev.ioexp_dev.input_port_num[IOEXP_INT_TYPE];
    unsigned valid_num = valid_num_get(ldata->valid);

    if (0 == intr_num) {
        return REC_SFF_IO_UNSUPPORTED;
    }
    if (intr_num != valid_num) {
        return -EBADRQC;
    }

    *bitmap = ldata->bitmap;
    return 0;
}
int ioexp_rx_los_all_get(int lc_id, unsigned long *bitmap)
{
    struct ldata_format_t *ldata = &(ioDev.ioexp_dev.input_st[IOEXP_RXLOS_TYPE]);
    int rxlos_num = ioDev.ioexp_dev.input_port_num[IOEXP_RXLOS_TYPE];
    unsigned valid_num = valid_num_get(ldata->valid);

    if (0 == rxlos_num) {
        return REC_SFF_IO_UNSUPPORTED;
    }
    if (rxlos_num != valid_num) {
        return -EBADRQC;
    }

    *bitmap = ldata->bitmap;
    return 0;
}
int ioexp_tx_fault_all_get(int lc_id, unsigned long *bitmap)
{
    struct ldata_format_t *ldata = &(ioDev.ioexp_dev.input_st[IOEXP_TXFAULT_TYPE]);
    int txfault_num = ioDev.ioexp_dev.input_port_num[IOEXP_TXFAULT_TYPE];
    unsigned valid_num = valid_num_get(ldata->valid);

    if (0 == txfault_num) {
        return REC_SFF_IO_UNSUPPORTED;
    }

    if (txfault_num != valid_num) {
        return -EBADRQC;
    }

    *bitmap = ldata->bitmap;
    return 0;
}

static int ioexp_power_set(int lc_id, int port, u8 val)
{
    return 0;
}
static int ioexp_power_get(int lc_id, int port, u8 *val)
{
    /*always true*/
    *val = 1;
    return 0;
}

static void ioexp_i2c_clients_destroy(struct ioexp_dev_t *ioexp_dev)
{
    int id = 0;
    int size = ioexp_dev->i2c_ch_map->size;

    if (!p_valid(ioexp_dev)) {
        return;
    }
    if (p_valid(ioexpI2cClient)) {
        kfree(ioexpI2cClient);
    }
    for (id = 0; id < size; id++) {
        ioexp_dev->ioexp_client[id].client = NULL;
    }
}

static void ioexp_i2c_clients_deinit(struct ioexp_dev_t *ioexp_dev)
{
    struct i2c_client *client = NULL;
    int id = 0;
    int size = ioexp_dev->i2c_ch_map->size;

    if (!p_valid(ioexp_dev)) {
        return;
    }

    for (id = 0; id < size; id++) {
        client = ioexp_dev->ioexp_client[id].client;
        if (p_valid(client)) {
            if (p_valid(client->adapter)) {
                i2c_put_adapter(client->adapter);
            }
        }
    }
}

static void ioexp_clients_destroy(struct ioexp_dev_t *ioexp_dev)
{
    if (p_valid(ioexp_dev->ioexp_client)) {
        kfree(ioexp_dev->ioexp_client);
    }
}
static int ioexp_clients_create(struct ioexp_dev_t *ioexp_dev)
{
    int size = ioexp_dev->i2c_ch_map->size;
    struct io_dev_client_t *ioexp_client = NULL;
    ioexp_client = kzalloc(sizeof(struct io_dev_client_t) * size, GFP_KERNEL);
    if (!p_valid(ioexp_client)) {
        return -ENOMEM;
    }
    ioexp_dev->ioexp_client = ioexp_client;
    return 0;
}

int ioexp_i2c_client_init(int ch, struct i2c_client **client)
{
    struct i2c_adapter *adap = NULL;

    if (!p_valid(*client)) {
        return -EBADRQC;
    }
    adap = i2c_get_adapter(ch);
    if (!p_valid(adap)) {
    SFF_IO_LOG_ERR("get adapter fail ch:%d\n", ch);
    return -EBADRQC;
    }

    SFF_IO_LOG_DBG("get adapter ok ch:%d\n", ch);
    (*client)->adapter = adap;

    return 0;
}

static int ioexp_i2c_clients_init(struct ioexp_dev_t *ioexp_dev)
{
    int ret = 0;
    int i = 0;
    int size = ioexp_dev->i2c_ch_map->size;
    int *tbl = ioexp_dev->i2c_ch_map->tbl;
    struct io_dev_client_t *ioexp_client = NULL;

    for (i = 0; i < size; i++) {
        ioexp_client = &ioexp_dev->ioexp_client[i];
        if ((ret = ioexp_i2c_client_init(tbl[i], &(ioexp_client->client))) < 0) {
            break;
        }
        mutex_init(&(ioexp_client->lock));

    }
    if (ret < 0) {
        ioexp_i2c_clients_deinit(ioexp_dev);
        return ret;
    }
    return 0;
}
static int ioexp_i2c_clients_create(struct ioexp_dev_t *ioexp_dev)
{
    int i = 0;
    int size = ioexp_dev->i2c_ch_map->size;
    struct i2c_client *client = NULL;

    client = kzalloc(sizeof(struct i2c_client)*size, GFP_KERNEL);
    if (!p_valid(client)) {
        return -EBADRQC;
    }
    ioexpI2cClient = client;
    /*build a link*/
    for (i = 0; i < size; i++) {
        ioexp_dev->ioexp_client[i].client = &ioexpI2cClient[i];
    }
    return 0;
}
static void cpld_io_i2c_clients_destroy(struct cpld_io_t *cpld_io)
{
    struct i2c_client *client = NULL;
    int id = 0;
    int size = cpld_io->config->i2c_ch_map->size;
    
    for (id = 0; id < size; id++) {
        client = cpld_io->cpld_client[id].client;
        if (p_valid(client)) {
            if (p_valid(client->adapter)) {
                i2c_put_adapter(client->adapter);
            }
            kfree(client);
        }
    }
}

static void cpld_io_clients_destroy(struct cpld_io_t *cpld_io)
{
    if (p_valid(cpld_io->cpld_client)) {
        kfree(cpld_io->cpld_client);
    }
}
static int cpld_io_clients_create(struct cpld_io_t *cpld_io)
{
    int size = cpld_io->config->i2c_ch_map->size;
    struct io_dev_client_t *cpld_client = NULL;
    cpld_client = kzalloc(sizeof(struct io_dev_client_t) * size, GFP_KERNEL);
    if (!p_valid(cpld_client)) {
        return -ENOMEM;
    }
    cpld_io->cpld_client = cpld_client;
    return 0;
}


static struct i2c_client *cpld_io_i2c_client_create_init(int ch)
{
    struct i2c_client *client = NULL;
    struct i2c_adapter *adap = NULL;

    client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);
    if (!p_valid(client)) {
        goto exit_err;
    }
    adap = i2c_get_adapter(ch);
    if (!p_valid(adap)) {
        SFF_IO_LOG_ERR("get adapter fail ch:%d\n", ch);
        goto exit_kfree_i2c_client;
    }
    client->adapter = adap;
    return client;

exit_kfree_i2c_client:
    kfree(client);
exit_err:
    return NULL;
}
static int cpld_io_i2c_clients_create_init(struct cpld_io_t *cpld_io)
{
    int i = 0;
    int size = cpld_io->config->i2c_ch_map->size;
    int *tbl = cpld_io->config->i2c_ch_map->tbl;
    struct i2c_client *client = NULL;

    for (i = 0; i < size; i++) {
        client = cpld_io_i2c_client_create_init(tbl[i]);
        if (!p_valid(client)) {
            break;
        }
        cpld_io->cpld_client[i].client = client;
        mutex_init(&(cpld_io->cpld_client[i].lock));

    }
    if (i < size) {
        cpld_io_i2c_clients_destroy(cpld_io);
        return -EBADRQC;
    }
    return 0;
}

int io_dev_mux_rst_gpio_init(int mux_rst_gpio)
{
    /*pull high*/
    int ret = 0;
    /*<TBD> insmod gpiobase later on*/
    mux_rst_gpio += gpio_base;
    SFF_IO_LOG_DBG("mux rst gpio:%d \n", mux_rst_gpio);
    if ((ret = gpio_is_valid(mux_rst_gpio)) < 0) {
        SFF_IO_LOG_ERR("invaid gpio:%d ret:%d\n", mux_rst_gpio, ret);
        return -1;
    }
    if ((ret = gpio_request(mux_rst_gpio, MUX_GPIO_LABEL)) < 0) {
        SFF_IO_LOG_ERR("gpio:%d request fail\n", mux_rst_gpio);
    }
    if(gpio_direction_output(mux_rst_gpio, 1) < 0) {
        SFF_IO_LOG_ERR("init NG\n");
        return -1;
    }
    mdelay(1); /*1ms*/

    SFF_IO_LOG_ERR("init Pass\n");
    return 0;
}

int io_dev_mux_reset_get(int lc_id, int *val)
{
    int lv = 0;
    lv = gpio_get_value(ioDev.mux_rst_gpio);
    *val = lv;
    return 0;
}

/*io mux control*/
int io_dev_mux_reset_set(int lc_id, int value)
{
    gpio_set_value(ioDev.mux_rst_gpio, value);
    return 0;
}

static bool _ioexp_is_channel_ready(struct ioexp_dev_t *ioexp_dev, struct ioexp_config_t *config)
{
    int ret = 0;
    struct pca95xx_func_t *func = NULL;
    struct io_dev_client_t *ioexp_client = NULL;

    if (!p_valid(config)) {
        SFF_IO_LOG_ERR("NULL ptr\n");
        return false;
    }

    if (!p_valid(ioexp_client = ioexp_client_find(ioexp_dev, config->ch_id, config->addr))) {
        return false;
    }
    if (!p_valid(func = _pca95xx_func_find(config))) {
        return false;
    }
    if ((ret = func->read(ioexp_client, func->offset[PCA95XX_CONFIG])) < 0) {
        return false;
    }
    if (ret != config->val) {
        SFF_IO_LOG_ERR("ch:%d config mismatch default:%x reg:%x\n", config->ch_id, config->val, ret);
        return false;
    }
    return true;
}
bool ioexp_is_channel_ready(int lc_id)
{
    bool h_ready = false;
    bool t_ready = false;
    (void)lc_id;

    if (!(h_ready = _ioexp_is_channel_ready(&(ioDev.ioexp_dev), ioexp_head))) {
        return false;
    }
    if (!(t_ready = _ioexp_is_channel_ready(&(ioDev.ioexp_dev), ioexp_tail))) {
        return false;
    }

    return true;
}

int ioexp_config_init(struct ioexp_dev_t *ioexp_dev)
{
    int ioexp_id = 0;
    int ret = 0;
    struct ioexp_config_t *config = NULL;
    struct ioexp_config_t *tbl = NULL;
    struct pca95xx_func_t *func = NULL;
    struct io_dev_client_t *ioexp_client = NULL;
    int size = 0;

    tbl = ioexp_dev->config_map->tbl;
    size = ioexp_dev->config_map->size;

    for (ioexp_id = 0; ioexp_id < size; ioexp_id++) {
        config = &tbl[ioexp_id];

        if (!p_valid(ioexp_client = ioexp_client_find(ioexp_dev, config->ch_id, config->addr))) {
            ret = -EBADRQC;
            break;
        }
        if (!p_valid(func = pca95xx_func_find(ioexp_dev->config_map, ioexp_id))) {
            ret = -EBADRQC;
            break;
        }

        if ((ret = func->write(ioexp_client, func->offset[PCA95XX_CONFIG], config->val)) < 0) {
            SFF_IO_LOG_ERR("io config fail: ch_id:0x%x addr:0x%x val:0x%x\n", config->ch_id, config->addr, config->val);
            break;
        }
    }
    if (ret < 0) {
        return ret;
    }
    /*assign for ioexp channel check*/
    for (ioexp_id = 0; ioexp_id < size; ioexp_id++) {
        config = &tbl[ioexp_id];
        if (config->val != 0) {
            ioexp_head = config;
            break;
        }
    }
    for (ioexp_id = size - 1; ioexp_id >= 0; ioexp_id--) {
        config = &tbl[ioexp_id];
        if (config->val != 0) {
            ioexp_tail = config;
            break;
        }

    }

    if (NULL == ioexp_head ||
            NULL == ioexp_tail) {
        SFF_IO_LOG_ERR("ioexp_head tail config fail\n");
        return -1;
    } else {
        SFF_IO_LOG_INFO("ioexp_head ch_id:%d addr:0x%x\n", ioexp_head->ch_id, ioexp_head->addr);
        SFF_IO_LOG_INFO("ioexp_tail ch_id:%d addr:0x%x\n", ioexp_tail->ch_id, ioexp_tail->addr);
    }
    return 0;
}

struct io_dev_client_t *ioexp_client_find(struct ioexp_dev_t *dev, int ch_id, u8 addr)
{
    struct io_dev_client_t *ioexp_client = NULL;
    ioexp_client = &(dev->ioexp_client[ch_id]);
    ioexp_client->client->addr = addr;

    return ioexp_client;
}

/*update the pin status of corresonding ioexpander control(ex: prsL) by port*/
void ioexp_input_st_update(struct ldata_format_t ioexp_input_st[],
                           struct pin_layout_t layout_tbl[],
                           int data)
{
    int port = 0;
    int bit = 0;
    int st = 0;
    int old_st = 0;
    int i = 0;
    struct ldata_format_t *input_st = NULL;
    struct pin_layout_t *layout = NULL;
    char *port_name = NULL;

    for (i = 0; true != layout_tbl[i].end_of_layout; i++) {

        layout = &layout_tbl[i];
        port = layout->port_min;
        input_st = &(ioexp_input_st[layout->type]);

        for (bit = layout->bit_min; bit <= layout->bit_max; bit++, port++) {
            st = ((test_bit(bit, (unsigned long *)&data)) ? 1 : 0);
            /*debug msg*/
            old_st = ((test_bit(port, &(input_st->bitmap))) ? 1 : 0);
            if (old_st != st) {
                port_name = port_name_get(0, port);
                if (p_valid(port_name)) {
                    SFF_IO_LOG_DBG("%s %s st:%d->%d\n", ioexp_input_st_name[layout->type], port_name, old_st, st);
                }
            }
            if (st) {
                set_bit(port, &(input_st->bitmap));
            } else {
                clear_bit(port, &(input_st->bitmap));
            }
            set_bit(port, &(input_st->valid));
            //SFF_IO_LOG_DBG("%s data:%d ch:%d addr:0x%x st[%d]:%d\n", name, data, ioexp->ch, ioexp->addr, port, val);
        }
    }
}

void ioexp_input_st_update_fail(struct ldata_format_t ioexp_input_st[],
                                struct pin_layout_t layout_tbl[])
{
    int port = 0;
    int bit = 0;
    int i = 0;
    struct ldata_format_t *input_st = NULL;
    struct pin_layout_t *layout = NULL;

    for (i = 0; true != layout_tbl[i].end_of_layout; i++) {

        layout = &layout_tbl[i];
        port = layout->port_min;
        input_st = &(ioexp_input_st[layout->type]);

        for (bit = layout->bit_min; bit <= layout->bit_max; bit++, port++) {
            clear_bit(port, &(input_st->valid));
        }
    }
}
static struct pca95xx_func_t *_pca95xx_func_find(struct ioexp_config_t *ioexp_config)
{
    if (!p_valid(ioexp_config)) {
        return NULL;
    }

    if (PCA_UNKOWN_TYPE == ioexp_config->type) {
        return NULL;
    }

    return &pca95xx_func[ioexp_config->type];
}
static struct pca95xx_func_t *pca95xx_func_find(struct ioexp_config_map_t *map, int ioexp_id)
{
    /*ioexp_id range check*/
    if (ioexp_id < 0 && ioexp_id >= map->size) {
        return NULL;
    }
    return _pca95xx_func_find(&(map->tbl[ioexp_id]));
}

static int pca9555_byte_read(struct io_dev_client_t *ioexp_client, u8 offset)
{
    int ret = 0;
    mutex_lock(&ioexp_client->lock);
    ret = i2c_smbus_read_byte_data_retry(ioexp_client->client, offset);
    mutex_unlock(&ioexp_client->lock);

    return ret;
}
static int pca9555_byte_write(struct io_dev_client_t *ioexp_client, u8 offset, u16 val)
{
    int ret = 0;
    mutex_lock(&ioexp_client->lock);
    ret = i2c_smbus_write_byte_data_retry(ioexp_client->client, offset, val);
    mutex_unlock(&ioexp_client->lock);

    return ret;
}
static int pca9555_word_read(struct io_dev_client_t *ioexp_client, u8 offset)
{
    int ret = 0;
    mutex_lock(&ioexp_client->lock);
    ret = i2c_smbus_read_word_data_retry(ioexp_client->client, offset);
    mutex_unlock(&ioexp_client->lock);

    return ret;
}
static int pca9555_word_write(struct io_dev_client_t *ioexp_client, u8 offset, u16 val)
{
    int ret = 0;
    mutex_lock(&ioexp_client->lock);
    ret = i2c_smbus_write_word_data_retry(ioexp_client->client, offset, val);
    mutex_unlock(&ioexp_client->lock);

    return ret;
}
struct ioexp_output_t *find_ioexp_output_byPort(struct ioexp_output_t map[], int port)
{
    int i = 0;
    int port_max = 0;
    for (i = 0; map[i].end_of_tbl != true; i++) {
        port_max = map[i].port_min + (map[i].bit_max - map[i].bit_min);
        if (port <= port_max) {
            return &map[i];
        }
    }
    return NULL;
}
int ioexp_output_set(struct ioexp_dev_t *dev, ioexp_output_type_t type, int port, u8 val)
{
    int ret = 0;
    int bit = 0;
    u16 reg = 0;
    struct io_dev_client_t *ioexp_client = NULL;
    struct pca95xx_func_t *func = NULL;
    struct ioexp_output_t *ioexp = NULL;

    if (!p_valid(dev->output[type])) {
        if (p_valid(ioexp_output_name[type])) {
            SFF_IO_LOG_INFO("%s not supported\n", ioexp_output_name[type]);
        }
        return REC_SFF_IO_UNSUPPORTED;
    }

    if (!p_valid(ioexp = find_ioexp_output_byPort(dev->output[type], port))) {
        return -1;
    }
    if (!p_valid(ioexp_client = ioexp_client_find(dev, ioexp->ch_id, ioexp->addr))) {
        return -1;
    }

    if (!p_valid(func = pca95xx_func_find(dev->config_map, ioexp->ioexp_id))) {
        return -1;
    }

    if ((ret = func->read(ioexp_client, func->offset[PCA95XX_OUTPUT])) < 0) {
        SFF_IO_LOG_ERR("read fail! ioexp_id:%d ch_id:%d addr:0x%x\n",
                       ioexp->ioexp_id, ioexp->ch_id, ioexp->addr);
        return ret;
    }
    reg = ret;
    bit = ioexp->bit_min + (port - ioexp->port_min);
    if (val) {
        set_bit(bit, (unsigned long *)&reg);
    } else {
        clear_bit(bit, (unsigned long *)&reg);
    }

    if ((ret = func->write(ioexp_client, func->offset[PCA95XX_OUTPUT], reg)) < 0) {
        return ret;
    }
    SFF_IO_LOG_DBG("%s set ok ch_id:%d addr:0x%x reg:0x%x\n", ioexp_output_name[type], ioexp->ch_id, ioexp->addr, reg);
    return 0;
}
static void set_reg_update(struct ioexp_output_t *ioexp, unsigned long bitmap, u16 *reg)
{
    u16 new_reg = 0;
    int port_min = 0;
    int port_max = 0;
    u16 set_bits = 0;
    int bit = 0;
    int port = 0;
    
    port_min = ioexp->port_min;
    port_max = ioexp->port_min + (ioexp->bit_max - ioexp->bit_min);
    
    for (port = port_min, bit = ioexp->bit_min; port <= port_max; port++, bit++) {
        if (test_bit(port, &bitmap)) {
            set_bit(bit, (unsigned long *)&set_bits);
        }
    }
    
    SFF_IO_LOG_DBG("bitmap:0x%lx set_bits:0x%x\n", bitmap, set_bits);
    for (bit = ioexp->bit_min; bit <= ioexp->bit_max; bit++) {
        if (test_bit(bit, (unsigned long *)&set_bits)) {
            set_bit(bit, (unsigned long *)&new_reg);
        } else {
            clear_bit(bit, (unsigned long *)&new_reg);
        }
    }
    *reg = new_reg;
}

static void recv_reg_update(struct ioexp_output_t *ioexp, u16 reg, unsigned long *bitmap)
{
    int port = 0;
    int bit = 0;
    unsigned long recv_bitmap = 0;

    recv_bitmap = *bitmap;
    
    for (port = ioexp->port_min, bit = ioexp->bit_min; bit <= ioexp->bit_max; bit++, port++) {
        if (test_bit(bit, (unsigned long *)&reg)) {
            set_bit(port, &recv_bitmap);
        } else {
            clear_bit(port, &recv_bitmap);
        }
    }
    //SFF_IO_LOG_DBG("bitmap:0x%lx, recv_bit:0x%x\n", recv_bitmap, reg);
    
    *bitmap = recv_bitmap;
}
int ioexp_output_all_set(struct ioexp_dev_t *dev, ioexp_output_type_t type, unsigned long bitmap)
{
    int ret = 0;
    u16 reg = 0;
    struct io_dev_client_t *ioexp_client = NULL;
    struct pca95xx_func_t *func = NULL;
    struct ioexp_output_t *tbl = NULL;
    struct ioexp_output_t *ioexp = NULL;
    int count = 0;

    if (!p_valid(dev->output[type])) {
        if (p_valid(ioexp_output_name[type])) {
            SFF_IO_LOG_INFO("%s not supported\n", ioexp_output_name[type]);
        }
        return REC_SFF_IO_UNSUPPORTED;
    }
    tbl = dev->output[type];

    for (count = 0; tbl[count].end_of_tbl != true; count++) {
        ioexp = &tbl[count];
        if (!p_valid(ioexp_client = ioexp_client_find(dev, ioexp->ch_id, ioexp->addr))) {
            ret = -EBADRQC;
            break;
        }

        if (!p_valid(func = pca95xx_func_find(dev->config_map, ioexp->ioexp_id))) {
            ret = -EBADRQC;
            break;
        }

        if ((ret = func->read(ioexp_client, func->offset[PCA95XX_OUTPUT])) < 0) {
            SFF_IO_LOG_ERR("read fail! ioexp_id:%d ch_id:%d addr:0x%x\n",
                           ioexp->ioexp_id, ioexp->ch_id, ioexp->addr);
            break;
        }
        reg = ret;
        set_reg_update(ioexp, bitmap, &reg);

        if ((ret = func->write(ioexp_client, func->offset[PCA95XX_OUTPUT], reg)) < 0) {
            break;
        }

        SFF_IO_LOG_INFO("%s set ok ch_id:%d addr:0x%x reg:0x%x\n", ioexp_output_name[type], ioexp->ch_id, ioexp->addr, reg);
    }
    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int ioexp_reset_all_set(int lc_id, unsigned long bitmap)
{
    return ioexp_output_all_set(&(ioDev.ioexp_dev), IOEXP_RESET_TYPE, bitmap);
}

static int ioexp_lpmode_all_set(int lc_id, unsigned long bitmap)
{
    return ioexp_output_all_set(&(ioDev.ioexp_dev), IOEXP_LPMODE_TYPE, bitmap);
}

int ioexp_output_get(struct ioexp_dev_t *dev, ioexp_output_type_t type, int port, u8 *val)
{
    int ret = 0;
    int bit = 0;
    u16 reg = 0;
    struct io_dev_client_t *ioexp_client = NULL;
    struct pca95xx_func_t *func = NULL;
    struct ioexp_output_t *ioexp = NULL;

    if (!p_valid(dev->output[type])) {
        if (p_valid(ioexp_output_name[type])) {
            SFF_IO_LOG_INFO("%s not supported\n", ioexp_output_name[type]);
        }
        return REC_SFF_IO_UNSUPPORTED;
    }

    if (!p_valid(ioexp = find_ioexp_output_byPort(dev->output[type], port))) {
        return -1;
    }
    if (!p_valid(ioexp_client = ioexp_client_find(dev, ioexp->ch_id, ioexp->addr))) {
        return -1;
    }

    if (!p_valid(func = pca95xx_func_find(dev->config_map, ioexp->ioexp_id))) {
        return -1;
    }

    if ((ret = func->read(ioexp_client, func->offset[PCA95XX_OUTPUT])) < 0) {
        SFF_IO_LOG_ERR("read fail! ch_id:%d addr:0x%x\n", ioexp->ch_id, ioexp->addr);
        return ret;
    }
    reg = ret;

    bit = ioexp->bit_min + (port - ioexp->port_min);
    if (test_bit(bit, (unsigned long *)&reg)) {
        *val = 1;
    } else {
        *val = 0;
    }

    return 0;
}

int ioexp_output_all_get(struct ioexp_dev_t *dev, ioexp_output_type_t type, unsigned long *bitmap)
{
    int ret = 0;
    u16 reg = 0;
    struct io_dev_client_t *ioexp_client = NULL;
    struct pca95xx_func_t *func = NULL;
    struct ioexp_output_t *tbl = NULL;
    struct ioexp_output_t *ioexp = NULL;
    int count = 0;
    unsigned long new_bitmap = 0;
    
    if (!p_valid(dev->output[type])) {
        if (p_valid(ioexp_output_name[type])) {
            SFF_IO_LOG_INFO("%s not supported\n", ioexp_output_name[type]);
        }
        return REC_SFF_IO_UNSUPPORTED;
    }
    tbl = dev->output[type];

    for (count = 0; tbl[count].end_of_tbl != true; count++) {
        ioexp = &tbl[count];
        if (!p_valid(ioexp_client = ioexp_client_find(dev, ioexp->ch_id, ioexp->addr))) {
            ret = -EBADRQC;
            break;
        }

        if (!p_valid(func = pca95xx_func_find(dev->config_map, ioexp->ioexp_id))) {
            ret = -EBADRQC;
            break;
        }

        if ((ret = func->read(ioexp_client, func->offset[PCA95XX_OUTPUT])) < 0) {
            SFF_IO_LOG_ERR("read fail! ioexp_id:%d ch_id:%d addr:0x%x\n",
                           ioexp->ioexp_id, ioexp->ch_id, ioexp->addr);
            break;
        }
        reg = ret;
        recv_reg_update(ioexp, reg, &new_bitmap);
    }
    if (ret < 0) {
        return ret;
    }
    *bitmap = new_bitmap;
    
    return 0;
}

static int ioexp_reset_all_get(int lc_id, unsigned long *bitmap)
{
    return ioexp_output_all_get(&(ioDev.ioexp_dev), IOEXP_RESET_TYPE, bitmap);
}

static int ioexp_lpmode_all_get(int lc_id, unsigned long *bitmap)
{
    return ioexp_output_all_get(&(ioDev.ioexp_dev), IOEXP_LPMODE_TYPE, bitmap);
}

int ioexp_lpmode_set(int lc_id, int port, u8 val)
{
    return ioexp_output_set(&(ioDev.ioexp_dev), IOEXP_LPMODE_TYPE, port, val);
}
EXPORT_SYMBOL(ioexp_lpmode_set);

int ioexp_lpmode_get(int lc_id, int port, u8 *lpmode)
{
    int ret = 0;

    ret = ioexp_output_get(&(ioDev.ioexp_dev), IOEXP_LPMODE_TYPE, port, lpmode);
    if (ret < 0 ||
            ret == REC_SFF_IO_UNSUPPORTED) {
        return ret;
    }
    return 0;
}
EXPORT_SYMBOL(ioexp_lpmode_get);

int ioexp_reset_set(int lc_id, int port, u8 val)
{
    return ioexp_output_set(&(ioDev.ioexp_dev), IOEXP_RESET_TYPE, port, val);
}
EXPORT_SYMBOL(ioexp_reset_set);

int ioexp_reset_get(int lc_id, int port, u8 *lv)
{
    int ret = 0;

    ret = ioexp_output_get(&(ioDev.ioexp_dev), IOEXP_RESET_TYPE, port, lv);
    if (ret < 0 ||
            ret == REC_SFF_IO_UNSUPPORTED) {
        return ret;
    }
    return 0;
}
EXPORT_SYMBOL(ioexp_reset_get);

int ioexp_modesel_set(int lc_id, int port, u8 val)
{
    return ioexp_output_set(&(ioDev.ioexp_dev), IOEXP_MODESEL_TYPE, port, val);
}
EXPORT_SYMBOL(ioexp_modesel_set);

int ioexp_modesel_get(int lc_id, int port, u8 *val)
{
    int ret = 0;

    ret = ioexp_output_get(&(ioDev.ioexp_dev), IOEXP_MODESEL_TYPE, port, val);
    if (ret < 0 ||
            ret == REC_SFF_IO_UNSUPPORTED) {
        return ret;
    }
    return 0;
}
EXPORT_SYMBOL(ioexp_modesel_get);

int ioexp_tx_disable_set(int lc_id, int port, u8 val)
{
    return ioexp_output_set(&(ioDev.ioexp_dev), IOEXP_TXDISABLE_TYPE, port, val);
}
EXPORT_SYMBOL(ioexp_tx_disable_set);

int ioexp_tx_disable_get(int lc_id, int port, u8 *val)
{
    int ret = 0;

    ret = ioexp_output_get(&(ioDev.ioexp_dev), IOEXP_TXDISABLE_TYPE, port, val);
    if (ret < 0 ||
            ret == REC_SFF_IO_UNSUPPORTED) {
        return ret;
    }
    return 0;

}
EXPORT_SYMBOL(ioexp_tx_disable_get);


int ioexp_input_polling(struct ioexp_dev_t *dev)
{
    int ret = 0;
    int data = 0;
    int id = 0;
    struct io_dev_client_t *ioexp_client = NULL;
    struct ioexp_input_t *tbl = dev->input_map->tbl;
    int size = dev->input_map->size;
    struct pca95xx_func_t *func = NULL;
    struct ioexp_input_t *ioexp = NULL;

    for (id = 0; id < size; id++) {
        ioexp = &tbl[id];
        if (!p_valid(ioexp_client = ioexp_client_find(dev, ioexp->ch_id, ioexp->addr))) {
            ret = -EBADRQC;
            break;
        }
        if (!p_valid(func = pca95xx_func_find(dev->config_map, ioexp->ioexp_id))) {
            ret = -EBADRQC;
            break;
        }

        if ((ret = func->read(ioexp_client, func->offset[PCA95XX_INPUT])) < 0) {
            SFF_IO_LOG_ERR("read fail! ioexp_in_id:%d ioexp_id:%d ch_id:%d addr:0x%x\n",
                           id, ioexp->ioexp_id, ioexp->ch_id, ioexp->addr);
            ioexp_input_st_update_fail(dev->input_st, ioexp->layout);
            break;
        }
        data = ret;
        ioexp_input_st_update(dev->input_st, ioexp->layout, data);
    }
    if (ret < 0) {
        return ret;
    }
    return 0;
}

int ioexp_input_intr_hdlr(struct ioexp_dev_t *dev)
{
    int ret = 0;
    int data = 0;
    int id = 0;
    struct io_dev_client_t *ioexp_client = NULL;
    struct ioexp_input_t *tbl = dev->input_map->tbl;
    int size = dev->input_map->size;
    struct pca95xx_func_t *func = NULL;
    struct ioexp_input_t *ioexp = NULL;
    struct cpld_io_t *cpld_io = NULL;
    struct io_dev_t *io_dev = ioexp_to_io(dev);
    u32 intr_reg = 0;

    if (!p_valid(io_dev)) {
        return -EBADRQC;
    }
    cpld_io = &(io_dev->cpld_io);
    if (!p_valid(cpld_io)) {
        return -EBADRQC;
    }

    if (cpld_io_intr_is_asserted(cpld_io)) {
        SFF_IO_LOG_DBG("cpld_io_intr_is_asserted\n");

        check_pfunc(cpld_io->intr_st_get);
        if ((ret = cpld_io->intr_st_get(cpld_io, &intr_reg)) < 0) {
            return ret;
        }

        for (id = 0; id < size; id++) {

            if (test_bit(id, (unsigned long *)&intr_reg)) {
                SFF_IO_LOG_DBG("intr_reg:0x%x\n", intr_reg);
                ioexp = &tbl[id];
                if (!p_valid(ioexp_client = ioexp_client_find(dev, ioexp->ch_id, ioexp->addr))) {
                    ret = -EBADRQC;
                    break;
                }
                if (!p_valid(func = pca95xx_func_find(dev->config_map, ioexp->ioexp_id))) {
                    ret = -EBADRQC;
                    break;
                }

                if ((ret = func->read(ioexp_client, func->offset[PCA95XX_INPUT])) < 0) {
                    SFF_IO_LOG_ERR("read fail! ioexp_in_id:%d ioexp_id:%d ch_id:%d addr:0x%x\n",
                                   id, ioexp->ioexp_id, ioexp->ch_id, ioexp->addr);
                    ioexp_input_st_update_fail(dev->input_st, ioexp->layout);
                    break;
                }
                data = ret;
                ioexp_input_st_update(dev->input_st, ioexp->layout, data);
            }
        }

    }
    return 0;
}

int io_dev_hdlr(void)
{
    struct ioexp_dev_t *ioexp_dev = &ioDev.ioexp_dev;

    check_pfunc(ioexp_dev->input_hdlr);
    return ioexp_dev->input_hdlr(ioexp_dev);
}
int ioexp_input_handler(void)
{
    struct ioexp_dev_t *ioexp_dev = &ioDev.ioexp_dev;
    check_pfunc(ioexp_dev->input_hdlr);
    return ioexp_dev->input_hdlr(ioexp_dev);
}
int ioexp_input_init(struct ioexp_dev_t *dev)
{
    return ioexp_input_polling(dev);
}
int _ioexp_output_config(struct ioexp_output_t *ioexp, int default_val, u16 *data)
{
    u16 ret = 0;
    int bit = 0;
    int bit_min = 0;
    int bit_max = 0;
    bit_min = ioexp->bit_min;
    bit_max = ioexp->bit_max;

    ret = *data;
    if (default_val) {
        for (bit = bit_min; bit <= bit_max; bit++) {
            set_bit(bit , (unsigned long *)&ret);
        }
    } else {
        for (bit = bit_min; bit <= bit_max; bit++) {
            clear_bit(bit , (unsigned long *)&ret);
        }

    }
    //SFF_IO_LOG_DBG("bit_min %d ch:%d addr:0x%x ret:%d\n", bit_min, ioexp->ch, ioexp->addr, ret);
    *data = ret;
    return 0;
}
int ioexp_output_init(struct ioexp_dev_t *dev, int io_no_init)
{
    int count = 0;
    int ret = 0;
    u16 data = 0;
    int i = 0;
    struct ioexp_output_t *tbl = NULL;
    struct ioexp_output_t *ioexp = NULL;
    struct io_dev_client_t *ioexp_client = NULL;
    struct pca95xx_func_t *func = NULL;

    if (io_no_init) {
        SFF_IO_LOG_INFO("io no init!");
        return 0;
    }

    /*assign ioexp out*/
    for (i = IOEXP_LPMODE_TYPE; i < IOEXP_OUTPUT_TYPE_NUM; i++) {
        tbl = dev->output[i];
        if (!p_valid(tbl)) {
            if (p_valid(ioexp_output_name[i])) {
                SFF_IO_LOG_INFO("%s not supported\n", ioexp_output_name[i]);
            }
            continue;
        }

        for (count = 0; tbl[count].end_of_tbl != true; count++) {
            ioexp = &tbl[count];
            if (!p_valid(ioexp_client = ioexp_client_find(dev, ioexp->ch_id, ioexp->addr))) {
                return -EBADRQC;
            }

            if (!p_valid(func = pca95xx_func_find(dev->config_map, ioexp->ioexp_id))) {
                return -EBADRQC;
            }
            if ((ret = func->read(ioexp_client, func->offset[PCA95XX_OUTPUT])) < 0) {
                return ret;
            }

            data = ret;
            _ioexp_output_config(ioexp, ioexp_out_def_val[i], &data);
            if ((ret = func->write(ioexp_client, func->offset[PCA95XX_OUTPUT], data)) < 0) {
                return ret;
            }
        }
    }
    return 0;
}

static struct platform_io_info_t *platform_io_info_load(int platform_id)
{

    int i = 0;
    for (i = 0; platform_io_info_tbl[i].platform_id != PLATFORM_END; i++) {
        if (platform_io_info_tbl[i].platform_id == platform_id) {

            return &platform_io_info_tbl[i];
        }

    }
    return NULL;

}
static int table_load(int platform_id, struct io_dev_t *io_dev)
{
    struct platform_io_info_t *io_info = NULL;
    struct ioexp_dev_t *ioexp_dev = &(io_dev->ioexp_dev);
    io_info = platform_io_info_load(platform_id);
    if (!io_info) {
        return -1;
    }

    ioexp_dev->i2c_ch_map = io_info->i2c_ch_map;
    ioexp_dev->config_map = io_info->config_map;
    ioexp_dev->input_map = io_info->input_map;
    ioexp_dev->input_port_num = io_info->input_port_num;
    ioexp_dev->output = io_info->output;
    io_dev->cpld_io.config = io_info->cpld_config;
    io_dev->mux_rst_gpio = io_info->mux_rst_gpio;
    io_dev->pltfm_id = platform_id;

    if (p_valid(io_dev->cpld_io.config)) {
        io_dev->intr_mode_supported = true;
        ioexp_dev->input_hdlr = ioexp_input_intr_hdlr;
    } else {
        io_dev->intr_mode_supported = false;
        ioexp_dev->input_hdlr = ioexp_input_polling;
    }
    return 0;
}

struct io_dev_client_t *cpld_client_find(struct cpld_io_t *cpld_io, int ch_id, u8 addr)
{
    struct io_dev_client_t *cpld_client = NULL;
    cpld_client = &(cpld_io->cpld_client[ch_id]);
    cpld_client->client->addr = addr;

    return cpld_client;
}

static int cpld_io_intr_gpio_init(int gpio_no)
{
    int result;
    /*cpld ioexp int gpio init*/
    result = gpio_is_valid(gpio_no);
    if (result < 0) {
        SFF_IO_LOG_ERR("valid gpio:%d ret:%d\n", gpio_no, result);
        return -1;
    }

    return 0;
}
static bool cpld_io_intr_is_asserted(struct cpld_io_t *cpld_io)
{
    int val = 0;
    val = gpio_get_value(cpld_io->config->sff_intr_gpio);
    return ((val == 0) ? true : false);
}

static int cpld_io_intr_enable(struct cpld_io_t *cpld_io)
{
    int ret = 0;
    int cpld_id = 0;
    struct io_dev_client_t *cpld_client = NULL;
    u8 offset = 0;
    offset = cpld_io->config->intr_en.offset;
    cpld_id = cpld_io->config->intr_en.cpld_id;

    if (!p_valid(cpld_client = cpld_client_find(cpld_io,
                               cpld_io->config->i2c_config[cpld_id].ch_id,
                               cpld_io->config->i2c_config[cpld_id].addr)) < 0) {
        return -EBADRQC;
    }
    mutex_lock(&cpld_client->lock);
    ret = i2c_smbus_write_byte_data_retry(cpld_client->client, offset, 0x01);
    mutex_unlock(&cpld_client->lock);
    if (ret < 0) {
        return ret;
    }

    return 0;
}
/*maple platform*/
static int cpld_io_intr_st_get_type1(struct cpld_io_t *cpld_io, u32 *reg)
{
    int ret = 0;
    u32 sfp = 0;
    u32 qsfp = 0;
    u8 offset = 0;
    int cpld_id = 0;
    struct io_dev_client_t *cpld_client = NULL;

    offset = cpld_io->config->intr_st.offset;
    cpld_id = cpld_io->config->intr_st.cpld_id;

    if (!p_valid(cpld_client = cpld_client_find(cpld_io,
                               cpld_io->config->i2c_config[cpld_id].ch_id,
                               cpld_io->config->i2c_config[cpld_id].addr)) < 0) {
        return -EBADRQC;
    }
    mutex_lock(&cpld_client->lock);
    ret = i2c_smbus_read_word_data_retry(cpld_client->client, offset);
    mutex_unlock(&cpld_client->lock);
    if (ret < 0) {
        return ret;
    }
    /*reg remapping to match ioexp_input_map order*/
    /*sfp 0x35 bit 0:7 0x36 bit 0:4
     * qsfp 0x36 bit 7, merge it into one data*/
    sfp = (ret & 0xfff) << 1;
    qsfp = (ret & 0x8000) >> 15;

    /*bit = 0: interrupt occurs , reverse it for easy logic*/

    *reg = (~(sfp|qsfp)) & 0x1fff;
    return 0;
}
/*for nba820 platform*/
static int cpld_io_intr_st_get_type2(struct cpld_io_t *cpld_io, u32 *reg)
{
    int ret = 0;
    u32 qsfp = 0;
    u8 offset = 0;
    int cpld_id = 0;
    struct io_dev_client_t *cpld_client = NULL;

    offset = cpld_io->config->intr_st.offset;
    cpld_id = cpld_io->config->intr_st.cpld_id;

    if (!p_valid(cpld_client = cpld_client_find(cpld_io,
                               cpld_io->config->i2c_config[cpld_id].ch_id,
                               cpld_io->config->i2c_config[cpld_id].addr)) < 0) {
        return -EBADRQC;
    }
    mutex_lock(&cpld_client->lock);
    ret = i2c_smbus_read_byte_data_retry(cpld_client->client, offset);
    mutex_unlock(&cpld_client->lock);
    if (ret < 0) {
        return ret;
    }
    /*reg remapping to match ioexp_input_map order*/
    qsfp = (ret & 0xf);
    /*bit = 0: interrupt occurs , reverse it for easy logic*/
    *reg = (~qsfp) & 0xf;
    return 0;
}
#if 0
/*cedar platform, <TBD> check with new cedar HW spec*/
static int cpld_io_intr_st_get_type3(struct cpld_io_t *cpld_io, u32 *reg)
{
    int ret = 0;
    u32 qsfp = 0;
    u8 offset = 0;
    int cpld_id = 0;
    struct io_dev_client_t *cpld_client = NULL;
    u16 bit_mask[4] = {0x4, 0x80, 0x400, 0x8000};
    int i = 0;
    offset = cpld_io->config->intr_st.offset;
    cpld_id = cpld_io->config->intr_st.cpld_id;

    if (!p_valid(cpld_client = cpld_client_find(cpld_io,
                               cpld_io->config->i2c_config[cpld_id].ch_id,
                               cpld_io->config->i2c_config[cpld_id].addr)) < 0) {
        return -EBADRQC;
    }
    mutex_lock(&cpld_client->lock);
    ret = i2c_smbus_read_word_data_retry(cpld_client->client, offset);
    mutex_unlock(&cpld_client->lock);
    if (ret < 0) {
        return ret;
    }
    /*reg remapping to match ioexp_input_map order*/
    for (i = 0; i < 4; i++) {
        if (!(ret & bit_mask[i])) {
            set_bit(i, (unsigned long *)&qsfp);
        }
    }
    *reg = qsfp;
    return 0;
}
#endif
static int cpld_io_func_init(struct cpld_io_t *cpld_io)
{
    struct io_dev_t *io_dev = cpld_to_io(cpld_io);

    if (!p_valid(io_dev)) {
        return -EBADRQC;
    }
    if (PLATFORM_NBA820 == io_dev->pltfm_id) {
        cpld_io->intr_st_get = cpld_io_intr_st_get_type2;
    } else {
        SFF_IO_LOG_ERR("unkown function\n");
        return -EBADRQC;
    }
    return 0;
}
static int cpld_io_init(struct cpld_io_t *cpld_io)
{
    int ret = 0;

    if ((ret = cpld_io_func_init(cpld_io)) < 0) {
        goto exit_err;
    }
    if ((ret = cpld_io_clients_create(cpld_io)) < 0) {
        goto exit_err;
    }
    if ((ret = cpld_io_i2c_clients_create_init(cpld_io)) < 0) {
        goto exit_kfree_clients;
    }
    if ((ret = cpld_io_intr_gpio_init(cpld_io->config->sff_intr_gpio)) < 0) {
        goto exit_kfree_i2c_clients;
    }
    if ((ret = cpld_io_intr_enable(cpld_io)) < 0) {
        goto exit_kfree_i2c_clients;
    }

    SFF_IO_LOG_INFO("ok\n");
    return 0;

exit_kfree_i2c_clients:
    cpld_io_i2c_clients_destroy(cpld_io);
exit_kfree_clients:
    cpld_io_clients_destroy(cpld_io);
exit_err:
    return ret;
}

static void cpld_io_deinit(struct cpld_io_t *cpld_io)
{
    cpld_io_i2c_clients_destroy(cpld_io);
    cpld_io_clients_destroy(cpld_io);
    SFF_IO_LOG_INFO("ok\n");
}

int io_dev_init(int platform_id, int io_no_init)
{
    int ret = 0;

    if ((ret = table_load(platform_id, &ioDev)) < 0) {
        SFF_IO_LOG_ERR("ioexp_table load fail\n");
        goto exit_err;
    }

    if ((ret = ioexp_clients_create(&ioDev.ioexp_dev)) < 0) {
        SFF_IO_LOG_ERR("ioexp_clients_create fail\n");
        goto exit_err;
    }
    if ((ret = ioexp_i2c_clients_create(&ioDev.ioexp_dev)) < 0) {
        SFF_IO_LOG_ERR("ioexp_i2c_clients_create fail\n");
        goto exit_kfree_clients;
    }
    if ((ret = ioexp_i2c_clients_init(&ioDev.ioexp_dev)) < 0) {
        SFF_IO_LOG_ERR("ioexp_i2c_clients_init fail\n");
        goto exit_kfree_i2c_clients;
    }
    if (!io_no_init) {
        if (ioexp_config_init(&ioDev.ioexp_dev) < 0) {
            SFF_IO_LOG_ERR("ioexp_config_init fail\n");
            goto deinit_i2c_clients;
        }
    }
    if (ioexp_input_init(&ioDev.ioexp_dev) < 0) {
        SFF_IO_LOG_ERR("ioexp_input_init fail\n");
        goto deinit_i2c_clients;
    }

    if (ioexp_output_init(&ioDev.ioexp_dev, io_no_init) < 0) {
        SFF_IO_LOG_ERR("ioexp_out_init fail\n");
        goto deinit_i2c_clients;
    }
    if (ioDev.intr_mode_supported) {
        if (cpld_io_init(&ioDev.cpld_io) < 0) {
            SFF_IO_LOG_ERR("cpld_io_init fail\n");
            goto deinit_i2c_clients;
        }
    }
    if(io_dev_mux_rst_gpio_init(ioDev.mux_rst_gpio) < 0) {
        SFF_IO_LOG_ERR("io_dev_mux_rst_gpio_init fail\n");
        goto deinit_i2c_clients;
    }
    SFF_IO_LOG_DBG("OK\n");
    return 0;
deinit_i2c_clients:
    ioexp_i2c_clients_deinit(&ioDev.ioexp_dev);    
exit_kfree_i2c_clients:
    ioexp_i2c_clients_destroy(&ioDev.ioexp_dev);
exit_kfree_clients:
    ioexp_clients_destroy(&ioDev.ioexp_dev);
exit_err:
    return ret;
}

void io_dev_deinit(void)
{
    ioexp_i2c_clients_deinit(&ioDev.ioexp_dev);    
    ioexp_i2c_clients_destroy(&ioDev.ioexp_dev);
    ioexp_clients_destroy(&ioDev.ioexp_dev);

    if (ioDev.intr_mode_supported) {
        cpld_io_deinit(&ioDev.cpld_io);
    }
    gpio_free(ioDev.mux_rst_gpio);
    SFF_IO_LOG_DBG("OK\n");
}




