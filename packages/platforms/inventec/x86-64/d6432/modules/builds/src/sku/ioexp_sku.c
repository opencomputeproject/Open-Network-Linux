/*ioexp_sku.c
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
#include <linux/i2c-mux.h>
#include <linux/err.h>

#include "sku_common.h"
#include "../inv_def.h"
#include "ioexp_sku.h"
#include "ioexp_sku_config/ioexp_sku_config.h"
#include "inv_pca954x_data.h"

#define MUX_GPIO_LABEL "MUX_RST"
#define CPLD_INTR_GPIO_LABEL "CPLD_IO_INTR"
#define INV_MUX_NAME ("inv_i2c_mux")

struct cpld_io_t {
    struct cpld_config_t *config;
    struct inv_i2c_client_t *cpld_client;
    int (*intr_st_get)(struct cpld_io_t *cpld_io, u32 *reg);
    int sff_intr_gpio;
};

struct ioexp_dev_t {
    struct int_vector_t *i2c_ch_map;
    struct ioexp_config_map_t *config_map;
    struct ioexp_input_map_t *input_map;
    unsigned long *aval_sff_input;
    struct ioexp_output_t **output;
    struct ldata_format_t input_st[IOEXP_INPUT_TYPE_NUM];
    struct inv_i2c_client_t *ioexp_client;
    int (*input_hdlr)(struct ioexp_dev_t *self);
};

struct ioexp_sku_t {
    int pltfm_id;
    struct ioexp_dev_t ioexp_dev;
    bool intr_mode_supported;
    struct cpld_io_t cpld_io;
    int mux_rst_gpio;
};

struct ioexp_sku_t ioexp_sku;

#define ioexp_to_io(x) container_of(x, struct ioexp_sku_t, ioexp_dev)
#define cpld_to_io(x) container_of(x, struct ioexp_sku_t, cpld_io)
struct mux_func_t {
    struct device *dev;
    struct pca954x *pca;
    struct int_vector_t *ch_2_port;
};
struct mux_func_t muxFunc;
struct ioexp_config_t *ioexp_head = NULL;
struct ioexp_config_t *ioexp_tail = NULL;
struct i2c_client *ioexpI2cClient = NULL;
static int gpio_base = 0xffff;
static int pca9555_word_read(struct inv_i2c_client_t *ioexp_client, u8 offset);
static int pca9555_word_write(struct inv_i2c_client_t *ioexp_client, u8 offset, u16 val);
static int pca9555_byte_read(struct inv_i2c_client_t *ioexp_client, u8 offset);
static int pca9555_byte_write(struct inv_i2c_client_t *ioexp_client, u8 offset, u16 val);
struct inv_i2c_client_t *ioexp_client_find(struct ioexp_dev_t *self, int ch_id, u8 addr);
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
    IOEXP_MODSEL_TYPE,
    IOEXP_TXDIABLE2_TYPE,
    IOEXP_OUTPUT_TYPE_NUM,
} ioexp_output_type_t;
*/
const int ioexp_out_def_val[IOEXP_OUTPUT_TYPE_NUM] = {
    [IOEXP_LPMODE_TYPE] = IO_LPMODE_DEFAULT,
    [IOEXP_RESET_TYPE] = IO_RESET_DEFAULT,
    [IOEXP_TXDISABLE_TYPE] = IO_TXDISABLE_DEFAULT,
    [IOEXP_MODSEL_TYPE] = IO_MODSEL_DEFAULT,
    [IOEXP_TXDISABLE2_TYPE] = IO_TXDISABLE_DEFAULT,
};

const char *ioexp_output_name[IOEXP_OUTPUT_TYPE_NUM] = {
    [IOEXP_LPMODE_TYPE] = "IOEXP_LPMODE",
    [IOEXP_RESET_TYPE] = "IOEXP_RESET",
    [IOEXP_TXDISABLE_TYPE] = "IOEXP_TXDISABLE",
    [IOEXP_MODSEL_TYPE] = "IOEXP_MODSEL_TYPE",
    [IOEXP_TXDISABLE2_TYPE] = "IOEXP_TXDISABLE2",
};
/*
typedef enum {
    IOEXP_PRS_TYPE,
    IOEXP_TXFAULT_TYPE,
    IOEXP_RXLOS_TYPE,
    IOEXP_INTR_TYPE,
    IOEXP_TXFAULT2_TYPE,
    IOEXP_RXLOS2_TYPE,
    IOEXP_INPUT_TYPE_NUM,
} ioexp_input_type_t;
*/
const char *ioexp_input_st_name[IOEXP_INPUT_TYPE_NUM] = {
    [IOEXP_PRS_TYPE] = "IOEXP_PRS",
    [IOEXP_TXFAULT_TYPE] = "IOEXP_TXFAULT",
    [IOEXP_RXLOS_TYPE] = "IOEXP_RXLOS",
    [IOEXP_INTR_TYPE] = "IOEXP_INTR",
    [IOEXP_TXFAULT2_TYPE] = "IOEXP_TXFAULT2",
    [IOEXP_RXLOS2_TYPE] = "IOEXP_RXLOS2",
};

