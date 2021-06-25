/*
 * Copyright (C)  Alex Lai <alex_lai@edge-core.com>
 *
 * Based on:
 *	pca954x.c from Kumar Gala <galak@kernel.crashing.org>
 * Copyright (C) 2006
 *
 * Based on:
 *	pca954x.c from Ken Harrenstien
 * Copyright (C) 2004 Google, Inc. (Ken Harrenstien)
 *
 * Based on:
 *	i2c-virtual_cb.c from Brian Kuschak <bkuschak@yahoo.com>
 *      and pca9540.c from Jean Delvare <khali@linux-fr.org>.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <linux/hwmon-sysfs.h>
#include <linux/ipmi.h>
#include <linux/ipmi_smi.h>
#include <linux/platform_device.h>

#define DRVNAME "as9926_24db_led"
#define ACCTON_IPMI_NETFN   0x34
#define IPMI_LED_READ_CMD   0x1A
#define IPMI_LED_WRITE_CMD  0x1B
#define IPMI_TIMEOUT (20 * HZ)

static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data);
static ssize_t set_led(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t show_led(struct device *dev, struct device_attribute *attr, 
			char *buf);
static int as9926_24db_led_probe(struct platform_device *pdev);
static int as9926_24db_led_remove(struct platform_device *pdev);

enum led_data_index {
	LOC_INDEX,
 	DIAG_RED_INDEX
};

struct ipmi_data {
	struct completion   read_complete;
	struct ipmi_addr    address;
	ipmi_user_t         user;
	int                 interface;

	struct kernel_ipmi_msg tx_message;
	long                   tx_msgid;

	void            *rx_msg_data;
	unsigned short   rx_msg_len;
	unsigned char    rx_result;
	int              rx_recv_type;

	struct ipmi_user_hndl ipmi_hndlrs;
};

struct as9926_24db_led_data {
	struct platform_device *pdev;
	struct mutex     update_lock;
	char             valid;           /* != 0 if registers are valid */
	unsigned long    last_updated;    /* In jiffies */
	unsigned char    ipmi_resp[2];    /* 0: LOC LED, 1: DIAG Red LED */
	struct ipmi_data ipmi;
};

struct as9926_24db_led_data *data = NULL;

static struct platform_driver as9926_24db_led_driver = {
	.probe      = as9926_24db_led_probe,
	.remove     = as9926_24db_led_remove,
	.driver     = {
		.name   = DRVNAME,
		.owner  = THIS_MODULE,
	},
};

enum ipmi_led_light_mode {
	IPMI_LED_MODE_OFF,
	IPMI_LED_MODE_GREEN = 4,
	IPMI_LED_MODE_GREEN_BLINKING = 5,
	IPMI_LED_MODE_ORANGE = 16,
	IPMI_LED_MODE_ORANGE_BLINKING = 17,
};

enum led_light_mode {
	LED_MODE_OFF,
	LED_MODE_RED = 10,
	LED_MODE_RED_BLINKING = 11,
	LED_MODE_ORANGE = 12,
	LED_MODE_ORANGE_BLINKING = 13,
	LED_MODE_YELLOW = 14,
	LED_MODE_YELLOW_BLINKING = 15,
	LED_MODE_GREEN = 16,
	LED_MODE_GREEN_BLINKING = 17,
	LED_MODE_BLUE = 18,
	LED_MODE_BLUE_BLINKING = 19,
	LED_MODE_PURPLE = 20,
	LED_MODE_PURPLE_BLINKING = 21,
	LED_MODE_AUTO = 22,
	LED_MODE_AUTO_BLINKING = 23,
	LED_MODE_WHITE = 24,
	LED_MODE_WHITE_BLINKING = 25,
	LED_MODE_CYAN = 26,
	LED_MODE_CYAN_BLINKING = 27,
	LED_MODE_UNKNOWN = 99
};

enum as9926_24db_led_sysfs_attrs {
	LED_LOC,
	LED_DIAG,
	LED_PSU1,
	LED_PSU2,
	LED_FAN
};

static SENSOR_DEVICE_ATTR(led_loc, S_IWUSR | S_IRUGO, show_led, set_led, LED_LOC);
static SENSOR_DEVICE_ATTR(led_diag, S_IWUSR | S_IRUGO, show_led, set_led, LED_DIAG);
static SENSOR_DEVICE_ATTR(led_psu1, S_IWUSR | S_IRUGO, show_led, set_led, LED_PSU1);
static SENSOR_DEVICE_ATTR(led_psu2, S_IWUSR | S_IRUGO, show_led, set_led, LED_PSU2);
static SENSOR_DEVICE_ATTR(led_fan, S_IWUSR | S_IRUGO, show_led, set_led, LED_FAN);

static struct attribute *as9926_24db_led_attributes[] = {
	&sensor_dev_attr_led_loc.dev_attr.attr,
	&sensor_dev_attr_led_diag.dev_attr.attr,
	&sensor_dev_attr_led_psu1.dev_attr.attr,
	&sensor_dev_attr_led_psu2.dev_attr.attr,
	&sensor_dev_attr_led_fan.dev_attr.attr,
	NULL
};

