/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#define PMBUS_PAGE	0x0
#define PMBUS_VOUT_MODE	0x20
#define PMBUS_READ_VOUT	0x8b


static int retrys = 1;
module_param(retrys, int, 0);
MODULE_PARM_DESC(retrys, "Inventec UCD90160 read retry times (default 1).");

struct ucd90160_data {
	struct device *hwmon_dev;
	s32	vout[16];
	s32 vout_mode[16];
	s32 pvout[16];
};

static s32 calc_voltage(struct device *dev, s32 vout, s32 vout_mode)
{
	s32 mode = (vout_mode & 0xe0) >> 5;
	s32 parameter = (vout_mode & 0x1f);

	switch (mode) {
		case 0:
			{
				s32 exp = (parameter & 0x10) ? - ((32 - parameter) & 0xf) : (parameter & 0xf);
				if (exp >= 0)
					return ((vout * 1000) << exp);
				else
					return ((vout * 1000) >> -exp);
			}
		case 1:
			dev_err(dev, "Not Support for VID mode");
			return 0;
		case 2:
			dev_err(dev, "Not Support for Direct mode");
			return 0;
		default:
			dev_err(dev, "Unknown Mode");
			return 0;
	}
}

static void update_vout_mode(struct i2c_client *client, u8 page, s32 clean)
{
	struct ucd90160_data *data = i2c_get_clientdata(client);
	s32 temp = 0, read = 0, trys = 0;

	if (clean) {
		data->vout_mode[page] = 0;
		return;
	}

	if (data->vout_mode[page])
		return;

	while (trys <= retrys) {
		read = i2c_smbus_read_byte_data(client, PMBUS_VOUT_MODE);
		temp = i2c_smbus_read_byte_data(client, PMBUS_PAGE);
		if (page != temp) {
			trys++;
			if (trys > retrys)
				return;
			continue;
		}
		break;
	}

	data->vout_mode[page] = read;
}