int ioexp_sku_mux_reset_set(int lc_id, int val);
int ioexp_sku_mux_reset_get(int lc_id, int *val);
bool ioexp_is_channel_ready(int lc_id);
bool ioexp_is_i2c_ready(void);
int ioexp_sku_init(int platform_id, int io_no_init);
void ioexp_sku_deinit(void);
int ioexp_sku_hdlr(void);
static int ioexp_sku_sff_get_ready_action(int lc_id, int port);
static int ioexp_sku_sff_detected_action(int lc_id, int port);
/*sff io*/
/*in*/
static int ioexp_prs_get(int lc_id, unsigned long *bitmap);
static int ioexp_intr_get(int lc_id, unsigned long *bitmap);
static int ioexp_rxlos_get(int lc_id, unsigned long *bitmap) ;
static int ioexp_txfault_get(int lc_id, unsigned long *bitmap);
/*out*/
static int ioexp_reset_set(int lc_id, unsigned long bitmap);
static int ioexp_reset_get(int lc_id, unsigned long *bitmap);
static int ioexp_lpmode_set(int lc_id, unsigned long bitmap);
static int ioexp_lpmode_get(int lc_id, unsigned long *bitmap);
static int ioexp_modsel_set(int lc_id, unsigned long bitmap);
static int ioexp_modsel_get(int lc_id, unsigned long *bitmap);
static int ioexp_txdisable_set(int lc_id, unsigned long bitmap);
static int ioexp_txdisable_get(int lc_id, unsigned long *bitmap);
/*mux operation*/
static int sku_mux_failed_ch_get(int lc_id, unsigned long *ch);
static int sku_mux_blocked_ch_set(int lc_id, unsigned long ch);
static int sku_mux_fail_set(int lc_id, bool is_fail);
static int sku_mux_fail_get(int lc_id, bool *is_fail);
static int sku_mux_ch_to_port(int lc_id, int ch);
static int sku_mux_port_to_ch(int lc_id, int port);
static int mux_func_load(struct mux_func_t *func);

struct sff_io_driver_t ioexp_sku_sff_io_drv = {
    .prs = {
        .set = NULL,
        .get = ioexp_prs_get
    },

    .intr = {
        .set = NULL,
        .get = ioexp_intr_get
    },

    .rxlos = {
        .set = NULL,
        .get = ioexp_rxlos_get
    },

    .txfault = {
        .set = NULL,
        .get = ioexp_txfault_get
    },
    .reset = {
        .set = ioexp_reset_set,
        .get = ioexp_reset_get
    },
    .lpmode = {
        .set = ioexp_lpmode_set,
        .get = ioexp_lpmode_get
    },
    .modsel = {
        .set = ioexp_modsel_set,
        .get = ioexp_modsel_get
    },
    .txdisable = {
        .set = ioexp_txdisable_set,
        .get = ioexp_txdisable_get
    },
};

struct pltfm_func_t ioexp_sku_pltfm_func = {
    .init = ioexp_sku_init,
    .deinit = ioexp_sku_deinit,
    .io_hdlr = ioexp_sku_hdlr,
    .mux_reset_set = ioexp_sku_mux_reset_set,
    .mux_reset_get = ioexp_sku_mux_reset_get,
    .i2c_is_alive = ioexp_is_channel_ready,
    .sff_get_ready_action = ioexp_sku_sff_get_ready_action,
    .sff_detected_action = ioexp_sku_sff_detected_action,
    .mux_fail_set = sku_mux_fail_set,
    .mux_fail_get = sku_mux_fail_get,
    .mux_failed_ch_get =sku_mux_failed_ch_get,
    .mux_blocked_ch_set = sku_mux_blocked_ch_set,
    .mux_ch_to_port = sku_mux_ch_to_port,
    .mux_port_to_ch = sku_mux_port_to_ch
};

typedef enum {
    PCA95XX_INPUT,
    PCA95XX_OUTPUT,
    PCA95XX_CONFIG,
    PCA95XX_OFFSET_TYPE_NUM,

} pca95xx_offset_type_t;