static const struct attribute_group as9926_24db_led_group = {
	.attrs = as9926_24db_led_attributes,
};

/* Functions to talk to the IPMI layer */

/* Initialize IPMI address, message buffers and user data */
static int init_ipmi_data(struct ipmi_data *ipmi, int iface,
			  struct device *dev)
{
	int err;

	init_completion(&ipmi->read_complete);

	/* Initialize IPMI address */
	ipmi->address.addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE;
	ipmi->address.channel = IPMI_BMC_CHANNEL;
	ipmi->address.data[0] = 0;
	ipmi->interface = iface;

	/* Initialize message buffers */
	ipmi->tx_msgid = 0;
	ipmi->tx_message.netfn = ACCTON_IPMI_NETFN;

	ipmi->ipmi_hndlrs.ipmi_recv_hndl = ipmi_msg_handler;

	/* Create IPMI messaging interface user */
	err = ipmi_create_user(ipmi->interface, &ipmi->ipmi_hndlrs,
			       ipmi, &ipmi->user);
	if (err < 0) {
		dev_err(dev, "Unable to register user with IPMI "
			"interface %d\n", ipmi->interface);
		return -EACCES;
	}

	return 0;
}

/* Send an IPMI command */
static int ipmi_send_message(struct ipmi_data *ipmi, unsigned char cmd,
			     unsigned char *tx_data, unsigned short tx_len,
			     unsigned char *rx_data, unsigned short rx_len)
{
	int err;

	ipmi->tx_message.cmd      = cmd;
	ipmi->tx_message.data     = tx_data;
	ipmi->tx_message.data_len = tx_len;
	ipmi->rx_msg_data         = rx_data;
	ipmi->rx_msg_len          = rx_len;

	err = ipmi_validate_addr(&ipmi->address, sizeof(ipmi->address));
	if (err)
		goto addr_err;

	ipmi->tx_msgid++;
	err = ipmi_request_settime(ipmi->user, &ipmi->address, ipmi->tx_msgid,
				   &ipmi->tx_message, ipmi, 0, 0, 0);
	if (err)
		goto ipmi_req_err;

	err = wait_for_completion_timeout(&ipmi->read_complete, IPMI_TIMEOUT);
	if (!err)
		goto ipmi_timeout_err;

	return 0;

ipmi_timeout_err:
	err = -ETIMEDOUT;
	dev_err(&data->pdev->dev, "request_timeout=%x\n", err);
	return err;
ipmi_req_err:
	dev_err(&data->pdev->dev, "request_settime=%x\n", err);
	return err;
addr_err:
	dev_err(&data->pdev->dev, "validate_addr=%x\n", err);
	return err;
}

/* Dispatch IPMI messages to callers */
static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data)
{
	unsigned short rx_len;
	struct ipmi_data *ipmi = user_msg_data;

	if (msg->msgid != ipmi->tx_msgid) {
		dev_err(&data->pdev->dev, "Mismatch between received msgid "
			"(%02x) and transmitted msgid (%02x)!\n",
			(int)msg->msgid,
			(int)ipmi->tx_msgid);
		ipmi_free_recv_msg(msg);
		return;
	}

	ipmi->rx_recv_type = msg->recv_type;
	if (msg->msg.data_len > 0)
		ipmi->rx_result = msg->msg.data[0];
	else
		ipmi->rx_result = IPMI_UNKNOWN_ERR_COMPLETION_CODE;

	if (msg->msg.data_len > 1) {
		rx_len = msg->msg.data_len - 1;
		if (ipmi->rx_msg_len < rx_len)
			rx_len = ipmi->rx_msg_len;
		ipmi->rx_msg_len = rx_len;
		memcpy(ipmi->rx_msg_data, msg->msg.data + 1, ipmi->rx_msg_len);
	} else
		ipmi->rx_msg_len = 0;

	ipmi_free_recv_msg(msg);
	complete(&ipmi->read_complete);
}

static struct as9926_24db_led_data *as9926_24db_led_update_device(void)
{
	int status = 0;

	if (time_before(jiffies, data->last_updated + HZ * 5) && data->valid)
		return data;

	data->valid = 0;
	status = ipmi_send_message(&data->ipmi, IPMI_LED_READ_CMD, NULL, 0,
				   data->ipmi_resp, sizeof(data->ipmi_resp));
	if (unlikely(status != 0))
		goto exit;

	if (unlikely(data->ipmi.rx_result != 0)) {
		status = -EIO;
		goto exit;
	}

	data->last_updated = jiffies;
	data->valid = 1;

exit:
	return data;
}