static ssize_t read_voltage_out(struct device *dev, struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct ucd90160_data *data = i2c_get_clientdata(client);
	s32 temp = 0, vout = 0, trys = 0;
	u8 page = attr->index;

	while (trys <= retrys) {
		temp = i2c_smbus_write_byte_data(client, PMBUS_PAGE, page);
		if (temp < 0) {
			update_vout_mode(client, page, 1);
			trys++;
			if (trys > retrys)	goto absent;
			continue;
		}

		update_vout_mode(client, page, 0);
		vout = i2c_smbus_read_word_data(client, PMBUS_READ_VOUT);
		if (vout < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		temp = i2c_smbus_read_byte_data(client, PMBUS_PAGE);
		if (page != temp) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	data->pvout[page] = data->vout[page];
	data->vout[page] = calc_voltage(dev, vout, data->vout_mode[page]);

	return sprintf(buf, "%d\n", data->vout[page]);

fail:
	return  sprintf(buf, "%d\n", data->pvout[page]);

absent:
	return  sprintf(buf, "0\n");
}

static SENSOR_DEVICE_ATTR(in1_input, S_IRUGO, read_voltage_out, 0, 0);
static SENSOR_DEVICE_ATTR(in2_input, S_IRUGO, read_voltage_out, 0, 1);
static SENSOR_DEVICE_ATTR(in3_input, S_IRUGO, read_voltage_out, 0, 2);
static SENSOR_DEVICE_ATTR(in4_input, S_IRUGO, read_voltage_out, 0, 3);
static SENSOR_DEVICE_ATTR(in5_input, S_IRUGO, read_voltage_out, 0, 4);
static SENSOR_DEVICE_ATTR(in6_input, S_IRUGO, read_voltage_out, 0, 5);
static SENSOR_DEVICE_ATTR(in7_input, S_IRUGO, read_voltage_out, 0, 6);
static SENSOR_DEVICE_ATTR(in8_input, S_IRUGO, read_voltage_out, 0, 7);
static SENSOR_DEVICE_ATTR(in9_input, S_IRUGO, read_voltage_out, 0, 8);
static SENSOR_DEVICE_ATTR(in10_input, S_IRUGO, read_voltage_out, 0, 9);
static SENSOR_DEVICE_ATTR(in11_input, S_IRUGO, read_voltage_out, 0, 10);
static SENSOR_DEVICE_ATTR(in12_input, S_IRUGO, read_voltage_out, 0, 11);
static SENSOR_DEVICE_ATTR(in13_input, S_IRUGO, read_voltage_out, 0, 12);
static SENSOR_DEVICE_ATTR(in14_input, S_IRUGO, read_voltage_out, 0, 13);
static SENSOR_DEVICE_ATTR(in15_input, S_IRUGO, read_voltage_out, 0, 14);
static SENSOR_DEVICE_ATTR(in16_input, S_IRUGO, read_voltage_out, 0, 15);

static struct attribute *ucd90160_attributes[] = {
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in2_input.dev_attr.attr,
	&sensor_dev_attr_in3_input.dev_attr.attr,
	&sensor_dev_attr_in4_input.dev_attr.attr,
	&sensor_dev_attr_in5_input.dev_attr.attr,
	&sensor_dev_attr_in6_input.dev_attr.attr,
	&sensor_dev_attr_in7_input.dev_attr.attr,
	&sensor_dev_attr_in8_input.dev_attr.attr,
	&sensor_dev_attr_in9_input.dev_attr.attr,
	&sensor_dev_attr_in10_input.dev_attr.attr,
	&sensor_dev_attr_in11_input.dev_attr.attr,
	&sensor_dev_attr_in12_input.dev_attr.attr,
	&sensor_dev_attr_in13_input.dev_attr.attr,
	&sensor_dev_attr_in14_input.dev_attr.attr,
	&sensor_dev_attr_in15_input.dev_attr.attr,
	&sensor_dev_attr_in16_input.dev_attr.attr,
	NULL
};

static const struct attribute_group ucd90160_group = {
	.attrs = ucd90160_attributes,
};

/*-----------------------------------------------------------------------*/
/* device probe and removal */
static int
inv_ucd90160_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ucd90160_data *data;
	int status;

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA))
		return -EIO;

	data = kzalloc(sizeof(struct ucd90160_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	i2c_set_clientdata(client, data);

	/* Register sysfs hooks */
	status = sysfs_create_group(&client->dev.kobj, &ucd90160_group);

	if (status)
		goto exit_free;

	data->hwmon_dev = hwmon_device_register(&client->dev);
	if (IS_ERR(data->hwmon_dev)) {
		status = PTR_ERR(data->hwmon_dev);
		goto exit_remove;
	}

	return 0;

exit_remove:
	sysfs_remove_group(&client->dev.kobj, &ucd90160_group);
exit_free:
	i2c_set_clientdata(client, NULL);
	kfree(data);
	return status;
}

static int inv_ucd90160_remove(struct i2c_client *client)
{
	struct ucd90160_data *data = i2c_get_clientdata(client);

	sysfs_remove_group(&client->dev.kobj, &ucd90160_group);
	hwmon_device_unregister(data->hwmon_dev);
	i2c_set_clientdata(client, NULL);
	kfree(data);
	return 0;
}

static const struct i2c_device_id inv_ucd90160_ids[] = {
	{ "inv_ucd90160" , 0, },
	{ "inv_ucd90160_cpu" , 0, },
	{ "inv_ucd90160_switch" , 0, },
	{ "inv_ucd90160_line" , 0, },
	{ /* LIST END */ }
};
MODULE_DEVICE_TABLE(i2c, inv_ucd90160_ids);

static struct i2c_driver ucd90160_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "inv_ucd90160",
	},
	.probe		= inv_ucd90160_probe,
	.remove		= inv_ucd90160_remove,
	.id_table	= inv_ucd90160_ids,
};

static int __init inv_ucd90160_init(void)
{
	return i2c_add_driver(&ucd90160_driver);
}

static void __exit inv_ucd90160_exit(void)
{
	i2c_del_driver(&ucd90160_driver);
}

MODULE_AUTHOR("Roger Chang <chang.rogermc@inventec>");
MODULE_DESCRIPTION("inventec ucd90160 driver");
MODULE_LICENSE("GPL");

module_init(inv_ucd90160_init);
module_exit(inv_ucd90160_exit);