struct pca95xx_func_t {
    u8 offset[PCA95XX_OFFSET_TYPE_NUM];
    int (*read)(struct inv_i2c_client_t *ioexp_client, u8 offset);
    int (*write)(struct inv_i2c_client_t *ioexp_client, u8 offset, u16 val);
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

/*[note] when sff module is prsL , corrensponding bit be set*/
int ioexp_prs_get(int lc_id, unsigned long *bitmap)
{
    unsigned long aval_prs = ioexp_sku.ioexp_dev.aval_sff_input[IOEXP_PRS_TYPE];
    struct ldata_format_t *ldata = &(ioexp_sku.ioexp_dev.input_st[IOEXP_PRS_TYPE]);

    if (aval_prs != ldata->valid) {
        return -EBADRQC;
    }
    *bitmap = ldata->bitmap;
    return 0;
}
EXPORT_SYMBOL(ioexp_prs_get);

int ioexp_intr_get(int lc_id, unsigned long *bitmap)
{
    struct ldata_format_t *ldata = &(ioexp_sku.ioexp_dev.input_st[IOEXP_INTR_TYPE]);
    unsigned long aval_intr = ioexp_sku.ioexp_dev.aval_sff_input[IOEXP_INTR_TYPE];

    if (0 == aval_intr) {
        return -ENOSYS;
    }
    if (aval_intr != ldata->valid) {
        return -EBADRQC;
    }

    *bitmap = ldata->bitmap;
    return 0;
}
int ioexp_rxlos_get(int lc_id, unsigned long *bitmap)
{
    struct ldata_format_t *ldata = &(ioexp_sku.ioexp_dev.input_st[IOEXP_RXLOS_TYPE]);
    unsigned long aval_rxlos = ioexp_sku.ioexp_dev.aval_sff_input[IOEXP_RXLOS_TYPE];

    if (0 == aval_rxlos) {
        return -ENOSYS;
    }
    if (aval_rxlos != ldata->valid) {
        return -EBADRQC;
        }

        *bitmap = ldata->bitmap;
        return 0;
}
int ioexp_txfault_get(int lc_id, unsigned long *bitmap)
{
    struct ldata_format_t *ldata = &(ioexp_sku.ioexp_dev.input_st[IOEXP_TXFAULT_TYPE]);
    unsigned long aval_txfault = ioexp_sku.ioexp_dev.aval_sff_input[IOEXP_TXFAULT_TYPE];

    if (0 == aval_txfault) {
        return -ENOSYS;
    }

    if (aval_txfault != ldata->valid) {
        return -EBADRQC;
    }

    *bitmap = ldata->bitmap;
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
    struct inv_i2c_client_t *ioexp_client = NULL;
    ioexp_client = kzalloc(sizeof(struct inv_i2c_client_t) * size, GFP_KERNEL);
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
        SKU_LOG_ERR("get adapter fail ch:%d\n", ch);
        return -EBADRQC;
    }

    SKU_LOG_DBG("get adapter ok ch:%d\n", ch);
    (*client)->adapter = adap;

    return 0;
}

static int ioexp_i2c_clients_init(struct ioexp_dev_t *ioexp_dev)
{
    int ret = 0;
    int i = 0;
    int size = ioexp_dev->i2c_ch_map->size;
    int *tbl = ioexp_dev->i2c_ch_map->tbl;
    struct inv_i2c_client_t *ioexp_client = NULL;

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
    struct inv_i2c_client_t *cpld_client = NULL;
    cpld_client = kzalloc(sizeof(struct inv_i2c_client_t) * size, GFP_KERNEL);
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
        SKU_LOG_ERR("get adapter fail ch:%d\n", ch);
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

int ioexp_sku_mux_rst_gpio_init(int mux_rst_gpio)
{
    /*pull high*/
    int ret = 0;
    if ((ret = gpio_is_valid(mux_rst_gpio)) < 0) {
        SKU_LOG_ERR("invaid gpio:%d ret:%d\n", mux_rst_gpio, ret);
        return -1;
    }
    if ((ret = gpio_request(mux_rst_gpio, MUX_GPIO_LABEL)) < 0) {
        SKU_LOG_ERR("gpio:%d request fail\n", mux_rst_gpio);
        return -1;
    }
    if(gpio_direction_output(mux_rst_gpio, 1) < 0) {
        SKU_LOG_ERR("init NG\n");
        return -1;
    }
    mdelay(1); /*1ms*/

    SKU_LOG_DBG("mux rst gpio:%d init pass\n", mux_rst_gpio);
    return 0;
}

int ioexp_sku_mux_reset_get(int lc_id, int *val)
{
    int lv = 0;
    lv = gpio_get_value(ioexp_sku.mux_rst_gpio);
    *val = lv;
    return 0;
}

/*io mux control*/
int ioexp_sku_mux_reset_set(int lc_id, int value)
{
    gpio_set_value(ioexp_sku.mux_rst_gpio, value);
    return 0;
}

static bool _ioexp_is_channel_ready(struct ioexp_dev_t *ioexp_dev, struct ioexp_config_t *config)
{
    int ret = 0;
    struct pca95xx_func_t *func = NULL;
    struct inv_i2c_client_t *ioexp_client = NULL;

    if (!p_valid(config)) {
        SKU_LOG_ERR("NULL ptr\n");
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
        SKU_LOG_ERR("ch:%d config mismatch default:%x reg:%x\n", config->ch_id, config->val, ret);
        return false;
    }
    return true;
}
bool ioexp_is_channel_ready(int lc_id)
{
    bool h_ready = false;
    bool t_ready = false;
    (void)lc_id;

    if (!(h_ready = _ioexp_is_channel_ready(&(ioexp_sku.ioexp_dev), ioexp_head))) {
        return false;
    }
    if (!(t_ready = _ioexp_is_channel_ready(&(ioexp_sku.ioexp_dev), ioexp_tail))) {
        return false;
    }

    return true;
}

int ioexp_config_init(struct ioexp_dev_t *ioexp_dev, bool need_init)
{
    int ioexp_id = 0;
    int ret = 0;
    struct ioexp_config_t *config = NULL;
    struct ioexp_config_t *tbl = NULL;
    struct pca95xx_func_t *func = NULL;
    struct inv_i2c_client_t *ioexp_client = NULL;
    int size = 0;

    tbl = ioexp_dev->config_map->tbl;
    size = ioexp_dev->config_map->size;
    if (need_init) {
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
                SKU_LOG_ERR("io config fail: ch_id:0x%x addr:0x%x val:0x%x\n", config->ch_id, config->addr, config->val);
                break;
            }
        }
        if (ret < 0) {
            return ret;
        }
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
        SKU_LOG_ERR("ioexp_head tail config fail\n");
        return -1;
    } else {
        SKU_LOG_INFO("ioexp_head ch_id:%d addr:0x%x\n", ioexp_head->ch_id, ioexp_head->addr);
        SKU_LOG_INFO("ioexp_tail ch_id:%d addr:0x%x\n", ioexp_tail->ch_id, ioexp_tail->addr);
    }
    return 0;
}

