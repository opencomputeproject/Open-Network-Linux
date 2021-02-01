/*
 * An I2C multiplexer dirver for accton as5712 fpga
 *
 * Copyright (C) 2015 Accton Technology Corporation.
 * Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * This module supports the accton fpga that hold the channel select
 * mechanism for other i2c slave devices, such as SFP.
 * This includes the:
 *     Accton as5915_18x fpga1/fpga2/fpga3
 *
 * Based on:
 *    pca954x.c from Kumar Gala <galak@kernel.crashing.org>
 * Copyright (C) 2006
 *
 * Based on:
 *    pca954x.c from Ken Harrenstien
 * Copyright (C) 2004 Google, Inc. (Ken Harrenstien)
 *
 * Based on:
 *    i2c-virtual_cb.c from Brian Kuschak <bkuschak@yahoo.com>
 * and
 *    pca9540.c from Jean Delvare <khali@linux-fr.org>.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/version.h>
#include <linux/stat.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>

#define I2C_RW_RETRY_COUNT        10
#define I2C_RW_RETRY_INTERVAL     60 /* ms */

#define NUM_OF_FPGA_CHANS         0x4
#define FPGA_CHANNEL_SELECT_REG   0x80
#define FPGA_DESELECT_CHANNEL     0x10

static LIST_HEAD(fpga_client_list);
static struct mutex     list_lock;

struct fpga_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

enum fpga_mux_type {
    as5915_18x_fpga
};

struct as5915_18x_fpga_data {
    enum fpga_mux_type type;
    struct i2c_client *client;
    u8 last_chan;  /* last register value */

    struct device      *hwmon_dev;
    struct mutex        update_lock;
};

struct chip_desc {
    u8   nchans;
    u8   deselectChan;
};

/* Provide specs for the PCA954x types we know about */
static const struct chip_desc chips[] = {
    [as5915_18x_fpga] = {
    .nchans        = NUM_OF_FPGA_CHANS,
    .deselectChan  = FPGA_DESELECT_CHANNEL,
    }
};

static const struct i2c_device_id as5915_18x_fpga_mux_id[] = {
    { "as5915_18x_fpga", as5915_18x_fpga },
    { }
};
MODULE_DEVICE_TABLE(i2c, as5915_18x_fpga_mux_id);

enum as5915_18x_fpga_sysfs_attributes {
    FPGA_VERSION,
    PCB_VERSION,
    ACCESS,
};

/* sysfs attributes for hwmon
 */
static ssize_t access(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count);
static ssize_t show_version(struct device *dev, struct device_attribute *da,
             char *buf);
static int as5915_18x_fpga_read_internal(struct i2c_client *client, u8 reg);
static int as5915_18x_fpga_write_internal(struct i2c_client *client, u8 reg, u8 value);

static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_version, NULL, FPGA_VERSION);
static SENSOR_DEVICE_ATTR(pcb_version, S_IRUGO, show_version, NULL, PCB_VERSION);
static SENSOR_DEVICE_ATTR(access, S_IWUSR, NULL, access, ACCESS);

static struct attribute *as5915_18x_fpga_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_pcb_version.dev_attr.attr,
    &sensor_dev_attr_access.dev_attr.attr,
    NULL
};

static const struct attribute_group as5915_18x_fpga_group = {
    .attrs = as5915_18x_fpga_attributes,
};

