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

#define PMBUS_VOUT_MODE		0x20
#define PMBUS_FAN_PWM		0x3b
#define PMBUS_READ_VIN		0x88
#define PMBUS_READ_IIN		0x89
#define PMBUS_READ_VOUT		0x8b
#define PMBUS_READ_IOUT		0x8c
#define PMBUS_READ_TEMP1	0x8d
#define PMBUS_READ_TEMP2	0x8e
#define PMBUS_READ_TEMP3	0x8f
#define PMBUS_READ_FAN1		0x90
#define PMBUS_READ_FAN2		0x91
#define PMBUS_READ_POUT		0x96
#define PMBUS_READ_PIN		0x97
#define PMBUS_READ_MFR_ID	0x99
#define PMBUS_READ_MFR_MODEL	0x9a
#define PMBUS_READ_MFR_REV	0x9b
#define PMBUS_READ_MFR_LOC	0x9c
#define PMBUS_READ_MFR_DATE	0x9d
#define PMBUS_READ_MFR_SERIAL	0x9e

#define DEFAULT_MFR_FAIL_STR	"NA"

static int retrys = 1;
module_param(retrys, int, 0);
MODULE_PARM_DESC(retrys, "PSU driver read retry times (default 1).");

static int read_only_once = 0;
module_param(read_only_once, int, 0);
MODULE_PARM_DESC(read_only_once, "PSU driver MFR field read only once flag (default 0).");

#define FAN_NUM	2
#define TMP_NUM	3

struct psu_data {
	struct device *hwmon_dev;
	s32 vin;
	s32 vout;
	s32 cin;
	s32 cout;
	s32 pin;
	s32 pout;
	s32 vout_mode;
	s32 rpm[FAN_NUM];
	s32	temp[TMP_NUM];
	s32 pwm;
	u8 id[I2C_SMBUS_BLOCK_MAX + 2];
	u8 model[I2C_SMBUS_BLOCK_MAX + 2];
	u8 revision[I2C_SMBUS_BLOCK_MAX + 2];
	u8 location[I2C_SMBUS_BLOCK_MAX + 2];
	u8 date[I2C_SMBUS_BLOCK_MAX + 2];
	u8 serial[I2C_SMBUS_BLOCK_MAX + 2];
};

static s32 reg2data_linear(s32 value, s32 mode, u8 reg)
{
	s16 exponent;
	s32 mantissa;
	s32 val;

	if (mode) {
		s32 parameter = (mode & 0x1f);
		exponent = (parameter & 0x10) ? - ((32 - parameter) & 0xf) : (parameter & 0xf);
		mantissa = (u16) value;
	} else {
		exponent = ((s16)value) >> 11;
		mantissa = ((s16)((value & 0x7ff) << 5)) >> 5;
	}

	val = mantissa;

	if (reg != PMBUS_READ_FAN1 && reg != PMBUS_READ_FAN2 && reg != PMBUS_FAN_PWM)
		val = val * 1000;

	if (exponent >= 0)
		val <<= exponent;
	else
		val >>= -exponent;

	return val;
}

static void get_vout_mode(struct i2c_client *client, s32 clean)
{
	struct psu_data *data = i2c_get_clientdata(client);
	s32 read = 0, trys = 0;

	if (clean) {
		data->vout_mode = 0;
		return;
	}

	if (data->vout_mode)
		return;

	while (trys <= retrys) {
		read = i2c_smbus_read_byte_data(client, PMBUS_VOUT_MODE);
		if (read < 0) {
			trys++;
			if (trys > retrys)
				return;
			continue;
		}
		break;
	}

	data->vout_mode = read;
}

static ssize_t read_voltage_out(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 vout = 0, trys = 0;

	while (trys <= retrys) {

		vout = i2c_smbus_read_word_data(client, PMBUS_READ_VOUT);
		if (vout < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			get_vout_mode(client, 1);
			continue;
		}

		get_vout_mode(client, 0);

		break;
	}

	data->vout = reg2data_linear(vout, data->vout_mode, PMBUS_READ_VOUT);

	return sprintf(buf, "%d\n", data->vout);

fail:
	return  sprintf(buf, "0\n");
}