static ssize_t show_led(struct device *dev, struct device_attribute *da,
			char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	int value = 0;
	int error = 0;

	mutex_lock(&data->update_lock);

	data = as9926_24db_led_update_device();
	if (!data->valid) {
		error = -EIO;
		goto exit;
	}

	switch (attr->index) {
	case LED_LOC:
		value = data->ipmi_resp[LOC_INDEX] ? LED_MODE_ORANGE_BLINKING 
						   : LED_MODE_OFF;
		break;
	case LED_DIAG:
	{
		if(data->ipmi_resp[DIAG_RED_INDEX] == IPMI_LED_MODE_ORANGE)
			value = LED_MODE_ORANGE;
		else if (data->ipmi_resp[DIAG_RED_INDEX]
			 == IPMI_LED_MODE_GREEN)
			value = LED_MODE_GREEN;
		else if (data->ipmi_resp[DIAG_RED_INDEX] 
			 == IPMI_LED_MODE_GREEN_BLINKING)
			value = LED_MODE_GREEN_BLINKING;
		break;
	}
	case LED_PSU1:
	case LED_PSU2:
	case LED_FAN:
		value = LED_MODE_AUTO;
		break;
	default:
		error = -EINVAL;
		goto exit;
	}

	mutex_unlock(&data->update_lock);
	return sprintf(buf, "%d\n", value);

exit:
	mutex_unlock(&data->update_lock);
	return error;
}

static ssize_t set_led(struct device *dev, struct device_attribute *da,
		       const char *buf, size_t count)
{
	long mode;
	int status;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

	status = kstrtol(buf, 10, &mode);
	if (status)
		return status;

	mutex_lock(&data->update_lock);

	data = as9926_24db_led_update_device();
	if (!data->valid) {
		status = -EIO;
		goto exit;
	}

	switch (attr->index) {
	case LED_LOC:
		if (mode == LED_MODE_OFF) {
			data->ipmi_resp[LOC_INDEX] = IPMI_LED_MODE_OFF;
		}
		else if (mode == LED_MODE_ORANGE_BLINKING) {
			data->ipmi_resp[LOC_INDEX] = 
			IPMI_LED_MODE_ORANGE_BLINKING;
		}
		else {
			status = -EINVAL;
			goto exit;
		}
		break;
	case LED_DIAG:
	{
		if(mode == LED_MODE_ORANGE) {
			data->ipmi_resp[DIAG_RED_INDEX] = IPMI_LED_MODE_ORANGE;
		}
		else if(mode == LED_MODE_GREEN) {
			data->ipmi_resp[DIAG_RED_INDEX] = IPMI_LED_MODE_GREEN;
		}
		else if(mode == LED_MODE_GREEN_BLINKING) {
			data->ipmi_resp[DIAG_RED_INDEX] = 
			IPMI_LED_MODE_GREEN_BLINKING;
		}
		else {
			status = -EINVAL;
			goto exit;
		}
		break;
	}
	default:
		status = -EINVAL;
		goto exit;
	}

	/* Send IPMI write command */
	status = ipmi_send_message(&data->ipmi, IPMI_LED_WRITE_CMD,
				   data->ipmi_resp, sizeof(data->ipmi_resp), 
				   NULL, 0);
	if (unlikely(status != 0))
		goto exit;

	if (unlikely(data->ipmi.rx_result != 0)) {
		status = -EIO;
		goto exit;
	}

	status = count;

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static int as9926_24db_led_probe(struct platform_device *pdev)
{
	int status = -1;

	/* Register sysfs hooks */
	status = sysfs_create_group(&pdev->dev.kobj, &as9926_24db_led_group);
	if (status)
		goto exit;

    
	dev_info(&pdev->dev, "device created\n");

	return 0;

exit:
	return status;
}

static int as9926_24db_led_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &as9926_24db_led_group);

	return 0;
}

static int __init as9926_24db_led_init(void)
{
	int ret;

	data = kzalloc(sizeof(struct as9926_24db_led_data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto alloc_err;
	}

	mutex_init(&data->update_lock);
	data->valid = 0;

	ret = platform_driver_register(&as9926_24db_led_driver);
	if (ret < 0)
		goto dri_reg_err;

	data->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
	if (IS_ERR(data->pdev)) {
		ret = PTR_ERR(data->pdev);
		goto dev_reg_err;
	}

	/* Set up IPMI interface */
	ret = init_ipmi_data(&data->ipmi, 0, &data->pdev->dev);
	if (ret)
		goto ipmi_err;

	return 0;

ipmi_err:
	platform_device_unregister(data->pdev);
dev_reg_err:
	platform_driver_unregister(&as9926_24db_led_driver);
dri_reg_err:
	kfree(data);
alloc_err:
	return ret;
}

static void __exit as9926_24db_led_exit(void)
{
	ipmi_destroy_user(data->ipmi.user);
	platform_device_unregister(data->pdev);
	platform_driver_unregister(&as9926_24db_led_driver);
	kfree(data);
}

MODULE_AUTHOR("Alex Lai <alex_lai@edge-core.com>");
MODULE_DESCRIPTION("AS9926 24DB led driver");
MODULE_LICENSE("GPL");

module_init(as9926_24db_led_init);
module_exit(as9926_24db_led_exit);