static ssize_t access(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    struct as5915_18x_fpga_data *data = i2c_mux_priv(muxc);
    int status;
    u32 addr, val;

    if (sscanf(buf, "0x%x 0x%x", &addr, &val) != 2) {
        return -EINVAL;
    }

    if (addr > 0xFF || val > 0xFF) {
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    status = as5915_18x_fpga_write_internal(client, addr, val);
    if (unlikely(status < 0)) {
        goto exit;
    }
    mutex_unlock(&data->update_lock);
    return count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

/* Write to mux register. Don't use i2c_transfer()/i2c_smbus_xfer()
   for this as they will try to lock adapter a second time */
static int as5915_18x_fpga_mux_reg_write(struct i2c_adapter *adap,
                 struct i2c_client *client, u8 val)
{
    unsigned long orig_jiffies;
    unsigned short flags;
    union i2c_smbus_data data;
    int try;
    s32 res = -EIO;

    data.byte = val;
    flags = client->flags;
    flags &= I2C_M_TEN | I2C_CLIENT_PEC;

    if (adap->algo->smbus_xfer) {
        /* Retry automatically on arbitration loss */
        orig_jiffies = jiffies;
        for (res = 0, try = 0; try <= adap->retries; try++) {
            res = adap->algo->smbus_xfer(adap, client->addr, flags,
                             I2C_SMBUS_WRITE, FPGA_CHANNEL_SELECT_REG,
                             I2C_SMBUS_BYTE_DATA, &data);
            if (res != -EAGAIN)
                break;
            if (time_after(jiffies,
                orig_jiffies + adap->timeout))
                break;
        }
    }

    return res;
}

static int as5915_18x_fpga_mux_select_chan(struct i2c_mux_core *muxc,
    u32 chan)
{
    struct as5915_18x_fpga_data *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;
    u8 regval;
    int ret = 0;

    regval = (chan + 1) << 5;
    /* Only select the channel if its different from the last channel */
    if (data->last_chan != regval) {
        ret = as5915_18x_fpga_mux_reg_write(muxc->parent, client, regval);
        data->last_chan = regval;
    }

    return ret;
}

static int as5915_18x_fpga_mux_deselect_mux(struct i2c_mux_core *muxc,
    u32 chan)
{
    struct as5915_18x_fpga_data *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;

    /* Deselect active channel */
    data->last_chan = chips[data->type].deselectChan;

    return as5915_18x_fpga_mux_reg_write(muxc->parent, client, data->last_chan);
}

static void as5915_18x_fpga_add_client(struct i2c_client *client)
{
    struct fpga_client_node *node = kzalloc(sizeof(struct fpga_client_node), GFP_KERNEL);

    if (!node) {
        dev_dbg(&client->dev, "Can't allocate fpga_client_node (0x%x)\n", client->addr);
        return;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &fpga_client_list);
    mutex_unlock(&list_lock);
}

static void as5915_18x_fpga_remove_client(struct i2c_client *client)
{
    struct list_head    *list_node = NULL;
    struct fpga_client_node *fpga_node = NULL;
    int found = 0;

    mutex_lock(&list_lock);

    list_for_each(list_node, &fpga_client_list) {
        fpga_node = list_entry(list_node, struct fpga_client_node, list);

        if (fpga_node->client == client) {
            found = 1;
            break;
        }
    }

    if (found) {
        list_del(list_node);
        kfree(fpga_node);
    }

    mutex_unlock(&list_lock);
}

static ssize_t show_version(struct device *dev, struct device_attribute *da, char *buf)
{
    u8  reg = 0;
    int val = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    reg = (attr->index == FPGA_VERSION) ? 0x1 : 0x0; // 0 for PCB_VERSION
    val = i2c_smbus_read_byte_data(client, reg);

    if (val < 0) {
        dev_dbg(&client->dev, "fpga(0x%x) reg(0x%x) err %d\n", client->addr, reg, val);
    }

    return sprintf(buf, "%d\n", val);
}

/*
 * I2C init/probing/exit functions
 */
static int as5915_18x_fpga_mux_probe(struct i2c_client *client,
             const struct i2c_device_id *id)
{
    struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
    int num, force, class;
    struct i2c_mux_core *muxc;
    struct as5915_18x_fpga_data *data;
    int ret = 0;
    const struct attribute_group *group = &as5915_18x_fpga_group;

    if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE))
        return -ENODEV;

    muxc = i2c_mux_alloc(adap, &client->dev,
                 chips[id->driver_data].nchans, sizeof(*data), 0,
                 as5915_18x_fpga_mux_select_chan, as5915_18x_fpga_mux_deselect_mux);
    if (!muxc)
        return -ENOMEM;

    i2c_set_clientdata(client, muxc);
    data = i2c_mux_priv(muxc);
    data->client = client;
    data->type = id->driver_data;
    data->last_chan = chips[data->type].deselectChan;    /* force the first selection */
    mutex_init(&data->update_lock);

    /* Now create an adapter for each channel */
    for (num = 0; num < chips[data->type].nchans; num++) {
        force = 0;              /* dynamic adap number */
        class = 0;              /* no class by default */

        ret = i2c_mux_add_adapter(muxc, force, num, class);

        if (ret) {
            dev_err(&client->dev,
                "failed to register multiplexed adapter"
                " %d as bus %d\n", num, force);
            goto add_mux_failed;
        }
    }

    /* Register sysfs hooks */
    if (group) {
        ret = sysfs_create_group(&client->dev.kobj, group);
        if (ret) {
            goto add_mux_failed;
        }
    }

    if (chips[data->type].nchans) {
        dev_info(&client->dev,
             "registered %d multiplexed busses for I2C %s\n",
             num, client->name);
    }
    else {
        dev_info(&client->dev,
             "device %s registered\n", client->name);
    }

    as5915_18x_fpga_add_client(client);

    return 0;

add_mux_failed:
    i2c_mux_del_adapters(muxc);
    return ret;
}