struct inv_i2c_client_t *ioexp_client_find(struct ioexp_dev_t *dev, int ch_id, u8 addr)
{
    struct inv_i2c_client_t *ioexp_client = NULL;
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

    for (i = 0; true != layout_tbl[i].end_of_layout; i++) {

        layout = &layout_tbl[i];
        port = layout->port_1st;
        input_st = &(ioexp_input_st[layout->type]);

        for (bit = layout->bit_1st; bit <= layout->bit_last; bit++, port++) {
            st = ((test_bit(bit, (unsigned long *)&data)) ? 1 : 0);
            /*debug msg*/
            old_st = ((test_bit(port, &(input_st->bitmap))) ? 1 : 0);
            if (old_st != st) {
                SKU_LOG_DBG("%s phy_port%d st:%d->%d\n", ioexp_input_st_name[layout->type], port, old_st, st);
            }
            if (st) {
                set_bit(port, &(input_st->bitmap));
            } else {
                clear_bit(port, &(input_st->bitmap));
            }
            set_bit(port, &(input_st->valid));
            //SKU_LOG_DBG("%s data:%d ch:%d addr:0x%x st[%d]:%d\n", name, data, ioexp->ch, ioexp->addr, port, val);
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
        port = layout->port_1st;
        input_st = &(ioexp_input_st[layout->type]);

        for (bit = layout->bit_1st; bit <= layout->bit_last; bit++, port++) {
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

static int pca9555_byte_read(struct inv_i2c_client_t *ioexp_client, u8 offset)
{
    int ret = 0;
    mutex_lock(&ioexp_client->lock);
    ret = i2c_smbus_read_byte_data_retry(ioexp_client->client, offset);
    mutex_unlock(&ioexp_client->lock);

    return ret;
}
static int pca9555_byte_write(struct inv_i2c_client_t *ioexp_client, u8 offset, u16 val)
{
    int ret = 0;
    mutex_lock(&ioexp_client->lock);
    ret = i2c_smbus_write_byte_data_retry(ioexp_client->client, offset, val);
    mutex_unlock(&ioexp_client->lock);

    return ret;
}
static int pca9555_word_read(struct inv_i2c_client_t *ioexp_client, u8 offset)
{
    int ret = 0;
    mutex_lock(&ioexp_client->lock);
    ret = i2c_smbus_read_word_data_retry(ioexp_client->client, offset);
    mutex_unlock(&ioexp_client->lock);

    return ret;
}
static int pca9555_word_write(struct inv_i2c_client_t *ioexp_client, u8 offset, u16 val)
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
    int port_last = 0;
    for (i = 0; map[i].end_of_tbl != true; i++) {
        port_last = map[i].port_1st + (map[i].bit_last - map[i].bit_1st);
        if (port >= map[i].port_1st &&
                port <= port_last) {
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
    struct inv_i2c_client_t *ioexp_client = NULL;
    struct pca95xx_func_t *func = NULL;
    struct ioexp_output_t *ioexp = NULL;

    if (!p_valid(dev->output[type])) {
        if (p_valid(ioexp_output_name[type])) {
            SKU_LOG_INFO("%s not supported\n", ioexp_output_name[type]);
        }
        return -ENOSYS;
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
        SKU_LOG_ERR("read fail! ioexp_id:%d ch_id:%d addr:0x%x\n",
                       ioexp->ioexp_id, ioexp->ch_id, ioexp->addr);
        return ret;
    }
    reg = ret;
    bit = ioexp->bit_1st + (port - ioexp->port_1st);
    if (val) {
        set_bit(bit, (unsigned long *)&reg);
    } else {
        clear_bit(bit, (unsigned long *)&reg);
    }

    if ((ret = func->write(ioexp_client, func->offset[PCA95XX_OUTPUT], reg)) < 0) {
        return ret;
    }
    SKU_LOG_DBG("%s set ok ch_id:%d addr:0x%x reg:0x%x\n", ioexp_output_name[type], ioexp->ch_id, ioexp->addr, reg);
    return 0;
}
static int set_reg_update(struct ioexp_output_t *ioexp, unsigned long bitmap, u16 *reg)
{
    u16 new_reg = 0;
    int port_1st = 0;
    int port_last = 0;
    int bit = 0;
    int port = 0;
    
    if (!p_valid(ioexp) && !p_valid(reg)) {
        return -EBADRQC;
    }
    port_1st = ioexp->port_1st;
    port_last = ioexp->port_1st + (ioexp->bit_last - ioexp->bit_1st);
    new_reg = *reg;

    for (port = port_1st, bit = ioexp->bit_1st; port <= port_last; port++, bit++) {
        if (test_bit(port, &bitmap)) {
            set_bit(bit, (unsigned long *)&new_reg);
        } else {
            clear_bit(bit, (unsigned long *)&new_reg);
        }
    }
    SKU_LOG_DBG("bitmap:0x%lx set_bits:0x%x\n", bitmap, new_reg);
    *reg = new_reg;
    return 0;
}

static void recv_reg_update(struct ioexp_output_t *ioexp, u16 reg, unsigned long *bitmap)
{
    int port = 0;
    int bit = 0;
    unsigned long recv_bitmap = 0;

    recv_bitmap = *bitmap;

    for (port = ioexp->port_1st, bit = ioexp->bit_1st; bit <= ioexp->bit_last; bit++, port++) {
        if (test_bit(bit, (unsigned long *)&reg)) {
            set_bit(port, &recv_bitmap);
        } else {
            clear_bit(port, &recv_bitmap);
        }
    }
    //SKU_LOG_DBG("bitmap:0x%lx, recv_bit:0x%x\n", recv_bitmap, reg);

    *bitmap = recv_bitmap;
}
int ioexp_output_all_set(struct ioexp_dev_t *dev, ioexp_output_type_t type, unsigned long bitmap)
{
    int ret = 0;
    u16 reg = 0;
    struct inv_i2c_client_t *ioexp_client = NULL;
    struct pca95xx_func_t *func = NULL;
    struct ioexp_output_t *tbl = NULL;
    struct ioexp_output_t *ioexp = NULL;
    int count = 0;

    if (!p_valid(dev->output[type])) {
        if (p_valid(ioexp_output_name[type])) {
            SKU_LOG_INFO("%s not supported\n", ioexp_output_name[type]);
        }
        return -ENOSYS;
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
            SKU_LOG_ERR("read fail! ioexp_id:%d ch_id:%d addr:0x%x\n",
                           ioexp->ioexp_id, ioexp->ch_id, ioexp->addr);
            break;
        }
        reg = ret;
        if ((ret = set_reg_update(ioexp, bitmap, &reg)) < 0) {
            break;
        }

        if ((ret = func->write(ioexp_client, func->offset[PCA95XX_OUTPUT], reg)) < 0) {
            break;
        }

        SKU_LOG_INFO("%s set ok ch_id:%d addr:0x%x reg:0x%x\n", ioexp_output_name[type], ioexp->ch_id, ioexp->addr, reg);
    }
    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int ioexp_reset_set(int lc_id, unsigned long bitmap)
{
    return ioexp_output_all_set(&(ioexp_sku.ioexp_dev), IOEXP_RESET_TYPE, bitmap);
}

static int ioexp_lpmode_set(int lc_id, unsigned long bitmap)
{
    return ioexp_output_all_set(&(ioexp_sku.ioexp_dev), IOEXP_LPMODE_TYPE, bitmap);
}

static int ioexp_modsel_set(int lc_id, unsigned long bitmap)
{
    return ioexp_output_all_set(&(ioexp_sku.ioexp_dev), IOEXP_MODSEL_TYPE, bitmap);
}

static int ioexp_txdisable_set(int lc_id, unsigned long bitmap)
{
    return ioexp_output_all_set(&(ioexp_sku.ioexp_dev), IOEXP_TXDISABLE_TYPE, bitmap);
}

int ioexp_output_get(struct ioexp_dev_t *dev, ioexp_output_type_t type, int port, u8 *val)
{
    int ret = 0;
    int bit = 0;
    u16 reg = 0;
    struct inv_i2c_client_t *ioexp_client = NULL;
    struct pca95xx_func_t *func = NULL;
    struct ioexp_output_t *ioexp = NULL;

    if (!p_valid(dev->output[type])) {
        if (p_valid(ioexp_output_name[type])) {
            SKU_LOG_INFO("%s not supported\n", ioexp_output_name[type]);
        }
        return -ENOSYS;
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
        SKU_LOG_ERR("read fail! ch_id:%d addr:0x%x\n", ioexp->ch_id, ioexp->addr);
        return ret;
    }
    reg = ret;

    bit = ioexp->bit_1st + (port - ioexp->port_1st);
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
    struct inv_i2c_client_t *ioexp_client = NULL;
    struct pca95xx_func_t *func = NULL;
    struct ioexp_output_t *tbl = NULL;
    struct ioexp_output_t *ioexp = NULL;
    int count = 0;
    unsigned long new_bitmap = 0;

    if (!p_valid(dev->output[type])) {
        if (p_valid(ioexp_output_name[type])) {
            SKU_LOG_INFO("%s not supported\n", ioexp_output_name[type]);
        }
        return -ENOSYS;
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
            SKU_LOG_ERR("read fail! ioexp_id:%d ch_id:%d addr:0x%x\n",
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

static int ioexp_reset_get(int lc_id, unsigned long *bitmap)
{
    return ioexp_output_all_get(&(ioexp_sku.ioexp_dev), IOEXP_RESET_TYPE, bitmap);
}

static int ioexp_lpmode_get(int lc_id, unsigned long *bitmap)
{
    return ioexp_output_all_get(&(ioexp_sku.ioexp_dev), IOEXP_LPMODE_TYPE, bitmap);
}

static int ioexp_modsel_get(int lc_id, unsigned long *bitmap)
{
    return ioexp_output_all_get(&(ioexp_sku.ioexp_dev), IOEXP_MODSEL_TYPE, bitmap);
}

static int ioexp_txdisable_get(int lc_id, unsigned long *bitmap)
{
    return ioexp_output_all_get(&(ioexp_sku.ioexp_dev), IOEXP_TXDISABLE_TYPE, bitmap);
}

int ioexp_input_polling(struct ioexp_dev_t *dev)
{
    int ret = 0;
    int data = 0;
    int id = 0;
    struct inv_i2c_client_t *ioexp_client = NULL;
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
            SKU_LOG_ERR("read fail! ioexp_in_id:%d ioexp_id:%d ch_id:%d addr:0x%x\n",
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
    struct inv_i2c_client_t *ioexp_client = NULL;
    struct ioexp_input_t *tbl = dev->input_map->tbl;
    int size = dev->input_map->size;
    struct pca95xx_func_t *func = NULL;
    struct ioexp_input_t *ioexp = NULL;
    struct cpld_io_t *cpld_io = NULL;
    struct ioexp_sku_t *ioexp_sku = ioexp_to_io(dev);
    u32 intr_reg = 0;

    if (!p_valid(ioexp_sku)) {
        return -EBADRQC;
    }
    cpld_io = &(ioexp_sku->cpld_io);
    if (!p_valid(cpld_io)) {
        return -EBADRQC;
    }

    if (cpld_io_intr_is_asserted(cpld_io)) {
        SKU_LOG_DBG("cpld_io_intr_is_asserted\n");

        check_pfunc(cpld_io->intr_st_get);
        if ((ret = cpld_io->intr_st_get(cpld_io, &intr_reg)) < 0) {
            return ret;
        }

        for (id = 0; id < size; id++) {

            if (test_bit(id, (unsigned long *)&intr_reg)) {
                SKU_LOG_DBG("intr_reg:0x%x\n", intr_reg);
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
                    SKU_LOG_ERR("read fail! ioexp_in_id:%d ioexp_id:%d ch_id:%d addr:0x%x\n",
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

static int ioexp_sku_sff_get_ready_action(int lc_id, int port)
{
    SKU_LOG_DBG("phy_port%d do nothing\n", port);
    return 0;
}

static int ioexp_sku_sff_detected_action(int lc_id, int port)
{
    SKU_LOG_DBG("phy_port%d do nothing\n", port);
    return 0;
}

int ioexp_sku_hdlr(void)
{
    struct ioexp_dev_t *ioexp_dev = &ioexp_sku.ioexp_dev;

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
    int bit_1st = 0;
    int bit_last = 0;
    bit_1st = ioexp->bit_1st;
    bit_last = ioexp->bit_last;

    ret = *data;
    if (default_val) {
        for (bit = bit_1st; bit <= bit_last; bit++) {
            set_bit(bit, (unsigned long *)&ret);
        }
    } else {
        for (bit = bit_1st; bit <= bit_last; bit++) {
            clear_bit(bit, (unsigned long *)&ret);
        }

    }
    //SKU_LOG_DBG("bit_1st %d ch:%d addr:0x%x ret:%d\n", bit_1st, ioexp->ch, ioexp->addr, ret);
    *data = ret;
    return 0;
}
int ioexp_output_init(struct ioexp_dev_t *dev, bool need_init)
{
    int count = 0;
    int ret = 0;
    int def_val = 0;
    u16 data = 0;
    int i = 0;
    struct ioexp_output_t *tbl = NULL;
    struct ioexp_output_t *ioexp = NULL;
    struct inv_i2c_client_t *ioexp_client = NULL;
    struct pca95xx_func_t *func = NULL;

    if (!need_init) {
        SKU_LOG_INFO("io no init!");
        return 0;
    }

    /*assign ioexp out*/
    for (i = IOEXP_LPMODE_TYPE; i < IOEXP_OUTPUT_TYPE_NUM; i++) {
        tbl = dev->output[i];
        if (!p_valid(tbl)) {
            if (p_valid(ioexp_output_name[i])) {
                SKU_LOG_INFO("%s not supported\n", ioexp_output_name[i]);
            }
            continue;
        }
        def_val = ioexp_out_def_val[i];
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
            _ioexp_output_config(ioexp, def_val, &data);
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
static int table_load(int platform_id, struct ioexp_sku_t *ioexp_sku)
{
    struct platform_io_info_t *io_info = NULL;
    struct ioexp_dev_t *ioexp_dev = &(ioexp_sku->ioexp_dev);
    io_info = platform_io_info_load(platform_id);

    check_p(io_info);
    ioexp_dev->i2c_ch_map = io_info->i2c_ch_map;
    ioexp_dev->config_map = io_info->config_map;
    ioexp_dev->input_map = io_info->input_map;
    ioexp_dev->aval_sff_input = io_info->aval_sff_input;
    ioexp_dev->output = io_info->output;
    ioexp_sku->cpld_io.config = io_info->cpld_config;
    ioexp_sku->mux_rst_gpio = io_info->mux_rst_gpio;
    ioexp_sku->pltfm_id = platform_id;

    if (p_valid(ioexp_sku->cpld_io.config)) {
        ioexp_sku->intr_mode_supported = true;
        ioexp_dev->input_hdlr = ioexp_input_intr_hdlr;
    } else {
        ioexp_sku->intr_mode_supported = false;
        ioexp_dev->input_hdlr = ioexp_input_polling;
    }
    if (io_info->mux_func_supported) {

        if (!p_valid(io_info->mux_ch_2_port_map)) {
            return -EBADRQC;
        }
        muxFunc.ch_2_port = io_info->mux_ch_2_port_map;
        if (mux_func_load(&muxFunc) < 0) {
            SKU_LOG_ERR("mux func load fail\n");
            return -EBADRQC;
        }
    }
    return 0;
}

struct inv_i2c_client_t *ioexp_sku_cpld_client_find(struct cpld_io_t *cpld_io, int ch_id, u8 addr)
{
    struct inv_i2c_client_t *cpld_client = NULL;
    cpld_client = &(cpld_io->cpld_client[ch_id]);
    cpld_client->client->addr = addr;

    return cpld_client;
}

static int cpld_io_intr_gpio_init(int gpio_no)
{
    int result;
    /*cpld ioexp int gpio init*/
    result = gpio_is_valid(gpio_no);
    SKU_LOG_DBG("valid gpio:%d\n", gpio_no);
    if (result < 0) {
        SKU_LOG_ERR("invalid gpio:%d ret:%d\n", gpio_no, result);
        return -1;
    }

    return 0;
}
static bool cpld_io_intr_is_asserted(struct cpld_io_t *cpld_io)
{
    int val = 0;
    val = gpio_get_value(cpld_io->sff_intr_gpio);
    return ((val == 0) ? true : false);
}

static int cpld_io_intr_enable(struct cpld_io_t *cpld_io)
{
    int ret = 0;
    int cpld_id = 0;
    struct inv_i2c_client_t *cpld_client = NULL;
    u8 offset = 0;
    offset = cpld_io->config->intr_en.offset;
    cpld_id = cpld_io->config->intr_en.cpld_id;

    if (!p_valid(cpld_client = ioexp_sku_cpld_client_find(cpld_io,
                               cpld_io->config->i2c_config[cpld_id].ch_id,
                               cpld_io->config->i2c_config[cpld_id].addr))) {
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
    struct inv_i2c_client_t *cpld_client = NULL;

    offset = cpld_io->config->intr_st.offset;
    cpld_id = cpld_io->config->intr_st.cpld_id;

    if (!p_valid(cpld_client = ioexp_sku_cpld_client_find(cpld_io,
                               cpld_io->config->i2c_config[cpld_id].ch_id,
                               cpld_io->config->i2c_config[cpld_id].addr))) {
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
/*for banyan platform*/
static int cpld_io_intr_st_get_type2(struct cpld_io_t *cpld_io, u32 *reg)
{
    int ret = 0;
    u32 qsfp = 0;
    u8 offset = 0;
    int cpld_id = 0;
    struct inv_i2c_client_t *cpld_client = NULL;

    offset = cpld_io->config->intr_st.offset;
    cpld_id = cpld_io->config->intr_st.cpld_id;

    if (!p_valid(cpld_client = ioexp_sku_cpld_client_find(cpld_io,
                               cpld_io->config->i2c_config[cpld_id].ch_id,
                               cpld_io->config->i2c_config[cpld_id].addr))) {
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
    struct inv_i2c_client_t *cpld_client = NULL;
    u16 bit_mask[4] = {0x4, 0x80, 0x400, 0x8000};
    int i = 0;
    offset = cpld_io->config->intr_st.offset;
    cpld_id = cpld_io->config->intr_st.cpld_id;

    if (!p_valid(cpld_client = ioexp_sku_cpld_client_find(cpld_io,
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
    struct ioexp_sku_t *ioexp_sku = cpld_to_io(cpld_io);

    if (!p_valid(ioexp_sku)) {
        return -EBADRQC;
    }
    if (PLATFORM_MAPLE == ioexp_sku->pltfm_id) {
        cpld_io->intr_st_get = cpld_io_intr_st_get_type1;
    } else if (PLATFORM_BANYAN == ioexp_sku->pltfm_id) {
        cpld_io->intr_st_get = cpld_io_intr_st_get_type2;
    } else {
        SKU_LOG_ERR("unkown function\n");
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
    cpld_io->sff_intr_gpio = gpio_base + cpld_io->config->sff_intr_gpio;
    if ((ret = cpld_io_intr_gpio_init(cpld_io->sff_intr_gpio)) < 0) {
        goto exit_kfree_i2c_clients;
    }
    if ((ret = cpld_io_intr_enable(cpld_io)) < 0) {
        goto exit_kfree_i2c_clients;
    }

    SKU_LOG_INFO("ok\n");
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
    SKU_LOG_INFO("ok\n");
}

static int sku_mux_failed_ch_get(int lc_id, unsigned long *ch)
{
    u64 failed_ch = 0;
    (void)lc_id;
    check_p(ch);
    muxFunc.pca->current_channel_get(muxFunc.dev, &failed_ch);
    *ch = (unsigned long)failed_ch;
    return 0;
}    

static int sku_mux_blocked_ch_set(int lc_id, unsigned long ch)
{
    (void)lc_id;
    muxFunc.pca->block_channel_set(muxFunc.dev, (u64)ch);
    return 0;
}    

static int sku_mux_fail_set(int lc_id, bool is_fail)
{
    (void)lc_id;
    muxFunc.pca->mux_fail_set(muxFunc.dev, is_fail);
    return 0;
}

static int sku_mux_fail_get(int lc_id, bool *is_fail)
{
    (void)lc_id;
    check_p(is_fail);
    muxFunc.pca->mux_fail_get(muxFunc.dev, is_fail);
    return 0;
}

static int sku_mux_ch_to_port(int lc_id, int ch)
{
    (void)lc_id;
    if (ch >= muxFunc.ch_2_port->size) {
        SKU_LOG_ERR("ch out of range %d\n", ch);
        return -1;
    }
    return muxFunc.ch_2_port->tbl[ch];
}    

static int sku_mux_port_to_ch(int lc_id, int port)
{
    int ch = 0;
    int ch_num = muxFunc.ch_2_port->size;
    (void)lc_id;
    
    for (ch = 0; ch < ch_num; ch++) {
        if (muxFunc.ch_2_port->tbl[ch] == port) {
            break;
        }            
    }
    if (ch >= ch_num) {
        SKU_LOG_ERR("cant find ch\n");
        return -1;
    }
    return ch;
}

static int mux_func_load(struct mux_func_t *func)
{
    struct device *dev = NULL;
    struct i2c_client *client = NULL;
    struct i2c_mux_core *muxc = NULL;
    struct pca954x *pca = NULL;

    if (!p_valid(dev = find_dev(INV_MUX_NAME, &i2c_bus_type))) {
        SKU_LOG_ERR("Could not find INV_MUX_NAME(%s)\n", INV_MUX_NAME);
        return -EBADRQC;
    }

    client = i2c_verify_client(dev);
    muxc = i2c_get_clientdata(client);
    if (!p_valid(pca = i2c_mux_priv(muxc))) {
        return -EBADRQC;
    }
    
    func->dev = dev;
    func->pca = pca;
    
    return 0;
}    

int ioexp_sku_init(int platform_id, int io_no_init)
{
    bool need_init = false;
    
    need_init = (io_no_init == 1 ? false : true);
    if (gpio_base_find(&gpio_base) < 0) {
        SKU_LOG_ERR("find gpio name fail\n");
        goto exit_err;
    }
    SKU_LOG_DBG("gpio_base:%d\n", gpio_base);
    if (table_load(platform_id, &ioexp_sku) < 0) {
        SKU_LOG_ERR("ioexp_table load fail\n");
        goto exit_err;
    }
    if (ioexp_clients_create(&ioexp_sku.ioexp_dev) < 0) {
        SKU_LOG_ERR("ioexp_clients_create fail\n");
        goto exit_err;
    }
    if (ioexp_i2c_clients_create(&ioexp_sku.ioexp_dev) < 0) {
        SKU_LOG_ERR("ioexp_i2c_clients_create fail\n");
        goto exit_kfree_clients;
    }
    if (ioexp_i2c_clients_init(&ioexp_sku.ioexp_dev) < 0) {
        SKU_LOG_ERR("ioexp_i2c_clients_init fail\n");
        goto exit_kfree_i2c_clients;
    }
    if (ioexp_config_init(&ioexp_sku.ioexp_dev, need_init) < 0) {
        SKU_LOG_ERR("ioexp_config_init fail\n");
        goto deinit_i2c_clients;
    }
    if (ioexp_input_init(&ioexp_sku.ioexp_dev) < 0) {
        SKU_LOG_ERR("ioexp_input_init fail\n");
        goto deinit_i2c_clients;
    }

    if (ioexp_output_init(&ioexp_sku.ioexp_dev, need_init) < 0) {
        SKU_LOG_ERR("ioexp_out_init fail\n");
        goto deinit_i2c_clients;
    }
    if (ioexp_sku.intr_mode_supported) {
        if (cpld_io_init(&ioexp_sku.cpld_io) < 0) {
            SKU_LOG_ERR("cpld_io_init fail\n");
            goto deinit_i2c_clients;
        }
    }
    ioexp_sku.mux_rst_gpio += gpio_base;
    if(ioexp_sku_mux_rst_gpio_init(ioexp_sku.mux_rst_gpio) < 0) {
        SKU_LOG_ERR("ioexp_sku_mux_rst_gpio_init fail\n");
        goto deinit_i2c_clients;
    }
    SKU_LOG_DBG("OK\n");
    return 0;
deinit_i2c_clients:
    ioexp_i2c_clients_deinit(&ioexp_sku.ioexp_dev);
exit_kfree_i2c_clients:
    ioexp_i2c_clients_destroy(&ioexp_sku.ioexp_dev);
exit_kfree_clients:
    ioexp_clients_destroy(&ioexp_sku.ioexp_dev);
exit_err:
    return -EBADRQC;
}

void ioexp_sku_deinit(void)
{
    ioexp_i2c_clients_deinit(&ioexp_sku.ioexp_dev);
    ioexp_i2c_clients_destroy(&ioexp_sku.ioexp_dev);
    ioexp_clients_destroy(&ioexp_sku.ioexp_dev);

    if (ioexp_sku.intr_mode_supported) {
        cpld_io_deinit(&ioexp_sku.cpld_io);
    }
    gpio_free(ioexp_sku.mux_rst_gpio);
    SKU_LOG_DBG("OK\n");
}