static ssize_t read_voltage_in(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 vin = 0, trys = 0;

	while (trys <= retrys) {

		vin = i2c_smbus_read_word_data(client, PMBUS_READ_VIN);
		if (vin < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	data->vin = reg2data_linear(vin, 0, PMBUS_READ_VIN);

	return sprintf(buf, "%d\n", data->vin);

fail:
	return  sprintf(buf, "0\n");
}

static ssize_t read_current_in(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 cin = 0, trys = 0;

	while (trys <= retrys) {

		cin = i2c_smbus_read_word_data(client, PMBUS_READ_IIN);
		if (cin < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	data->cin = reg2data_linear(cin, 0, PMBUS_READ_IIN);

	return sprintf(buf, "%d\n", data->cin);

fail:
	return  sprintf(buf, "0\n");
}

static ssize_t read_current_out(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 cout = 0, trys = 0;

	while (trys <= retrys) {

		cout = i2c_smbus_read_word_data(client, PMBUS_READ_IOUT);
		if (cout < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	data->cout = reg2data_linear(cout, 0, PMBUS_READ_IOUT);

	return sprintf(buf, "%d\n", data->cout);

fail:
	return  sprintf(buf, "0\n");
}

static ssize_t read_power_in(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 pin = 0, trys = 0;

	while (trys <= retrys) {

		pin = i2c_smbus_read_word_data(client, PMBUS_READ_PIN);
		if (pin < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	data->pin = reg2data_linear(pin, 0, PMBUS_READ_PIN);

	return sprintf(buf, "%d\n", data->pin);

fail:
	return  sprintf(buf, "0\n");
}

static ssize_t read_power_out(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 pout = 0, trys = 0;

	while (trys <= retrys) {

		pout = i2c_smbus_read_word_data(client, PMBUS_READ_POUT);
		if (pout < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	data->pout = reg2data_linear(pout, 0, PMBUS_READ_POUT);

	return sprintf(buf, "%d\n", data->pout);

fail:
	return  sprintf(buf, "0\n");
}

static ssize_t read_temp(struct device *dev, struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 temp = 0, trys = 0;
	u8 nr = attr->index;

	while (trys <= retrys) {

		temp = i2c_smbus_read_word_data(client, PMBUS_READ_TEMP1 + nr);
		if (temp < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	data->temp[nr] = reg2data_linear(temp, 0, PMBUS_READ_TEMP1 + nr);

	return sprintf(buf, "%d\n", data->temp[nr]);

fail:
	return  sprintf(buf, "0\n");
}

static ssize_t read_fan(struct device *dev, struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 rpm = 0, trys = 0;
	u8 nr = attr->index;

	while (trys <= retrys) {

		rpm = i2c_smbus_read_word_data(client, PMBUS_READ_FAN1 + nr);
		if (rpm < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	data->rpm[nr] = reg2data_linear(rpm, 0, PMBUS_READ_FAN1 + nr);

	return sprintf(buf, "%d\n", data->rpm[nr]);

fail:
	return  sprintf(buf, "0\n");
}

static ssize_t read_pwm(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 pwm = 0, trys = 0;

	while (trys <= retrys) {

		pwm = i2c_smbus_read_word_data(client, PMBUS_FAN_PWM);
		if (pwm < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	data->pwm = reg2data_linear(pwm, 0, PMBUS_FAN_PWM);

	return sprintf(buf, "%d\n", data->pwm);

fail:
	return  sprintf(buf, "0\n");
}

static ssize_t set_pwm(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 temp = 0, trys = 0;

	u16 pwm = simple_strtol(buf, NULL, 10);
	if(pwm > 100) pwm = 100;

	while (trys <= retrys) {

		temp = i2c_smbus_write_word_data(client, PMBUS_FAN_PWM, pwm);
		if (temp < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	data->pwm = pwm;

	return count;

fail:
	return -1;
}

static ssize_t read_mfr_id(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 temp = 0, trys = 0;

	if (read_only_once && data->id[0] != 0)
		return sprintf(buf, "%s\n", data->id);

	while (trys <= retrys) {

		temp = i2c_smbus_read_block_data(client, PMBUS_READ_MFR_ID, data->id);
		if (temp < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	return sprintf(buf, "%s\n", data->id);

fail:
	memset(data->id, 0, I2C_SMBUS_BLOCK_MAX + 2);
	return  sprintf(buf, "%s\n", DEFAULT_MFR_FAIL_STR);
}

static ssize_t read_mfr_model(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 temp = 0, trys = 0;

	if (read_only_once && data->model[0] != 0)
		return sprintf(buf, "%s\n", data->model);

	while (trys <= retrys) {

		temp = i2c_smbus_read_block_data(client, PMBUS_READ_MFR_MODEL, data->model);
		if (temp < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	return sprintf(buf, "%s\n", data->model);

fail:
	memset(data->model, 0, I2C_SMBUS_BLOCK_MAX + 2);
	return  sprintf(buf, "%s\n", DEFAULT_MFR_FAIL_STR);
}

static ssize_t read_mfr_revision(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 temp = 0, trys = 0;

	if (read_only_once && data->revision[0] != 0)
		return sprintf(buf, "%s\n", data->revision);

	while (trys <= retrys) {

		temp = i2c_smbus_read_block_data(client, PMBUS_READ_MFR_REV, data->revision);
		if (temp < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	return sprintf(buf, "%s\n", data->revision);

fail:
	memset(data->revision, 0, I2C_SMBUS_BLOCK_MAX + 2);
	return  sprintf(buf, "%s\n", DEFAULT_MFR_FAIL_STR);
}

static ssize_t read_mfr_location(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 temp = 0, trys = 0;

	if (read_only_once && data->location[0] != 0)
		return sprintf(buf, "%s\n", data->location);

	while (trys <= retrys) {

		temp = i2c_smbus_read_block_data(client, PMBUS_READ_MFR_LOC, data->location);
		if (temp < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	return sprintf(buf, "%s\n", data->location);

fail:
	memset(data->location, 0, I2C_SMBUS_BLOCK_MAX + 2);
	return  sprintf(buf, "%s\n", DEFAULT_MFR_FAIL_STR);
}

static ssize_t read_mfr_date(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 temp = 0, trys = 0;

	if (read_only_once && data->date[0] != 0)
		return sprintf(buf, "%s\n", data->date);

	while (trys <= retrys) {

		temp = i2c_smbus_read_block_data(client, PMBUS_READ_MFR_DATE, data->date);
		if (temp < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	return sprintf(buf, "%s\n", data->date);

fail:
	memset(data->date, 0, I2C_SMBUS_BLOCK_MAX + 2);
	return  sprintf(buf, "%s\n", DEFAULT_MFR_FAIL_STR);
}

static ssize_t read_mfr_sn(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct psu_data *data = i2c_get_clientdata(client);
	s32 temp = 0, trys = 0;

	if (read_only_once && data->serial[0] != 0)
		return sprintf(buf, "%s\n", data->serial);

	while (trys <= retrys) {

		temp = i2c_smbus_read_block_data(client, PMBUS_READ_MFR_SERIAL, data->serial);
		if (temp < 0) {
			trys++;
			if (trys > retrys)	goto fail;
			continue;
		}

		break;
	}

	return sprintf(buf, "%s\n", data->serial);

fail:
	memset(data->serial, 0, I2C_SMBUS_BLOCK_MAX + 2);
	return  sprintf(buf, "%s\n", DEFAULT_MFR_FAIL_STR);
}

static SENSOR_DEVICE_ATTR(in1_input, S_IRUGO, read_voltage_in, 0, 0);
static SENSOR_DEVICE_ATTR(in2_input, S_IRUGO, read_voltage_out, 0, 0);
static SENSOR_DEVICE_ATTR(curr1_input, S_IRUGO, read_current_in, 0, 0);
static SENSOR_DEVICE_ATTR(curr2_input, S_IRUGO, read_current_out, 0, 0);
static SENSOR_DEVICE_ATTR(power1_input, S_IRUGO, read_power_in, 0, 0);
static SENSOR_DEVICE_ATTR(power2_input, S_IRUGO, read_power_out, 0, 0);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, read_temp, 0, 0);
static SENSOR_DEVICE_ATTR(temp2_input, S_IRUGO, read_temp, 0, 1);
static SENSOR_DEVICE_ATTR(temp3_input, S_IRUGO, read_temp, 0, 2);
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, read_fan, 0, 0);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO, read_fan, 0, 1);
static SENSOR_DEVICE_ATTR(pwm1, S_IWUSR|S_IRUGO, read_pwm, set_pwm, 0);
static SENSOR_DEVICE_ATTR(mfr_id, S_IRUGO, read_mfr_id, 0, 0);
static SENSOR_DEVICE_ATTR(mfr_model, S_IRUGO, read_mfr_model, 0, 0);
static SENSOR_DEVICE_ATTR(mfr_revision, S_IRUGO, read_mfr_revision, 0, 0);
static SENSOR_DEVICE_ATTR(mfr_location, S_IRUGO, read_mfr_location, 0, 0);
static SENSOR_DEVICE_ATTR(mfr_date, S_IRUGO, read_mfr_date, 0, 0);
static SENSOR_DEVICE_ATTR(mfr_serial_number, S_IRUGO, read_mfr_sn, 0, 0);

static struct attribute *psu_attributes[] = {
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in2_input.dev_attr.attr,
	&sensor_dev_attr_curr1_input.dev_attr.attr,
	&sensor_dev_attr_curr2_input.dev_attr.attr,
	&sensor_dev_attr_power1_input.dev_attr.attr,
	&sensor_dev_attr_power2_input.dev_attr.attr,
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp2_input.dev_attr.attr,
	&sensor_dev_attr_temp3_input.dev_attr.attr,
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_fan2_input.dev_attr.attr,
	&sensor_dev_attr_pwm1.dev_attr.attr,
	&sensor_dev_attr_mfr_id.dev_attr.attr,
	&sensor_dev_attr_mfr_model.dev_attr.attr,
	&sensor_dev_attr_mfr_revision.dev_attr.attr,
	&sensor_dev_attr_mfr_location.dev_attr.attr,
	&sensor_dev_attr_mfr_date.dev_attr.attr,
	&sensor_dev_attr_mfr_serial_number.dev_attr.attr,
	NULL
};

static const struct attribute_group psu_group = {
	.attrs = psu_attributes,
};

/*-----------------------------------------------------------------------*/
/* device probe and removal */
static int
net_psu_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct psu_data *data;
	int status;

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA))
		return -EIO;

	data = kzalloc(sizeof(struct psu_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	i2c_set_clientdata(client, data);

	/* Register sysfs hooks */
	status = sysfs_create_group(&client->dev.kobj, &psu_group);

	if (status)
		goto exit_free;

	data->hwmon_dev = hwmon_device_register(&client->dev);
	if (IS_ERR(data->hwmon_dev)) {
		status = PTR_ERR(data->hwmon_dev);
		goto exit_remove;
	}

	return 0;

exit_remove:
	sysfs_remove_group(&client->dev.kobj, &psu_group);
exit_free:
	i2c_set_clientdata(client, NULL);
	kfree(data);
	return status;
}

static int net_psu_remove(struct i2c_client *client)
{
	struct psu_data *data = i2c_get_clientdata(client);

	sysfs_remove_group(&client->dev.kobj, &psu_group);
	hwmon_device_unregister(data->hwmon_dev);
	i2c_set_clientdata(client, NULL);
	kfree(data);
	return 0;
}

static const struct i2c_device_id net_psu_ids[] = {
	{ "net_psu" , 0, },
	{ /* LIST END */ }
};
MODULE_DEVICE_TABLE(i2c, net_psu_ids);

static struct i2c_driver psu_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "net_psu",
	},
	.probe		= net_psu_probe,
	.remove		= net_psu_remove,
	.id_table	= net_psu_ids,
};

static int __init net_psu_init(void)
{
	return i2c_add_driver(&psu_driver);
}

static void __exit net_psu_exit(void)
{
	i2c_del_driver(&psu_driver);
}

MODULE_AUTHOR("support <support@netbergtw.com>");
MODULE_DESCRIPTION("PSU driver");
MODULE_LICENSE("GPL");

module_init(net_psu_init);
module_exit(net_psu_exit);