static int as5915_18x_fpga_mux_remove(struct i2c_client *client)
{
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);
    const struct attribute_group *group = &as5915_18x_fpga_group;

    as5915_18x_fpga_remove_client(client);

    /* Remove sysfs hooks */
    if (group) {
        sysfs_remove_group(&client->dev.kobj, group);
    }

    i2c_mux_del_adapters(muxc);

    return 0;
}

static int as5915_18x_fpga_read_internal(struct i2c_client *client, u8 reg)
{
    int status = 0, retry = I2C_RW_RETRY_COUNT;

    while (retry) {
        status = i2c_smbus_read_byte_data(client, reg);
        if (unlikely(status < 0)) {
            msleep(I2C_RW_RETRY_INTERVAL);
            retry--;
            continue;
        }

        break;
    }

    return status;
}

static int as5915_18x_fpga_write_internal(struct i2c_client *client, u8 reg, u8 value)
{
    int status = 0, retry = I2C_RW_RETRY_COUNT;

    while (retry) {
        status = i2c_smbus_write_byte_data(client, reg, value);
        if (unlikely(status < 0)) {
            msleep(I2C_RW_RETRY_INTERVAL);
            retry--;
            continue;
        }

        break;
    }

    return status;
}

int as5915_18x_fpga_read(unsigned short fpga_addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct fpga_client_node *fpga_node = NULL;
    int ret = -EPERM;

    mutex_lock(&list_lock);

    list_for_each(list_node, &fpga_client_list) {
        fpga_node = list_entry(list_node, struct fpga_client_node, list);

        if (fpga_node->client->addr == fpga_addr) {
            ret = as5915_18x_fpga_read_internal(fpga_node->client, reg);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(as5915_18x_fpga_read);

int as5915_18x_fpga_write(unsigned short fpga_addr, u8 reg, u8 value)
{
    struct list_head   *list_node = NULL;
    struct fpga_client_node *fpga_node = NULL;
    int ret = -EIO;

    mutex_lock(&list_lock);

    list_for_each(list_node, &fpga_client_list) {
        fpga_node = list_entry(list_node, struct fpga_client_node, list);

        if (fpga_node->client->addr == fpga_addr) {
            ret = as5915_18x_fpga_write_internal(fpga_node->client, reg, value);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(as5915_18x_fpga_write);

static struct i2c_driver as5915_18x_fpga_mux_driver = {
    .driver        = {
        .name    = "as5915_18x_fpga",
        .owner    = THIS_MODULE,
    },
    .probe        = as5915_18x_fpga_mux_probe,
    .remove        = as5915_18x_fpga_mux_remove,
    .id_table    = as5915_18x_fpga_mux_id,
};

static int __init as5915_18x_fpga_mux_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&as5915_18x_fpga_mux_driver);
}

static void __exit as5915_18x_fpga_mux_exit(void)
{
    i2c_del_driver(&as5915_18x_fpga_mux_driver);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@edge-core.com>");
MODULE_DESCRIPTION("Accton as5915-18x fpga driver");
MODULE_LICENSE("GPL");

module_init(as5915_18x_fpga_mux_init);
module_exit(as5915_18x_fpga_mux_exit);
